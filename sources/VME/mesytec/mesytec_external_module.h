#ifndef MesytecExternal_H
#define MesytecExternal_H

#include <sstream>
#include <iomanip>
#include "daq_source.h"

namespace Qpx {

class MesytecVME;

class MesytecExternal : public Source
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

  bool write_settings_bulk(Qpx::Setting &set) override;
  bool read_settings_bulk(Qpx::Setting &set) const override;

protected:
  uint16_t       module_ID_code_;
  MesytecVME     *controller_;
  uint16_t       modnum_;

  bool read_setting(Qpx::Setting& set) const;
  bool write_setting(Qpx::Setting& set);
  virtual void rebuild_structure(Qpx::Setting &set) {}
};

}

#endif // MesytecExternal_H
