#ifndef __GLOBALS_H
#define __GLOBALS_H

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
 *     12-1-2004
 *
 ******************************************************************************/


/* If this is compiled by a C++ compiler, make it */
/* clear that these are C routines.               */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DEFS_H
	#include "defs.h"
#endif

/***********************************************************
*		Global variables
***********************************************************/

extern PLX_UINT_PTR VAddr[PRESET_MAX_MODULES];			// Virtual addresses
extern U8 Phy_Slot_Wave[PRESET_MAX_MODULES];		// The slot number of the PXI crate
extern U8 Sys_Slot_Wave[PRESET_MAX_MODULES];		// The PCI device number
extern PLX_DEVICE_OBJECT Sys_hDevice[PRESET_MAX_MODULES];// PCI device handle
extern U8 Number_Modules;				// Total number of modules in the crate
extern U8 Offline;					// Indicator of offline analysis
extern U8 AutoProcessLMData;				// To control if the LM parse routine processes compressed LM data
extern U8 Max_Number_of_Modules;			// The maximum possible number of modules in the crate (acts as crate type ID)
extern U8 KeepCW;					// To control update and enforced minimum of coincidence wait
extern U8 Chosen_Module;				// Current module
extern U8 Chosen_Chan;					// Current channel
extern U8 Bus_Number[PRESET_MAX_MODULES];		// PCI bus number
extern U16 Random_Set[IO_BUFFER_LENGTH];		// Random indices used by TauFinder and BLCut
extern double SGA_Computed_Gain[N_SGA_GAIN];		// SGA computed gain
extern double Filter_Int[PRESET_MAX_MODULES];		// Energy filter interval (depends on Clock speed and filter range)
extern U16 Writing_IOBuffer_Address;			// The start address of I/O buffer for writing
extern U16 Writing_IOBuffer_Length;			// The number of words to write into the I/O buffer
extern double One_Cycle_Time;				// Number of ns for each wait cycle
extern S8  ErrMSG[256];					// A string for error messages
extern U32 MODULE_EVENTS[2*PRESET_MAX_MODULES];		// Internal copy of ModuleEvents array modified by task 0x7001 and used by other 0x7000 tasks
extern U8 ZDT;  // indictating DSP is ZDT version

//***********************************************************
//		Frequently used indices
//***********************************************************

extern U16 Run_Task_Index;							// RUNTASK
extern U16 Control_Task_Index;						// CONTROLTASK
extern U16 Resume_Index;							// RESUME
extern U16 SYNCHDONE_Index;							// SYNCHDONE
extern U16 FASTPEAKS_Index[NUMBER_OF_CHANNELS];		// FASTPEAKS
extern U16 LIVETIME_Index[NUMBER_OF_CHANNELS];		// LIVETIME
extern U16 RUNTIME_Index;							// RUNTIME
extern U16 NUMEVENTS_Index;							// NUMEVENTS

//***********************************************************
//		Common configuration files and parameter names
//		used by all Pixie modules
//***********************************************************

/* Configuration file names */
extern S8 Boot_File_Name_List[N_BOOT_FILES][MAX_FILE_NAME_LENGTH];

/* Communication FPGA configuration (Rev. B) */
extern U8 ComFPGA_Configuration_RevB[N_COMFPGA_BYTES];

/* Communication FPGA configuration (Rev. C) */
extern U8 ComFPGA_Configuration_RevC[N_COMFPGA_BYTES];

/* DSP executable code */
extern U16 DSP_Code[N_DSP_CODE_BYTES];

/* FIPPI configuration */
extern U8 FIPPI_Configuration[N_FIPPI_BYTES];



/* Virtex FIPPI configuration */

extern U8 Virtex_Configuration[N_VIRTEX_BYTES];

/* DSP I/O parameter names */					
extern S8 DSP_Parameter_Names[N_DSP_PAR][MAX_PAR_NAME_LENGTH];
// defined by .var file

/* DSP internal parameter names; not being used in this driver now */
//extern U8 DSP_Memory_Names[N_MEM_PAR][MAX_PAR_NAME_LENGTH];

/* Parameter names applicable to a Pixie channel */
extern S8 Channel_Parameter_Names[N_CHANNEL_PAR][MAX_PAR_NAME_LENGTH];
// defined by Igor or globals.c

/* Parameter names applicable to a Pixie module */
extern S8 Module_Parameter_Names[N_MODULE_PAR][MAX_PAR_NAME_LENGTH];
// defined by Igor or globals.c

/* Parameter names applicable to a whole Pixie system */
extern S8 System_Parameter_Names[N_SYSTEM_PAR][MAX_PAR_NAME_LENGTH];
// defined by Igor or globals.c

