/*******************************************************************************
 *
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 *
 * This software can be redistributed and/or modified freely provided that
 * any derivative works bear some notice that they are derived from it, and
 * any modified versions bear some notice that they have been modified.
 *
 * Author(s):
 *      Martin Shetty (NIST)
 *
 * Description:
 *      QpxVmePlugin
 *
 ******************************************************************************/

#include "vme_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"
#include "vmecontroller.h"
#include "vmemodule.h"
#include "producer_factory.h"


#include "vmusb.h"
#include "vmusb2.h"

namespace Qpx {

static ProducerRegistrar<QpxVmePlugin> registrar("VME");

QpxVmePlugin::QpxVmePlugin() {

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  controller_ = nullptr;


  runner_ = nullptr;
  parser_ = nullptr;
  raw_queue_ = nullptr;
  run_status_.store(0);
}


QpxVmePlugin::~QpxVmePlugin() {
  die();
}

bool QpxVmePlugin::daq_start(SynchronizedQueue<Spill*>* out_queue) {
  if (run_status_.load() > 0)
    return false;

  if (runner_ != nullptr)
    delete runner_;

  if (parser_ != nullptr)
    delete parser_;

  if (!daq_init())
    return false;

  run_status_.store(1);
  raw_queue_ = new SynchronizedQueue<Spill*>();
  //runner_ = new boost::thread(&worker_run_dbl, this, raw_queue_);
  //parser_ = new boost::thread(&worker_parse, this, raw_queue_, out_queue);

  return true;
}

bool QpxVmePlugin::daq_init() {
  if (!controller_ || !controller_->connected())
    return false;

  controller_->daq_init();
}

bool QpxVmePlugin::daq_stop() {
  if (run_status_.load() == 0)
    return false;

  run_status_.store(2);

  if ((runner_ != nullptr) && runner_->joinable()) {
    runner_->join();
    delete runner_;
    runner_ = nullptr;
  }

  wait_ms(500);
  while (raw_queue_->size() > 0)
    wait_ms(1000);
  wait_ms(500);
  raw_queue_->stop();
  wait_ms(500);

  if ((parser_ != nullptr) && parser_->joinable()) {
    parser_->join();
    delete parser_;
    parser_ = nullptr;
  }
  delete raw_queue_;
  raw_queue_ = nullptr;

  run_status_.store(0);
  return true;
}

bool QpxVmePlugin::daq_running() {
  if (run_status_.load() == 3)
    daq_stop();
  return (run_status_.load() > 0);
}


bool QpxVmePlugin::read_settings_bulk(Qpx::Setting &set) const {
  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if (q.metadata.setting_type == Qpx::SettingType::stem) {
      if ((q.metadata.setting_type == Qpx::SettingType::text) && (q.id_ == "VME/ControllerID")) {
        q.metadata.writable = (!(status_ & ProducerStatus::booted));
      } else if (modules_.count(q.id_) && modules_.at(q.id_)) {
        modules_.at(q.id_)->read_settings_bulk(q);
      } else if (q.id_ == "VME/Registers") {
        for (auto &k : q.branches.my_data_) {
          if (k.id_ == "VME/Registers/IRQ_Mask") {
            if (controller_)
              k.value_int = controller_->readIrqMask();
          } else {
            read_register(k);
          }
        }
      }
    }
  }
  return true;
}

void QpxVmePlugin::rebuild_structure(Qpx::Setting &set) {

}


bool QpxVmePlugin::write_settings_bulk(Qpx::Setting &set) {
  if (set.id_ != device_name())
    return false;

  set.enrich(setting_definitions_);

  boost::filesystem::path dir(profile_path_);
  dir.make_preferred();
  boost::filesystem::path path = dir.remove_filename();

  std::set<std::string> device_types;
  for (auto &q : Qpx::ProducerFactory::getInstance().types())
    device_types.insert(q);

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Qpx::SettingType::text) && (q.id_ == "VME/ControllerID")) {
      if (!(status_ & ProducerStatus::booted))
        controller_name_ = q.value_text;
    } else if (q.metadata.setting_type == Qpx::SettingType::stem) {
//      DBG << "<VmePlugin> looking at " << q.id_;
      if (modules_.count(q.id_) && modules_[q.id_]) {
        modules_[q.id_]->write_settings_bulk(q);
      } else if (device_types.count(q.id_) && (q.id_.size() > 4) && (q.id_.substr(0,4) == "VME/")) {
        boost::filesystem::path dev_settings = path / q.value_text;
        modules_[q.id_] = std::dynamic_pointer_cast<VmeModule>(ProducerFactory::getInstance().create_type(q.id_, dev_settings.string()));
        DBG << "<VmePlugin> added module " << q.id_ << " with settings at " << dev_settings.string();
        modules_[q.id_]->write_settings_bulk(q);
      } else if (q.id_ == "VME/Registers") {
        for (auto &k : q.branches.my_data_) {
          if (k.id_ == "VME/Registers/IRQ_Mask") {
            if (controller_ && (controller_->readIrqMask() != k.value_int))
              controller_->writeIrqMask(k.value_int);
          } else {
            Qpx::Setting s = k;
            if (s.metadata.writable && read_register(s) && (s != k)) {
              DBG << "VmUsb register write " << k.id_;
              write_register(k);
            }
          }
        }
      }
    }
  }
  return true;
}


