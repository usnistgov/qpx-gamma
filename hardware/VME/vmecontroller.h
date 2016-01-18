#ifndef VMECONTROLLER_H
#define VMECONTROLLER_H

#include <string>
#include <list>

enum class AddressModifier : int {
  A32_LockCommand                = 0x05,
  A32_NonprivBlockTransfer64     = 0x08,
  A32_NonprivData                = 0x09,
  A32_NonprivProgram             = 0x0A,
  A32_NonprivBlockTransfer       = 0x0B,
  A32_SupervisoryBlockTransfer64 = 0x0C,
  A32_SupervisoryData            = 0x0D,
  A32_SupervisoryProgram         = 0x0E,
  A32_SupervisoryBlockTransfer   = 0x0F,

  A16_Nonprivileged              = 0x29,
  A16_LockCommand                = 0x2C,
  A16_Supervisory                 = 0x2D
};


class VmeController
{
public:
  VmeController()	{}
  virtual ~VmeController() {}

  virtual std::string controllerName(void) { return "VmeController"; }
  virtual std::string serialNumber(void) { return std::string(); }

  virtual std::list<std::string> controllers(void) { return std::list<std::string>(); }
  virtual bool connect(int target) { return false; }
  virtual bool connect(std::string target) { return false; }
	virtual bool connected() { return false; }
  virtual void systemReset() = 0;

  virtual std::string address() const	{ return std::string();	}
  virtual bool setAddress(std::string address) { return false; }

//  virtual void setLiveInsertion(bool liveInsertion) {}
//	virtual bool liveInsertion() const { return false; }
//  virtual std::string busStatus() { return std::string(); }

  virtual std::list<std::string> moduleList() { return std::list<std::string>(); }

  virtual void writeShort(int vmeAddress, AddressModifier am, int data, int *errorCode = NULL) = 0;
  virtual int   readShort(int vmeAddress, AddressModifier am,           int *errorCode = NULL) = 0;

  virtual void writeLong(int vmeAddress, AddressModifier am, long data, int *errorCode = NULL) = 0;
  virtual long  readLong(int vmeAddress, AddressModifier am,            int *errorCode = NULL) = 0;

  virtual void writeRegister(long vmeAddress, long data, int *errorCode = NULL) = 0;
  virtual long readRegister(long vmeAddress, int *errorCode = NULL) = 0;

  virtual void start_daq() {}
  virtual void stop_daq() {}

  virtual void clear_registers() {}
  virtual void trigger_USB() {}
  virtual void scaler_dump() {}
  virtual void trigger_IRQ(short) {}

};

#endif // VMECONTROLLER_H
