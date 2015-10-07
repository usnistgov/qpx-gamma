#ifndef VMEMODULE_H
#define VMEMODULE_H

#include "basemodule.h"
#include "vmecontroller.h"
#include "const.h"


#define ISEG_VENDOR_ID									0x69736567
#define VHS_ADDRESS_SPACE_LENGTH						0x0400
#define VDS_ADDRESS_SPACE_LENGTH						0x0800
#define VME_ADDRESS_MODIFIER							0x29

class VmeModule : public BaseModule
{
  //Q_OBJECT

public:
	VmeModule(BaseController *controller, int baseAddress);
	~VmeModule();

//public slots:
	bool    connected(void);

  std::string alarmMessage(void) { return std::string(); }
	bool currentControl() const
	{
		return m_deviceClass == V12C0;
	}

	// --- Module commands ----------------------------------------------------
	int     deviceClass(void);
//	uint32_t GetModulePlacedChannels(void);
	int     serialNumber(void);
  std::string firmwareRelease(void);
  uint32_t decoderRelease(void);

	void    doClear(void);
  std::string address() {   std::stringstream stream;
                            stream << "VME BA 0x"
                                   << std::setfill ('0') << std::setw(sizeof(uint16_t)*2)
                                   << std::hex << m_baseAddress;
                            return stream.str();}
  uint16_t baseAddress() { return m_baseAddress; }
  void    setBaseAddress(uint16_t baseAddress);

	float   temperature(int index = 0);
	float   supplyP5(void);
	float   supplyP12(void);
	float   supplyN12(void);

  uint32_t adcSamplesPerSecond(void);
  void    setAdcSamplesPerSecond(uint32_t sps);
  uint32_t digitalFilter(void);
  void    setDigitalFilter(uint32_t filter);

	float   voltageLimit(void);
	float   currentLimit(void);

  uint32_t status(void);
  uint32_t eventStatus(void);
  void    setEventStatus(uint32_t status);
	void    clearEventStatus(void) { setEventStatus(0xFFFFFFFF); }
  uint32_t eventChannelStatus(void);
	void    clearEventChannelStatus(void);
  uint32_t eventChannelMask(void);
  void    setEventChannelMask(uint32_t mask);
  uint32_t eventGroupStatus(void);
  uint32_t eventGroupMask(void);
  void    setEventGroupMask(uint32_t mask);
	void    clearEventGroupStatus(void);
  uint32_t eventMask(void);
  void    setEventMask(uint32_t mask);
  uint32_t control(void);
  void    setControl(uint32_t control);

	float   voltageRampSpeed(void);
	void    setVoltageRampSpeed(float vramp);
	float   currentRampSpeed(void);
	void    setCurrentRampSpeed(float iramp);

	bool    killEnable(void);
	void    setKillEnable(bool enable);

	int     channelNumber(void);

  uint32_t restartTime(void);
  void    setRestartTime(uint32_t restartTime);

  void    setSelectedChannelsOn(uint32_t mask);
  void    setSelectedChannelsOff(uint32_t mask);
  void    setSelectedChannelsEmergencyOff(uint32_t mask);
  void    clearSelectedChannelsEmergencyOff(uint32_t mask);

	void    setAllChannelsVoltageSet(float vset);
	void    setAllChannelsCurrentSet(float iset);

	// --- Channel commands ---------------------------------------------------
  uint32_t channelStatus(int channel);
  uint32_t channelEventStatus(int channel);
  void    setChannelEventStatus(int channel, uint32_t status);
	void    clearChannelEventStatus(int channel) { setChannelEventStatus(channel, 0xFFFFFFFF); }

  uint32_t channelEventMask(int channel);
  void    setChannelEventMask(int channel, uint32_t mask);
  uint32_t channelControl(int channel);
  void    setChannelControl(int channel, uint32_t control);

	void    setChannelEmergencyOff(int channel);
	void    clearChannelEmergencyOff(int channel);

	float   channelVoltageNominal(int channel);
	float   channelCurrentNominal(int channel);

	float   channelVoltageSet(int channel);
	void    setChannelVoltageSet(int channel, float vset);
	float   channelVoltageMeasure(int channel);
	float   channelCurrentSet(int channel);
	void    setChannelCurrentSet(int channel, float iset);
	float   channelCurrentMeasure(int channel);
	bool    channelDelayedTripEnabled(int channel);
	bool    channelDelayedTripRampDown(int channel);
	int     channelDelayedTripTime(int channel);
	void    setChannelDelayedTripAction(int channel, OutputSupervisionAction action, int delay);