bool QpxVmePlugin::boot() {
  DBG << "<VmePlugin> Attempting to boot";

  if (!(status_ & ProducerStatus::can_boot)) {
    WARN << "<VmePlugin> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;

  if (controller_ != nullptr)
    delete controller_;

  if (controller_name_ == "VmUsb") {
    controller_ = new VmUsb();
    controller_->connect(0);
  } else if (controller_name_ == "VmUsb2") {
    controller_ = new VmUsb2();
    controller_->connect(0);
  }

  if (!controller_->connected()) {
    WARN << "<VmePlugin> Could not connect to controller " << controller_->controllerName();
    delete controller_;
    controller_ = nullptr;
    return false;
  } else {
    DBG << "<VmePlugin> Connected to controller " << controller_->controllerName()
           << " SN:" << controller_->serialNumber();
    //controller_->clear_registers();
  }

  bool success = false;

  for (auto &q : modules_) {
    if ((q.second) && (!q.second->connected())) {
      DBG << "<VmePlugin> Searching for module " << q.first;
      for (long base_address = 0; base_address < 0xFFFFFFFF; base_address += q.second->baseAddressSpaceLength()) {
        q.second->connect(controller_, base_address);

        if (q.second->connected()) {
          q.second->boot(); //disregard return value
          DBG << "<VmePlugin> Connected to module " << q.first
                 << "[" << q.second->address()
                 << "] firmware=" << q.second->firmwareName()
                 << " booted=" << (0 != (q.second->status() & ProducerStatus::booted));
          success = true;
          break;
        }
      }
    }
  }

  if (success) {
    status_ = ProducerStatus::loaded | ProducerStatus::booted;
    return true;
  } else
    return false;
}

bool QpxVmePlugin::die() {
  DBG << "<VmePlugin> Disconnecting";

  daq_stop();
  if (runner_ != nullptr) {
    runner_->detach();
    delete runner_;
  }
  if (parser_ != nullptr) {
    parser_->detach();
    delete parser_;
  }
  if (raw_queue_ != nullptr) {
    raw_queue_->stop();
    delete raw_queue_;
  }

  modules_.clear();

  if (controller_ != nullptr) {
    delete controller_;
    controller_ = nullptr;
  }

  status_ = ProducerStatus::loaded | ProducerStatus::can_boot;
  return true;
}

void QpxVmePlugin::get_all_settings() {
}


bool QpxVmePlugin::read_register(Qpx::Setting& set) const {
  if (!(status_ & Qpx::ProducerStatus::booted))
    return false;
  if (set.metadata.address < 0)
    return false;

  int wordsize = 0;

  if ((set.metadata.setting_type == Qpx::SettingType::binary)
       || (set.metadata.setting_type == Qpx::SettingType::integer)
       || (set.metadata.setting_type == Qpx::SettingType::int_menu))
  {
    if (set.metadata.flags.count("u32")) {
      long data = controller_->readRegister(set.metadata.address);
      set.value_int = data;
      return true;
    }
    else if (set.metadata.flags.count("u16")) {
      long data = controller_->readRegister(set.metadata.address);
      data &= 0x0000FFFF;
      set.value_int = data;
      return true;
    }
  }
  DBG << "<VmePlugin> Setting " << set.id_ << " does not have a well defined hardware type";
  return false;
}

bool QpxVmePlugin::write_register(Qpx::Setting& set) {
  if (!(status_ & Qpx::ProducerStatus::booted))
    return false;

  if (set.metadata.address < 0)
    return false;

  int wordsize = 0;

  if ((set.metadata.setting_type == Qpx::SettingType::binary)
       || (set.metadata.setting_type == Qpx::SettingType::integer)
       || (set.metadata.setting_type == Qpx::SettingType::int_menu))
  {
    if (set.metadata.flags.count("u32")) {
      long data = set.value_int;
      controller_->writeRegister(set.metadata.address, data);
      return true;
    }
    else if (set.metadata.flags.count("u16")) {
      long data = set.value_int &= 0x0000FFFF;
      controller_->writeRegister(set.metadata.address, data);
      return true;
    }
  }
  DBG << "<VmePlugin> Setting " << set.id_ << " does not have a well defined hardware type";
  return false;
}


}
