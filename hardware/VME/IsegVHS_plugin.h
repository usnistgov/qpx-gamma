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

#ifndef QPX_ISEGVHS_PLUGIN
#define QPX_ISEGVHS_PLUGIN

#include "detector.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>

#include "vmemodule.h"
#include "vmecontroller.h"
#include "generic_setting.h"

#define VHS_ADDRESS_SPACE_LENGTH				0x0400

namespace Qpx {

class QpxIsegVHSPlugin : public VmeModule {
  
public:
  QpxIsegVHSPlugin();
  ~QpxIsegVHSPlugin();

  //QpxPlugin
  static std::string plugin_name() {return "VME/IsegVHS";}
  std::string device_name() const override {return plugin_name();}

  bool write_settings_bulk(Gamma::Setting &set) override;
  bool read_settings_bulk(Gamma::Setting &set) const override;
  void get_all_settings() override;
  bool boot() override;
  bool die() override;

  bool execute_command(Gamma::Setting &set) override;

  //VmeModule
  virtual bool    connect(VmeController *controller, int baseAddress);
  virtual bool    connected() const;
  virtual void    disconnect();

  std::string firmwareName() const;

  uint16_t    baseAddress() const { return m_baseAddress; }
  virtual uint16_t baseAddressSpaceLength() const {return VHS_ADDRESS_SPACE_LENGTH;}
  bool        setBaseAddress(uint16_t baseAddress);
  std::string address() const;

  void    programBaseAddress(uint16_t address);
  uint16_t verifyBaseAddress(void) const;


private:
  //no copying
  void operator=(QpxIsegVHSPlugin const&);
  QpxIsegVHSPlugin(const QpxIsegVHSPlugin&);

protected:
  void rebuild_structure(Gamma::Setting &set);
  bool read_setting(Gamma::Setting& set, int address_modifier) const;
  bool write_setting(Gamma::Setting& set, int address_modifier);

  uint16_t m_baseAddress;
  VmeController *m_controller;

  static uint16_t mirrorShort(uint16_t data);
  static uint32_t mirrorLong(uint32_t data);



  uint16_t readShort(int address) const;
  void    writeShort(int address, uint16_t data);
  uint16_t readShortBitfield(int address) const;
  void    writeShortBitfield(int address, uint16_t data);
  uint32_t readLong(int address) const;
  void    writeLong(int address, uint32_t data);
  uint32_t readLongBitfield(int address) const;
  void    writeLongBitfield(int address, uint32_t data);
  float   readFloat(int address) const;
  void    writeFloat(int address, float data);

};


}

#endif