	float   channelVoltageInterlockMaxSet(int channel);
	void    setChannelVoltageInterlockMaxSet(int channel, float ilkmax);
	float   channelVoltageInterlockMinSet(int channel);
	void    setChannelVoltageInterlockMinSet(int channel, float ilkmin);

	float   channelCurrentInterlockMaxSet(int channel);
	void    setChannelCurrentInterlockMaxSet(int channel, float ilkmax);
	float   channelCurrentInterlockMinSet(int channel);
	void    setChannelCurrentInterlockMinSet(int channel, float ilkmin);

	void    setChannelOn(int channel);
	void    setChannelOff(int channel);

	// --- Interlock Out control / status commands ----------------------------
  uint32_t GetModuleIlkOutStatus(void);
  uint32_t GetModuleIlkOutControl(void);
  void    SetModuleIlkOutControl(uint32_t control);
  uint32_t GetModuleIlkOutCount(void);
  uint32_t GetModuleIlkOutLastTrigger(void);
  uint32_t GetModuleIlkOutChnActualActive(void);
  uint32_t GetModuleIlkOutChnEverTriggered(void);

	// --- Variable Groups ----------------------------------------------------
  uint32_t flexGroupMemberList(int group);
  uint32_t flexGroupType(int group);
  void    setFlexGroup(int group, uint32_t member, uint32_t type);

	float   channelVoltageHardwareNominal(int channel);
	void    setChannelVoltageNominal(int channel, float maxset);
	float   channelCurrentHardwareNominal(int channel);
	void    setChannelCurrentNominal(int channel, float maxset);

	// --- Special Control ----------------------------------------------------
  void    setSpecialControlCommand(uint32_t command);
  uint32_t specialControlCommand(void);
  uint32_t specialControlStatus(void);
  void    sendHexLine(std::vector<uint8_t> record);

  void    programBaseAddress(uint16_t address);
  uint16_t verifyBaseAddress(void);

private:
  uint16_t readShort(int address);
  void    writeShort(int address, uint16_t data);
  uint16_t readShortBitfield(int address);
  void    writeShortBitfield(int address, uint16_t data);
  uint32_t readLong(int address);
  void    writeLong(int address, uint32_t data);
  uint32_t readLongBitfield(int address);
  void    writeLongBitfield(int address, uint32_t data);
	float   readFloat(int address);
	void    writeFloat(int address, float data);

  uint16_t mirrorShort(uint16_t data);
  uint32_t mirrorLong(uint32_t data);

	float   siVoltageToUserVoltage(float value);
	float   userVoltageToSiVoltage(float value);
	float   siCurrentToUserCurrent(float siCurrent);
	float   userCurrentToSiCurrent(float userCurrent);

  uint32_t m_vendorId;
  uint16_t m_baseAddress;

  uint16_t m_channelOffset[MAX_CHANNELS];
  uint16_t m_voltageMaxSetOffset[MAX_CHANNELS];
  uint16_t m_currentMaxSetOffset[MAX_CHANNELS];
  uint16_t m_flexGroupOffset[MAX_GROUPS];

	int     m_channelNumber;

	VmeController *m_controller;
};

// --- Special Mode ------------------------------------------------------------
#define SPECIALCONTROL_COMMAND_STANDBY					0x0001
#define SPECIALCONTROL_COMMAND_DATASENT					0x0002
#define SPECIALCONTROL_COMMAND_WRITE					0x0003
#define SPECIALCONTROL_COMMAND_COMPARE					0x0004
#define SPECIALCONTROL_COMMAND_COMPAREAFTERWRITESTATE	0x0005
#define SPECIALCONTROL_COMMAND_POSTFLASH				0x0006
#define SPECIALCONTROL_COMMAND_ENDSPECIAL				0x0007
#define SPECIALCONTROL_COMMAND_RESTART					0x0009
#define SPECIALCONTROL_COMMAND_SYNC						0x000B
#define SPECIALCONTROL_COMMAND_LIVE_INSERTION			0x000C
#define SPECIALCONTROL_COMMAND_FPGA_UPDATE				0x000D
#define SPECIALCONTROL_COMMAND_S12X_UPDATE				0x000E

#define SPECIALCONTROL_COMMAND_EEPROM_ACCESS			0x0801

