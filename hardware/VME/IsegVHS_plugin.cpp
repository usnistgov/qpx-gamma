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
 *      QpxIsegVHSPlugin
 *
 ******************************************************************************/

#include "IsegVHS_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

namespace Qpx {

static DeviceRegistrar<QpxIsegVHSPlugin> registrar("VME/IsegVHS");

QpxIsegVHSPlugin::QpxIsegVHSPlugin() {

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
}


QpxIsegVHSPlugin::~QpxIsegVHSPlugin() {
  die();
}


bool QpxIsegVHSPlugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == "VME/IsegVHS") {
    double voltage_max = 0;
    double current_max = 0;
    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::floating)
          || (k.metadata.setting_type == Gamma::SettingType::binary)
          || (k.metadata.setting_type == Gamma::SettingType::integer)
          || (k.metadata.setting_type == Gamma::SettingType::boolean)
          || (k.metadata.setting_type == Gamma::SettingType::int_menu))
      {
        if (!read_setting(k, 0)) {}
//          PL_DBG << "Could not read " << k.id_;
        if (k.id_ == "VME/IsegVHS/VoltageMax") {
          voltage_max = k.value_dbl;
          //                PL_DBG << "voltage max = " << voltage_max;
        }
        if (k.id_ == "VME/IsegVHS/CurrentMax") {
          current_max = k.value_dbl;
          //                PL_DBG << "current max = " << current_max;
        }
      }
      else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {
          if (p.metadata.setting_type == Gamma::SettingType::command) {
            p.metadata.writable =  ((status_ & DeviceStatus::can_exec) != 0);
            //PL_DBG << "command " << p.id_ << p.index << " now " << p.metadata.writable;
          }
          else if ((p.metadata.setting_type == Gamma::SettingType::floating)
                   || (p.metadata.setting_type == Gamma::SettingType::integer)
                   || (p.metadata.setting_type == Gamma::SettingType::boolean)
                   || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                   || (p.metadata.setting_type == Gamma::SettingType::binary)) {
            if (!read_setting(p, ((48 * channum) + 96))) {}
//              PL_DBG << "Could not read " << p.id_;
          }
        }

        for (auto &p : k.branches.my_data_) {
          if (p.id_ == "VME/IsegVHS/Channel/Status") {
            Gamma::Setting status = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelStatus"), Gamma::Match::id);
            if (status.value_int & 0x0010)
              p.value_int = 1;
            else if (status.value_int & 0x0008)
              p.value_int = 2;
            else
              p.value_int = 0;
          } else if (p.id_ == "VME/IsegVHS/Channel/VoltageSet") {
            Gamma::Setting nominal = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/VoltageNominalMaxSet"), Gamma::Match::id);
            p.metadata.maximum = nominal.value_dbl;// * voltage_max;
            //                  PL_DBG << "Volatge bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
          } else if (p.id_ == "VME/IsegVHS/Channel/CurrentSetOrTrip") {
            Gamma::Setting nominal = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/CurrentNominalMaxSet"), Gamma::Match::id);
            p.metadata.maximum = nominal.value_dbl;// * current_max;
            //                  PL_DBG << "Current bounds "  << nominal.value_dbl << " " << p.metadata.maximum;
          }
        }
      }
    }

  }
  return true;
}

void QpxIsegVHSPlugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxIsegVHSPlugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ == "VME/IsegVHS") {
//    PL_DBG << "writing VME/IsegVHS";
    rebuild_structure(set);

    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::floating)
          || (k.metadata.setting_type == Gamma::SettingType::integer)
          || (k.metadata.setting_type == Gamma::SettingType::boolean)
          || (k.metadata.setting_type == Gamma::SettingType::int_menu)
          || (k.metadata.setting_type == Gamma::SettingType::binary)) {
        Gamma::Setting s = k;
        if (k.metadata.writable && read_setting(s, 0) && (s != k)) {
//          PL_DBG << "writing " << k.id_;
          if (!write_setting(k, 0)) {}
//            PL_DBG << "Could not write " << k.id_;
        }
      }
      else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
//        PL_DBG << "writing VME/IsegVHS/Channel";
        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {
          if ((p.metadata.setting_type == Gamma::SettingType::floating)
              || (p.metadata.setting_type == Gamma::SettingType::integer)
              || (p.metadata.setting_type == Gamma::SettingType::boolean)
              || (p.metadata.setting_type == Gamma::SettingType::int_menu)
              || (p.metadata.setting_type == Gamma::SettingType::binary)) {
            Gamma::Setting s = p;
            if (p.metadata.writable && read_setting(s, ((48 * channum) + 96)) && (s != p)) {
//              PL_DBG << "writing " << p.id_;
              if (!write_setting(p, ((48 * channum) + 96))) {}
//                PL_DBG << "Could not write " << k.id_;
            }
          }
        }
      }
    }
  }
  return true;
}

bool QpxIsegVHSPlugin::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  //  if (set.id_ != device_name())
  //    return false;

  if (set.id_ == "VME/IsegVHS") {

    for (auto &k : set.branches.my_data_) {
      if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {

        uint16_t channum = k.index;
        for (auto &p : k.branches.my_data_) {

          if ((p.metadata.setting_type == Gamma::SettingType::command) && (p.value_int == 1)) {
            if (p.id_ == "VME/IsegVHS/Channel/OnOff") {
              //                  PL_DBG << "Triggered ON/OFF for " << p.index;
              p.value_int = 0;
              Gamma::Setting ctrl = k.get_setting(Gamma::Setting("VME/IsegVHS/Channel/ChannelControl"), Gamma::Match::id);
              if (p.metadata.writable && read_setting(ctrl, ((48 * channum) + 96))) {
                ctrl.value_int  ^= 0x0008;
                if (!write_setting(ctrl, ((48 * channum) + 96)))
                  PL_DBG << "Could not write " << k.id_;
              }
              return true;
            }
          }
        }
      }
    }
  }
  return false;
}


bool QpxIsegVHSPlugin::boot() {
  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<IsegVHS> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  if (!connected()) {
    PL_WARN << "<IsegVHS> Not connected to controller";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::booted | DeviceStatus::can_exec;
  return true;
}

bool QpxIsegVHSPlugin::die() {
  PL_DBG << "<IsegVHS> Disconnecting";

  disconnect();
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void QpxIsegVHSPlugin::get_all_settings() {
}



}
