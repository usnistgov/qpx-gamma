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
 *     Globals.h
 *
 * Description:
 *
 *     Definitions of global variables and data structures
 *
 * Revision:
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "PlxTypes.h"   
#include "PciTypes.h"
#include "Plx.h"

#include "globals.h"

/***********************************************************
*		Global variables
***********************************************************/

PLX_UINT_PTR	VAddr[PRESET_MAX_MODULES];			// Virtual addresses	
U8 Phy_Slot_Wave[PRESET_MAX_MODULES];			// The slot number of the PXI crate
U8 Sys_Slot_Wave[PRESET_MAX_MODULES];			// The PCI device number
PLX_DEVICE_OBJECT Sys_hDevice[PRESET_MAX_MODULES];	// PCI device handle
U8 Number_Modules;					// Total number of modules in the crate
U8 Offline;						// Indicator of offline analysis
U8 AutoProcessLMData;					// To control if the LM parse routine processes compressed LM data
U8 Max_Number_of_Modules;				// The maximum possible number of modules in the crate (acts as crate type ID)
U8 KeepCW;						// To control update and enforced minimum of coincidence wait
U8 Chosen_Module;					// Current module
U8 Chosen_Chan;						// Current channel
U8 Bus_Number[PRESET_MAX_MODULES];			// PCI bus number
U16 Random_Set[IO_BUFFER_LENGTH];			// Random indices used by TauFinder and BLCut
double SGA_Computed_Gain[N_SGA_GAIN];			// SGA computed gain
double Filter_Int[PRESET_MAX_MODULES];			// Energy filter interval (depends on Clock speed and filter range)
U16 Writing_IOBuffer_Address;				// The start address of I/O buffer for writing
U16 Writing_IOBuffer_Length;				// The number of words to write into the I/O buffer
double One_Cycle_Time;					// Number of ns for each wait cycle
S8  ErrMSG[256];					// A string for error messages
U32 MODULE_EVENTS[2*PRESET_MAX_MODULES];		// Internal copy of ModuleEvents array modified by task 0x7001 and used by other 0x7000 tasks
U8 ZDT;              // indictating DSP is ZDT version

//***********************************************************
//		Frequently used indices
//***********************************************************

U16 Run_Task_Index;							// RUNTASK
U16 Control_Task_Index;						// CONTROLTASK
U16 Resume_Index;							// RESUME
U16 SYNCHDONE_Index;						// SYNCHDONE
U16 FASTPEAKS_Index[NUMBER_OF_CHANNELS];	// FASTPEAKS
U16 LIVETIME_Index[NUMBER_OF_CHANNELS];		// LIVETIME
U16 RUNTIME_Index;							// RUNTIME
U16 NUMEVENTS_Index;						// NUMEVENTS


//***********************************************************
//		Common configuration files and parameter names
//		used by all Pixie modules
//***********************************************************

/* Configuration file names */
S8 Boot_File_Name_List[N_BOOT_FILES][MAX_FILE_NAME_LENGTH];

/* Communication FPGA configuration (Rev. B) */
U8 ComFPGA_Configuration_RevB[N_COMFPGA_BYTES];

/* Communication FPGA configuration (Rev. C) */
U8 ComFPGA_Configuration_RevC[N_COMFPGA_BYTES];

/* DSP executable code */
U16 DSP_Code[N_DSP_CODE_BYTES];

/* FIPPI configuration */
U8 FIPPI_Configuration[N_FIPPI_BYTES];

/* Virtex FIPPI configuration */
U8 Virtex_Configuration[N_VIRTEX_BYTES];

/* DSP I/O parameter names */					
S8 DSP_Parameter_Names[N_DSP_PAR][MAX_PAR_NAME_LENGTH];
// defined by .var file

/* DSP internal parameter names; not being used in this driver now */
//U8 DSP_Memory_Names[N_MEM_PAR][MAX_PAR_NAME_LENGTH];