#define SPECIALCONTROL_STATUS_READY						0x0081
#define SPECIALCONTROL_STATUS_BUSY						0x0082
#define SPECIALCONTROL_STATUS_ACKNOWLEDGE				0x0084
#define SPECIALCONTROL_STATUS_ERROR						0x0090
#define SPECIALCONTROL_STATUS_ERROR_CHKSUM				0x0092
#define SPECIALCONTROL_STATUS_ERROR_COMPARE				0x0096
#define SPECIALCONTROL_STATUS_ERROR_HVON				0x009a
#define SPECIALCONTROL_STATUS_NOADDR					0x00a0
#define SPECIALCONTROL_STATUS_WRONGADDR					0x00a1
#define SPECIALCONTROL_STATUS_DIFF						0x00a2

// --- VHS12 ------------------------------------------------------------------
#define VhsModuleStatusOFFSET						0
#define VhsModuleControlOFFSET						2
#define VhsModuleEventStatusOFFSET					4
#define VhsModuleEventMaskOFFSET					6
#define VhsModuleEventChannelStatusOFFSET			8
#define VhsModuleEventChannelMaskOFFSET				10
#define VhsModuleEventGroupStatusOFFSET				12
#define VhsModuleEventGroupMaskOFFSET				16

#define VhsVoltageRampSpeedOFFSET					20
#define VhsCurrentRampSpeedOFFSET					24
#define VhsVoltageMaxOFFSET							28
#define VhsCurrentMaxOFFSET							32
#define VhsSupplyP5OFFSET							36
#define VhsSupplyP12OFFSET							40
#define VhsSupplyN12OFFSET							44
#define VhsTemperatureOFFSET						48

#define VhsSerialNumberOFFSET						52
#define VhsFirmwareReleaseOFFSET					56
#define VhsPlacedChannelsOFFSET						60
#define VhsDeviceClassOFFSET						62

#define VhsModuleIlkOutStatusOFFSET					64
#define VhsModuleIlkOutControlOFFSET				66
#define VhsModuleIlkCountOFFSET						68
#define VhsModuleIlkLastTriggerOFFSET				70
#define VhsModuleIlkChnActualActiveOFFSET			72
#define VhsModuleIlkChnEverTriggeredOFFSET			74

#define VhsRestartTimeOFFSET						80
#define VhsADCSamplesPerSecondOFFSET				88
#define VhsDigitalFilterOFFSET						90
#define VhsVendorIdOFFSET							92

#define VhsChannel00OFFSET							0x0060
#define VhsChannel01OFFSET							VhsChannel00OFFSET + 0x30
#define VhsChannel02OFFSET							VhsChannel01OFFSET + 0x30
#define VhsChannel03OFFSET							VhsChannel02OFFSET + 0x30
#define VhsChannel04OFFSET							VhsChannel03OFFSET + 0x30
#define VhsChannel05OFFSET							VhsChannel04OFFSET + 0x30
#define VhsChannel06OFFSET							VhsChannel05OFFSET + 0x30
#define VhsChannel07OFFSET							VhsChannel06OFFSET + 0x30
#define VhsChannel08OFFSET							VhsChannel07OFFSET + 0x30
#define VhsChannel09OFFSET							VhsChannel08OFFSET + 0x30
#define VhsChannel10OFFSET							VhsChannel09OFFSET + 0x30
#define VhsChannel11OFFSET							VhsChannel10OFFSET + 0x30

#define VhsChannelStatusOFFSET						0
#define VhsChannelControlOFFSET						2
#define VhsChannelEventStatusOFFSET					4
#define VhsChannelEventMaskOFFSET					6
#define VhsChannelVoltageSetOFFSET					8
#define VhsChannelCurrentSetOFFSET					12
#define VhsChannelVoltageMeasureOFFSET				16
#define VhsChannelCurrentMeasureOFFSET				20
#define VhsChannelVoltageBoundsOFFSET				24
#define VhsChannelVoltageIlkMaxSetOFFSET			24
#define VhsChannelCurrentBoundsOFFSET				28
#define VhsChannelCurrentIlkMaxSetOFFSET			28
#define VhsChannelVoltageNominalOFFSET				32
#define VhsChannelCurrentNominalOFFSET				36
#define VhsChannelVoltageIlkMinSetOFFSET			40
#define VhsChannelCurrentIlkMinSetOFFSET			44

