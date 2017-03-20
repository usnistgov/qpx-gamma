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
 * File name:
 *
 *      pixie_c.c
 *
 * Description:
 *
 *      This file contains all the main driver funtions which
 *		can be used to control the Pixie modules.
 *
 * Revision:
 *
 *		3-18-2004
 *
 * Member functions:
 *
 *		Pixie_Hand_Down_Names
 *		Pixie_Boot_System
 *		Pixie_User_Par_IO
 *		Pixie_Acquire_Data
 *		Pixie_Set_Current_ModChan
 *		Pixie_Buffer_IO
 *
******************************************************************************/

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "PlxTypes.h"
#include "Plx.h"

#include "globals.h"
#include "sharedfiles.h"
#include "utilities.h"

/****************************************************************
 *	Pixie_Hand_Down_Names function:
 *		Download file names or parameter names from the host
 *		to the driver.
 *
 *		Names is a two dimensional string array containing
 *		either the All_Files or parameter names.
 *
 *		All_Files is a string array with 6 entries which include
 *		the communication FPGA file, FiPPI file, DSP code bianry
 *		file(DSPcode.bin), DSP code I/O parameter names file (DSPcode.var),
 *		DSP code all parameter names file (.lst), and DSP I/O parameter
 *		values file (.itx).
 *
 *		Return Value:
 *			0 - download successful
 *		   -1 - invalid names
 *
 ****************************************************************/

S32 Pixie_Hand_Down_Names (S8 *Names[],	// a two dimensional string array
			   S8 *Name )		// a string indicating which type of names to be downloaded
{

	U32 len, k, n;
	S8  *pSource, *pDest;

	if(strcmp(Name, "ALL_FILES")==0)
	{
		/* Download file names */
		for( k = 0; k < N_BOOT_FILES; k ++ )
		{
			if (Names[k] == NULL) {                 // in case there is a null element in the array
				sprintf(ErrMSG, "*ERROR* Pixie_Hand_Down_Names(ALL_FILES): null array element %d", k);
				Pixie_Print_MSG(ErrMSG);
				continue;
			}
			len = MIN(strlen(Names[k]), MAX_FILE_NAME_LENGTH);
			pSource = Names[k];
			pDest = Boot_File_Name_List[k];
			n = 0;
			while( n++ < len ) *pDest++ = *pSource++;			
			*pDest = 0;

		}
	}
	else if((strcmp(Name, "SYSTEM")==0) || (strcmp(Name, "MODULE")==0)
					|| (strcmp(Name, "CHANNEL")==0))
	{
		/* Download names of global variables applicable to all modules or individual modules */
		
		if(strcmp(Name, "SYSTEM")==0)
		{
			/* Download names of global variables applicable to all modules */
			for( k = 0; k < N_SYSTEM_PAR; k ++ )
			{
				if (Names[k] == NULL) {                 // in case there is a null element in the array
					sprintf(ErrMSG, "*ERROR* Pixie_Hand_Down_Names(ALL_FILES): null array element %d", k);
					Pixie_Print_MSG(ErrMSG);
					continue;
				}
				len = MIN(strlen(Names[k]), MAX_PAR_NAME_LENGTH);
				pSource = Names[k];
				pDest = System_Parameter_Names[k];
				n = 0;
				while( n++ < len ) *pDest++ = *pSource++;			
				*pDest = 0;
			}
		}
		if(strcmp(Name, "MODULE")==0)
		{
			/* Download names of global variables applicable to individual modules */
			for( k = 0; k < N_MODULE_PAR; k ++ )
			{
				if (Names[k] == NULL) {                 // in case there is a null element in the array
					sprintf(ErrMSG, "*ERROR* Pixie_Hand_Down_Names(ALL_FILES): null array element %d", k);
					Pixie_Print_MSG(ErrMSG);
					continue;
				}
				len = MIN(strlen(Names[k]), MAX_PAR_NAME_LENGTH);
				pSource = Names[k];
				pDest = Module_Parameter_Names[k];
				n = 0;
				while( n++ < len )
				{
					*pDest++ = *pSource++;
				}

				*pDest = 0;
			}
		}
		if(strcmp(Name, "CHANNEL")==0)
		{
			/* Download user variable names applicable to individual channels */
			for( k = 0; k < N_CHANNEL_PAR; k ++ )
			{
				
				if (Names[k] == NULL) {                 // in case there is a null element in the array
					sprintf(ErrMSG, "*ERROR* Pixie_Hand_Down_Names(ALL_FILES): null array element %d", k);
					Pixie_Print_MSG(ErrMSG);
					continue;
				}
				len = MIN(strlen(Names[k]), MAX_PAR_NAME_LENGTH);
				pSource = Names[k];
				pDest = Channel_Parameter_Names[k];
				n = 0;
				while( n++ < len )
				{
					*pDest++ = *pSource++;
				}
				*pDest = 0;
			}
		}
	}
	else
	{
		/* Invalid names */
		sprintf(ErrMSG, "*ERROR* (Pixie_Hand_Down_Names): invalid name %s", Name);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	return (0);
}


/****************************************************************
 *	Pixie_Boot_System function:
 *		Boot all the Pixie modules in the system.
 *
 *		Boot_Pattern is a bit mask:
 *			bit 0:	Boot communication FPGA
 *			bit 1:	Boot FIPPI
 *			bit 2:	Boot DSP
 *			bit 3:	Load DSP parameters
 *			bit 4:	Apply DSP parameters (calls Set_DACs and
 *					Program_FIPPI)
 *
 *		Return Value:
 *			 0 - boot successful
 *			-1 - unable to scan crate slots
 *			-2 - unable to read communication FPGA configuration (Rev. B)
 *			-3 - unable to read communication FPGA configuration (Rev. B)
 *			-4 - unable to read FiPPI configuration
 *			-5 - unable to read DSP executable code
 *			-6 - unable to read DSP parameter values
 *			-7 - unable to initialize DSP parameter names
 *			-8 - failed to boot all modules present in the system
 *
 ****************************************************************/

S32 Pixie_Boot_System (
			U16 Boot_Pattern )	// bit mask
{

	S32 retval = 0;


	/* Scan all crate slots and find the address for each slot where a PCI device is installed */
	retval=Pixie_Scan_Crate_Slots(Number_Modules, &Phy_Slot_Wave[0]);
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Scanning crate slots unsuccessful.");
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	
	/* Read communication FPGA configuration (Rev. B) */
//	retval=Load_U16(Boot_File_Name_List[0], COM_FPGA_CONFIG_REV_B, (N_COMFPGA_BYTES/4));
//	if(retval<0)
//	{
//		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read communication FPGA configuration (Rev. B).");
//		Pixie_Print_MSG(ErrMSG);
//		return(-2);
//	}
	// in this P500 variant (and any newer P4 code), there is no need to read Rev. B Com FPGA data

	/* Read Virtex FPGA  (P500) */
	retval=Load_U16(Boot_File_Name_List[0], VIRTEX_CONFIG, (N_VIRTEX_BYTES/4));
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read Virtex configuration.");
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}

	/* Read communication FPGA configuration (Rev. C) */
	retval=Load_U16(Boot_File_Name_List[1], COM_FPGA_CONFIG_REV_C, (N_COMFPGA_BYTES/4));
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read communication FPGA configuration (Rev. C).");
		Pixie_Print_MSG(ErrMSG);
		return(-3);
	}

	/* Read FiPPI configuration */
	retval=Load_U16(Boot_File_Name_List[2], FIPPI_CONFIG, (N_FIPPI_BYTES/4));
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read FiPPI configuration.");
		Pixie_Print_MSG(ErrMSG);
		return(-4);
	}

	/* Read DSP executable code */
	retval=Load_U16(Boot_File_Name_List[3], DSP_CODE, (N_DSP_CODE_BYTES/4));
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read DSP executable code.");
		Pixie_Print_MSG(ErrMSG);
		return(-5);
	}

	/* Read DSP parameter values */
	retval=Load_U16(Boot_File_Name_List[4], DSP_PARA_VAL, N_DSP_PAR);
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to read DSP parameter values.");
		Pixie_Print_MSG(ErrMSG);
		return(-6);
	}

	/* Initialize DSP variable names */
	retval=Pixie_Init_VarNames();
	if(retval<0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Boot_System): Unable to initialize DSP parameter names.");
		Pixie_Print_MSG(ErrMSG);
		return(-7);
	}
	
	/* Initialize several global variables */
	retval=Pixie_Init_Globals();

	/* Boot all modules present in the system */
	if (Offline == 1) {
		sprintf(ErrMSG, "System started up successfully in offline mode");
		Pixie_Print_MSG(ErrMSG);
	}
	else {
		retval = Pixie_Boot(Boot_Pattern);
		if( retval < 0 )
		{
			return(-8);
		}
	}
	return(0);
}


