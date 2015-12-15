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
 *      QpxHV8Plugin
 *
 ******************************************************************************/

#include "hv8_plugin.h"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "custom_logger.h"
#include "custom_timer.h"

namespace Qpx {

static DeviceRegistrar<QpxHV8Plugin> registrar("HV8");

QpxHV8Plugin::QpxHV8Plugin()
  : portname("/dev/pts/8")
  , baudrate(19200)
  , charactersize(8)
  , parity(boost::asio::serial_port_base::parity::none)
  , stopbits(boost::asio::serial_port_base::stop_bits::one)
  , flowcontrol(boost::asio::serial_port_base::flow_control::none)
{
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
}


QpxHV8Plugin::~QpxHV8Plugin() {
  die();
}


bool QpxHV8Plugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "VME/IsegVHS")) {
        //PL_DBG << "reading VME/IsegVHS";
        int modnum = 0;
        for (auto &k : q.branches.my_data_) {
          if (k.metadata.setting_type == Gamma::SettingType::floating)
            k.value_dbl = k.metadata.address * 0.1;// channel_parameter_values_[k.metadata.address + modnum];
          else if ((k.metadata.setting_type == Gamma::SettingType::integer)
                   || (k.metadata.setting_type == Gamma::SettingType::boolean)
                   || (k.metadata.setting_type == Gamma::SettingType::int_menu)
                   || (k.metadata.setting_type == Gamma::SettingType::binary))
            k.value_int = k.metadata.address; //channel_parameter_values_[k.metadata.address + modnum];
          else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
            //PL_DBG << "reading VME/IsegVHS/Channel";
            uint16_t channum = k.index;
            for (auto &p : k.branches.my_data_) {
              if (p.metadata.setting_type == Gamma::SettingType::floating)
                p.value_dbl = p.metadata.address + (30 * channum) + 0x0060;// channel_parameter_values_[k.metadata.address + modnum];
              else if ((p.metadata.setting_type == Gamma::SettingType::integer)
                       || (p.metadata.setting_type == Gamma::SettingType::boolean)
                       || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                       || (p.metadata.setting_type == Gamma::SettingType::binary))
                p.value_dbl = p.metadata.address + (30 * channum) + 0x0060; //channel_parameter_values_[k.metadata.address + modnum];
            }
          }
        }
      }
    }
  }
  return true;
}

void QpxHV8Plugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxHV8Plugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_);

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::text) && (q.id_ == "VME/ControllerID")) {
      PL_DBG << "<HV8Plugin> controller expected " << q.value_text;
      if (controller_name_.empty())
        controller_name_ = q.value_text;
    } else       if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "VME/IsegVHS")) {
      rebuild_structure(q);

      //PL_DBG << "writing VME/IsegVHS";
      int modnum = 0;
      for (auto &k : q.branches.my_data_) {
        if (k.metadata.setting_type == Gamma::SettingType::floating) {
         // k.value_dbl = k.metadata.address * 0.1;// channel_parameter_values_[k.metadata.address + modnum];
        }
        else if ((k.metadata.setting_type == Gamma::SettingType::integer)
                 || (k.metadata.setting_type == Gamma::SettingType::boolean)
                 || (k.metadata.setting_type == Gamma::SettingType::int_menu)
                 || (k.metadata.setting_type == Gamma::SettingType::binary)) {
         // k.value_int = k.metadata.address; //channel_parameter_values_[k.metadata.address + modnum];
        }
        else if ((k.metadata.setting_type == Gamma::SettingType::stem) && (k.id_ == "VME/IsegVHS/Channel")) {
          //PL_DBG << "writing VME/IsegVHS/Channel";
          uint16_t channum = k.index;
          for (auto &p : k.branches.my_data_) {
            if (p.metadata.setting_type == Gamma::SettingType::floating) {
             // p.value_dbl = p.metadata.address + (30 * channum) + 0x0060;// channel_parameter_values_[k.metadata.address + modnum];
            }
            else if ((p.metadata.setting_type == Gamma::SettingType::integer)
                     || (p.metadata.setting_type == Gamma::SettingType::boolean)
                     || (p.metadata.setting_type == Gamma::SettingType::int_menu)
                     || (p.metadata.setting_type == Gamma::SettingType::binary)) {
            //  p.value_dbl = p.metadata.address + (30 * channum) + 0x0060; //channel_parameter_values_[k.metadata.address + modnum];
            }
          }
        }
      }
    }
  }
  return true;
}

bool QpxHV8Plugin::execute_command(Gamma::Setting &set) {
  if (!(status_ & DeviceStatus::can_exec))
    return false;

  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::command) && (q.value_dbl == 1)) {
        q.value_dbl = 0;
        if (q.id_ == "Vme/Iseg/Sing a song")
          return true;
      }
    }
  }

  return false;
}


bool QpxHV8Plugin::boot() {
  PL_DBG << "<HV8Plugin> Attempting to boot";

  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<HV8Plugin> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  port.open(portname, baudrate, charactersize, parity, stopbits, flowcontrol);

  port.writeString("\r\n");
  port.writeString("\r\n");
  boost::this_thread::sleep(boost::posix_time::seconds(1));

  bool success = false;
  bool logged = false;
  for (int i=0; i < 10; ++i) {
    std::string line = port.readLine(0);
    PL_DBG << "Received :\n" << line;
    if (line == "->") {
      PL_DBG << "not logged in";
      success = true;
      break;
    } else if (line == "+>") {
      PL_DBG << "logged in";
      success = true;
      logged = true;
      break;
    }
  }

  if (success) {
    port.flush(flush_both);
    if (!logged) {
//      port.writeString("Q");
//      port.writeString("\r\n");
      port.writeString("L");
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      port.writeString("\r\n");
      boost::this_thread::sleep(boost::posix_time::seconds(1));
      port.writeString("\r\n");
      boost::this_thread::sleep(boost::posix_time::seconds(3));

      success = false;
      for (int i=0; i < 10; ++i) {
        std::string line = port.readLine(0);
        PL_DBG << "L: received :\n" << line;
        if (line == "Login:") {
          PL_DBG << "login prompt";
          success = true;
          break;
        }
      }

      if (success) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        char c;
        c = 'H';
        port.send((void*) &c, 1);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        c = 'V';
        port.send((void*) &c, 1);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        c = '8';
        port.send((void*) &c, 1);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        c = '\r';
        port.send((void*) &c, 1);
        boost::this_thread::sleep(boost::posix_time::seconds(1));
//        c = '\n';
//        port.send((void*) &c, 1);
//        boost::this_thread::sleep(boost::posix_time::seconds(1));

        port.writeString("\r\n");
        boost::this_thread::sleep(boost::posix_time::seconds(3));

        for (int i=0; i < 10; ++i) {
          std::string line = port.readLine(0);
          PL_DBG << "LP: received :\n" << line;
          if (line == "+>") {
            PL_DBG << "logged in";
            logged = true;
            break;
          }
        }

      }
    }
    port.flush(flush_both);


    if (logged) {
      status_ = DeviceStatus::loaded | DeviceStatus::booted;
      return true;
    }
  } else
    return false;

}

bool QpxHV8Plugin::die() {
  PL_DBG << "<HV8Plugin> Disconnecting";

  port.close();

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  return true;
}

void QpxHV8Plugin::get_all_settings() {
}



}
