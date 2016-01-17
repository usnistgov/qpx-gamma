#ifndef VMECONTROLLER_H
#define VMECONTROLLER_H

#include <string>
#include <list>


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
  virtual int systemReset() = 0;

  virtual std::string address() const	{ return std::string();	}
  virtual bool setAddress(std::string address) { return false; }

//  virtual void setLiveInsertion(bool liveInsertion) {}
//	virtual bool liveInsertion() const { return false; }
//  virtual std::string busStatus() { return std::string(); }

  virtual std::list<std::string> moduleList() { return std::list<std::string>(); }

  virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL) = 0;
	virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL) = 0;

  virtual void writeRegister(long vmeAddress, long data, int *errorCode = NULL) = 0;
  virtual long readRegister(long vmeAddress, int *errorCode = NULL) = 0;

};

#endif // VMECONTROLLER_H