/****************************************************************
 *	Pixie_User_Par_IO function
 *		Download or upload User Values from or to the host.
 *
 *		User_Par_Values is a double precision array containing
 *		the User Values.
 *
 *		User_Par_Name is a string variable which indicates which user
 *		parameter should be downloaded or uploaded.
 *
 *		User_Par_Type is a string variable specifying the type of
 *		user parameters (System, Module or Channel).
 *
 *		direction:
 *			0:	download from the host to this library.
 *			1:	upload from this library to the host.
 *
 *		Return Value:
 *			 0 - download successful
 *			-1 - null pointer for user parameter values
 *			-2 - invalid user parameter name
 *			-3 - invalid user parameter type
 *			-4 - invalid I/O direction
 *			-5 - invalid Pixie module number
 *			-6 - invalid Pixie channel number
 *
 ****************************************************************/

S32 Pixie_User_Par_IO (
			double *User_Par_Values,	// user parameters to be transferred
			S8 *User_Par_Name,			// user parameter name
			S8 *User_Par_Type,			// user parameter type
			U16 direction,				// transfer direction (read or write)
			U8  ModNum,					// number of the module to work on
			U8  ChanNum )				// channel number of the Pixie module
{
	S32 retval;

	/* Check validity of input parameters */

	if(User_Par_Values == NULL)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): NULL User_Par_Values pointer");
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	if(direction > 1)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): invalid I/O direction, direction=%d", direction);
		Pixie_Print_MSG(ErrMSG);
		return(-4);
	}

	if((strcmp(User_Par_Type, "SYSTEM") != 0) && (ModNum >= Number_Modules))
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): invalid Pixie module number, ModNum=%d", ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-5);
	}
	if(ChanNum >= NUMBER_OF_CHANNELS)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): invalid Pixie channel number, ChanNum=%d", ChanNum);
		Pixie_Print_MSG(ErrMSG);
		return(-6);
	}

	/* Start I/O transfer */

	if((strcmp(User_Par_Type, "SYSTEM") == 0) || (strcmp(User_Par_Type, "MODULE") == 0) ||
			(strcmp(User_Par_Type, "CHANNEL") == 0))
	{

		retval = UA_PAR_IO(User_Par_Values, User_Par_Name, User_Par_Type, direction, ModNum, ChanNum);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): invalid user parameter name, name=%s", User_Par_Name);
			Pixie_Print_MSG(ErrMSG);
			return(-2);
		}
	}
	else
	{
		/* Invalid user parameter types */
		sprintf(ErrMSG, "*ERROR* (Pixie_User_Par_IO): invalid user parameter type, type=%s", User_Par_Type);
		Pixie_Print_MSG(ErrMSG);
		return(-3);
	}

	return(0);
}


