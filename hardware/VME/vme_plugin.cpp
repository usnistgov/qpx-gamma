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
#include "vmusb.h"

namespace Qpx {

static DeviceRegistrar<QpxVmePlugin> registrar("VME");

QpxVmePlugin::QpxVmePlugin() {

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  controller_ = nullptr;

}


QpxVmePlugin::~QpxVmePlugin() {
  die();
}


bool QpxVmePlugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if (q.metadata.setting_type == Gamma::SettingType::stem) {
        if (modules_.count(q.id_) && modules_.at(q.id_)) {
//          PL_DBG << "read settings bulk " << q.id_;
          modules_.at(q.id_)->read_settings_bulk(q);
        } else if (q.id_ == "VME/Registers") {
          for (auto &k : q.branches.my_data_) {
            if (k.id_ == "VME/Registers/IRQ_Mask") {
              PL_DBG << "IRQ Mask read";
              // do something fancy
            } else {
//              PL_DBG << "VmUsb register read";
              read_register(k);
            }
          }
        }
      }
    }
  }
  return true;
}

void QpxVmePlugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxVmePlugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ != device_name())
    return false;

  boost::filesystem::path dir(profile_path_);
  dir.make_preferred();
  boost::filesystem::path path = dir.remove_filename();

  std::set<std::string> device_types;
  for (auto &q : Qpx::DeviceFactory::getInstance().types())
    device_types.insert(q);

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::text) && (q.id_ == "VME/ControllerID")) {
      PL_DBG << "<VmePlugin> controller expected " << q.value_text;
      if (controller_name_.empty())
        controller_name_ = q.value_text;
    } else if (q.metadata.setting_type == Gamma::SettingType::stem) {
//      PL_DBG << "<VmePlugin> looking at " << q.id_;
      if (modules_.count(q.id_) && modules_[q.id_]) {
        modules_[q.id_]->write_settings_bulk(q);
      } else if (device_types.count(q.id_) && (q.id_.size() > 4) && (q.id_.substr(0,4) == "VME/")) {
        boost::filesystem::path dev_settings = path / q.value_text;
        modules_[q.id_] = dynamic_cast<VmeModule*>(DeviceFactory::getInstance().create_type(q.id_, dev_settings.string()));
        PL_DBG << "added module " << q.id_ << " with settings at " << dev_settings.string();
        modules_[q.id_]->write_settings_bulk(q);
      } else if (q.id_ == "VME/Registers") {
        for (auto &k : q.branches.my_data_) {
          if (k.id_ == "VME/Registers/IRQ_Mask") {
            PL_DBG << "IRQ Mask write";
            // do something fancy
          } else {
            Gamma::Setting s = k;
            read_register(s);
            if (s != k) {
              PL_DBG << "VmUsb register write " << k.id_;
              write_register(k);
            }
          }
        }
      }
    }
  }
  return true;
}

bool QpxVmePlugin::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "VME/IsegVHS")) {
      if (modules_.count(q.id_)) {
      QpxIsegVHSPlugin *mod = dynamic_cast<QpxIsegVHSPlugin*>(modules_.at(q.id_));
        PL_DBG << "Executing for module " << q.id_;


      }
    }
  }
  return false;
}


bool QpxVmePlugin::boot() {
  PL_DBG << "<VmePlugin> Attempting to boot";

  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<VmePlugin> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (controller_ != nullptr)
    delete controller_;

//  if (controller_name_ == "VmUsb") {
    controller_ = new VmUsb();
    controller_->connect(0);
//  }

  if (!controller_->connected()) {
    PL_WARN << "<VmePlugin> Could not connect to " << controller_name_;
    return false;
  } else {
    PL_DBG << "<VmePlugin> Connected to " << controller_->controllerName()
           << " SN:" << controller_->serialNumber();
    controller_->clear_registers();
  }

  bool success = false;

  for (auto &q : modules_) {
    if ((q.second) && (!q.second->connected())) {
      PL_DBG << "module " << q.first << " not yet connected";
      for (long base_address = 0; base_address < 0xFFFFFFFF; base_address += q.second->baseAddressSpaceLength()) {
        q.second->connect(controller_, base_address);

        if (q.second->connected()) {
          q.second->boot(); //disregard return value
          PL_DBG << "<VmePlugin> Adding module [" << base_address << "] firmwareName=" << q.second->firmwareName();
          success = true;
          break;
        }
      }
    }
  }

  if (success) {
    status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_exec;
    return true;
  } else
    return false;
}

bool QpxVmePlugin::die() {
  PL_DBG << "<VmePlugin> Disconnecting";

  for (auto &q : modules_)
    delete q.second;
  modules_.clear();

  if (controller_ != nullptr) {
    delete controller_;
    controller_ = nullptr;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void QpxVmePlugin::get_all_settings() {
}


bool QpxVmePlugin::read_register(Gamma::Setting& set) const {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  int wordsize = 0;

  if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      wordsize = 32;
    else if (set.metadata.maximum == 16)
      wordsize = 16;
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      wordsize = 32;
    else if (set.metadata.hardware_type == "u16")
      wordsize = 16;
  }

  if (wordsize == 16) {
    long data = controller_->readRegister(set.metadata.address);
    data &= 0x0000FFFF;
    set.value_int = data;
    return true;
  } else if (wordsize == 32) {
    long data = controller_->readRegister(set.metadata.address);
    set.value_int = data;
    return true;
  } else {
    PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
    return false;
  }
  return true;
}

bool QpxVmePlugin::write_register(Gamma::Setting& set) {
  if (!(status_ & Qpx::DeviceStatus::booted))
    return false;

  int wordsize = 0;

  if (set.metadata.setting_type == Gamma::SettingType::binary) {
    if (set.metadata.maximum == 32)
      wordsize = 32;
    else if (set.metadata.maximum == 16)
      wordsize = 16;
  }
  else if ((set.metadata.setting_type == Gamma::SettingType::integer)
           || (set.metadata.setting_type == Gamma::SettingType::int_menu))
  {
    if (set.metadata.hardware_type == "u32")
      wordsize = 32;
    else if (set.metadata.hardware_type == "u16")
      wordsize = 16;
  }

  if (wordsize == 16) {
    long data = set.value_int &= 0x0000FFFF;
    controller_->writeRegister(set.metadata.address, data);
    return true;
  } else if (wordsize == 32) {
    long data = set.value_int;
    controller_->writeRegister(set.metadata.address, data);
    return true;
  } else {
    PL_DBG << "Setting " << set.id_ << " does not have a well defined hardware type";
    return false;
  }
  return true;
}


}
