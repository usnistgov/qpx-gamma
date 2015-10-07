#ifndef CONST_H
#define CONST_H

//#include <QtGlobal>
#include <stdint.h>

// Set to one if your compiler orders bitfields MSB first
#define BITFIELDS_MSB_FIRST								0

#define MAX_CRATES										64
#define MAX_MODULES										64
#define MAX_CHANNELS									64
#define MAX_GROUPS										32

// ----------------------------------------------------------------------------
// Channel bit definitions
// ----------------------------------------------------------------------------
typedef union ChStatusUnion {
  uint16_t Word;
	struct ChStatusBits {
    uint16_t	Reserved1:2;
    uint16_t	InputError:1;
    uint16_t	isOn:1;
    uint16_t	isRamping:1;
    uint16_t	isEmergency:1;
    uint16_t	ControlledByCurrent:1;
    uint16_t	ControlledByVoltage:1;

    uint16_t	Reserved:2;
    uint16_t	CurrentBounds:1;
    uint16_t	VoltageBounds:1;
    uint16_t	ExternalInhibit:1;
    uint16_t	CurrentTrip:1;
    uint16_t	CurrentLimit:1;
    uint16_t	VoltageLimit:1;
	} Bit;
} ChStatusSTRUCT;

typedef union ChEventStatusUnion {
  uint16_t Word;
	struct ChEventStatusBits {
    uint16_t	Reserved1:2;
    uint16_t	EventInputError:1;
    uint16_t	EventOnToOff:1;
    uint16_t	EventEndOfRamping:1;
    uint16_t	EventEmergency:1;
    uint16_t	EventControlledByCurrent:1;
    uint16_t	EventControlledByVoltage:1;

    uint16_t	Reserved3:2;
    uint16_t	EventCurrentBounds:1;
    uint16_t	EventVoltageBounds:1;
    uint16_t	EventExternalInhibit:1;
    uint16_t	EventCurrentTrip:1;
    uint16_t	EventCurrentLimit:1;
    uint16_t	EventVoltageLimit:1;
	} Bit;
} ChEventStatusSTRUCT;

typedef union ChEventMaskUnion {
  uint16_t Word;
	struct ChEventMaskBits {
    uint16_t	Reserved1:2;
    uint16_t	MaskEventInputError:1;
    uint16_t	MaskEventOnToOff:1;
    uint16_t	MaskEventEndOfRamping:1;
    uint16_t	MaskEventEmergency:1;
    uint16_t	MaskEventControlledByCurrent:1;
    uint16_t	MaskEventControlledByVoltage:1;

    uint16_t	Reserved3:2;
    uint16_t	MaskEventCurrentBounds:1;
    uint16_t	MaskEventVoltageBounds:1;
    uint16_t	MaskEventExternalInhibit:1;
    uint16_t	MaskEventCurrentTrip:1;
    uint16_t	MaskEventCurrentLimit:1;
    uint16_t	MaskEventVoltageLimit:1;
	} Bit;
} ChEventMaskSTRUCT;

typedef union ChControlUnion {
  uint16_t Word;
	struct ChControlBits {
    uint16_t	Reserved1: 1;
    uint16_t setExternalMeasurementOn: 1;
    uint16_t setRippleMeasurementOn: 1;
    uint16_t	setON: 1;
    uint16_t	Reserved2: 1;
    uint16_t	setEmergency: 1;
    uint16_t	Reserved3: 4;
    uint16_t	setACBND: 1;
    uint16_t	setAVBND: 1;
    uint16_t	Reserved4: 4;
	} Bit;
} ChControlSTRUCT;

// ----------------------------------------------------------------------------
// Module bit definitions
// ----------------------------------------------------------------------------

typedef union ModuleStatusUnion {
  uint16_t Word;
	struct ModuleStatusBits {
    uint16_t IsAdjustment:1;
    uint16_t IsInterlockOut:1;
    uint16_t IsStop:1; // isLiveInsertion
    uint16_t Reserved1:1;
    uint16_t IsServiceNeeded:1;
    uint16_t IsInputError:1; // isHardwareVoltageLimitGood
    uint16_t IsSpecialMode:1; // isInputError
    uint16_t IsCommandComplete:1;

    uint16_t IsNoSumError:1;
    uint16_t IsNoRamp:1;
    uint16_t IsSafetyLoopGood:1;
    uint16_t IsEventActive:1;
    uint16_t IsModuleGood:1;
    uint16_t IsSupplyGood:1;
    uint16_t IsTemperatureGood:1;
    uint16_t IsKillEnable:1;
	} Bit;
} ModuleStatusSTRUCT;

