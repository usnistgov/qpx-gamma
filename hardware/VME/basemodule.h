#ifndef BASEMODULE_H
#define BASEMODULE_H

//#include "../const.h"

//#include <QObject>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

class BaseController;

typedef enum EnumModuleConnectionState {
  Connecting,
  ConnectionEstablished,
  Connected,
  NotConnected
} ModuleConnectionState;

typedef enum CurrentMeasurementRange {
  HighRange = 0,
  LowRange = 1
} CurrentMeasurementRange;

typedef enum EnumIsegDeviceClasses
{
  V12C0  = 20,		// VHS 12 channel common ground VME
  E16D0  = 21,		// 71xxxx
  E24D1  = 21,
  H101C0 = 22,
  H201C0 = 23,
  E08C0  = 24,		// 73xxxx
  E08F0  = 25,		// 74xxxx
  E08F2  = 26,		// 72xxxx
  E08C2  = 27,		// 76xxxx
  E08B0  = 28,		// 77xxxx
  V24D0  = 29,		// VDS 24 channel distributor VME
  E01C0  = 30,
  E01C1  = 31,
  E01C2  = 32,
  N02C0  = 35,
  N02C1  = 36,
  N02C2  = 37,
  S02C2  = 38,
  N06C2  = 39,
  MICC   = 40,		// MMC Crate 4...10 Slots
  MICP   = 41,		// PHQ Interface 32 Channels
  H101C1 = 60,
  E16C1  = 70,		// 32 Channel Module
  L12C0  = 90			// 12 channel load unit
} IsegDeviceClasses;

typedef enum EnumOutputSupervisionActions {
  IgnoreFailure = 0,
  RampDownChannel = 1,
  ShutDownChannel = 2,
  ShutDownModule = 3
} OutputSupervisionAction;

typedef enum {
  Volt, KiloVolt
} EnumVoltageUnit;

typedef enum {
  NanoAmpere, MicroAmpere, MilliAmpere, Ampere
} EnumCurrentUnit;

class BaseModule //: public QObject
{
  //Q_OBJECT

public:
  BaseModule() :
    m_connectionState(Connecting),
    m_voltageUnit(Volt),
    m_voltagePrecision(1),
    m_currentUnit(MilliAmpere),
    m_currentPrecision(3)
  {
  }

  //	virtual BaseModule(BaseController *controller, int address)
  //	{
  //		//Q_UNUSED(controller);
  //		//Q_UNUSED(address);
  //	}

  //	virtual BaseModule(BaseController *controller, std::string path)
  //	{
  //		//Q_UNUSED(controller);
  //		//Q_UNUSED(path);
  //	}

  virtual ~BaseModule()
  {
    m_deviceClass = -1;
  }

//public slots:
public:
  virtual bool    connected(void) = 0;

  virtual std::string alarmMessage(void) = 0;

  virtual std::string firmwareName()
  {
    return m_firmwareName;
  }

  void setFirmwareName(int deviceClass)
  {
    if (deviceClass == 0 || !m_firmwareName.empty())
      return;

    switch (deviceClass) {
    case V12C0:  m_firmwareName = "V12C0"; break;
    case E16D0:  m_firmwareName = "E16D0"; break;
      //		case E24D1:  m_firmwareName = "E24D1"; break;
    case H101C0: m_firmwareName = "H101C0"; break;
    case H201C0: m_firmwareName = "H201C0"; break;
    case E08C0:  m_firmwareName = "E08C0"; break;
    case E08F0:  m_firmwareName = "E08F0"; break;
    case E08F2:  m_firmwareName = "E08F2"; break;
    case E08C2:  m_firmwareName = "E08C2"; break;
    case E08B0:  m_firmwareName = "E08B0"; break;
    case V24D0:  m_firmwareName = "V24D0"; break;
    case E01C0:  m_firmwareName = "E01C0"; break;
    case E01C1:  m_firmwareName = "E01C1"; break;
    case E01C2:  m_firmwareName = "E01C2"; break;
    case N02C0:  m_firmwareName = "N02C0"; break;
    case N02C1:  m_firmwareName = "N02C1"; break;
    case N02C2:  m_firmwareName = "N02C2"; break;
    case S02C2:  m_firmwareName = "S02C2"; break;
    case N06C2:  m_firmwareName = "N06C2"; break;
    case MICC:   m_firmwareName = "MICC"; break;
    case MICP:   m_firmwareName = "MICP"; break;
    case H101C1: m_firmwareName = "H101C1"; break;
    case E16C1:  m_firmwareName = "E16C1"; break;
    case L12C0:  m_firmwareName = "L12C0"; break;
    default:     m_firmwareName = std::to_string(m_deviceClass); break;
    }
  }

  int deviceClass()
  {
    return m_deviceClass;
  }

