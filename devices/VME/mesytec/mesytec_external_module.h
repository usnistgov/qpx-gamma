#ifndef MesytecExternal_H
#define MesytecExternal_H

#include <sstream>
#include <iomanip>
#include "daq_device.h"

namespace Qpx {

class MesytecVME;

class MesytecExternal : public DaqDevice
{

public:
  MesytecExternal();
  virtual ~MesytecExternal();

  bool boot() override;
  bool die() override;

  virtual bool connect(MesytecVME *controller, int addr);
  virtual void disconnect();

  virtual uint32_t modnum() const { return modnum_; }
  virtual bool connected() const;

  bool write_settings_bulk(Gamma::Setting &set) override;
  bool read_settings_bulk(Gamma::Setting &set) const override;

protected:
  uint16_t       module_ID_code_;
  MesytecVME     *controller_;
  uint16_t       modnum_;

  bool read_setting(Gamma::Setting& set) const;
  bool write_setting(Gamma::Setting& set);
  virtual void rebuild_structure(Gamma::Setting &set) {}
};

}

#endif // MesytecExternal_H