#define VhsFixGroupOFFSET							0x02a0
#define VhsSetVoltageAllChannelsOFFSET				VhsFixGroupOFFSET
#define VhsSetCurrentAllChannelsOFFSET				(VhsSetVoltageAllChannelsOFFSET+4)
#define VhsSetVoltageBoundsAllChannelsOFFSET		(VhsSetCurrentAllChannelsOFFSET+4)
#define VhsSetCurrentBoundsAllChannelsOFFSET		(VhsSetVoltageBoundsAllChannelsOFFSET+4)
#define VhsSetEmergencyAllChannelsOFFSET			(VhsSetCurrentBoundsAllChannelsOFFSET+4)
#define VhsSetOnOffAllChannelsOFFSET				(VhsSetEmergencyAllChannelsOFFSET+4)
#define VhsSetVoltageMaxAllChannelsOFFSET			(VdsSetOnOffAllChannelsOFFSET+4)
#define VhsSetCurrentMaxAllChannelsOFFSET			(VdsSetVoltageMaxAllChannelsOFFSET+4)

#define VhsFlexGroupOFFSET							0x02c0
#define VhsFlexGroupMemberOFFSET					0
#define VhsFlexGroupTypeOFFSET						2
#define VhsFlexGroup00OFFSET						VhsFlexGroupOFFSET
#define VhsFlexGroup01OFFSET						VhsFlexGroup00OFFSET + 4
#define VhsFlexGroup02OFFSET						VhsFlexGroup01OFFSET + 4
#define VhsFlexGroup03OFFSET						VhsFlexGroup02OFFSET + 4
#define VhsFlexGroup04OFFSET						VhsFlexGroup03OFFSET + 4
#define VhsFlexGroup05OFFSET						VhsFlexGroup04OFFSET + 4
#define VhsFlexGroup06OFFSET						VhsFlexGroup05OFFSET + 4
#define VhsFlexGroup07OFFSET						VhsFlexGroup06OFFSET + 4
#define VhsFlexGroup08OFFSET						VhsFlexGroup07OFFSET + 4
#define VhsFlexGroup09OFFSET						VhsFlexGroup08OFFSET + 4
#define VhsFlexGroup10OFFSET						VhsFlexGroup09OFFSET + 4
#define VhsFlexGroup11OFFSET						VhsFlexGroup10OFFSET + 4
#define VhsFlexGroup12OFFSET						VhsFlexGroup11OFFSET + 4
#define VhsFlexGroup13OFFSET						VhsFlexGroup12OFFSET + 4
#define VhsFlexGroup14OFFSET						VhsFlexGroup13OFFSET + 4
#define VhsFlexGroup15OFFSET						VhsFlexGroup14OFFSET + 4
#define VhsFlexGroup16OFFSET						VhsFlexGroup15OFFSET + 4
#define VhsFlexGroup17OFFSET						VhsFlexGroup16OFFSET + 4
#define VhsFlexGroup18OFFSET						VhsFlexGroup17OFFSET + 4
#define VhsFlexGroup19OFFSET						VhsFlexGroup18OFFSET + 4
#define VhsFlexGroup20OFFSET						VhsFlexGroup19OFFSET + 4
#define VhsFlexGroup21OFFSET						VhsFlexGroup20OFFSET + 4
#define VhsFlexGroup22OFFSET						VhsFlexGroup21OFFSET + 4
#define VhsFlexGroup23OFFSET						VhsFlexGroup22OFFSET + 4
#define VhsFlexGroup24OFFSET						VhsFlexGroup23OFFSET + 4
#define VhsFlexGroup25OFFSET						VhsFlexGroup24OFFSET + 4
#define VhsFlexGroup26OFFSET						VhsFlexGroup25OFFSET + 4
#define VhsFlexGroup27OFFSET						VhsFlexGroup26OFFSET + 4
#define VhsFlexGroup28OFFSET						VhsFlexGroup27OFFSET + 4
#define VhsFlexGroup29OFFSET						VhsFlexGroup28OFFSET + 4
#define VhsFlexGroup30OFFSET						VhsFlexGroup29OFFSET + 4
#define VhsFlexGroup31OFFSET						VhsFlexGroup30OFFSET + 4