  void setDeviceClass(std::string firmwareName)
  {
    m_deviceClass = -1;

    if (firmwareName == "E08C0")
      m_deviceClass = E08C0;
    else if (firmwareName == "E08F0")
      m_deviceClass = E08F0;
    else if (firmwareName == "E08F2")
      m_deviceClass = E08F2;
    else if (firmwareName == "E08C2")
      m_deviceClass = E08C2;
    else if (firmwareName == "E08B0")
      m_deviceClass = E08B0;
    else if (firmwareName == "E01C0")
      m_deviceClass = E01C0;
    else if (firmwareName == "E01C1")
      m_deviceClass = E01C1;
    else if (firmwareName == "E01C2")
      m_deviceClass = E01C2;
    else if (firmwareName == "N02C0")
      m_deviceClass = N02C0;
    else if (firmwareName == "N02C1")
      m_deviceClass = N02C1;
    else if (firmwareName == "N02C2")
      m_deviceClass = N02C2;
    else if (firmwareName == "N06C2")
      m_deviceClass = N06C2;
    else if (firmwareName == "V12C0")
      m_deviceClass = V12C0;
    else if (firmwareName == "E16C1")
      m_deviceClass = E16C1;
    else if (firmwareName == "E16D0")
      m_deviceClass = E16D0;
    else if (firmwareName == "E24D1")
      m_deviceClass = E24D1;
    else if (firmwareName == "H101C0")
      m_deviceClass = H101C0;
    else if (firmwareName == "H201C0")
      m_deviceClass = H201C0;
    else if (firmwareName == "H101C1")
      m_deviceClass = H101C1;
    else if (firmwareName == "V24D0")
      m_deviceClass = V24D0;
    else if (firmwareName == "S02C2")
      m_deviceClass = S02C2;
    else if (firmwareName == "MICC")
      m_deviceClass = MICC;
    else if (firmwareName == "MICP")
      m_deviceClass = MICP;
    else if (firmwareName == "L12C0")
      m_deviceClass = L12C0;

  }

  ModuleConnectionState connectionState() const
  {
    return m_connectionState;
  }

  void setConnectionState(ModuleConnectionState value)
  {
    m_connectionState = value;
  }

  /**
   * @brief currentControl
   * @return true if the module has hardware current control, otherwise false
   */
  virtual bool currentControl() const
  {
    return true;
  }

  // --- Module commands ----------------------------------------------------
  virtual int     serialNumber(void) = 0;
  virtual std::string firmwareRelease(void) = 0;
  virtual uint32_t decoderRelease(void) = 0;

  virtual void    doClear(void) = 0;
  virtual void    setAdjustment(bool set)
  {
    //Q_UNUSED(set);
  }
  virtual bool    adjustment(void)
  {
    return false;
  }

  virtual std::string address() = 0;

  // for VME modules
  virtual uint16_t baseAddress()
  {
    return 0;
  }
  virtual void    setBaseAddress(uint16_t baseAddress)
  {
    //Q_UNUSED(baseAddress);
  }

  virtual float   temperature(int index = 0) = 0;
  virtual float   supplyP5(void) = 0;
  virtual float   supplyP12(void) = 0;
  virtual float   supplyN12(void) = 0;

  virtual uint32_t adcSamplesPerSecond(void) = 0;
  virtual void    setAdcSamplesPerSecond(uint32_t sps) = 0;
  virtual uint32_t digitalFilter(void) = 0;
  virtual void    setDigitalFilter(uint32_t filter) = 0;

  virtual float   voltageLimit(void) = 0;
  virtual float   currentLimit(void) = 0;

  virtual uint32_t status(void) = 0;
  virtual uint32_t eventStatus(void) = 0;
  virtual void    setEventStatus(uint32_t status) = 0;
  virtual void    clearEventStatus(void) = 0;
  virtual uint32_t eventChannelStatus(void) = 0;
  virtual void    clearEventChannelStatus(void) = 0;
  virtual uint32_t eventChannelMask(void) = 0;
  virtual void    setEventChannelMask(uint32_t mask) = 0;
  virtual uint32_t eventGroupStatus(void) = 0;
  virtual uint32_t eventGroupMask(void) = 0;
  virtual void    setEventGroupMask(uint32_t mask) = 0;
  virtual void    clearEventGroupStatus(void) = 0;
  virtual uint32_t eventMask(void) = 0;
  virtual void    setEventMask(uint32_t mask) = 0;
  virtual uint32_t control(void) = 0;
  virtual void    setControl(uint32_t control) = 0;

  virtual float   voltageRampSpeed(void) = 0;
  virtual void    setVoltageRampSpeed(float vramp) = 0;
  virtual float   currentRampSpeed(void) = 0;
  virtual void    setCurrentRampSpeed(float iramp) = 0;

