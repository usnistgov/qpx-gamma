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
  : portname("/dev/tty")
  , baudrate(9600)
  , charactersize(boost::asio::serial_port_base::character_size(8))
  , parity(boost::asio::serial_port_base::parity::none)
  , stopbits(boost::asio::serial_port_base::stop_bits::one)
  , flowcontrol(boost::asio::serial_port_base::flow_control::none)
{
  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;
  voltages.resize(8);
}


QpxHV8Plugin::~QpxHV8Plugin() {
  die();
}


bool QpxHV8Plugin::read_settings_bulk(Gamma::Setting &set) const {
  if (set.id_ == device_name()) {
    for (auto &q : set.branches.my_data_) {
      if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "HV8/Channels")) {
        for (auto &k : q.branches.my_data_) {
          if ((k.metadata.setting_type == Gamma::SettingType::floating) && (k.metadata.address < voltages.size()))
            k.value_dbl = voltages[k.metadata.address];
        }
      } else {
        //q.metadata.writable = !(status_ & DeviceStatus::booted);
      }
    }
  }
  return true;
}

void QpxHV8Plugin::rebuild_structure(Gamma::Setting &set) {

}


bool QpxHV8Plugin::write_settings_bulk(Gamma::Setting &set) {
  set.enrich(setting_definitions_, true);

  if (set.id_ != device_name())
    return false;

  for (auto &q : set.branches.my_data_) {
    if ((q.metadata.setting_type == Gamma::SettingType::file_path) && (q.id_ == "HV8/PortName"))
      portname = q.value_text;
    else if ((q.metadata.setting_type == Gamma::SettingType::int_menu) && (q.id_ == "HV8/BaudRate"))
      baudrate = q.value_int;
    else if ((q.metadata.setting_type == Gamma::SettingType::integer) && (q.id_ == "HV8/CharacterSize"))
      charactersize = boost::asio::serial_port_base::character_size(q.value_int);
    else if ((q.metadata.setting_type == Gamma::SettingType::int_menu) && (q.id_ == "HV8/Parity")) {
      if (q.value_int == 2)
        parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd);
      else if (q.value_int == 3)
        parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even);
      else
        parity = boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none);
    }
    else if ((q.metadata.setting_type == Gamma::SettingType::int_menu) && (q.id_ == "HV8/StopBits")) {
      if (q.value_int == 2)
        stopbits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive);
      else if (q.value_int == 3)
        stopbits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two);
      else
        stopbits = boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one);
    }
    else if ((q.metadata.setting_type == Gamma::SettingType::int_menu) && (q.id_ == "HV8/FlowControl")) {
      if (q.value_int == 2)
        flowcontrol = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::software);
      else if (q.value_int == 3)
        flowcontrol = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware);
      else
        flowcontrol = boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none);
    }
    else if ((q.metadata.setting_type == Gamma::SettingType::stem) && (q.id_ == "HV8/Channels")) {
      rebuild_structure(q);

      //PL_DBG << "writing VME/IsegVHS";
      int modnum = 0;
      for (auto &k : q.branches.my_data_) {
        if ((k.metadata.setting_type == Gamma::SettingType::floating) && (k.metadata.address < voltages.size())) {
          if (k.value_dbl != voltages[k.metadata.address])
            set_voltage(k.metadata.address, k.value_dbl);
         // k.value_dbl = k.metadata.address * 0.1;// channel_parameter_values_[k.metadata.address + modnum];
        }
      }
    }
  }
  return true;
}

