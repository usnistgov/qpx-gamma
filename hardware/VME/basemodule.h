#ifndef BASEMODULE_H
#define BASEMODULE_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

class BaseController;

class BaseModule
{

public:
  BaseModule() {}

  virtual ~BaseModule() {}

public:
  virtual bool    connected(void) = 0;

  virtual std::string firmwareName() {return m_firmwareName;}

  virtual std::string address() = 0;

  virtual uint16_t baseAddress() { return 0; }
  virtual void    setBaseAddress(uint16_t baseAddress) {}

  virtual void    programBaseAddress(uint16_t address) = 0;
  virtual uint16_t verifyBaseAddress(void) = 0;

protected:

  std::string m_firmwareName;

};

#endif // BASEMODULE_H