  virtual bool    killEnable(void) = 0;
  virtual void    setKillEnable(bool enable) = 0;

  virtual bool    fineAdjustmentOn()
  {
    return true;
  }

  virtual void    setFineAdjustmentOn(bool on)
  {
    //Q_UNUSED(on);
  }

  virtual int     channelNumber(void) = 0;

  virtual uint32_t restartTime(void) = 0;
  virtual void    setRestartTime(uint32_t restartTime) = 0;

  virtual void    setSelectedChannelsOn(uint32_t mask) = 0;
  virtual void    setSelectedChannelsOff(uint32_t mask) = 0;
  virtual void    setSelectedChannelsEmergencyOff(uint32_t mask) = 0;
  virtual void    clearSelectedChannelsEmergencyOff(uint32_t mask) = 0;

  virtual void    setAllChannelsVoltageSet(float vset) = 0;
  virtual void    setAllChannelsCurrentSet(float iset) = 0;
  virtual void    setAllChannelsDelayedTripAction(OutputSupervisionAction action, int delay)
  {
    //Q_UNUSED(action);
    //Q_UNUSED(delay);
  }

  virtual void    setAllChannelsInhibitAction(OutputSupervisionAction action)
  {
    //Q_UNUSED(action);
  }

  virtual void    setChannelExternalMeasurementEnabled(int channel, bool enabled)
  {
    //Q_UNUSED(channel);
    //Q_UNUSED(enabled);
  }

  virtual void    setChannelRippleMeasurementEnabled(int channel, bool enabled)
  {
    //Q_UNUSED(channel);
    //Q_UNUSED(enabled);
  }

  virtual void    setChannelBothMeasurementsEnabled(int channel, bool enabled)
  {
    //Q_UNUSED(channel);
    //Q_UNUSED(enabled);
  }

  virtual void    doMultiplex()
  {
  }

  virtual void    setResistor(int resistor)
  {
    //Q_UNUSED(resistor);
  }

  virtual int     resistor()
  {
    return 0;
  }

  // --- Channel commands ---------------------------------------------------
  virtual uint32_t channelStatus(int channel) = 0;
  virtual uint32_t channelEventStatus(int channel) = 0;
  virtual void    setChannelEventStatus(int channel, uint32_t status) = 0;
  virtual void    clearChannelEventStatus(int channel) = 0;

  virtual uint32_t channelEventMask(int channel) = 0;
  virtual void    setChannelEventMask(int channel, uint32_t mask) = 0;
  virtual uint32_t channelControl(int channel) = 0;
  virtual void    setChannelControl(int channel, uint32_t control) = 0;

  virtual void    setChannelEmergencyOff(int channel) = 0;
  virtual void    clearChannelEmergencyOff(int channel) = 0;

  virtual float   channelVoltageNominal(int channel) = 0;
  virtual float   channelCurrentNominal(int channel) = 0;

  virtual float   channelVoltageSet(int channel) = 0;
  virtual void    setChannelVoltageSet(int channel, float vset) = 0;
  virtual float   channelVoltageMeasure(int channel) = 0;
  virtual float   channelCurrentSet(int channel) = 0;
  virtual void    setChannelCurrentSet(int channel, float iset) = 0;
  virtual bool    channelDelayedTripEnabled(int channel)
  {
    //Q_UNUSED(channel);
    return false;
  }
  virtual bool    channelDelayedTripRampDown(int channel)
  {
    //Q_UNUSED(channel);
    return false;
  }
  virtual int     channelDelayedTripTime(int channel)
  {
    //Q_UNUSED(channel);
    return 0;
  }
  virtual void    setChannelDelayedTripAction(int channel, OutputSupervisionAction action, int delay)
  {
    //Q_UNUSED(channel);
    //Q_UNUSED(action);
    //Q_UNUSED(delay);
  }

  virtual OutputSupervisionAction channelInhibitAction(int channel)
  {
    //Q_UNUSED(channel);

    return IgnoreFailure;
  }

  virtual void setChannelInhibitAction(int channel, OutputSupervisionAction action)
  {
    //Q_UNUSED(channel);
    //Q_UNUSED(action);
  }

  virtual float   channelCurrentMeasure(int channel) = 0;

  virtual CurrentMeasurementRange channelCurrentMeasurementRange(int channel)
  {
    //Q_UNUSED(channel);
    return HighRange;
  }

  virtual float   channelVoltageInterlockMaxSet(int channel) = 0;
  virtual void    setChannelVoltageInterlockMaxSet(int channel, float ilkmax) = 0;
  virtual float   channelVoltageInterlockMinSet(int channel) = 0;
  virtual void    setChannelVoltageInterlockMinSet(int channel, float ilkmin) = 0;