typedef union ModuleControlUnion {
  uint16_t Word;
	struct ModuleControlBits {
    uint16_t	SetSpecialMode:1;
    uint16_t	DoRecallSetValues:1;
    uint16_t	SetStop:1;
    uint16_t	SetActionOn:1;
    uint16_t	Reserved1:2;
    uint16_t	DoClear:1;
    uint16_t	Reserved2:1;

    uint16_t	IntLevel0:1;
    uint16_t	IntLevel1:1;
    uint16_t	IntLevel2:1;
    uint16_t	Reserved3:1;
    uint16_t	SetAdjustment:1;
    uint16_t	Reserved4:1;
    uint16_t	SetKillEnable:1;
    uint16_t	DoSaveSetValues:1;
	} Bit;
} ModuleControlSTRUCT;

typedef union ModuleEventStatusUnion {
  uint16_t Word;
	struct ModuleEventStatusBits {
    uint16_t	EventPowerFail: 1;
    uint16_t	EventRestart: 1;
    uint16_t EventStop: 1;
    uint16_t	Reserved2: 1;
    uint16_t	EventServiceNeeded: 1;
    uint16_t	EventInputError: 1;
    uint16_t	Reserved3: 4;

    uint16_t	EventSafetyLoopNotGood: 1;
    uint16_t	Reserved4: 2;
    uint16_t	EventSupplyNotGood: 1;
    uint16_t	EventTemperatureNotGood: 1;
    uint16_t	Reserved5: 1;
		} Bit;
} ModuleEventStatusSTRUCT;

typedef union ModuleEventMaskUnion {
  uint16_t Word;
	struct ModuleEventMaskBits {
    uint16_t	Reserved1: 1;
    uint16_t	MaskEventRestart: 1;
    uint16_t	Reserved2: 2;
    uint16_t	MaskEventServiceNeeded: 1;
    uint16_t	MaskEventInputError: 1;
    uint16_t	Reserved3: 4;

    uint16_t	MaskEventSafetyLoopNotGood: 1;
    uint16_t	Reserved4: 2;
    uint16_t	MaskEventSupplyNotGood: 1;
    uint16_t	MaskEventTemperatureNotGood: 1;
    uint16_t	Reserved5: 1;
	} Bit;
} ModuleEventMaskSTRUCT;

// ----------------------------------------------------------------------------
// Group bit definitions
// ----------------------------------------------------------------------------

typedef union GroupMember1Union {
	struct GroupMember1Bits {
    uint16_t	Channel0:1;
    uint16_t	Channel1:1;
    uint16_t	Channel2:1;
    uint16_t	Channel3:1;
    uint16_t	Channel4:1;
    uint16_t	Channel5:1;
    uint16_t	Channel6:1;
    uint16_t	Channel7:1;
    uint16_t	Channel8:1;
    uint16_t	Channel9:1;
    uint16_t	Channel10:1;
    uint16_t	Channel11:1;
    uint16_t	Channel12:1;
    uint16_t	Channel13:1;
    uint16_t	Channel14:1;
    uint16_t	Channel15:1;
	} Bit;
  uint16_t Word;
} GroupMemberSTRUCT1;

typedef union GroupMember2Union {
	struct GroupMember2Bits {
    uint16_t	Channel16:1;
    uint16_t	Channel17:1;
    uint16_t	Channel18:1;
    uint16_t	Channel19:1;
    uint16_t	Channel20:1;
    uint16_t	Channel21:1;
    uint16_t	Channel22:1;
    uint16_t	Channel23:1;
    uint16_t	Channel24:1;
    uint16_t	Channel25:1;
    uint16_t	Channel26:1;
    uint16_t	Channel27:1;
    uint16_t	Channel28:1;
    uint16_t	Channel29:1;
    uint16_t	Channel30:1;
    uint16_t	Channel31:1;
	} Bit;
  uint16_t Word;
} GroupMemberSTRUCT2;