/****************************************************************
 *	Pixie_Acquire_Data function:
 *		This is the main function used for data acquisition in MCA run
 *		or list mode run mode. Additionally, it can be used to parse list
 *		mode data files offline.
 *
 *		Run_Type is a 16-bit long word whose lower 12-bit
 *		specifies either data_run_type or control_task_run_type
 *		and upper 4-bit specifies actions(start\stop\poll)
 *
 *			Lower 12-bit:
 *				0x100,0x101,0x102,0x103 list mode runs
 *				0x200,0x201,0x202,0x203	fast list mode runs. discontinued in V2, but keep here for backwards compatibility (treat as 0x10?)
 *				0x301					MCA run
 *				0x1 -> 0x15				control task runs
 *
 *			Upper 4-bit:
 *				0x0000					start a control task run
 *				0x1000					start a new data run
 *				0x2000					resume a data run
 *				0x3000					stop a data run
 *				0x4000					poll run status
 *				0x5000					read histogram data and save it to a file
 *				0x6000					read list mode buffer data and save it to a file
 *				0x7000					offline list mode data parse routines
 *					0x7001					parse list mode data file
 *					0x7002					locate traces
 *					0x7003					read traces
 *					0x7004					read energies
 *					0x7005					read PSA values
 *					0x7006					read extended PSA values
 *					0x7007					locate events
 *					0x7008					read events
 *
 *
 *				0x8000					manually read spectrum from a previously saved MCA file
 *				0x9000					external memory (EM) I/O
 *					0x9001					read histogram memory section of EM 
 *					0x9002					write to histogram memory section of EM
 *					0x9003					read list mode memory section of EM
 *					0x9004					write to list mode memory section of EM
 *				0xA000 					special tasks
 *					0xA001					read data, then resume
 *
 *		User_data receives either the histogram, list mode data
 *		or the ADC trace.
 *
 *		filnam needs to have complete path.
 *
 *		Return Value:
 *
 *			Run type 0x0000
 *				 0x0  - success
 *				-0x1  - invalid Pixie module number
 *				-0x2  - failure to adjust offsets
 *				-0x3  - failure to acquire ADC traces
 *				-0x4  - failure to start the control task run
 *
 *			Run type 0x1000
 *				 0x10 - success
 *				-0x11 - invalid Pixie module number
 *				-0x12 - failure to start the data run
 *
 *			Run type 0x2000
 *				 0x20 - success
 *				-0x21 - invalid Pixie module number
 *				-0x22 - failure to resume the data run
 *
 *			Run type 0x3000
 *				 0x30 - success
 *				-0x31 - invalid Pixie module number
 *				-0x32 - failure to end the run
 *
 *			Run type 0x4000
 *				-0x41 - invalid Pixie module number
 *					0 - no run in progress
 *					1 - run in progress
 *				CSR value - when run tpye = 0x40FF
 *
 *			Run type 0x5000
 *				 0x50 - success
 *				-0x51 - failure to save histogram data to a file
 *
 *			Run type 0x6000
 *				 0x60 - success
 *				-0x61 - failure to save list mode data to a file
 *
 *			Run type 0x7000
 *				 0x70 - success
 *				-0x71 - failure to parse the list mode data file
 *				-0x72 - failure to locate list mode traces
 *				-0x73 - failure to read list mode traces
 *				-0x74 - failure to read event energies
 *				-0x75 - failure to read PSA values
 *				-0x76 - invalid list mode parse analysis request
 *
 *			Run type 0x8000
 *				 0x80 - success
 *				-0x81 - failure to read out MCA spectrum from the MCA file
 *
 *			Run type 0x9000
 *				 0x90 - success
 *				-0x91 - failure to read out MCA section of external memory
 *				-0x92 - failure to write to MCA section of external memory
 *				-0x93 - failure to read out LM section of external memory
 *				-0x94 - failure to write to LM section of external memory
 *				-0x95 - invalid external memory I/O request
 *
 *                      Run type 0xA000
 *				 0xA0 - success
 *              -0xA1 - invalid pixie module number
 *				-0xA2 - failure to stop
 *				-0xA3 - failure to write
 *				-0xA4 - failure to resume
 *              -0xA5 - invalid external runtype (lower 12 bits)        
 *
 ****************************************************************/