  virtual float   channelCurrentInterlockMaxSet(int channel) = 0;
  virtual void    setChannelCurrentInterlockMaxSet(int channel, float ilkmax) = 0;
  virtual float   channelCurrentInterlockMinSet(int channel) = 0;
  virtual void    setChannelCurrentInterlockMinSet(int channel, float ilkmin) = 0;

  virtual void    setChannelOn(int channel) = 0;
  virtual void    setChannelOff(int channel) = 0;

  // --- Convenience commands -----------------------------------------------
  virtual bool    useControllerUnits(void)
  {
    return false;
  }
  virtual void    setUseControllerUnits(bool controllerUnits)
  {
    //Q_UNUSED(controllerUnits);
  }

  virtual std::string formatVoltage(double value)
  {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(m_voltagePrecision) << value;
    return stream.str();
  }

  virtual std::string formatVoltageUnit(double value)
  {
    return formatVoltage(value) + voltageUnit(0);
  }

  virtual std::string voltageUnit(int channel) const
  {
    //Q_UNUSED(channel);

    std::string result("V");
    if (m_voltageUnit == KiloVolt)
      result = "kV";

    return result;
  }

  virtual void setVoltageUnit(std::string unit)
  {
    if (unit == "kV")
      m_voltageUnit = KiloVolt;
    else
      m_voltageUnit = Volt;
  }

  virtual int voltagePrecision(void) const
  {
    return m_voltagePrecision;
  }

  virtual void setVoltagePrecision(int precision)
  {
    m_voltagePrecision = precision;
  }

  virtual std::string formatCurrent(double value)
  {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(m_currentPrecision) << value;
    return stream.str();
  }

  virtual std::string formatCurrentUnit(double value)
  {
    return formatCurrent(value) + currentUnit(0);
  }

  virtual std::string currentUnit(int channel) const
  {
    //Q_UNUSED(channel);

    std::string result;

    switch (m_currentUnit) {
    case MilliAmpere: result = "mA"; break;
    case MicroAmpere: result = microAmpere();       break;
    case NanoAmpere:  result = "nA"; break;
    default:          result = "A";  break;
    }

    return result;
  }

  virtual void setCurrentUnit(std::string unit)
  {
    if (unit == "nA")
      m_currentUnit = NanoAmpere;
    else if (unit == "uA" || unit == microAmpere())
      m_currentUnit = MicroAmpere;
    else if (unit == "mA")
      m_currentUnit = MilliAmpere;
    else
      m_currentUnit = Ampere;
  }

  virtual int currentPrecision(void) const
  {
    return m_currentPrecision;
  }

  virtual void setCurrentPrecision(int precision)
  {
    m_currentPrecision = precision;
  }

  // --- Interlock Out control / status commands ----------------------------
  virtual uint32_t GetModuleIlkOutStatus(void) = 0;
  virtual uint32_t GetModuleIlkOutControl(void) = 0;
  virtual void    SetModuleIlkOutControl(uint32_t control) = 0;
  virtual uint32_t GetModuleIlkOutCount(void) = 0;
  virtual uint32_t GetModuleIlkOutLastTrigger(void) = 0;
  virtual uint32_t GetModuleIlkOutChnActualActive(void) = 0;
  virtual uint32_t GetModuleIlkOutChnEverTriggered(void) = 0;

  // --- Variable Groups ----------------------------------------------------
  virtual void    setFlexGroup(int group, uint32_t member, uint32_t type) = 0;
  virtual uint32_t flexGroupMemberList(int group) = 0;
  virtual uint32_t flexGroupType(int group) = 0;

  virtual float   channelVoltageHardwareNominal(int channel) = 0;
  virtual void    setChannelVoltageNominal(int channel, float maxset) = 0;
  virtual float   channelCurrentHardwareNominal(int channel) = 0;
  virtual void    setChannelCurrentNominal(int channel, float maxset) = 0;

  // --- Special Control (VME) -----------------------------------------------
  virtual void    setSpecialControlCommand(uint32_t command) = 0;
  virtual uint32_t specialControlCommand(void) = 0;
  virtual uint32_t specialControlStatus(void) = 0;
  virtual void    sendHexLine(std::vector<uint8_t> record) = 0;

  virtual void    programBaseAddress(uint16_t address) = 0;
  virtual uint16_t verifyBaseAddress(void) = 0;

protected:
  std::string microAmpere() const
  {
    std::string result = "\u03BCA";
    return result;
  }

  std::string m_firmwareName;
  int m_deviceClass;

  ModuleConnectionState m_connectionState;

  EnumVoltageUnit m_voltageUnit;
  int  m_voltagePrecision;
  EnumCurrentUnit m_currentUnit;
  int  m_currentPrecision;
};

#endif // BASEMODULE_H
