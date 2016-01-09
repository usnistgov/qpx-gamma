#ifndef VMECONTROLLER_H
#define VMECONTROLLER_H

#include <stddef.h>
#include <string>
#include <list>

#include "basecontroller.h"

class VmeController : public BaseController
{
public:
	VmeController()
	{
	}

	virtual ~VmeController()
	{
	}

  virtual std::string errorString(void) = 0;
  virtual std::string controllerName(void) { return "VmeController"; }
  virtual std::string information(void) = 0;
	virtual void systemReset(int *errorCode = NULL) = 0;

	virtual bool connected() { return false; }

  virtual std::list<std::string> moduleList() { return std::list<std::string>(); }

	virtual void writeShort(int vmeAddress, int addressModifier, int data, int *errorCode = NULL) = 0;
	virtual int  readShort(int vmeAddress, int addressModifier, int *errorCode = NULL) = 0;
};

#endif // VMECONTROLLER_H