S32 Pixie_Acquire_Data (
			U16 Run_Type,		// data acquisition run type
			U32 *User_data,		// an array holding data to be transferred back to the host
			S8  *file_name,		// file name for storing run data 
			U8  ModNum )		// pixie module number
{
	U8      str[256];
	U8	k, m;
	U16	upper, lower, i;
	U16     idx = 65535;
	U32	CSR;
	U32     value;
	S32	retval=0;
	double 	BLcut, tau;

	upper=(U16)(Run_Type & 0xF000);
	lower=(U16)(Run_Type & 0x0FFF);

	if (Offline == 1 && upper != 0x7000 && upper != 0x8000) {
		sprintf(ErrMSG, "(Pixie_Acquire_Data): Offline mode. No I/O operations possible");
		Pixie_Print_MSG(ErrMSG);
		return (0);
	}

	switch(upper)
	{
		case 0x0000:  /* Start a control task run in a Pixie module */

			if (lower == 0x0003) lower = ADJUST_OFFSETS;		// legacy run type (controltask) 3 and 4 are now 0x83 and 0x84
			if (lower == 0x0004) lower = ACQUIRE_ADC_TRACES;
			
			/* Control task should run in only one Pixie module, not all modules at the same time */
			if(ModNum > Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number for control task run, ModNum=%d", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x1);
			}

			switch(lower)
			{  
				case ADJUST_OFFSETS:  
					retval=Adjust_Offsets(ModNum);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to adjust offsets in Module %d, retval=%d", ModNum, retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x2);
					}

					break;
				case ACQUIRE_ADC_TRACES:
					if(ModNum == Number_Modules)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number for 'get traces' controltask, ModNum=%d", ModNum);
						Pixie_Print_MSG(ErrMSG);
						return(-0x1);
					}
					retval=Get_Traces(User_data, ModNum, NUMBER_OF_CHANNELS);	/* Acquire traces for all channels */
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to acquire ADC traces in Module %d, retval=%d", ModNum, retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x3);
					}

					break;
				case 24:
					if(ModNum == Number_Modules)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number for 'get traces' controltask, ModNum=%d", ModNum);
						Pixie_Print_MSG(ErrMSG);
						return(-0x1);
					}
					retval=Get_Slow_Traces(User_data, ModNum, file_name);	/* Acquire traces for all channels */
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to acquire ADC traces in Module %d, retval=%d", ModNum, retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x3);
					}

					break;
					
				case GET_BLCUT:
				
				    /* Find baseline cut value */
				    /* k is the current channel number */
				    /* m is the current module number */
				    
				    if (ModNum == Number_Modules) m = 0;
				    else {
					m              = ModNum;
					Number_Modules = ModNum + 1;
				    }
				    
				    for( ; m < Number_Modules; m++) {
					for(k = 0; k < NUMBER_OF_CHANNELS; k++) {
					    BLcut_Finder(m, k, &BLcut);
					}
					/* Program FiPPI */
					Control_Task_Run(m, PROGRAM_FIPPI, 1000);
					sprintf(ErrMSG, "Module %d finished adjusting BLcut", m);
					Pixie_Print_MSG(ErrMSG);
				    }
				    
				    break;
				    
				case FIND_TAU:
				
				    /* Find tau value */
				    /* k is the current channel number */
				    /* m is the current module number */
				    
					if (ModNum == Number_Modules) {
						m = 0;
					}
				    else {
						m              = ModNum;
						Number_Modules = ModNum + 1;
				    }
				    
				    for( ; m < Number_Modules; m++) {
						for(k = 0; k < NUMBER_OF_CHANNELS; k++) {
							/* The index offset for channel parameters */
							idx=Find_Xact_Match("TAU", Channel_Parameter_Names, N_CHANNEL_PAR);
							tau = Pixie_Devices[m].Channel_Parameter_Values[k][idx]*1.0e-6; 
							Tau_Finder(m, (U8)k, &tau);			
							tau /= 1.0e-6;
							Pixie_Devices[m].Channel_Parameter_Values[k][idx]=tau;
							/* Update DSP parameters */
							sprintf(str,"PREAMPTAUA%d",k);
							idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
							Pixie_Devices[m].DSP_Parameter_Values[idx]=(U16)floor(tau);
							/* Download to the data memory */
							value = (U32)Pixie_Devices[m].DSP_Parameter_Values[idx];
							Pixie_IODM(m, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
							sprintf(str,"PREAMPTAUB%d",k);
							idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
							Pixie_Devices[m].DSP_Parameter_Values[idx]=(U16)((tau-floor(tau))*65536);
							/* Download to the data memory */
							value = (U32)Pixie_Devices[m].DSP_Parameter_Values[idx];
							Pixie_IODM(m, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);			
							BLcut_Finder(m, (U8)k, &BLcut);
						}
						/* Program FiPPI */
						Control_Task_Run(m, PROGRAM_FIPPI, 1000);
						sprintf(ErrMSG, "Module %d finished adjusting tau", m);
						Pixie_Print_MSG(ErrMSG);
				    }
				    
				    break;
				
				case READ_EEPROM_MEMORY:
				
				    for (i = 0; i < EEPROM_MEMORY_SIZE; i++) 
				          I2C24LC16B_Read_One_Byte (ModNum, i, (U8 *)&User_data[i]);
				    break;
				    
				case WRITE_EEPROM_MEMORY:
				    for (i = 0; i < EEPROM_MEMORY_SIZE; i++) {
					I2C24LC16B_Write_One_Byte (ModNum, i, (U8 *)&User_data[i]);
					Pixie_Sleep(6);
				    }
				    break;
				
				case WRITE_EEPROM_MEMORY_SHORT: // Write only 64 bytes of most frequently changed data
				    for (i = 0; i < 64; i++) {
					I2C24LC16B_Write_One_Byte (ModNum, i, (U8 *)&User_data[i]);
					Pixie_Sleep(6);
				    }
				    break;

			
				
				default:  /* note, host must check for run end of the control tasks */
				    retval=Start_Run(ModNum, NEW_RUN, 0, lower);
				    if(retval < 0)
				    {
					sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to start control run in Module %d, retval=%d", ModNum, retval);
					Pixie_Print_MSG(ErrMSG);
					return(-0x4);
				    }
				    break;
			}

			retval=0x0;
			break;

		case 0x1000:  /* Start a new data acquisition run */

			/* Check validity of ModNum */
			if(ModNum > Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x11);
			}

			retval=Start_Run(ModNum, NEW_RUN, lower, 0);
			if(retval < 0)  /* failure to start a new run */
			{
				if(ModNum == Number_Modules)
				{
					/* need to stop the run in case some modules started the run OK */
					End_Run(Number_Modules);
				}
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to start a data run, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x12);
			}

			retval=0x10;
			break;

		case 0x2000:  /* Resume data acquisition run in all modules */

			/* Check validity of ModNum */
			if(ModNum > Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x21);
			}

			retval=Start_Run(ModNum, RESUME_RUN, lower, 0);
			if(retval < 0)  /* failure to resume run in all modules */
			{
				if(ModNum == Number_Modules)
				{
					/* need to stop the run in case some modules started the run OK */
					End_Run(Number_Modules);
				}
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to resume a data run, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x22);
			}
	
			retval=0x20;
			break;

		case 0x3000:  /* Stop a data run */

			/* Check validity of ModNum */
			if(ModNum > Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", lower, ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x31);
			}

			retval=End_Run(ModNum);  /* Stop the run */
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to end the run, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x32);
			}

			retval=0x30;
			break;

		case 0x4000:  /* Poll run status */

			/* Check validity of ModNum */
			if(ModNum >= Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x41);
			}

			switch(lower)
			{	 		
				
				case 0x0F0:  /* read Config Status Register */
					Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, &CSR);
					retval=CSR & 0xFFFF;
					break;
				case 0x0F1:  /* read Config Version Register */
					Pixie_Register_IO(ModNum, PCI_VERSION, MOD_READ, &CSR);
					retval=CSR & 0xFFFF;
					break;
				case 0x0F2:  /* read Config I2C Register */
					Pixie_Register_IO(ModNum, PCI_I2C, MOD_READ, &CSR);
					retval=CSR & 0xFFFF;
					break;
				case 0x0FC:  /* write/clear SP1 */
					CSR=0;
					Pixie_Register_IO(ModNum, PCI_SP1, MOD_WRITE, &CSR);
					break; 		
				case 0x0FD:  /* read SP1 */
					Pixie_Register_IO(ModNum, PCI_SP1, MOD_READ, &CSR);
					retval=CSR;
					break;
				case 0x0FE:  /* Check run status; return value of WCR */
					Pixie_RdWrdCnt(ModNum, &CSR);
					retval=CSR & 0xFFFF;
					break;
				case 0x0FF:  /* Check run status; return value of CSR */
					Pixie_ReadCSR(ModNum, &CSR);
					retval=CSR & 0xFFFF;
					break;
				default:  /* Check run status; only return value of ACTIVE bit */
					retval=Check_Run_Status(ModNum);
					break;
			}
			break;

		case 0x5000:  /* Read histogram data from Pixie's external memory and save it to a file */

			retval=Write_Spectrum_File(file_name);
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to save histogram data to a file, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x51);
			}

			retval=0x50;
			break;


		case 0x6000:  /* Read list mode buffer data from Pixie's external memory and save it to a file */

			retval=Write_List_Mode_File(file_name);
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to save list mode data to a file, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x61);
			}
			
			retval=0x60;
			break;

		case 0x7000:  /* Offline list mode data parse routines */

			switch(lower)
			{
				case 0x1:  /* Parse list mode data file */
					retval=Pixie_Parse_List_Mode_Events(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to parse list mode data file, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x71);
					}
					break;

				case 0x2:  /* locate traces */
					retval=Pixie_Locate_List_Mode_Traces(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to locate list mode traces, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x72);
					}

					break;

				case 0x3:  /* read traces */
					retval=Pixie_Read_List_Mode_Traces(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read list mode traces, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x73);
					}

					break;

				case 0x4:  /* read energies */
					retval=Pixie_Read_Energies(file_name, User_data, ModNum);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read event energies, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x74);
					}

					break;

				case 0x5:  /* read PSA values */
					retval=Pixie_Read_Event_PSA(file_name, User_data, ModNum);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read PSA values, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x75);
					}

					break;

					// XENON modification
				case 0x6:  /* read extended PSA values */
					retval=Pixie_Read_Long_Event_PSA(file_name, User_data, ModNum);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read extended PSA values, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x76);
					}

					break;

        			case 0x7:  /* locate events */
					retval=Pixie_Locate_List_Mode_Events(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to locate list mode traces, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x77);
					}

					break;

        			case 0x8:  /* read events */
					retval=Pixie_Read_List_Mode_Events(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read list mode traces, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x78);
					}

					break;
					
					case 0x10:  /* user reader function for arbitrary processing */
					retval=Pixie_User_List_Mode_Reader(file_name, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read list mode data, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x79);
					}

					break;

				default:
					sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid list mode parse analysis request, Run Type=%x", Run_Type);
					Pixie_Print_MSG(ErrMSG);
					return(-0x79);
			}

			retval=0x70;
			break;

		case 0x8000:  /* Manually read out MCA spectrum from a MCA file */

			/* Check validity of ModNum */
			if(ModNum >= Number_Modules)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", lower, ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-0x82);
			}

			retval = Read_Spectrum_File(ModNum, User_data, file_name);
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read out MCA spectrum from the MCA file, retval=%d", retval);
				Pixie_Print_MSG(ErrMSG);
				return(-0x81);
			}

			retval=0x80;
			break;

		case 0x9000:  /* Pixie external memory I/O */

			switch(lower)
			{
				case 0x1:  
					/* Read out MCA section of EM */
					retval=Pixie_IOEM(ModNum, HISTOGRAM_MEMORY_ADDRESS, MOD_READ, HISTOGRAM_MEMORY_LENGTH, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read out MCA section of external memory, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x91);
					}
					
					break;

				case 0x2:
					/* debug: write to MCA section of EM */
					retval=Pixie_IOEM(ModNum, HISTOGRAM_MEMORY_ADDRESS, MOD_WRITE, HISTOGRAM_MEMORY_LENGTH, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to write to MCA section of external memory, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x92);
					}

					break;

				case 0x3:
					/* debug: read LM section of EM */
					retval=Pixie_IOEM(ModNum, LIST_MEMORY_ADDRESS, MOD_READ, LIST_MEMORY_LENGTH, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to read out LM section of external memory, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x93);
					}

					break;

				case 0x4:	
					/* debug: write to LM section of EM */
					retval=Pixie_IOEM(ModNum, LIST_MEMORY_ADDRESS, MOD_WRITE, LIST_MEMORY_LENGTH, User_data);
					if(retval < 0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to write to LM section of external memory, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0x94);
					}

					break;

				case 0x100: /* Phoswitch BAA */
					User_data[0] = 0xB;
					User_data[1] = 0xA;
					User_data[2] = 0xA;
					User_data[3] = 2;
					User_data[4] = 0;
					User_data[5] = 1;
					User_data[6] = 2;

					break;

				default:
					sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid external memory I/O request, Run Type=%x", Run_Type);
					Pixie_Print_MSG(ErrMSG);
					return(-0x95);
			}
			break;
			
		case 0xA000:		// special functions
			switch(lower)
			{
				case 0x0:
				case 0x1:  /* poll, write, resume a data run */
				case 0x2:
				case 0x3:				
					/* Check validity of ModNum */
					if(ModNum >= Number_Modules)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid Pixie module number, ModNum=%d", ModNum);
						Pixie_Print_MSG(ErrMSG);
						return(-0xA4);
					}
									
					retval = Read_Resume_Run(ModNum, (U8)lower, file_name); /* poll, write, resume */
					if(retval < 0)  /* failure */
					{
						/* need to stop the run in case some modules started the run OK */
						End_Run(Number_Modules);
						sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): failure to poll/read/resume modules, retval=%d", retval);
						Pixie_Print_MSG(ErrMSG);
						return(-0xA0 + retval);
					}
					
					retval=0xA0+retval;
					break;

				default:
					sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid special function request, Run Type=%x", Run_Type);
					Pixie_Print_MSG(ErrMSG);
					return(-0xAF);
			}
			break;
			
		default:
			sprintf(ErrMSG, "*ERROR* (Pixie_Acquire_Data): invalid run type, Run Type=%x", Run_Type);
			Pixie_Print_MSG(ErrMSG);
			return(-0xA0);
	}

	return(retval);

}