/// TODO Extended Member struct for VDS 24

// --- Set Group --------------------------------------------------------------
typedef union GroupSet1PropertyUnion {
	struct GroupSet1PropertyBits{
    uint16_t	MasterChannel:4;
    uint16_t	Control:4;
    uint16_t	Mode:1;
    uint16_t	Reserved1:5;
    uint16_t	GroupType:2;
	} Bit;
  uint16_t Word;
} GroupSet1PropertySTRUCT;

typedef union GroupSet2PropertyUnion {
	struct GroupSet2PropertyBits {
    uint16_t	MasterChannel:5;
    uint16_t	Reserved1:11;
	} Bit;
  uint16_t Word;
} GroupSet2PropertySTRUCT;

// --- Status Group -----------------------------------------------------------
typedef struct GroupStatus1PropertyUnion {
	struct GroupStatus1PropertyBits {
    uint16_t	Reserved0:4;
    uint16_t	Item:4;
    uint16_t	Reserved1:6;
    uint16_t	GroupType:2;
	} Bit;
  uint16_t Word;
} GroupStatus1PropertySTRUCT;

// --- Monitoring Group -------------------------------------------------------
typedef union GroupMonitoring1PropertyUnion {
	struct GroupMonitoring1PropertyBits {
    uint16_t	Reserved0:4;
    uint16_t	Monitor:4;
    uint16_t	Mode:1;
    uint16_t	Reserved1:3;
    uint16_t	Action:2;
    uint16_t	GroupType:2;
	} Bit;
  uint16_t Word;
} GroupMonitoring1PropertySTRUCT;

// --- Timeout Group ----------------------------------------------------------
typedef union GroupTimeout2PropertyUnion {
	struct GroupTimeout2PropertyBits {
    uint16_t	TimeoutTime:16;
	} Bit;
  uint16_t Word;
} GroupTimeout2PropertySTRUCT;

typedef union GroupTimeout1PropertyUnion {
  uint16_t Word;
	struct GroupTimeout1PropertyBits {
    uint16_t	TimeoutTime:12;
    uint16_t	Action:2;
    uint16_t	GroupType:2;
	} Bit;
} GroupTimeout1PropertySTRUCT;

#define GROUP_TYPE_SET            0
#define GROUP_TYPE_STATUS         1
#define GROUP_TYPE_MONITOR        2
#define GROUP_TYPE_TIMEOUT        3

#define GROUP_SET_VSET            1
#define GROUP_SET_ISET            2
#define GROUP_SET_VILK_MAX        4
#define GROUP_SET_IILK_MAX        5
#define GROUP_SET_VILK_MIN        6
#define GROUP_SET_IILK_MIN        7
#define GROUP_SET_SETON          10
#define GROUP_SET_EMCY           11
#define GROUP_SET_CLONE          15

#define GROUP_STATUS_ISON         3
#define GROUP_STATUS_ISRAMP       4
#define GROUP_STATUS_ISCC         6
#define GROUP_STATUS_ISCV         7
#define GROUP_STATUS_ISIBND      10
#define GROUP_STATUS_ISVBND      11
#define GROUP_STATUS_ISEINH      12
#define GROUP_STATUS_ISTRIP      13
#define GROUP_STATUS_ISILIM      14
#define GROUP_STATUS_ISVLIM      15

#define GROUP_MONITOR_ISON        3
#define GROUP_MONITOR_ISRAMP      4
#define GROUP_MONITOR_ISCC        6
#define GROUP_MONITOR_ISCV        7
#define GROUP_MONITOR_ISIBND     10
#define GROUP_MONITOR_ISVBND     11
#define GROUP_MONITOR_ISEINH     12
#define GROUP_MONITOR_ISTRIP     13
#define GROUP_MONITOR_ISILIM     14
#define GROUP_MONITOR_ISVLIM     15

#define GROUP_ACTION_NO_ACTION    0
#define GROUP_ACTION_RAMP_DOWN    1
#define GROUP_ACTION_SHUT_GROUP   2
#define GROUP_ACTION_SHUT_MODULE  3