// VHS Voltage Max Set
#define VhsVoltageMaxSet00OFFSET					0x340
#define VhsVoltageMaxSet01OFFSET					VhsVoltageMaxSet00OFFSET + 8
#define VhsVoltageMaxSet02OFFSET					VhsVoltageMaxSet01OFFSET + 8
#define VhsVoltageMaxSet03OFFSET					VhsVoltageMaxSet02OFFSET + 8
#define VhsVoltageMaxSet04OFFSET					VhsVoltageMaxSet03OFFSET + 8
#define VhsVoltageMaxSet05OFFSET					VhsVoltageMaxSet04OFFSET + 8
#define VhsVoltageMaxSet06OFFSET					VhsVoltageMaxSet05OFFSET + 8
#define VhsVoltageMaxSet07OFFSET					VhsVoltageMaxSet06OFFSET + 8
#define VhsVoltageMaxSet08OFFSET					VhsVoltageMaxSet07OFFSET + 8
#define VhsVoltageMaxSet09OFFSET					VhsVoltageMaxSet08OFFSET + 8
#define VhsVoltageMaxSet10OFFSET					VhsVoltageMaxSet09OFFSET + 8
#define VhsVoltageMaxSet11OFFSET					VhsVoltageMaxSet10OFFSET + 8

// VHS Current Max Set
#define VhsCurrentMaxSet00OFFSET					0x344
#define VhsCurrentMaxSet01OFFSET					VhsCurrentMaxSet00OFFSET + 8
#define VhsCurrentMaxSet02OFFSET					VhsCurrentMaxSet01OFFSET + 8
#define VhsCurrentMaxSet03OFFSET					VhsCurrentMaxSet02OFFSET + 8
#define VhsCurrentMaxSet04OFFSET					VhsCurrentMaxSet03OFFSET + 8
#define VhsCurrentMaxSet05OFFSET					VhsCurrentMaxSet04OFFSET + 8
#define VhsCurrentMaxSet06OFFSET					VhsCurrentMaxSet05OFFSET + 8
#define VhsCurrentMaxSet07OFFSET					VhsCurrentMaxSet06OFFSET + 8
#define VhsCurrentMaxSet08OFFSET					VhsCurrentMaxSet07OFFSET + 8
#define VhsCurrentMaxSet09OFFSET					VhsCurrentMaxSet08OFFSET + 8
#define VhsCurrentMaxSet10OFFSET					VhsCurrentMaxSet09OFFSET + 8
#define VhsCurrentMaxSet11OFFSET					VhsCurrentMaxSet10OFFSET + 8

#define VhsNewBaseAddressOFFSET						0x03A0
#define VhsNewBaseAddressXorOFFSET					0x03A2
#define VhsOldBaseAddressOFFSET						0x03A4
#define VhsNewBaseAddressAcceptedOFFSET				0x03A6
#define VhsSpecialControlStatusOFFSET				0x03B0
#define VhsSpecialControlCommandOFFSET				0x03B2
#define VhsSpecialControlPageOFFSET					0x03B4
#define VhsSpecialControlAddressOFFSET				0x03B6
#define VhsSpecialControlReadDataOFFSET				0x03B8
#define VhsSpecialControlWriteDataOFFSET			0x03B8

// --- VDS24 ------------------------------------------------------------------
#define VdsModuleStatusOFFSET						0
#define VdsModuleEventStatusOFFSET					4
#define VdsModuleEventMaskOFFSET					8
#define VdsModuleControlOFFSET						12

#define VdsModuleEventChannelStatusOFFSET			16
#define VdsModuleEventChannelMaskOFFSET				20
#define VdsModuleEventGroupStatusOFFSET				24
#define VdsModuleEventGroupMaskOFFSET				28

#define VdsVoltageRampSpeedOFFSET					32
#define VdsCurrentRampSpeedOFFSET					36
#define VdsVoltageMaxOFFSET							40
#define VdsCurrentMaxOFFSET							44
#define VdsSerialNumberOFFSET						48
#define VdsFirmwareReleaseOFFSET					52
#define VdsPlacedChannelOFFSET						56
#define VdsChannelNumberOFFSET						60
#define VdsDeviceClassOFFSET						62
#define VdsSupplyP5OFFSET							64
#define VdsSupplyP12OFFSET							68
#define VdsSupplyN12OFFSET							72
#define VdsTemperatureOFFSET						76
#define VdsRestartTimeOFFSET						80
#define VdsADCSamplesPerSecondOFFSET				84
#define VdsDigitalFilterOFFSET						88
#define VdsVendorIdOFFSET							92

#define VdsDecoderReleaseOFFSET						124
#define VdsBootLoaderReleaseOFFSET					126



#define VdsNewBaseAddressOFFSET						0x00A0
#define VdsNewBaseAddressXorOFFSET					0x00A2
#define VdsOldBaseAddressOFFSET						0x00A4
#define VdsNewBaseAddressAcceptedOFFSET				0x00A8
#define VdsSpecialControlStatusOFFSET				0x00AC
#define VdsSpecialControlCommandOFFSET				0x00B0