/****************************************************************
 *	Pixie_Set_Current_ModChan function:
 *		Set the current Module number and Channel number.
 *
 *		Return Value:
 *			 0 if successful
 *			-1 invalid module number
 *			-2 invalid channel number
 *
 ****************************************************************/

S32 Pixie_Set_Current_ModChan (
			U8 Module,			// module name
			U8 Channel )		// channel name
{
	U16 idx;
	U32 value;

	

	if (Module >= Number_Modules)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Set_Current_ModChan): invalid module number, ModNum=%d", Module);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	if(Channel >= NUMBER_OF_CHANNELS)	// This is a NUMBER_OF_CHANNELS-channel device
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Set_Current_ModChan): invalid channel number, ChanNum=%d", Channel);
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}

	Chosen_Chan = Channel;
	Chosen_Module = Module;

	/* Update ChanNum in DSP */
	idx = Find_Xact_Match("CHANNUM", DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[Chosen_Module].DSP_Parameter_Values[idx] = Chosen_Chan;

	/* Download to the data memory */
	value = (U32)Pixie_Devices[Chosen_Module].DSP_Parameter_Values[idx] ;
	Pixie_IODM(Chosen_Module, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	return(0);
}


/****************************************************************
 *	Pixie_Buffer_IO function:
 *		Download or upload I/O parameter values from or to the
 *		host.
 *
 *		Direction:	0 (write), 1 (read)
 *		Type:     	0 (DSP I/O parameters)
 *					1 (All DSP variables)
 *					2 (Settings file I/O)
 *					3 (Copy or extract Settings)
 *					4 (Set the address and length of I/O buffer
 *					   for writing)
 *		Values:		unsigned short array
 *
 *		Return Value:
 *			 0 - Success
 *			-1 - failure to set DACs after writing DSP parameters
 *			-2 - failure to program Fippi after writing DSP parameters
 *			-3 - failure to set DACs after loading DSP parameters
 *			-4 - failure to program Fippi after loading DSP parameters
 *			-5 - can't open settings file for loading
 *			-6 - can't open settings file for reading
 *			-7 - can't open settings file to extract settings
 *			-8 - failure to set DACs after copying or extracting settings
 *			-9 - failure to program Fippi after copying or extracting settings
 *		   -10 - invalid module number
 *		   -11 - invalid I/O direction
 *		   -12 - invalid I/O type
 *
 ****************************************************************/

S32 Pixie_Buffer_IO (
			U16 *Values,		// an array hold the data for I/O
			U8 type,			// I/O type
			U8 direction,		// I/O direction
			S8 *file_name,		// file name
			U8 ModNum)			// number of the module to work on
{
	U32 buffer[DATA_MEMORY_LENGTH];
	U16 len, i, j, DSPIOValues[N_DSP_PAR], idx;
	S8  filnam[MAX_FILE_NAME_LENGTH];
	FILE *settingsFile = NULL;
	S32 retval;

	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize ot Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters

	/* Check the validity of Modnum */
	if (ModNum >= Number_Modules)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid Module number %d", ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-10);
	}

	/* Ensure file name length not greater than the maximum length */
	len = (U16)MIN(strlen(file_name), MAX_FILE_NAME_LENGTH);
	strncpy(filnam, file_name, len);
	filnam[len]=0;

	if(type==0)  /* DSP I/O parameter values */
	{
		if(direction==1)  /* Read */
		{
			if(Offline == 1)  /* Load previously saved parameter values for offline analysis */
			{
				for(i=0; i<N_DSP_PAR; i++) *Values++ = Pixie_Devices[ModNum].DSP_Parameter_Values[i];
				return(0);
			}
			/* Read out all the DSP I/O parameters */
			Pixie_IODM(ModNum, DATA_MEMORY_ADDRESS, MOD_READ, N_DSP_PAR, buffer);
			for(i=0; i<N_DSP_PAR; i++)
			{
				*Values++ = (U16) buffer[i];
				Pixie_Devices[ModNum].DSP_Parameter_Values[i] = (U16) buffer[i];
			}
		}
		else if(direction==0)  /* Write */
		{
			/* Load DSP values into DSP_Parameter_Values */
			for(i=0; i<DSP_IO_BORDER; i++)
			{
				Pixie_Devices[ModNum].DSP_Parameter_Values[i] = *Values;
				buffer[i] = (U32)*Values++;
			}
			if (Offline == 1) return (0); //No I/O in offline mode
			Pixie_IODM(ModNum, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, buffer);
			retval = Control_Task_Run(ModNum, SET_DACS, 10000);
			if(retval<0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to set DACs in Module %d after writing DSP parameters.", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-1);
			}
			retval = Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
			if(retval<0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to program Fippi in Module %d after writing DSP parameters.", ModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-2);
			}
		}
		else if(direction==2)  /* Write, but do not apply */
		{
			sprintf(ErrMSG, "*INFO* (Pixie_Buffer_IO): write only.");
			Pixie_Print_MSG(ErrMSG);
			/* Load DSP values into DSP_Parameter_Values */
			for(i=0; i<DSP_IO_BORDER; i++)
			{
				Pixie_Devices[ModNum].DSP_Parameter_Values[i] = *Values;
				buffer[i] = (U32)*Values++;
			}
			if (Offline == 1) return (0);
			Pixie_IODM(ModNum, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, buffer);
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid I/O direction %d", direction);
			Pixie_Print_MSG(ErrMSG);
			return(-11);
		}
	}
	else if(type==1)  /* All DSP variables, but read only */
	{
		if(direction==1)  /* Read */
		{
			if (Offline == 1) return (0);  /* Returns immediately for offline analysis */
			/* Read out all the DSP I/O parameters */
			Pixie_IODM(ModNum, DATA_MEMORY_ADDRESS, MOD_READ, DATA_MEMORY_LENGTH, buffer);
			for(i=0; i<N_MEM_PAR; i++)
			{
				*Values++ = (U16)buffer[i];
				//Pixie_Devices[ModNum].DSP_Memory_Values[i] = (U16) buffer[i];
			}
		}
		else if(direction==1)  /* Write */
		{
			/* Skip the first Writing_IOBuffer_Address words of the DSP data memory buffer */
			Values += Writing_IOBuffer_Address;
			/* Load DSP values into DSP_Parameter_Values */
			for(i=0; i<Writing_IOBuffer_Length; i++) buffer[i] = (U32)*Values++;
			if (Offline == 1) return (0);
			Pixie_IODM(ModNum, DATA_MEMORY_ADDRESS+Writing_IOBuffer_Address, MOD_WRITE, Writing_IOBuffer_Length, buffer);
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid I/O direction %d", direction);
			Pixie_Print_MSG(ErrMSG);
			return(-11);
		}
	}
	else if(type==2)  /* Settings file I/O */
	{
		if(direction==1)  /* Load */
		{
			settingsFile = fopen(filnam, "rb");
			if(settingsFile != NULL)
			{
				for(i=0;i<PRESET_MAX_MODULES;i++)
					fread(Pixie_Devices[i].DSP_Parameter_Values, 2, N_DSP_PAR, settingsFile);
				fclose(settingsFile);
				if (Offline == 1) return (0);
				/* Find the index of FILTERRANGE in the DSP; energy filter interval needs update */
				idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
				for(i=0;i<Number_Modules;i++)
				{
					/* Convert data type */
					for(j=0; j<DSP_IO_BORDER; j++) buffer[j] = (U32)Pixie_Devices[i].DSP_Parameter_Values[j];
					/* Download new settings into the module */
					Pixie_IODM((U8)i, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, buffer);
					retval = Control_Task_Run((U8)i, SET_DACS, 10000);
					if(retval<0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to set DACs in Module %d after loading DSP parameters.", i);
						Pixie_Print_MSG(ErrMSG);
						return(-3);
					}
					retval = Control_Task_Run((U8)i, PROGRAM_FIPPI, 1000);
					if(retval<0)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to program Fippi in Module %d after loading DSP parameters.", i);
						Pixie_Print_MSG(ErrMSG);
						return(-4);
					}
					// Define clock constants according to BoardRevision 
					Pixie_Define_Clocks ((U8)i,(U8)0,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale,&ThisADCclkMHz );
					/* Update energy filter interval */
					Filter_Int[i]=pow(2.0, (double)Pixie_Devices[i].DSP_Parameter_Values[idx])/FILTER_CLOCK_MHZ;
				}
			}
			else
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): can't open settings file %s for loading", filnam);
				Pixie_Print_MSG(ErrMSG);
				return(-5);
			}
		}
		else if(direction==0)  /* Save */
		{
			settingsFile = fopen(filnam, "wb");
			if(settingsFile != NULL)
			{
				/* Read out the DSP I/O parameters for all the Pixie modules in the system */
				if (Offline == 0) {
					for(i=0; i<Number_Modules; i++)
					{
						Pixie_IODM((U8)i, DATA_MEMORY_ADDRESS, MOD_READ, N_DSP_PAR, buffer);
						for(j=0; j<N_DSP_PAR; j++) Pixie_Devices[i].DSP_Parameter_Values[j] = (U16)buffer[j];
					}
				}
				for(i=0; i<PRESET_MAX_MODULES; i++) 
					fwrite(Pixie_Devices[i].DSP_Parameter_Values, 2, N_DSP_PAR, settingsFile);
				fclose(settingsFile);
			}
			else
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): can't open settings file %s for reading", filnam);
				Pixie_Print_MSG(ErrMSG);
				return(-6);
			}
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid I/O direction %d", direction);
			Pixie_Print_MSG(ErrMSG);
			return(-11);
		}
	}
	else if(type==3)  /* Copy or extract settings */
	{
		if(direction==1)  /* Copy */
		{
			/* Load settings in source module and channel to DSPIOValues */
			for(i=0;i<N_DSP_PAR;i++) DSPIOValues[i] = Pixie_Devices[Values[0]].DSP_Parameter_Values[i];
		}
		else if(direction==0)  /* Extract */
		{
			/* Open the settings file */
			settingsFile = fopen(filnam, "rb");
			if(settingsFile != NULL)
			{
				/* Position settingsFile to the source module location */
				fseek(settingsFile, Values[0]*N_DSP_PAR*2, SEEK_SET);
				fread(DSPIOValues, 2, N_DSP_PAR, settingsFile);
				fclose(settingsFile);
			}
			else
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): can't open settings file %s to extract settings", filnam);
				Pixie_Print_MSG(ErrMSG);
				return(-7);
			}
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid I/O direction %d", direction);
			Pixie_Print_MSG(ErrMSG);
			return(-11);
		}

		/* Copy settings to the destination modules and channels */
		for(i=0;i<Number_Modules;i++)
		{
			for(j=0;j<NUMBER_OF_CHANNELS;j++) 
				if( TstBit(j, Values[i+3]) ) 
					Pixie_CopyExtractSettings((U8)Values[1], Values[2], (U8)i, (U8)j, DSPIOValues);
		}
		if (Offline == 1) return (0);
		/* Download new settings into each module present in the system */
		for(i=0;i<Number_Modules;i++)
		{			
			for(j=0; j<DSP_IO_BORDER; j++) buffer[j] = (U32)Pixie_Devices[i].DSP_Parameter_Values[j];
			Pixie_IODM((U8)i, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, buffer);
			retval = Control_Task_Run((U8)i, SET_DACS, 10000);
			if(retval<0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to set DACs in Module %d after copying or extracting settings.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-8);
			}
			retval = Control_Task_Run((U8)i, PROGRAM_FIPPI, 1000);
			if(retval<0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): failure to program Fippi in Module %d after copying or extracting settings.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-9);
			}
		}
	}
	else if(type==4)	/* Set the address and length of I/O buffer for writing */
	{
		Writing_IOBuffer_Address = *Values++;
		Writing_IOBuffer_Length = *Values;
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Buffer_IO): invalid I/O type %d", type);
		Pixie_Print_MSG(ErrMSG);
		return(-12);
	}

	return(0);
}