typedef struct GroupStruct {
	GroupMemberSTRUCT2                 MemberList2;
	GroupMemberSTRUCT1                 MemberList1;
	union GroupType2Union {
		GroupSet2PropertySTRUCT        Set;
		GroupTimeout2PropertySTRUCT    Timeout;
    uint16_t                        Word;
	} Type2;
	union GroupType1Union {
		GroupSet1PropertySTRUCT        Set;
		GroupStatus1PropertySTRUCT     Status;
		GroupMonitoring1PropertySTRUCT Monitor;
		GroupTimeout1PropertySTRUCT    Timeout;
    uint16_t                        Word;
	} Type1;
} GroupSTRUCT;

/******************************************************************************
 * Interlock Output Control / Status
 *****************************************************************************/

typedef union ModuleIlkOutStatusUnion {
  uint16_t Word;
	struct ModuleIlkOutStatusBits {
    uint16_t	IsIlkTestLoops0:1;
    uint16_t	IsIlkTestLoops1:1;
    uint16_t	IsIlkTestLoops2:1;
    uint16_t	IsIlkTestLoops3:1;
    uint16_t	Reserved1:4;
    uint16_t	Reserved2:5;
    uint16_t	IsIlkStandby:1;
    uint16_t	IsIlkActive:1;
    uint16_t	IsIlkEnabled:1;
	} Bit;
} ModuleIlkOutStatusSTRUCT;

typedef union ModuleIlkOutControlUnion {
  uint16_t Word;
	struct ModuleIlkOutControlBits {
    uint16_t	doIlkTestLoops0: 1;
    uint16_t	doIlkTestLoops1: 1;
    uint16_t	doIlkTestLoops2: 1;
    uint16_t	doIlkTestLoops3: 1;
    uint16_t	Reserved1:1;
    uint16_t	setIlkIset:1;
    uint16_t	setIlkCLim:1;
    uint16_t	setIlkVBnds:1;
    uint16_t	setIlkVLim:1;
    uint16_t	setIlkCBnds:1;
    uint16_t	Reserved2:4;
    uint16_t	clrIlkRegs:1;
    uint16_t	setIlkEnable:1;
	} Bit;
} ModuleIlkOutControlSTRUCT;

#define MODULE_ILKOUT_CONTROL_DO_TEST_LOOPS		0
#define MODULE_ILKOUT_CONTROL_TEST_ISET			4
#define MODULE_ILKOUT_CONTROL_TEST_CLIM			5
#define MODULE_ILKOUT_CONTROL_TEST_VBNDS		6
#define MODULE_ILKOUT_CONTROL_TEST_VLIM			7
#define MODULE_ILKOUT_CONTROL_CLEAR_REGS		14
#define MODULE_ILKOUT_CONTROL_ENABLE			15

typedef union ModuleIlkLastTriggerUnion {
  uint16_t Word;
	struct ModuleIlkLastTriggerBits {
    uint16_t	IlkChn0:1;
    uint16_t	IlkChn1:1;
    uint16_t	IlkChn2:1;
    uint16_t	IlkChn3:1;
    uint16_t	IlkChn4:1;
    uint16_t	IsIlkIset:1;
    uint16_t	IsIlkCLim:1;
    uint16_t	IsIlkVBnds:1;
    uint16_t	IsIlkVLim:1;
    uint16_t	IsIlkIBnds:1;
    uint16_t	Reserved1:4;
    uint16_t	IsIlkTest:1;
    uint16_t	Reserved2:1;
	} Bit;
} ModuleIlkLastTriggerSTRUCT;

typedef union ModuleChannelUnion {
  uint16_t Word;
	struct ModuleChannelBits {
    uint16_t Chn0: 1;
    uint16_t Chn1: 1;
    uint16_t Chn2: 1;
    uint16_t Chn3: 1;
    uint16_t Chn4: 1;
    uint16_t Chn5: 1;
    uint16_t Chn6: 1;
    uint16_t Chn7: 1;
    uint16_t Chn8: 1;
    uint16_t Chn9: 1;
    uint16_t Chn10: 1;
    uint16_t Chn11: 1;
    uint16_t Chn12: 1;
    uint16_t Chn13: 1;
    uint16_t Chn14: 1;
    uint16_t Chn15: 1;
	} Bit;
} ModuleChannelSTRUCT;

#endif // CONST_H