/// FIXME add addresses
#define VdsSpecialControlPageOFFSET					0xFFFF
#define VdsSpecialControlAddressOFFSET				0xFFFF
#define VdsSpecialControlReadDataOFFSET				0xFFFF
#define VdsSpecialControlWriteDataOFFSET			0xFFFF

#define VdsModuleIlkOutStatusOFFSET					0xFFFF
#define VdsModuleIlkOutControlOFFSET				0xFFFF
#define VdsModuleIlkCountOFFSET						0xFFFF
#define VdsModuleIlkLastTriggerOFFSET				0xFFFF
#define VdsModuleIlkChnActualActiveOFFSET			0xFFFF
#define VdsModuleIlkChnEverTriggeredOFFSET			0xFFFF

#define VdsChannel00OFFSET							0x0100
#define VdsChannel01OFFSET							VdsChannel00OFFSET + 0x40
#define VdsChannel02OFFSET							VdsChannel01OFFSET + 0x40
#define VdsChannel03OFFSET							VdsChannel02OFFSET + 0x40
#define VdsChannel04OFFSET							VdsChannel03OFFSET + 0x40
#define VdsChannel05OFFSET							VdsChannel04OFFSET + 0x40
#define VdsChannel06OFFSET							VdsChannel05OFFSET + 0x40
#define VdsChannel07OFFSET							VdsChannel06OFFSET + 0x40
#define VdsChannel08OFFSET							VdsChannel07OFFSET + 0x40
#define VdsChannel09OFFSET							VdsChannel08OFFSET + 0x40
#define VdsChannel10OFFSET							VdsChannel09OFFSET + 0x40
#define VdsChannel11OFFSET							VdsChannel10OFFSET + 0x40
#define VdsChannel12OFFSET							VdsChannel11OFFSET + 0x40
#define VdsChannel13OFFSET							VdsChannel12OFFSET + 0x40
#define VdsChannel14OFFSET							VdsChannel13OFFSET + 0x40
#define VdsChannel15OFFSET							VdsChannel14OFFSET + 0x40
#define VdsChannel16OFFSET							VdsChannel15OFFSET + 0x40
#define VdsChannel17OFFSET							VdsChannel16OFFSET + 0x40
#define VdsChannel18OFFSET							VdsChannel17OFFSET + 0x40
#define VdsChannel19OFFSET							VdsChannel18OFFSET + 0x40
#define VdsChannel20OFFSET							VdsChannel19OFFSET + 0x40
#define VdsChannel21OFFSET							VdsChannel20OFFSET + 0x40
#define VdsChannel22OFFSET							VdsChannel21OFFSET + 0x40
#define VdsChannel23OFFSET							VdsChannel22OFFSET + 0x40

#define VdsChannelStatusOFFSET						0
#define VdsChannelEventStatusOFFSET					4
#define VdsChannelEventMaskOFFSET					8
#define VdsChannelControlOFFSET						12

#define VdsChannelVoltageSetOFFSET					16
#define VdsChannelCurrentSetOFFSET					20
#define VdsChannelVoltageMeasureOFFSET				24
#define VdsChannelCurrentMeasureOFFSET				28
#define VdsChannelVoltageIlkMaxSetOFFSET			32
#define VdsChannelVoltageIlkMinSetOFFSET			36
#define VdsChannelCurrentIlkMaxSetOFFSET			40
#define VdsChannelCurrentIlkMinSetOFFSET			44
#define VdsChannelVoltageNominalOFFSET				48
#define VdsChannelCurrentNominalOFFSET				52

#define VdsAddrFixGroupsOFFSET						0x00d0

