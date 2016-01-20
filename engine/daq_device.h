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
 *      Gamma::DaqDevice abstract class
 *
 ******************************************************************************/

#ifndef GAMMA_DAQ_DEVICE
#define GAMMA_DAQ_DEVICE

#include "generic_setting.h"
#include "synchronized_queue.h"
#include "daq_types.h"

namespace Qpx {

enum DeviceStatus {
  dead      = 0,
  loaded    = 1 << 0,
  booted    = 1 << 1,
  can_boot  = 1 << 2,
  can_run   = 1 << 3,
  can_oscil = 1 << 4
};

inline DeviceStatus operator|(DeviceStatus a, DeviceStatus b) {return static_cast<DeviceStatus>(static_cast<int>(a) | static_cast<int>(b));}
inline DeviceStatus operator&(DeviceStatus a, DeviceStatus b) {return static_cast<DeviceStatus>(static_cast<int>(a) & static_cast<int>(b));}
inline DeviceStatus operator^(DeviceStatus a, DeviceStatus b) {return static_cast<DeviceStatus>(static_cast<int>(a) ^ static_cast<int>(b));}


class DaqDevice {

public:

  DaqDevice() : status_(DeviceStatus(0)) {}
  static std::string plugin_name() {return std::string();}

  virtual std::string device_name() const {return std::string();}

  bool load_setting_definitions(std::string file);
  bool save_setting_definitions(std::string file);


  DeviceStatus status() const {return status_;}
  virtual bool boot() {return false;}
  virtual bool die() {status_ = DeviceStatus(0); return true;}

  virtual bool write_settings_bulk(Gamma::Setting &set) {return false;}
  virtual bool read_settings_bulk(Gamma::Setting &set) const {return false;}
  virtual void get_all_settings() {}

  virtual std::map<int, std::vector<uint16_t>> oscilloscope() {return std::map<int, std::vector<uint16_t>>();}


  virtual bool daq_start(SynchronizedQueue<Spill*>* out_queue) {return false;}
  virtual bool daq_stop() {return true;}
  virtual bool daq_running() {return false;}

private:
  //no copying
  void operator=(DaqDevice const&);
  DaqDevice(const DaqDevice&);


protected:
  DeviceStatus                              status_;
  std::map<std::string, Gamma::SettingMeta> setting_definitions_;
  std::string                               profile_path_;

};

class DeviceFactory {
public:
  static DeviceFactory& getInstance()
  {
    static DeviceFactory singleton_instance;
    return singleton_instance;
  }

  DaqDevice* create_type(std::string type, std::string file)
  {
    DaqDevice *instance = nullptr;
    auto it = constructors.find(type);
    if(it != constructors.end())
      instance = it->second();
    if (instance != nullptr) {
      if (instance->load_setting_definitions(file))
        return instance;
      else
        delete instance;
    }
    return nullptr;
  }

  void register_type(std::string name, std::function<DaqDevice*(void)> typeConstructor)
  {
    PL_INFO << "<DeviceFactory> registering source '" << name << "'";
    constructors[name] = typeConstructor;
  }

  const std::vector<std::string> types() {
    std::vector<std::string> all_types;
    for (auto &q : constructors)
      all_types.push_back(q.first);
    return all_types;
  }

private:
  std::map<std::string, std::function<DaqDevice*(void)>> constructors;

  //singleton assurance
  DeviceFactory() {}
  DeviceFactory(DeviceFactory const&);
  void operator=(DeviceFactory const&);
};

template<class T>
class DeviceRegistrar {
public:
  DeviceRegistrar(std::string)
  {
    DeviceFactory::getInstance().register_type(T::plugin_name(),
                                               [](void) -> DaqDevice * { return new T();});
  }
};



}

#endif
