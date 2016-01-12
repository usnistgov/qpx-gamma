#ifndef BASECONTROLLER_H
#define BASECONTROLLER_H

#include <string>
#include <list>

class BaseController
{
public:
  BaseController() {}
  virtual ~BaseController() {}

  virtual std::string controllerName(void) { return "BaseController"; }
  virtual std::string serialNumber(void) { return std::string(); }

  virtual bool connect(int target) { return false; }
  virtual bool connect(std::string target) { return false; }
  virtual bool connected() { return false; }



  virtual std::string address() const	{ return std::string();	}
  virtual bool setAddress(std::string address) { return false; }

  virtual void setLiveInsertion(bool liveInsertion) {}
	virtual bool liveInsertion() const { return false; }
  virtual std::string busStatus() { return std::string(); }


  virtual std::list<std::string> moduleList() = 0;

};

#endif // BASECONTROLLER_H