#define VdsFlexGroupOFFSET							0x0700
#define VdsFlexGroupMember2OFFSET					0
#define VdsFlexGroupMember1OFFSET					2
#define VdsFlexGroupType2OFFSET						4
#define VdsFlexGroupType1OFFSET						6
#define VdsFlexGroup00OFFSET						VdsFlexGroupOFFSET
#define VdsFlexGroup01OFFSET						VdsFlexGroup00OFFSET + 8
#define VdsFlexGroup02OFFSET						VdsFlexGroup01OFFSET + 8
#define VdsFlexGroup03OFFSET						VdsFlexGroup02OFFSET + 8
#define VdsFlexGroup04OFFSET						VdsFlexGroup03OFFSET + 8
#define VdsFlexGroup05OFFSET						VdsFlexGroup04OFFSET + 8
#define VdsFlexGroup06OFFSET						VdsFlexGroup05OFFSET + 8
#define VdsFlexGroup07OFFSET						VdsFlexGroup06OFFSET + 8
#define VdsFlexGroup08OFFSET						VdsFlexGroup07OFFSET + 8
#define VdsFlexGroup09OFFSET						VdsFlexGroup08OFFSET + 8
#define VdsFlexGroup10OFFSET						VdsFlexGroup09OFFSET + 8
#define VdsFlexGroup11OFFSET						VdsFlexGroup10OFFSET + 8
#define VdsFlexGroup12OFFSET						VdsFlexGroup11OFFSET + 8
#define VdsFlexGroup13OFFSET						VdsFlexGroup12OFFSET + 8
#define VdsFlexGroup14OFFSET						VdsFlexGroup13OFFSET + 8
#define VdsFlexGroup15OFFSET						VdsFlexGroup14OFFSET + 8
#define VdsFlexGroup16OFFSET						VdsFlexGroup15OFFSET + 8
#define VdsFlexGroup17OFFSET						VdsFlexGroup16OFFSET + 8
#define VdsFlexGroup18OFFSET						VdsFlexGroup17OFFSET + 8
#define VdsFlexGroup19OFFSET						VdsFlexGroup18OFFSET + 8
#define VdsFlexGroup20OFFSET						VdsFlexGroup19OFFSET + 8
#define VdsFlexGroup21OFFSET						VdsFlexGroup20OFFSET + 8
#define VdsFlexGroup22OFFSET						VdsFlexGroup21OFFSET + 8
#define VdsFlexGroup23OFFSET						VdsFlexGroup22OFFSET + 8
#define VdsFlexGroup24OFFSET						VdsFlexGroup23OFFSET + 8
#define VdsFlexGroup25OFFSET						VdsFlexGroup24OFFSET + 8
#define VdsFlexGroup26OFFSET						VdsFlexGroup25OFFSET + 8
#define VdsFlexGroup27OFFSET						VdsFlexGroup26OFFSET + 8
#define VdsFlexGroup28OFFSET						VdsFlexGroup27OFFSET + 8
#define VdsFlexGroup29OFFSET						VdsFlexGroup28OFFSET + 8
#define VdsFlexGroup30OFFSET						VdsFlexGroup29OFFSET + 8
#define VdsFlexGroup31OFFSET						VdsFlexGroup30OFFSET + 8

// === In VDS, Groups and Voltage/CurrentMaxSet are multiplexed ===============

// VDS Voltage Max Set
#define VdsVoltageMaxSet00OFFSET					0x0700
#define VdsVoltageMaxSet01OFFSET					VdsVoltageMaxSet00OFFSET + 8
#define VdsVoltageMaxSet02OFFSET					VdsVoltageMaxSet01OFFSET + 8
#define VdsVoltageMaxSet03OFFSET					VdsVoltageMaxSet02OFFSET + 8
#define VdsVoltageMaxSet04OFFSET					VdsVoltageMaxSet03OFFSET + 8
#define VdsVoltageMaxSet05OFFSET					VdsVoltageMaxSet04OFFSET + 8
#define VdsVoltageMaxSet06OFFSET					VdsVoltageMaxSet05OFFSET + 8
#define VdsVoltageMaxSet07OFFSET					VdsVoltageMaxSet06OFFSET + 8
#define VdsVoltageMaxSet08OFFSET					VdsVoltageMaxSet07OFFSET + 8
#define VdsVoltageMaxSet09OFFSET					VdsVoltageMaxSet08OFFSET + 8
#define VdsVoltageMaxSet10OFFSET					VdsVoltageMaxSet09OFFSET + 8
#define VdsVoltageMaxSet11OFFSET					VdsVoltageMaxSet10OFFSET + 8
#define VdsVoltageMaxSet12OFFSET					VdsVoltageMaxSet11OFFSET + 8
#define VdsVoltageMaxSet13OFFSET					VdsVoltageMaxSet12OFFSET + 8
#define VdsVoltageMaxSet14OFFSET					VdsVoltageMaxSet13OFFSET + 8
#define VdsVoltageMaxSet15OFFSET					VdsVoltageMaxSet14OFFSET + 8
#define VdsVoltageMaxSet16OFFSET					VdsVoltageMaxSet15OFFSET + 8
#define VdsVoltageMaxSet17OFFSET					VdsVoltageMaxSet16OFFSET + 8
#define VdsVoltageMaxSet18OFFSET					VdsVoltageMaxSet17OFFSET + 8
#define VdsVoltageMaxSet19OFFSET					VdsVoltageMaxSet18OFFSET + 8
#define VdsVoltageMaxSet20OFFSET					VdsVoltageMaxSet19OFFSET + 8
#define VdsVoltageMaxSet21OFFSET					VdsVoltageMaxSet20OFFSET + 8
#define VdsVoltageMaxSet22OFFSET					VdsVoltageMaxSet21OFFSET + 8
#define VdsVoltageMaxSet23OFFSET					VdsVoltageMaxSet22OFFSET + 8

