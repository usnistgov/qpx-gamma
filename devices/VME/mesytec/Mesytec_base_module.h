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
 *      MesytecVME
 *
 ******************************************************************************/

#ifndef MESYTEC_BASE_MODULE
#define MESYTEC_BASE_MODULE

#include "vmemodule.h"

#define MESYTEC_ADDRESS_SPACE_LENGTH				0x10000000

static const AddressModifier initamod = AddressModifier::A32_UserData;   //  setup using user data access.
static const AddressModifier readamod = AddressModifier::A32_UserBlock;  //  Read in block mode.
static const AddressModifier cbltamod = AddressModifier::A32_UserBlock;
static const AddressModifier mcstamod = AddressModifier::A32_UserData;

namespace Qpx {

class MesytecExternal;

class MesytecVME : public VmeModule {
  
public:
  MesytecVME();
  ~MesytecVME();

  bool boot() override;
  bool die() override;
  bool write_settings_bulk(Gamma::Setting &set) override;
  bool read_settings_bulk(Gamma::Setting &set) const override;

  //VmeModule
  uint32_t baseAddressSpaceLength() const override { return MESYTEC_ADDRESS_SPACE_LENGTH; }
  bool connected() const override;
  std::string firmwareName() const;

  //MesytecRC
  bool RC_wait(double millisex = 5.0) const;
  bool RC_get_ID(uint16_t module, uint16_t &data) const;
  bool RC_on(uint16_t module);
  bool RC_off(uint16_t module);
  bool RC_read(uint16_t module, uint16_t setting, uint16_t &data) const;
  bool RC_write(uint16_t module, uint16_t setting, uint16_t data);

private:
  //no copying
  void operator=(MesytecVME const&);
  MesytecVME(const MesytecVME&);

protected:
  uint16_t module_firmware_code_;

  std::map<std::string, MesytecExternal*> ext_modules_;

  virtual void rebuild_structure(Gamma::Setting &set) {}
  bool read_setting(Gamma::Setting& set) const;
  bool write_setting(Gamma::Setting& set);

  uint16_t readShort(int address) const;
  void    writeShort(int address, uint16_t data) const;  //const?
  float   readFloat(int address) const;
  void    writeFloat(int address, float data) const; //const?

  bool      rc_bus;
  uint16_t  rc_busno,
            rc_modnum,
            rc_opcode,
            rc_opcode_on,
            rc_opcode_off,
            rc_opcode_read_id,
            rc_opcode_read_data,
            rc_opcode_write_data,
            rc_adr,
            rc_dat,
            rc_return_status,
            rc_return_status_active_mask,
            rc_return_status_collision_mask,
            rc_return_status_no_response_mask;
};


}

#endif
