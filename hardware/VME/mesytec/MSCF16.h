#ifndef MSCF16_H
#define MSCF16_H

#include "mesytec_external_module.h"

namespace Qpx {

class MSCF16 : public MesytecExternal
{

public:
  MSCF16();
  ~MSCF16();

  static std::string plugin_name() {return "VME/MesytecRC/MSCF16";}
  std::string device_name() const override {return plugin_name();}

private:
  //no copying
  void operator=(MSCF16 const&);
  MSCF16(const MSCF16&);

protected:
  uint16_t       module_ID_code_;
  MesytecVME     *controller_;
  uint16_t       modnum_;

  virtual void rebuild_structure(Gamma::Setting &set) {}
};

}

#endif // MSCF16_H