// VDS Current Max Set
#define VdsCurrentMaxSet00OFFSET					0x0704
#define VdsCurrentMaxSet01OFFSET					VdsCurrentMaxSet00OFFSET + 8
#define VdsCurrentMaxSet02OFFSET					VdsCurrentMaxSet01OFFSET + 8
#define VdsCurrentMaxSet03OFFSET					VdsCurrentMaxSet02OFFSET + 8
#define VdsCurrentMaxSet04OFFSET					VdsCurrentMaxSet03OFFSET + 8
#define VdsCurrentMaxSet05OFFSET					VdsCurrentMaxSet04OFFSET + 8
#define VdsCurrentMaxSet06OFFSET					VdsCurrentMaxSet05OFFSET + 8
#define VdsCurrentMaxSet07OFFSET					VdsCurrentMaxSet06OFFSET + 8
#define VdsCurrentMaxSet08OFFSET					VdsCurrentMaxSet07OFFSET + 8
#define VdsCurrentMaxSet09OFFSET					VdsCurrentMaxSet08OFFSET + 8
#define VdsCurrentMaxSet10OFFSET					VdsCurrentMaxSet09OFFSET + 8
#define VdsCurrentMaxSet11OFFSET					VdsCurrentMaxSet10OFFSET + 8
#define VdsCurrentMaxSet12OFFSET					VdsCurrentMaxSet11OFFSET + 8
#define VdsCurrentMaxSet13OFFSET					VdsCurrentMaxSet12OFFSET + 8
#define VdsCurrentMaxSet14OFFSET					VdsCurrentMaxSet13OFFSET + 8
#define VdsCurrentMaxSet15OFFSET					VdsCurrentMaxSet14OFFSET + 8
#define VdsCurrentMaxSet16OFFSET					VdsCurrentMaxSet15OFFSET + 8
#define VdsCurrentMaxSet17OFFSET					VdsCurrentMaxSet16OFFSET + 8
#define VdsCurrentMaxSet18OFFSET					VdsCurrentMaxSet17OFFSET + 8
#define VdsCurrentMaxSet19OFFSET					VdsCurrentMaxSet18OFFSET + 8
#define VdsCurrentMaxSet20OFFSET					VdsCurrentMaxSet19OFFSET + 8
#define VdsCurrentMaxSet21OFFSET					VdsCurrentMaxSet20OFFSET + 8
#define VdsCurrentMaxSet22OFFSET					VdsCurrentMaxSet21OFFSET + 8
#define VdsCurrentMaxSet23OFFSET					VdsCurrentMaxSet22OFFSET + 8

/**
 *  Defines of Offsets of the arrays of fix groups
 *    offset values in bytes !
 */
#define VdsSetVoltageAllChannelsOFFSET				VdsAddrFixGroupsOFFSET
#define VdsSetCurrentAllChannelsOFFSET				(VdsSetVoltageAllChannelsOFFSET+4)
#define VdsSetVoltageBoundsAllChannelsOFFSET		(VdsSetCurrentAllChannelsOFFSET+4)
#define VdsSetCurrentBoundsAllChannelsOFFSET		(VdsSetVoltageBoundsAllChannelsOFFSET+4)
#define VdsSetEmergencyAllChannelsOFFSET			(VdsSetCurrentBoundsAllChannelsOFFSET+4)
#define VdsSetOnOffAllChannelsOFFSET				(VdsSetEmergencyAllChannelsOFFSET+4)
#define VdsSetVoltageMaxAllChannelsOFFSET			(VdsSetOnOffAllChannelsOFFSET+4)
#define VdsSetCurrentMaxAllChannelsOFFSET			(VdsSetVoltageMaxAllChannelsOFFSET+4)

#endif // VMEMODULE_H
