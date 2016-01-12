#ifndef VMECONTROLLER_H
#define VMECONTROLLER_H

#include <stddef.h>
#include <string>
#include <list>

#include "basecontroller.h"

class VmeController : public BaseController
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

  virtual std::list<std::string> moduleList() { return std::list<std::string>(); }
	virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL) = 0;
	virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL) = 0;
};

#endif // VMECONTROLLER_H
