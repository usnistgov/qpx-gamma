#ifndef BASECRATE_H
#define BASECRATE_H

#include "basecontroller.h"
#include "basecontroller.h"

class BaseCrate
{
public:
	BaseCrate()
	{
	}

	virtual ~BaseCrate()
	{
	}

	virtual float   supply5V(void) = 0;
	virtual float   supply24V(void) = 0;
	virtual float   battery24V(void) = 0;
	virtual float   temperatureBackPlane(void) = 0;
	virtual float   temperaturePowerSupply(void) = 0;
	virtual int     fanSpeed(void) = 0;
	virtual void    setFanSpeed(int fanSpeed) = 0;
	virtual int     fanStageBackPlane(void) = 0;
	virtual int     fanStagePowerSupply(void) = 0;
	virtual int     status(void) = 0;
	virtual bool    acLineStatus(void) = 0;
	virtual bool    on(void) = 0;
	virtual void    setOn(bool on) = 0;
	virtual int     serialNumber(void) = 0;
  virtual std::string firmwareRelease(void) = 0;
	virtual int     bitRate(void) = 0;
	virtual bool    alive(void) = 0;
	virtual bool    alarm(void) = 0;
  virtual std::string alarmInformation(void) = 0;
	virtual float   alarmValue(void) = 0;

private:
};

#endif // BASECRATE_H
