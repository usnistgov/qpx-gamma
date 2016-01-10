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
      if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "VME/IsegVHS")) {
        if (modules_.count(q.id_)) {
          VmeModule *mod = dynamic_cast<VmeModule*>(modules_.at(q.id_));
          PL_DBG << "Reading out settings for module " << q.id_;
          for (auto &k : q.branches.my_data_) {
            if ((k.metadata.setting_type == Gamma::SettingType::floating)
                || (k.metadata.setting_type == Gamma::SettingType::binary)
                || (k.metadata.setting_type == Gamma::SettingType::integer)
                || (k.metadata.setting_type == Gamma::SettingType::boolean)
                || (k.metadata.setting_type == Gamma::SettingType::int_menu))
            {
              if (!mod->read_setting(k, 0))
                PL_DBG << "Could not read " << k.id_;
            }
            else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
              uint16_t channum = k.index;
              for (auto &p : k.branches.my_data_) {
                if (p.metadata.setting_type == Gamma::SettingType::command) {
                  p.metadata.writable =  ((status_ & DeviceStatus::can_exec) != 0);
                  PL_DBG << "command " << p.id_ << p.index << " now " << p.metadata.writable;
                }
                else if ((p.metadata.setting_type == Gamma::SettingType::floating)
                         || (p.metadata.setting_type == Gamma::SettingType::integer)
                         || (p.metadata.setting_type == Gamma::SettingType::boolean)
                         || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                         || (p.metadata.setting_type == Gamma::SettingType::binary)) {
                  if (!mod->read_setting(p, ((48 * channum) + 96)))
                    PL_DBG << "Could not read " << k.id_;
                }
              }

              for (auto &p : k.branches.my_data_) {
                if (p.id_ == "VME/IsegVHS/Channel/Status") {
                  if ((k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelStatus"), Gamma::Match::id).value_int & 0x0008) != 0)
                    p.value_int = 1;
                  else if ((k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelStatus"), Gamma::Match::id).value_int & 0x0010) != 0)
                    p.value_int = 2;
                  else
                    p.value_int = 0;
                }
              }


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

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::text) && (q.id_ == "VME/ControllerID")) {
      //PL_DBG << "<VmePlugin> controller expected " << q.value_text;
      if (controller_name_.empty())
        controller_name_ = q.value_text;
    } else       if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "VME/IsegVHS")) {
      rebuild_structure(q);

      if (modules_.count(q.id_)) {
        VmeModule *mod = dynamic_cast<VmeModule*>(modules_.at(q.id_));
//        PL_DBG << "Writing settings for module " << q.id_;

        for (auto &k : q.branches.my_data_) {
          if ((k.metadata.setting_type == Gamma::SettingType::floating)
              || (k.metadata.setting_type == Gamma::SettingType::integer)
              || (k.metadata.setting_type == Gamma::SettingType::boolean)
              || (k.metadata.setting_type == Gamma::SettingType::int_menu)
              || (k.metadata.setting_type == Gamma::SettingType::binary)) {
            Gamma::Setting s = k;
            if (k.metadata.writable && mod->read_setting(s, 0) && (s != k)) {
              PL_DBG << "writing " << k.id_;
              if (!mod->write_setting(k, 0))
                PL_DBG << "Could not write " << k.id_;
            }
          }
          else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
            //PL_DBG << "writing VME/IsegVHS/Channel";
            uint16_t channum = k.index;
            for (auto &p : k.branches.my_data_) {
              if ((p.metadata.setting_type == Gamma::SettingType::floating)
                  || (p.metadata.setting_type == Gamma::SettingType::integer)
                  || (p.metadata.setting_type == Gamma::SettingType::boolean)
                  || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                  || (p.metadata.setting_type == Gamma::SettingType::binary)) {
                Gamma::Setting s = p;
                if (p.metadata.writable && mod->read_setting(s, ((48 * channum) + 96)) && (s != p)) {
                  PL_DBG << "writing " << p.id_;
                  if (!mod->write_setting(p, ((48 * channum) + 96)))
                    PL_DBG << "Could not write " << k.id_;
                }
              }
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
      VmeModule *mod = dynamic_cast<VmeModule*>(modules_.at(q.id_));
        PL_DBG << "Writing settings for module " << q.id_;

        for (auto &k : q.branches.my_data_) {
          if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {

            uint16_t channum = k.index;
            for (auto &p : k.branches.my_data_) {

              if ((p.metadata.setting_type == Gamma::SettingType::command) && (p.value_dbl == 1)) {
                if (p.id_ == "VME/IsegVHS/Channel/OnOff") {
                  PL_DBG << "Triggered ON/OFF for " << p.index;
                  p.value_dbl = 0;
                  Gamma::Setting ctrl = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelControl"), Gamma::Match::id);
                  if (p.metadata.writable && mod->read_setting(ctrl, ((48 * channum) + 96))) {
                    ctrl.value_int  ^= 0x0008;
                    if (!mod->write_setting(ctrl, ((48 * channum) + 96)))
                      PL_DBG << "Could not write " << k.id_;
                  }
                  return true;
                }
              }
            }
          }
        }
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

  if (controller_name_ == "VmUsb") {
    controller_ = new VmUsb();
    PL_DBG << "<VmePlugin> Controller status: " << controller_->information();
  }

  bool success = false;
  for (int base_address = 0; base_address < 0xFFFF; base_address += VHS_ADDRESS_SPACE_LENGTH) {
    //PL_DBG << "trying to load mod at BA " << base_address;
    VmeModule module(controller_, base_address);

    if (module.firmwareName() == "V12C0") {
      VmeModule *mod = new VmeModule(controller_, base_address);
      modules_["VME/IsegVHS"] = mod;
      PL_DBG << "<VmePlugin> Adding module [" << base_address << "] firmwareName=" << module.firmwareName();
      success = true;
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



}
