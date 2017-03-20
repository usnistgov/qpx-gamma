#ifndef __SHAREDFILES_H
#define __SHAREDFILES_H

/*----------------------------------------------------------------------
 * Copyright (c) 2004, 2009 XIA LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, 
 * with or without modification, are permitted provided 
 * that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above 
 *     copyright notice, this list of conditions and the 
 *     following disclaimer.
 *   * Redistributions in binary form must reproduce the 
 *     above copyright notice, this list of conditions and the 
 *     following disclaimer in the documentation and/or other 
 *     materials provided with the distribution.
 *   * Neither the name of XIA LLC
 *     nor the names of its contributors may be used to endorse 
 *     or promote products derived from this software without 
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE.
 *----------------------------------------------------------------------*/

/******************************************************************************
 *
 * File Name:
 *
 *     sharedfiles.h
 *
 * Description:
 *
 *     Pixie-4 Igor inteface header
 *
 * Revision:
 *
 *     3-11-2004
 *
 ******************************************************************************/


/* If this is compiled by a C++ compiler, make it clear that these are C routines */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DEFS_H
#include "defs.h"
#endif

/************************************/
/*		Function prototypes			*/
/************************************/

//****************************************************
//			%%% Communication functions %%%
//****************************************************

S32	Pixie_RdWrdCnt(U8 ModNum, U32 *WordCount);
S32 Pixie_ReadCSR(U8 ModNum, U32 *CSR);
S32 Pixie_WrtCSR(U8 ModNum, U32 CSR);
S32 Pixie_ReadVersion(U8 ModNum, U32 *Version);

//****************************************************
//			%%% I/O functions %%%
//****************************************************

S32 Pixie_Register_IO (
			U16 ModNum,		// the Pixie module to communicate to
			U32 address,	// register address
			U16 direction,	// either MOD_READ or MOD_WRITE
			U32 *value );	// holds or receives the data

S32 Pixie_RdWrdCnt (
			U8 ModNum,			// Pixie module number
			U32 *WordCount );	// word count value

S32 Pixie_ReadCSR (
			U8 ModNum,			// Pixie module number
			U32 *CSR );			// CSR value

S32 Pixie_WrtCSR (
			U8 ModNum,			// Pixie module number
			U32 CSR );			// CSR value

S32 Pixie_ReadVersion (
			U8 ModNum,			// Pixie module number
			U32 *Version );		// Version number


S32 Pixie_IODM (
			U8  ModNum,			// the Pixie module to communicate to
			U32 address,		// data memory address
			U8  direction,		// either MOD_READ or MOD_WRITE
			U16 nWords,			// the number of 32-bit words to be transferred
			U32 *buffer );		// holds or receives the data

S32 Pixie_IOEM (
			U8  ModNum,			// the Pixie module to communicate to
			U32 address,		// external memory address
			U8  direction,		// either MOD_READ or MOD_WRITE
			U32 nWords,			// the number of 32-bit words to be transferred
			U32 *buffer );		// holds or receives the data


S32 UA_PAR_IO (
			double *User_Par_Values,			// user parameters to be transferred
			S8     *user_variable_name,			// parameter name (string)
			S8     *user_variable_type,			// parameter type (string)
			U16    direction,					// Read or Write
			U8     ModNum,						// Pixie module number
			U8     ChanNum );					// Pixie channel number



//****************************************************
//			%%% Initialization functions %%%
//****************************************************

S32 Pixie_Init_Globals(void);


S32 Pixie_Scan_Crate_Slots (
			U8  NumModules,		// Total number of Pixie modules
			U8 *PXISlotMap );	// Slot-Pixie mapping array

S32 Pixie_Close_PCI_Devices (
			U16 ModNum );		// Pixie module number


S32 Pixie_Boot (
			U16 Boot_Pattern );		// boot pattern is a bit mask


S32 Pixie_Init_VarNames(void);

//****************************************************
//			%%% Control task functions %%%
//****************************************************

S32 Control_Task_Run (
		U8 ModNum,			// Pixie module number
		U8 ControlTask,		// Control task number
		U32 Max_Poll );		// Timeout control in unit of ms for control task run

S32 Adjust_Offsets(
		U8 ModNum);

//****************************************************
//			%%% Tools functions %%%
//****************************************************

U16 TstBit(U16 bit, U16 value);

U16 Find_Xact_Match (S8 *str,							// the string to be searched
		     S8 Names[][MAX_PAR_NAME_LENGTH],	// the array which contains the string
		     U16 Names_Array_Len );				// the length of the array


S32 Load_U16 (
			S8 *filnam,			// file name
			U8 ConfigName,		// determine which type of parameter to load
			U32 ndat);			// number of data words to load



S32 Pixie_Print_MSG (
			S8 *message );	// message to be printed



S32 Pixie_CopyExtractSettings (
			U8 SourceChannel,			// source Pixie channel
			U16 BitMask,				// copy/extract bit mask pattern
			U8 DestinationModule,		// destination module number
			U8 DestinationChannel,		// destination channel number
			U16 *DSPSourceSettings );	// DSP settings of the source channel

S32 BLcut_Finder (
			U8 ModNum,			// Pixie module number
			U8 ChanNum,			// Pixie channel number
			double *BLcut );	// BLcut return value


S32 Make_SGA_Gain_Table(void);

//****************************************************
//			%%% C-interface functions %%%
//****************************************************

S32 Pixie_Hand_Down_Names (
			S8 *Names[],	// a two dimensional string array
			S8 *Name );		// a string indicating which type of names to be downloaded


S32 Pixie_Boot_System (
			U16 Boot_Pattern );	// bit mask