/* Parameter values applicable to a whole Pixie system */
extern double System_Parameter_Values[N_SYSTEM_PAR];

//***********************************************************
//		Pixie global data structure
//***********************************************************

struct Pixie_Configuration
{
	/* Parameter values applicable to a Pixie channel */
	double Channel_Parameter_Values[NUMBER_OF_CHANNELS][N_CHANNEL_PAR];
	
	/* DSP I/O parameter values */
	U16 DSP_Parameter_Values[N_DSP_PAR];
	
	/* DSP internal parameter values; not being used in this driver now */
	//U16 DSP_Memory_Values[N_MEM_PAR];

	/* Parameter values applicable to a whole Pixie system */
	double Module_Parameter_Values[N_MODULE_PAR];
};

/* Define PRESET_MAX_MODULES Pixie devices */
extern struct Pixie_Configuration Pixie_Devices[PRESET_MAX_MODULES];

/* This structure is used to read and analyze list mode data files.
 * It contains a number of variables and pointers necessary to create
 * an arbitrary analysis application making use of data in Pixie list 
 * mode files. */

 struct LMReaderDataStruct{
    void  *par0; /* These void parameter pointers can carry  */
    void  *par1; /* any variable, array, structure or function */
    void  *par2; /* necessary for a particular analysis */
    void  *par3;
    void  *par4;
    void  *par5;
    void  *par6;
    void  *par7;
    void  *par8;
    void  *par9;
    FILE  *ListModeFile;	/* List mode file pointer used in file handling routines */
    U8    *ListModeFileName;	/* List mode file name pointer */
    U16    BufferHeader    [BUFFER_HEAD_LENGTH]; /*  BufferHeader[0]: Buffer Length
	                                        BufferHeader[1]: Module Number
	                                        BufferHeader[2]: Run Type
	                                        BufferHeader[3]: Run Start High Word 
	                                        BufferHeader[4]: Run Start Middle Word  
	                                        BufferHeader[5]: Run Start Low Word
                                             */
					     
    U16    EventHeader     [EVENT_HEAD_LENGTH];   /* EventHeader[0]: Hit Pattern
	                                        EventHeader[1]: Event Time High Word
	                                        EventHeader[2]: Event Time Low Word  
                                             */
					     
    U16    ChannelHeader   [MAX_CHAN_HEAD_LENGTH];/* (Types 0x100 and 0x101)      (Type 0x102)        (Type 0x103)
    
                        ChannelHeader[0]:     Channel Length         Fast Trigger Time   Fast Trigger Time
                        ChannelHeader[1]:     Trigger Time           Energy              Energy
						ChannelHeader[2]:     Energy                 XIA PSA Value
						ChannelHeader[3]:     XIA PSA Value          User PSA Value
						ChannelHeader[4]:     User PSA Value
						ChannelHeader[5]:     Unused
						ChannelHeader[6]:     Unused
						ChannelHeader[7]:     Unused
						ChannelHeader[8]:     Real Time High Word
                                             */
    U16    Trace[IO_BUFFER_LENGTH];		/* An array to contain individual trace data */
    U16    Channel;				/* Current channel number when reading an event */
    U16    ChanHeadLen;                 	/* Length of the channel header for the current run type */
    U16    ReadFirstBufferHeader;		/* A flag to read only the first buffer header to extract information about the file */
    U32    Buffers[PRESET_MAX_MODULES];		/* Buffer counters for every module */
    U32    Events [PRESET_MAX_MODULES];		/* Event  counters for every module */
    U32    Traces [PRESET_MAX_MODULES];		/* Trace  counters for every module */
    U32    TotalBuffers;			/* Buffer counter for all modules */
    U32    TotalEvents;				/* Event  counter for all modules */
    U32    TotalTraces;    			/* Trace  counter for all modules */
    S32  (*PreAnalysisAction)    (struct LMReaderDataStruct *); /* Pointer to the user function containing logic before file parsing starts */
    S32  (*BufferLevelAction)    (struct LMReaderDataStruct *);	/* Pointer to the user function containing logic at the buffer  level */
    S32  (*EventLevelAction)     (struct LMReaderDataStruct *);	/* Pointer to the user function containing logic at the event   level */
    S32  (*ChannelLevelAction)   (struct LMReaderDataStruct *);	/* Pointer to the user function containing logic at the channel level */
	S32  (*AuxChannelLevelAction)(struct LMReaderDataStruct *); /* Pointer to the user function containing logic for a channel not in the read pattern */
    S32  (*PostAnalysisAction)   (struct LMReaderDataStruct *);	/* Pointer to the user function containing logic after file parsing ends */
};

typedef struct LMReaderDataStruct * LMR_t;


#ifdef __cplusplus
}
#endif	/* End of notice for C++ compilers */

#endif	/* End of globals.h */