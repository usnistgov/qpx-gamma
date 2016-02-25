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
 *      MTDC32
 *
 ******************************************************************************/

#ifndef QPX_MTDC32_PLUGIN
#define QPX_MTDC32_PLUGIN

#include "Mesytec_base_module.h"

namespace Qpx {

class MTDC32 : public MesytecVME {
  
public:
  MTDC32();
  ~MTDC32();

  //QpxPlugin
  static std::string plugin_name() {return "VME/MTDC32";}
  std::string device_name() const override {return plugin_name();}

private:
  //no copying
  void operator=(MTDC32 const&);
  MTDC32(const MTDC32&);

protected:
  void rebuild_structure(Gamma::Setting &set) override;

};


}

#endif