S32 Pixie_Acquire_Data (
			U16 Run_Type,		// data acquisition run type
			U32 *User_data,		// an array holding data to be transferred back to the host
			S8  *file_name,		// file name for storing run data 
			U8  ModNum );		// pixie module number


S32 Pixie_Set_Current_ModChan (
			U8 Module,			// module name
			U8 Channel);		// channel name


S32 Pixie_User_Par_IO (
			double *User_Par_Values,	// user parameters to be transferred
			S8 *User_Par_Name,			// user parameter name
			S8 *User_Par_Type,			// user parameter type
			U16 direction,				// transfer direction (read or write)
			U8  ModNum,					// number of the module to work on
			U8  ChanNum );				// channel number of the Pixie module


S32 Pixie_Buffer_IO (
			U16 *Values,		// an array hold the data for I/O
			U8 type,			// I/O type
			U8 direction,		// I/O direction
			S8 *file_name,		// file name
			U8 ModNum);			// number of the module to work on







S32 Pixie_Parse_List_Mode_Events (
			S8  *filename,			// the list mode data file name (with complete path)
			U32 *ModuleEvents );	// receives number of events & traces for each module


S32 Pixie_Locate_List_Mode_Traces (
			S8  *filename,			// the list mode data file name (with complete path)
			U32 *ModuleTraces );	// receives trace length and location for each module


S32 Pixie_Read_List_Mode_Traces (
			S8  *filename,			// the list mode data file name (with complete path)
			U32 *ListModeTraces );	// receives list mode trace data


S32 Pixie_Read_Energies (
			S8  *filename,			// list mode data file name
			U32 *EventEnergies,		// receive event energy values
			U8  ModNum );			// Pixie module number

S32 Pixie_Read_Event_PSA (
			S8  *filename,			// list mode data file name
			U32 *EventPSA,			// receive event PSA values
			U8  ModNum );			// Pixie module number

S32 Pixie_Read_Long_Event_PSA (
			S8  *filename,			// list mode data file name
			U32 *EventPSA,			// receive event PSA values
			U8  ModNum );			// Pixie module number

S32 Pixie_Locate_List_Mode_Events (
			S8  *filename,			// the list mode data file name (with complete path)
			U32 *ModuleTraces );	// receives trace length and location for each module


S32 Pixie_Read_List_Mode_Events (
			S8  *filename,			// the list mode data file name (with complete path)
			U32 *ListModeTraces );	// receives list mode trace data



S32 Read_Spectrum_File (
			U8  ModNum,				// Pixie module number
			U32 *Data,				// Receives histogram data
			S8  *FileName );		// previously-saved histogram file


S32 Write_List_Mode_File (
			S8  *FileName );		// List mode data file name


S32 Write_Spectrum_File (
			S8 *FileName );			// histogram data file name


S32 Check_Run_Status (
		U8 ModNum );				// Pixie module number


S32 End_Run (
		U8 ModNum );				// Pixie module number


S32 Start_Run (
		U8  ModNum,					// Pixie module number
		U8  Type,					// run type (NEW_RUN or RESUME_RUN)
		U16 Run_Task,				// run task
		U16 Control_Task );			// control task
		
S32 Read_Resume_Run (
			U8  ModNum,	    // Pixie module number
			U8  Stop,		// stop/resume control 
			S8  *FileName    );  // Pixie module number

S32 Get_Traces (
			U32 *Trace_Buffer,		// ADC trace data
			U8  ModNum,				// Pixie module number
			U8  ChanNum );			// Pixie channel number

S32 Get_Slow_Traces (
			U32 *UserData,			// input data
			U8  ModNum,				// Pixie module number
			S8 *FileName );			// data file name

S32 Pixie_Define_Clocks (	
	U8  ModNum,				// Pixie module number
	U8  ChanNum,			// Pixie channel number
	U16 *SYSTEM_CLOCK_MHZ,	// system clock -- coincidence window, DSP time stamps
	U16 *FILTER_CLOCK_MHZ,	// filter/processing clock -- filter lengths, runs statistics
	U16	*ADC_CLOCK_MHZ,		// sampling clock of the ADC
	U16	*LTscale,			// The scaling factor for live time counters
	U16 *ThisADCclkMHz );	// for P500 Rev A: ch0 gets special treatment


//****************************************************
//				%%% Tools functions %%%
//****************************************************

U16 SetBit(U16 bit, U16 value);
U16 ClrBit(U16 bit, U16 value);
U16 TglBit(U16 bit, U16 value);
U32 RoundOff(double x);
S32 RandomSwap(void);


//****************************************************
//				%%% EEPROM related functions %%%
//****************************************************

void wait_for_a_short_time(S32 t);
S32 get_ns_per_cycle(double *ns_per_cycle);


S32 I2C24LC16B_init(U8 ModNum);
S32 I2C24LC16B_start(U8 ModNum);

S32 I2C24LC16B_stop(U8 ModNum);
S32 I2C24LC16B_byte_send(U8 ModNum, U8 ByteToSend);
S32 I2C24LC16B_byte_receive(U8 ModNum, U8 *ByteToReceive);

U8 I2C24LC16B_getACK(U8 ModNum);

S32 I2C24LC16B_Read_One_Byte (
		U8 ModNum,             // Pixie module number
		U16 Address,            // The address to read this byte
		U8 *ByteValue );       // The byte value

S32 I2C24LC16B_Write_One_Byte (
		U8 ModNum,             // Pixie module number
		U16 Address,            // The address to write this byte
		U8 *ByteValue );       // The byte value

#ifdef __cplusplus
}
#endif	/* End of notice for C++ compilers */

#endif	/* End of sharedfiles.h */


