#ifndef VME_H
#define VME_H

enum class AddressModifier : uint8_t {
  A16_Lock         = 0x2C,
  A16_User         = 0x29,
  A16_Priv         = 0x2D,

  A24_Lock         = 0x32,
  A24_UserData     = 0x39,
  A24_UserProgram  = 0x3A,
  A24_UserBlock    = 0x3B,
  A24_UserBlock64  = 0x38,

  A24_PrivData     = 0x3D,
  A24_PrivProgram  = 0x3E,
  A24_PrivBlock    = 0x3F,
  A24_PrivBlock64  = 0x3C,

  A32_Lock         = 0x05,
  A32_UserData     = 0x09,
  A32_UserProgram  = 0x0A,
  A32_UserBlock    = 0x0B,
  A32_UserBlock64  = 0x08,
  
  A32_PrivData     = 0x0D,
  A32_PrivProgram  = 0x0E,
  A32_PrivBlock    = 0x0F,
  A32_PrivBlock64  = 0x0C,

  A64_Lock         = 0x04,
  A64_SingleAccess = 0x01,
  A64_Block        = 0x03,
  A64_Block64      = 0x00,
};

#endif