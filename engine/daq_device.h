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

enum class LiveStatus : int {dead = 0, offline = 1, online = 2, history = 3};

class DaqDevice {

public:

  DaqDevice() : live_(LiveStatus::dead) {}
  DaqDevice(const DaqDevice& other)
    : live_(LiveStatus::history)
    , setting_definitions_(other.setting_definitions_)
  {}
  static std::string plugin_name() {return std::string();}

  virtual std::string device_name() const {return std::string();}

  bool load_setting_definitions(std::string file);
  bool save_setting_definitions(std::string file);


  LiveStatus live() {return live_;}
  virtual bool boot() {return false;}
  virtual bool die() {return true;}

  virtual bool write_settings_bulk(Gamma::Setting &set) {return false;}
  virtual bool read_settings_bulk(Gamma::Setting &set) const {return false;}
  virtual void get_all_settings() {}

  virtual bool execute_command(Gamma::Setting &set) {return false;}
  virtual std::map<int, std::vector<uint16_t>> oscilloscope() {return std::map<int, std::vector<uint16_t>>();}
  
  virtual bool daq_start(uint64_t timeout, SynchronizedQueue<Spill*>* out_queue) {return false;}
  virtual bool daq_stop() {return true;}
  virtual bool daq_running() {return false;}

protected:
  LiveStatus                                live_;
  std::map<std::string, Gamma::SettingMeta> setting_definitions_;

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

/*  DaqDevice* create_from_xml(const pugi::xml_node &root)
  {
    if (std::string(root.name()) != "DaqDevice")
      return nullptr;
    if (!root.attribute("type"))
      return nullptr;

    DaqDevice* instance = create_type(std::string(root.attribute("type").value()));
    if (instance != nullptr) {
      bool success = instance->from_xml(root);
      if (success)
        return instance;
      else {
        delete instance;
        return nullptr;
      }
    }
  }*/

  void register_type(std::string name, std::function<DaqDevice*(void)> typeConstructor)
  {
    PL_INFO << ">> registering device class '" << name << "'";
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