void QpxHV8Plugin::set_voltage(int chan, double voltage) {
  PL_DBG << "<HV8Plugin> Set voltage " << chan << " to " << voltage;
  int attempts = 5;

  if (!port.isOpen()) {
    PL_DBG << "<HV8Plugin> cannot read settings. Serial port is not open";
    return;
  }

  if (!get_prompt(5)) {
    PL_DBG << "<HV8Plugin> cannot write settings. Not logged into HV8";
    return;
  }

  bool success = false;
  std::vector<std::string> tokens;
  for (int i=0; i < attempts; ++i) {
//        boost::this_thread::sleep(boost::posix_time::seconds(1));

    if (get_prompt(5)) {
      std::stringstream ss;
      ss << "S " << chan << "\r";
      PL_DBG << "Will write " << ss.str();
      port.writeString(ss.str());
      boost::this_thread::sleep(boost::posix_time::seconds(0.2));

      if (get_prompt(5)) {
        std::stringstream ss2;
        ss2 << "V " << voltage << "\r";
        PL_DBG << "Will write " << ss2.str();
        port.writeString(ss2.str());
        boost::this_thread::sleep(boost::posix_time::seconds(0.2));

        if (get_prompt(5))
          success = true;
      }
    }


    if (success)
      break;

  }


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


bool QpxHV8Plugin::get_prompt(uint16_t attempts) {
  bool success = false;
  bool logged = false;
  for (int i=0; i < attempts; ++i) {
    port.writeString("\r");
    std::string lines;
    try { lines = port.readStringUntil(">"); }
    catch (...) { PL_ERR << "<HV8Plugin> could not read line from serial"; }
    boost::algorithm::trim_if(lines, boost::algorithm::is_any_of("\r\n\t "));
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, lines, boost::algorithm::is_any_of("\r\n\t "));

    std::string line;
    if (!tokens.empty())
      line = tokens[tokens.size() - 1];

    if (line == "->") {
      success = true;
      break;
    } else if (line == "+>") {
      success = true;
      logged = true;
      break;
    }
  }

  if (success) {
    if (!logged) {

      success = false;
      for (int i=0; i < attempts; ++i) {
        port.writeString("L\r");

        std::string line;
        try { line = port.readStringUntil(":"); }
        catch (...) { PL_ERR << "<HV8Plugin> could not read line from serial"; }
        boost::algorithm::trim_if(line, boost::algorithm::is_any_of("\r\n\t "));

        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, line, boost::algorithm::is_any_of("\r\n\t "));

//        PL_DBG << line;
        if (!tokens.empty() && (tokens[tokens.size() - 1] == "Login:")) {
//          PL_DBG << "login prompt";
          success = true;
          break;
        }

      }

      if (success) {
//        c = '\n';
//        port.send((void*) &c, 1);
//        boost::this_thread::sleep(boost::posix_time::seconds(1));

        for (int i=0; i < attempts; ++i) {
//          boost::this_thread::sleep(boost::posix_time::seconds(1));
          port.writeString("HV8\r");
          std::string line;
          try { line = port.readStringUntil(">"); }
          catch (...) { PL_ERR << "<HV8Plugin> could not read line from serial"; }
          boost::algorithm::trim_if(line, boost::algorithm::is_any_of("\r\n\t "));
          PL_DBG << line;
          if (line == "+>") {
//            PL_DBG << "logged in";
            logged = true;
            break;
          }
        }

      }
    }
  }
  return logged;
}


bool QpxHV8Plugin::boot() noexcept(false) {
  PL_DBG << "<HV8Plugin> Attempting to boot";

  int attempts = 5;

  if (!(status_ & DeviceStatus::can_boot)) {
    PL_WARN << "<HV8Plugin> Cannot boot. Failed flag check (can_boot == 0)";
    return false;
  }

  status_ = DeviceStatus::loaded | DeviceStatus::can_boot;

  try {
    port.open(portname, baudrate, parity, boost::asio::serial_port_base::character_size(charactersize), flowcontrol, stopbits);
    port.setTimeout(boost::posix_time::seconds(5));
  } catch (...) {
    PL_ERR << "<HV8Plugin> could not open serial port";
    return false;
  }

  if (!port.isOpen()) {
    PL_DBG << "<HV8Plugin> serial port could not be opened";
    return false;
  }


  if (get_prompt(5)) {
      PL_DBG << "<HV8Plugin> Logged in. Startup successful";
      status_ = DeviceStatus::loaded | DeviceStatus::booted;
      return true;
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
  int attempts = 5;

  if (!port.isOpen()) {
    PL_DBG << "<HV8Plugin> cannot read settings. Serial port is not open";
    return;
  }

  if (!get_prompt(5)) {
    PL_DBG << "<HV8Plugin> cannot read settings. Not logged into HV8";
    return;
  }


  bool success = false;
  std::vector<std::string> tokens;
  for (int i=0; i < attempts; ++i) {
//        boost::this_thread::sleep(boost::posix_time::seconds(1));

    if (tokens.size() < 3) {
      port.writeString("R\r");
      boost::this_thread::sleep(boost::posix_time::seconds(0.2));
      port.writeString("\r");
    }

    std::string line;

    try { line = port.readStringUntil(">"); }
    catch (...) { PL_ERR << "<HV8Plugin> could not read line from serial"; }

    PL_DBG << "will tokenize: " << line << "<end>";
    boost::algorithm::split(tokens, line, boost::algorithm::is_any_of("\r\n"));

    for (auto &q : tokens) {
      PL_DBG << "token: " << q;
    }

    PL_DBG << "tokenized size = " << tokens.size();
    if (tokens.size() > 6) {
      success = true;
      break;
    }
  }

  if (success) {
    for (auto &q : tokens) {
      boost::algorithm::trim_if(q, boost::algorithm::is_any_of("\r\n\t "));
      q.erase(std::unique(q.begin(), q.end(),
            [](char a, char b) { return a == ' ' && b == ' '; } ), q.end() );

      PL_DBG << "tokenizing " << q;
      std::vector<std::string> tokens2;
      boost::algorithm::split(tokens2, q, boost::algorithm::is_any_of("\t "));
      if (tokens2.size() > 2) {


        PL_DBG << "tokens2 size " << tokens2.size();
        for (auto &q : tokens2) {
          PL_DBG << " token: " << q;
        }
        int chan = boost::lexical_cast<int>(tokens2[0]);
        double voltage = boost::lexical_cast<double>(tokens2[1]);
        PL_DBG << "Voltage for chan " << chan << " reported as " << voltage;
        if ((chan >= 0) && (chan < voltages.size()))
          voltages[chan] = voltage;
      }
    }
  }

}



}