/* Parameter names applicable to a Pixie channel */
S8 Channel_Parameter_Names[N_CHANNEL_PAR][MAX_PAR_NAME_LENGTH] = {
	"CHANNEL_CSRA", 
	"CHANNEL_CSRB", 
	"ENERGY_RISETIME", 
	"ENERGY_FLATTOP", 
	"TRIGGER_RISETIME", 
	"TRIGGER_FLATTOP", 
	"TRIGGER_THRESHOLD", 
	"VGAIN", 
	"VOFFSET", 
	"TRACE_LENGTH", 
	"TRACE_DELAY", 
	"PSA_START", 
	"PSA_END", 
	"EMIN", 
	"BINFACTOR", 
	"TAU", 
	"BLCUT", 
	"XDT", 
	"BASELINE_PERCENT", 
	"CFD_THRESHOLD", 
	"INTEGRATOR", 
	"CHANNEL_CSRC", 
	"GATE_WINDOW", 
	"GATE_DELAY", 
	"COINC_DELAY",
	"BLAVG", 
	"LIVE_TIME", 
	"INPUT_COUNT_RATE", 
	"FAST_PEAKS", 
	"OUTPUT_COUNT_RATE", 
	"NOUT", 
	"GATE_RATE", 
	"GATE_COUNTS", 
	"FTDT", 
	"SFDT", 
	"GDT",
	"CURRENT_ICR",
	"CURRENT_OORF",
	"PSM_GAIN_AVG",
	"PSM_GAIN_AVG_LEN",
	"PSM_TEMP_AVG",
	"PSM_TEMP_AVG_LEN",
	"PSM_GAIN_CORR",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	""
};
// overwritten by Igor

/* Parameter names applicable to a Pixie module */
S8 Module_Parameter_Names[N_MODULE_PAR][MAX_PAR_NAME_LENGTH] = {

	"MODULE_NUMBER", 
	"MODULE_CSRA", 
	"MODULE_CSRB", 
	"MODULE_FORMAT", 
	"MAX_EVENTS", 
	"COINCIDENCE_PATTERN", 
	"ACTUAL_COINCIDENCE_WAIT", 
	"MIN_COINCIDENCE_WAIT", 
	"SYNCH_WAIT", 
	"IN_SYNCH", 
	"RUN_TYPE", 
	"FILTER_RANGE", 
	"MODULEPATTERN", 
	"NNSHAREPATTERN", 
	"DBLBUFCSR", 
	"MODULE_CSRC", 
	"BUFFER_HEAD_LENGTH", 
	"EVENT_HEAD_LENGTH", 
	"CHANNEL_HEAD_LENGTH", 
	"OUTPUT_BUFFER_LENGTH", 
	"NUMBER_EVENTS", 
	"RUN_TIME", 
	"EVENT_RATE", 
	"TOTAL_TIME", 
	"BOARD_VERSION", 
	"SERIAL_NUMBER", 
	"DSP_RELEASE", 
	"DSP_BUILD", 
	"FIPPI_ID", 
	"SYSTEM_ID",
	"XET_DELAY",
	"PDM_MASKA",
	"PDM_MASKB",
	"PDM_MASKC",
	"","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	""
};
// overwritten by Igor

/* Parameter names applicable to a whole Pixie system */
S8 System_Parameter_Names[N_SYSTEM_PAR][MAX_PAR_NAME_LENGTH] = {
	"NUMBER_MODULES",
	"OFFLINE_ANALYSIS",
	"AUTO_PROCESSLMDATA",
	"MAX_NUMBER_MODULES",
	"C_LIBRARY_RELEASE",
	"C_LIBRARY_BUILD",
	"KEEP_CW",
	"SLOT_WAVE",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","","",
	"","","","","","","",""
};
// overwritten by Igor

/* Parameter values applicable to a whole Pixie system */
double System_Parameter_Values[N_SYSTEM_PAR];
struct Pixie_Configuration Pixie_Devices[PRESET_MAX_MODULES];
/* Global data structure for the list mode reader and related functions */
struct LMReaderDataStruct ListModeDataStructure;
/* Pointer to the global data structure for the list mode reader and related functions */
LMR_t LMA = &ListModeDataStructure;
