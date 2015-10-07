#ifndef BASECONTROLLER_H
#define BASECONTROLLER_H

#include <string>
#include <list>

class BaseController
{
public:
	BaseController()
	{
	}

	virtual ~BaseController()
	{
	}

  virtual bool connect(std::string target)
	{
    //Q_UNUSED(target);

		return false;
	}

  virtual std::string address() const
	{
    return std::string();
	}

  virtual bool setAddress(std::string address)
	{
    //Q_UNUSED(address);

		return false;
	}

  virtual void setLiveInsertion(bool liveInsertion) {
    //Q_UNUSED(liveInsertion);
  }
	virtual bool liveInsertion() const { return false; }
  virtual std::string busStatus() { return std::string(); }

  virtual std::string errorString(void) = 0;
  virtual std::string controllerName(void) { return "BaseController"; } //QLatin1String
  virtual std::string information(void) = 0;

  virtual std::list<std::string> moduleList() = 0;

	virtual bool connected() { return false; }
};

#endif // BASECONTROLLER_H
