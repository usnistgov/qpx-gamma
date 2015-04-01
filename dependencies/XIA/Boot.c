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
 *      Boot.c
 *
 * Description:
 *
 *      This file contains all the funtions used to boot Pixie modules.
 *
 * Revision:
 *
 *		11-30-2004
 *
 * Member functions:
 *
 *		Pixie_Init_VarNames
 *		Load_Names
 *		Load_U16
 *		Pixie_Boot
 *		Pixie_Boot_ComFPGA
 *		Pixie_Boot_FIPPI

 *		Pixie_Boot_Virtex
 *		Pixie_Boot_DSP
 *		Pixie_Scan_Crate_Slots
 *		Pixie_Close_PCI_Devices
 *		Pixie_Init_Globals
 *
******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Plx.h"
#include "PexApi.h"

#include "globals.h"
#include "sharedfiles.h"
#include "boot.h"
#include "utilities.h"




// PXI system initialization arrays replacing the pxisys.ini files

U8  PCIBusNum_18Slots_ini[PRESET_MAX_PXI_SLOTS+1]    = {0, 0, 1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3};
U8  PCIBusNum_14Slots_ini[PRESET_MAX_PXI_SLOTS+1]    = {0, 0, 1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  0,  0,  0,  0};
U8  PCIBusNum_8Slots_ini [PRESET_MAX_PXI_SLOTS+1]    = {0, 0, 1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
U8  PCIBusNum_1062_ini [PRESET_MAX_PXI_SLOTS+1]      = {0, 0, 1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
U8  PCIDeviceNum_18Slots_ini[PRESET_MAX_PXI_SLOTS+1] = {0, 0, 15, 14, 13, 11, 10, 15, 14, 13, 11, 10, 9,  15, 14, 13, 12, 11, 10};		// slot 5 is not a PCI slot
U8  PCIDeviceNum_14Slots_ini[PRESET_MAX_PXI_SLOTS+1] = {0, 0, 15, 14, 13, 12, 11, 10, 8,  15, 14, 13, 12, 11, 10, 0,  0,  0,  0}; 
U8  PCIDeviceNum_8Slots_ini [PRESET_MAX_PXI_SLOTS+1] = {0, 0, 15, 14, 13, 12, 11, 10, 9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
U8  PCIDeviceNum_1062_ini [PRESET_MAX_PXI_SLOTS+1]   = {0, 0, 15, 14, 13, 13, 12, 11, 10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0};


#define PRINT_DEBUG_MSG 	0

/****************************************************************
 *	Pixie_Init_VarNames:
 *		Initialize DSP I/O parameter names and DSP internal memory
 *		parameter names from files, namely, DSPcode.var and DSPcode.lst
 *		file, by calling function Load_Names.
 *
 *		Return Value:
 *			 0 - load successful
 *			-1 - file not found
 *			-2 - requested number of names to be read exceeds limits
 *			-3 - premature EOF encountered
 *
*****************************************************************/

S32 Pixie_Init_VarNames(void)
{
	S32   retval;

	/* DSP I/O parameter names */
	retval = Load_Names(Boot_File_Name_List[5], DSP_PARA_NAM, N_DSP_PAR, MAX_PAR_NAME_LENGTH);
	if( retval == -1 )
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Init_VarNames): DSP I/O parameter file %s was not found", Boot_File_Name_List[5]); 
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	else if( retval == -2 )
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Init_VarNames): Requested number of DSP I/O parameter names to be read exceeds limits"); 
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}
	else if( retval == -3 )
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Init_VarNames): Premature EOF encountered when read DSP I/O parameter file %s", Boot_File_Name_List[5]); 
		Pixie_Print_MSG(ErrMSG);
		return(-3);
	}

	/* DSP internal memory parameter names; currently not being used. */
	/*
	retval = Load_Names(Boot_File_Name_List[6], DSP_MEM_NAM, N_MEM_PAR, MAX_MEM_NAME_LENGTH);
	if( retval == -1 ){
		sprintf(ErrMSG, "*ERROR* (Pixie_Init_VarNames): DSP all variable file %s was not found", Boot_File_Name_List[6]); 
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	else if( retval == -2 ){
		sprintf(ErrMSG, "*ERROR* (Pixie_Init_VarNames): Requested number of DSP internal memory parameter names to be read exceeds limits"); 
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}
	*/

	return 0;
}


/****************************************************************
 *	Load_Names:
 *		Receive DSP I/O parameter names and and DSP internal memory
 *		parameter names from files, namely, DSPcode.var and DSPcode.lst
 *		file.
 *
 *		Return Value:
 *			 0 - load successful
 *			-1 - file not found
 *			-2 - requested number of names to be read exceeds limits
 *			-3 - premature EOF encountered
 *
 ****************************************************************/

S32 Load_Names (
			S8 *filnam,			// file name
			U8 ConfigName,		// determine which type of name to load
			U32 nNames,			// number of names to be loaded
			U16 Name_Length)	// the limit of the name length
{
	U16  k, len;
	S32  retval=0;
	U8   i;

	S8	 DSPParaName[MAX_PAR_NAME_LENGTH];

	FILE* namesFile = NULL;
	namesFile = fopen(filnam, "r");

	if(namesFile != NULL)
	{
		switch(ConfigName)
		{

			case DSP_PARA_NAM:  /* DSP parameter names */
				
				/* Check if nNames is within limits of N_DSP_PAR */
				if(nNames > N_DSP_PAR)
				{
					retval = -2;
					break;
				}

				for(k=0; k<nNames; k++)
				{
					strcpy(DSP_Parameter_Names[k],"");  /* Clear all names */
				}

				for(k=0; k<nNames; k++)  /* Read names */
				{
					if( fgets(DSPParaName, Name_Length, namesFile) != NULL )  /* Read one line */
					{
						len=(U16)strlen(DSPParaName);  /*  Get the line length */
						if(len>6)  /* Check if the line contains real names; the index holds 5 spaces */
						{
							for(i=0;i<(len-5);i++)
							{
								DSP_Parameter_Names[k][i]=DSPParaName[i+5];
							}

							DSP_Parameter_Names[k][len-6]=0;  /* Remove the new line character at the end of the name */
						}
					}
					else  /* premature EOF encountered */
					{
						retval=-3;
						break;
					}
				}
				retval=0;
				break;

			/* Read DSP internal memory parameter names; currently not being used. */
			/*
			case DSP_MEM_NAM:	// All DSP variable names

 				// Check if nNames is within limits of N_MEM_PAR
				if(nNames > N_MEM_PAR)
				{
					retval = -2;
					return(retval);
				}

				for(k=0; k<nNames; k++)
				{
					strcpy(DSP_Memory_Names[k],"");  // Clear all names
				}


				do  // Read names
				{					
					if( fgets(DSPParaName, Name_Length, namesFile) != NULL )  // Read one line
					{
						for(i=0; i<5; i++)
						{
							NameConv[i]=DSPParaName[i];  // Get the name index
						}

						idx=atoi(NameConv);	 // Convert the index into numerical value
						len=strlen(DSPParaName); // Get the line length

						for(i=0;i<(len-7);i++)
						{
							DSP_Memory_Names[idx][i]=DSPParaName[i+7];
						}

						DSP_Memory_Names[idx][len-8]=0;  // Remove the new line character at the end of the name

					}
					else  // The end of file was reached
						break;
				}while(1);
				retval=0;
				break;
			*/
		}

		fclose(namesFile);  // Close file
		return(retval);		
	}
	else
	{
		return(-1);  // File not found
	}
}


/****************************************************************
 *	Load_U16:
 *		Load communication FPGA configuration, FIPPI configuration,
 *		DSP code, and DSP I/O variable values from files in unsigned
 *		16-bit format.
 *	
 *		Return Value:
 *			 0 - load successful
 *			-1 - file not found
 *			-2 - requested number of values to be read exceeds limits
 *			-3 - reading incomplete
 *
 ****************************************************************/

S32 Load_U16 (
			S8 *filnam,			// file name
			U8 ConfigName,		// determine which type of parameter to load
			U32 ndat)			// number of data words to load
{

	U32   dummy[N_DSP_CODE_BYTES/4], nWords;
	S32   retval=0;
	U16   k;

	FILE *dataFile = NULL;
	dataFile = fopen(filnam, "rb");
	if(dataFile != NULL)
	{	
		switch(ConfigName)
		{		
			case COM_FPGA_CONFIG_REV_B:	/* Communication FPGA configuration (Rev. B) */

				/* Check if ndat is within limits of N_COMFPGA_BYTES */
				if(ndat > N_COMFPGA_BYTES/4)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of communication FPGA configuration bytes exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				nWords = fread(ComFPGA_Configuration_RevB, 4, ndat, dataFile);
				if((ndat - nWords) > 1)
				{
					/* ndat differing from nWords by 1 is OK if N_COMFPGA_BYTES is an odd number */
					sprintf(ErrMSG,"*ERROR* (Load_U16): reading communication FPGA configuration (Rev. B) incomplete.");
					Pixie_Print_MSG(ErrMSG);
					retval = -3;
				}
				else
				{
					retval = 0;
				}

				break;

			case COM_FPGA_CONFIG_REV_C:	/* Communication FPGA configuration (Rev. C) */

				/* Check if ndat is within limits of N_COMFPGA_BYTES */
				if(ndat > N_COMFPGA_BYTES/4)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of communication FPGA configuration bytes exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				nWords = fread(ComFPGA_Configuration_RevC, 4, ndat, dataFile);
				if((ndat - nWords) > 1)
				{
					/* ndat differing from nWords by 1 is OK if N_COMFPGA_BYTES is an odd number */
					sprintf(ErrMSG,"*ERROR* (Load_U16): reading communication FPGA configuration (Rev. C) incomplete.");
					Pixie_Print_MSG(ErrMSG);
					retval = -3;
				}
				else
				{
					retval = 0;
				}

				break;

			case FIPPI_CONFIG:	/* FIPPI configuration */

				/* Check if ndat is within limits of N_FIPPI_BYTES */
				if(ndat > N_FIPPI_BYTES/4)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of FiPPI configuration bytes exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				nWords = fread(FIPPI_Configuration, 4, ndat, dataFile);
				if((ndat - nWords) > 1)
				{
					/* ndat differing from nWords by 1 is OK if N_FIPPI_BYTES is an odd number */
					sprintf(ErrMSG, "*ERROR* (Load_U16): reading FIPPI configuration in Module %d incomplete.", Chosen_Module);
					Pixie_Print_MSG(ErrMSG);
					retval = -3;
				}
				else
				{
					retval = 0;
				}

				break;



			case VIRTEX_CONFIG:	/* P500 Virtex configuration */


				/* Check if ndat is within limits of N_VIRTEX_BYTES */
				if(ndat > N_VIRTEX_BYTES/4)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of Virtex configuration bytes exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				nWords = fread(Virtex_Configuration, 4, ndat, dataFile);

				if((ndat - nWords) > 1)
				{
					/* ndat differing from nWords by 1 is OK if N_VIRTEX_BYTES is an odd number */
					sprintf(ErrMSG, "*ERROR* (Load_U16): reading FIPPI configuration in Module %d incomplete.", Chosen_Module);
					Pixie_Print_MSG(ErrMSG);
					retval = -3;
				}
				else
				{
					retval = 0;
				}

				break; 



			case DSP_CODE:	/* DSP code */

				/* Check if ndat is within limits of N_DSP_CODE_BYTES */
				if(ndat > N_DSP_CODE_BYTES/4)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of DSP executable code bytes exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				nWords = fread(dummy, 4, ndat, dataFile);
				if( nWords < ndat )
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): reading DSP code incomplete.");
					Pixie_Print_MSG(ErrMSG);
					retval = -3;
				}
				else
				{
					for(k=0;k<(N_DSP_CODE_BYTES/4);k++)
					{
						DSP_Code[2*k+1]= (U16)(dummy[k] & 0xFF);
						DSP_Code[2*k]= (U16)(dummy[k]>>8);
					}
					retval = 0;
				}
				break;

			case DSP_PARA_VAL:	/* DSP parameter values */

				/* Check if ndat is within limits of N_DSP_PAR */
				if(ndat > N_DSP_PAR)
				{
					sprintf(ErrMSG,"*ERROR* (Load_U16): Requested number of DSP parameter values exceeds limits.");
					Pixie_Print_MSG(ErrMSG);
					retval = -2;
					break;
				}

				/* Read DSP parameter values module by module from the settings file */
				for(k=0; k<PRESET_MAX_MODULES; k++)
				{
					nWords = fread(Pixie_Devices[k].DSP_Parameter_Values, 2, ndat, dataFile);
					if( nWords < ndat )
					{
						sprintf(ErrMSG, "*ERROR* (Load_U16): reading DSP parameter values incomplete.");
						Pixie_Print_MSG(ErrMSG);
						retval = -3;
						break;
					}

					/* Force the correct Module Number in the DSP Parameter Values.*/
					/* This is necessary since the wrong module number in the      */
					/* settings file could screw up the operation of that module   */
					/* without being easily detected.                              */
					Pixie_Devices[k].DSP_Parameter_Values[0] = k;

				}
				retval = 0;
				break;

			}

			fclose(dataFile);	/* Close file */
			if(PRINT_DEBUG_MSG){
				sprintf(ErrMSG, "*INFO* (Load_U16): finished loading file %s", filnam);
				Pixie_Print_MSG(ErrMSG);
			}
			return(retval);		
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Load_U16): file %s was not found", filnam);
		Pixie_Print_MSG(ErrMSG);
		return(-1);		/* File not found */
	}
}


/****************************************************************
 *	Pixie_Boot:
 *		Boot the Pixie according the Boot_Pattern.
 *		Boot_Pattern is a bit mask:
 *			bit 0:	Boot ComFPGA
 *			bit 1:	Boot FIPPI
 *			bit 2:	Boot DSP
 *			bit 3:	Load DSP parameters
 *			bit 4:	Apply DSP parameters (calls Set_DACs
 *					and Program_FIPPI)
 *
 *		Return Value:
 *			 0 - boot successful
 *			-1 - downloading communication FPGA failed
 *			-2 - downloading FiPPI configuration failed
 *			-3 - downloading DSP code failed
 *			-4 - failed to set DACs
 *			-5 - failed to program FiPPI
 *			-6 - failed to enable detector input
 *			-7 - incorrect boot pattern
 *
 ****************************************************************/

S32 Pixie_Boot (U16 Boot_Pattern)		// boot pattern is a bit mask
{

	S32 retval;
	U16 k;
	U32 buffer[DATA_MEMORY_LENGTH];
	U8  modulenumber, i;
	U16	BoardRevision;


	if(!Boot_Pattern){
	    sprintf(ErrMSG, "*INFO* (Pixie_Boot): 0x0 boot pattern, doing nothing");
	    Pixie_Print_MSG(ErrMSG);
	    return (0);
	}

	if(Boot_Pattern > 0x1F){
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot): incorrect boot pattern: %hu", Boot_Pattern);
	    Pixie_Print_MSG(ErrMSG);
	    return (-7);
	}

	// **************************************************************
	//	Set up pullup resisters (only one module should be set the
	//	pullup resisters to 1, but this module could be any module in
	//	in the system)
	// **************************************************************

	buffer[0] = 0x3F;	/* Connect pullup resisters on FT, ET, Veto, Time, Sync, and TRreturn */
	modulenumber = 0;	/* Here we set Module #0 */
	Pixie_Register_IO(modulenumber, PCI_PULLUP, MOD_WRITE, buffer);



	/* For all other modules, we disconnect pullup resisters */
	for(i=1; i<Number_Modules; i++)
	{
		buffer[0] = 0x00;	// Disconnect pullup resisters on FT, ET, Veto, Time, Sync, and TRreturn 
		Pixie_Register_IO(i, PCI_PULLUP, MOD_WRITE, buffer);
	}

	// **************************************************************
	// Download communication FPGA
	// **************************************************************
	if( Boot_Pattern & 0x1)  
	{
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot): Begin booting ComFPGA");
			Pixie_Print_MSG(ErrMSG);
		}
		// Boot communication FPGA 
		for(i=0; i<Number_Modules; i++)
		{
			retval=Pixie_Boot_ComFPGA(i);
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Boot): downloading communication FPGA configuration to module %d was not successful.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-1);
			}
		}
	}

	// **************************************************************
	// Download FIPPI
	// **************************************************************
	if( Boot_Pattern & 0x2)  
	{
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot): Begin booting FIPPI");
			Pixie_Print_MSG(ErrMSG);
		}

		for(i=0; i<Number_Modules; i++)
		{
			/* Returns immediately for offline analysis */
			if(Offline != 1) 
			{
		
				// Get Board Revision 
				BoardRevision = (U16)Pixie_Devices[i].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)];

				retval = -2;
				if((BoardRevision & 0x0F00) == 0x0700) retval=Pixie_Boot_FIPPI(i);		// For Pixie-4: Boot FIPPI 
	
				if((BoardRevision & 0x0F00) == 0x0300) retval=Pixie_Boot_Virtex(i);		// For P500: Boot Virtex 
				if(retval < 0)
				{
					sprintf(ErrMSG, "*ERROR* (Pixie_Boot): downloading FiPPI configuration to module %d was not successful.", i);
					Pixie_Print_MSG(ErrMSG);
					return(-2);
				}
			}
		}
	}

	/* enable LED */
	for(i=0; i<Number_Modules; i++)
	{
		buffer[0] = 0x38;
		Pixie_Register_IO(i, PCI_CFCTRL, MOD_WRITE, buffer);
	}


	// **************************************************************
	// Download DSP code
	// **************************************************************
	if( Boot_Pattern & 0x4)  
	{
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot): Begin booting DSP");
			Pixie_Print_MSG(ErrMSG);
		}

		/* Boot DSP */
		for(i=0; i<Number_Modules; i++)
		{
			retval=Pixie_Boot_DSP(i);
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Boot): downloading DSP code to module %d was not successful.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-3);
			}
		}
	}

	// **************************************************************
	// Download DSP parameter values
	// **************************************************************
	if( Boot_Pattern & 0x8)  
	{
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot): Begin DSP parameter download");
			Pixie_Print_MSG(ErrMSG);
		}

		/* Download DSP parameter values */
		for(i=0; i<Number_Modules; i++)
		{
			/* Convert data type */
			for(k=0; k<DSP_IO_BORDER; k++)
			{
				buffer[k] = (U32)Pixie_Devices[i].DSP_Parameter_Values[k];
			}

			/* Download new settings into the module */
			Pixie_IODM(i, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, buffer);
		}
	}


	// **************************************************************
	// Now start control tasks 0 and 5 to set DACs and program FIPPI
	// **************************************************************
	if( Boot_Pattern & (long) 0x10)
	{
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot): Begin Program FIPPI");
			Pixie_Print_MSG(ErrMSG);
		}
		
		for(i=0; i<Number_Modules; i++) 
		{
			retval=Control_Task_Run(i, SET_DACS, 10000); /* Set DACs */
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Boot): Set DACs in module %d failed.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-4);
			}

			retval=Control_Task_Run(i, PROGRAM_FIPPI, 1000); /* Program FiPPI */
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Boot): Program FiPPI in module %d failed.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-5);
			}

			retval=Control_Task_Run(i, ENABLE_INPUT, 1000); /* Connect detector input */
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Boot): Enable detector input in module %d failed.", i);
				Pixie_Print_MSG(ErrMSG);
				return(-6);
			}

            
		}
	}

	// Success
	for(i=0; i<Number_Modules; i++)
	{
		sprintf(ErrMSG, "Module %d in slot %d started up successfully!", i, Phy_Slot_Wave[i]);
		Pixie_Print_MSG(ErrMSG);
	}

	return(0);
}


/****************************************************************
 *	Pixie_Boot_ComFPGA:
 *		Download communication FPGA configuration.
 *
 *		Return Value:
 *			 0 - boot successful
 *			-1 - failed to read board version
 *			-2 - clearing communication FPGA timed out
 *			-3 - downloading communication FPGA timed out
 *
 ****************************************************************/

S32 Pixie_Boot_ComFPGA (
			U8 ModNum )		// Pixie module number
{

	U16	BoardRevision;
	U32	buffer[0x5];
	U32     k;
	U32     counter0;
	U32     counter1;

	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	/* Get Board Revision */
	BoardRevision = (U16)Pixie_Devices[ModNum].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)];

	sprintf(ErrMSG, "Module %d, Revision = %X", ModNum, BoardRevision);
	Pixie_Print_MSG(ErrMSG);

	/* Initialize counter0 to 0 */
	buffer[0] = counter0 = 0;
	/* if buffer[0] = 0x21, download Conf. FPGA successful */
	while ((buffer[0] & 0x21) != 0x21 && counter0 < 10) {
	    /* Set communication FPGA Program*=0 to clear it */
	    buffer[0] = 0x41; 
	    /* Write to an offset from the PCI-to-Local space window */
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);
	    /* Set communication FPGA Program*=1 to start configuration */
	    buffer[0] = 0x49;
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);
	    /* Initialize counter1 to 0. If buffer[0] = 0x20, finished clearing communication FPGA */
	    counter1 = 0;
	    while ((buffer[0] & 0x21) != 0x20 && counter1 < 100) {
		Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer);
	        counter1++;
	    }
	    if (counter1 == 100)
	    {
			sprintf(ErrMSG, "*ERROR* (Pixie_Boot_ComFPGA): Clearing communication FPGA in Module %d timed out.", ModNum);
			Pixie_Print_MSG(ErrMSG);
		return(-2);
	    }
	    /* Set communication FPGA Program *= 1 to start downloading */
	    // Rev. D uses same FPGA (for now)
	    for (k = 0; k < N_COMFPGA_BYTES; k++)
	    {
			if      (BoardRevision == 0xA701) Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&ComFPGA_Configuration_RevB[k]);  // Pixie-4, Rev B
			else if (BoardRevision == 0xA300) Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&ComFPGA_Configuration_RevC[k]);	// P500, Rev A uses P4 Rev C system
			else if (BoardRevision == 0xA301) Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&ComFPGA_Configuration_RevC[k]);	// Pixie-500, Rev B uses P4 Rev C system
			else if (BoardRevision == 0xA311) Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&ComFPGA_Configuration_RevC[k]);	// Pixie-500, Rev B (400 MHz) uses P4 Rev C system
			else if (BoardRevision >= 0xA702) Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&ComFPGA_Configuration_RevC[k]);  // Pixie-4, Rev C, D
			// a very short wait if it fails the first time, increasing as we fail more often
			counter1 = 0;
			while (counter1 < (100*counter0/One_Cycle_Time))		// 500 works for PXIe-8130 (mostly); longer delays seem (sometimes) necessary for Xe laptop
				counter1++;		
			Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer); // Dummy read except for the status read on the last byte
	    }
	    counter0++;
	}
	
	if(counter0 == 10)
	{
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot_ComFPGA): buffer = %x", buffer[0]);
	    Pixie_Print_MSG(ErrMSG);
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot_ComFPGA): Downloading communication FPGA to Module %d timed out", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-3);
	}
	else
	{

		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* Downloaded communication FPGA successfully to Module %d", ModNum);
			Pixie_Print_MSG(ErrMSG);	

		}
	}

	return(0);
}


/****************************************************************
 *	Pixie_Boot_FIPPI:
 *		Download RTPU FPGA (FIPPI).
 *
 *		Return Value:
 *			0  - boot successful
 *			-1 - clearing FiPPI timed out
 *			-2 - downloading FiPPI timed out
 *
 ****************************************************************/



S32 Pixie_Boot_FIPPI (
			U8 ModNum )		// Pixie module number
{

	U32	buffer[0x5], k, counter0, counter1;


	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);


	/* Initialize counter0 to 0 */
	buffer[0] = counter0 = 0;
	/* if buffer[0] = 0x3F, download FIPPI0&1 FPGA successful */
	while (buffer[0] != 0x3F && counter0 < 10) {
	    /* Set FIP0&FIP1 Chip Program*=0 to clear configuration */
	    buffer[0] = 0x4E; 

	    /* Write to an offset from the PCI-to-Local space window */
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);

	    /* Set FIP0&FIP1 Chip Program*=1 to start configuration */
	    buffer[0] = 0x7E;
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);

	    /* Initialize counter to 0 */
	    /* if buffer[0] = 0x39, finished clearing FIPPI configuration */
	    counter1 = 0;

	    while (buffer[0] != 0x39 && counter1 < 100) {
			Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer);
			counter1++;
	    }

	    if (counter1 == 100) {
			sprintf(ErrMSG, "*ERROR* (Pixie_Boot_FIPPI): Clearing FIPPI configuration timed out for Module %d", ModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
	    }

	    for (k = 0; k < N_FIPPI_BYTES; k++)	// now download
	    {
	        Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&FIPPI_Configuration[k]);
			// a very short wait if it fails the first time, increasing as we fail more often
			counter1 = 0;
			while (counter1 < (100*counter0/One_Cycle_Time))		// 500 works for PXIe-8130 (mostly); longer delays seem (sometimes) necessary for Xe laptop
				counter1++;		
			Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer); // Dummy read except the last byte to get status
    	}
	    counter0++;
	}
	

	if (counter0 == 10) 
	{
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot_FIPPI): Downloading FIP0&1 FPGA timed out for Module %d", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-2);
	}
	else
	{
	    if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* Downloaded FIP0&1 FPGAs Successfully for Module %d", ModNum);
			Pixie_Print_MSG(ErrMSG);
		}
	}


	return(0);
}



/****************************************************************
 *	Pixie_Boot_Virtex:
 *		Download Virtex FPGA (FIPPI) for P500.
 *
 *		Return Value:
 *			0  - boot successful
 *			-1 - clearing FiPPI timed out
 *			-2 - downloading FiPPI timed out
 *
 ****************************************************************/


S32 Pixie_Boot_Virtex (
			U8 ModNum )		// Pixie module number
{
	U32	buffer[0x5], k, counter0, counter1;

	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	/* Initialize counter0 to 0 */
	buffer[0] = 0;
	counter0 = 1;

	/* if buffer[0] = 0x3F, download FIPPI0&1 FPGA successful */
	while (((buffer[0] & 0x0A) != 0x0A) && (counter0 < 10)) {
		if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* (Pixie_Boot_Virtex): download cycle %d, CFSTATUS = %x",counter0,buffer[0]);
			Pixie_Print_MSG(ErrMSG);
		}
		sprintf(ErrMSG, "*INFO* (Pixie_Boot_Virtex): downloading FPGA configuration, this may take several seconds");
		Pixie_Print_MSG(ErrMSG);


	    /* Set FIP0&FIP1 Chip Program*=0 to clear configuration */
	    buffer[0] = 0x4E; 

	    /* Write to an offset from the PCI-to-Local space window */
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);

	    /* Set FIP0&FIP1 Chip Program*=1 to start configuration */
	    buffer[0] = 0x7E;
	    Pixie_Register_IO(ModNum, PCI_CFCTRL, MOD_WRITE, buffer);

	    /* Initialize counter to 0 */
	    /* if buffer[0] = 0x39, finished clearing FIPPI configuration */
	    counter1 = 0;

	    while (((buffer[0] & 0x08) != 0x08) && (counter1 < 100)) {
			Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer);
			counter1++;
	    }

	    if (counter1 == 100) {
			sprintf(ErrMSG, "*ERROR* (Pixie_Boot_Virtex): Clearing FIPPI configuration timed out for Module %d", ModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
	    }
		
		Pixie_Sleep(10*(counter0+1));			// this sleep seems necessary to ensure proper booting

	    for (k = 0; k < N_VIRTEX_BYTES; k++)	// now download
	    {
	        Pixie_Register_IO(ModNum, PCI_CFDATA, MOD_WRITE, (U32 *)&Virtex_Configuration[k]);
			// a very short wait
			counter1 = 0;
			while (counter1 < (500*counter0/One_Cycle_Time))		// 500 works for PXIe-8130 (mostly); longer delays seem (sometimes) necessary for Xe laptop
				counter1++;

		}
		Pixie_Sleep(10);		// this sleep is probably not necessary
		Pixie_Register_IO(ModNum, PCI_CFSTATUS, MOD_READ, buffer); // read to get status
	    counter0++;
	}

	if (counter0 == 10) 
	{
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot_Virtex): Downloading Virtex timed out for Module %d", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-2);
	}

	else
	{
	    if(PRINT_DEBUG_MSG){
			sprintf(ErrMSG, "*INFO* Downloaded Virtex Successfully for Module %d", ModNum);
			Pixie_Print_MSG(ErrMSG);
		}
	}

	return(0);

}

/*******************************************************************
 *	Pixie_Boot_DSP:
 *		Download DSP code. 	Note: The conversion routine that
 *		creates the binary DSP code will produce a fixed-length
 *		data array with 32768 16-bit words.
 *
 *		Return Value:
 *			 0 - boot successful
 *			-1 - downloading DSP code failed
 *
 ****************************************************************/

S32 Pixie_Boot_DSP (
			U8 ModNum )		// Pixie module number
{

	U32	buffer[0x5], k, counter;

	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	/* Initialize counter to 0 */
	k = counter = 0;
	while (k < N_DSP_CODE_BYTES/2 && counter < 3) {
	    /* reset DSP for 6 cycles */
	    buffer[0] = 0x14;
	    Pixie_Register_IO(ModNum, PCI_CSR, MOD_WRITE, buffer);
	    Pixie_Sleep(70);

	    /* Set RUNENABLE=0 */
	    buffer[0] = 0x00;
	    Pixie_Register_IO(ModNum, PCI_CSR, MOD_WRITE, buffer);
		
	    /*  DSP codes download, with ready enabled, but no burst. IDMA long write and long read */
	    /* write PM address starting from 0x01 */
	    buffer[0] = 0x01;
	    Pixie_Register_IO(ModNum, PCI_IDMAADDR, MOD_WRITE, buffer);
	    Pixie_Sleep(3);
	    for (k = 2; k < N_DSP_CODE_BYTES/2; k++) Pixie_Register_IO(ModNum, PCI_IDMADATA, MOD_WRITE, (U32 *)&DSP_Code[k]);
	    
		/* write IDMA address 0x00 */
	    buffer[0] = 0x00;
	    Pixie_Register_IO(ModNum, PCI_IDMAADDR, MOD_WRITE, buffer);
	    
		/* download DSP code at PM 00 address */
	    buffer[0]=(U32)DSP_Code[0];
	    Pixie_Register_IO(ModNum, PCI_IDMADATA, MOD_WRITE, buffer);
	    buffer[0]=(U32)DSP_Code[1];
	    Pixie_Register_IO(ModNum, PCI_IDMADATA, MOD_WRITE, buffer);
	    Pixie_Sleep(20);
	    
		/* write IDMA address 0x00 */
	    buffer[0] = 0x00;
	    Pixie_Register_IO(ModNum, PCI_IDMAADDR, MOD_WRITE, buffer);
	    Pixie_Sleep(3);
	    
		/* Read back PM for verifying */
	    k = 0;
	    while (Pixie_Register_IO(ModNum, PCI_IDMADATA, MOD_READ, buffer), buffer[0] == (U32)DSP_Code[k] && k < N_DSP_CODE_BYTES/2) { k++; }
	    counter++;
	}
	
	if (k < N_DSP_CODE_BYTES/2) {
	    sprintf(ErrMSG, "*ERROR* (Pixie_Boot_DSP): Downloading DSP code failed for module %d", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-1);
	}
	if(PRINT_DEBUG_MSG){
		sprintf(ErrMSG, "*INFO* Downloaded DSP codes sucessfully for module %d", ModNum);
		Pixie_Print_MSG(ErrMSG);

	}
	
	return(0);
}




/****************************************************************
*	Pixie_Scan_Crate_Slots:
*		Scan all PXI/CompactPCI crate slots and obtain virtual address
*		for each slot where a PCI device is installed.
*
*		Return Value:
*			 0 - Successful
*			-1 - Failed to measure host computer speed (ns per cycle)
*			-2 - Can't open PXI system initialization file
*			-3 - Unable to close the PLX device
*			-4 - Unable to find module 0 in the system
*			-5 - Could not open PCI Device
*			-6 - Unable to map a PCI BAR and obtain a virtual address 
*
****************************************************************/

S32 Pixie_Scan_Crate_Slots (
			U8 NumModules, // Number of modules present
			U8 *PXISlotMap ) // Slot-module correspondence map
{
	PLX_DEVICE_KEY DevKey;
	PLX_STATUS     rc;
	U8             ByteValue;
	U8             *PCIDeviceNum = NULL;
	U8             *PCIBusNum    = NULL;
	U16            k;
	U16            m;
	U32            BoardInfo;
	U32            BoardRevision;
	U32            ClassCodeReg;
	S8             BusNumOffset;		  // the logic ensures that this number is always >=0
	S32            retval;
	
	
	if(Offline == 1)  /* Returns immediately for offline analysis */
	{
		return(0);
	}
	
	
	// Measure host computer speed (ns per cycle)
	retval = get_ns_per_cycle(&One_Cycle_Time); 
	if(retval < 0)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Failed to measure host computer speed (ns per cycle), retval = %d", retval);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	if(PRINT_DEBUG_MSG){
		sprintf(ErrMSG, "*INFO* (Pixie_Scan_Crate_Slots): Host computer speed (ns per cycle) = %.3f", One_Cycle_Time);
		Pixie_Print_MSG(ErrMSG);
	}
	
	// Setting the slot - (device number) correspondence mimicing the pxisys.ini file
	if(PRINT_DEBUG_MSG){
		sprintf(ErrMSG, "*INFO* (Pixie_Scan_Crate_Slots): Maximum Number of Modules in a Crate = %i", Max_Number_of_Modules);
		Pixie_Print_MSG(ErrMSG);
	}
	
	if      (Max_Number_of_Modules == 7) {
	    PCIBusNum    = &PCIBusNum_8Slots_ini[0];
	    PCIDeviceNum = &PCIDeviceNum_8Slots_ini[0];
	}
	else if (Max_Number_of_Modules == 13) {
	    PCIBusNum    = &PCIBusNum_14Slots_ini[0];
	    PCIDeviceNum = &PCIDeviceNum_14Slots_ini[0];
	}
	else if (Max_Number_of_Modules == 17) {
	    PCIBusNum    = &PCIBusNum_18Slots_ini[0];
	    PCIDeviceNum = &PCIDeviceNum_18Slots_ini[0];
	}
	else if (Max_Number_of_Modules == 62) {
	    PCIBusNum    = &PCIBusNum_1062_ini[0];
	    PCIDeviceNum = &PCIDeviceNum_1062_ini[0];
	}
	else {
	    sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Cannot find PXI initialization constants for selected chassis, retval = %d", -2);
	    Pixie_Print_MSG(ErrMSG);
	    return(-2);
	}

	/* Determine bus number offset */
	memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));
	DevKey.VendorId = 0x10b5; // PLX Vendor ID is 0x10b5
	DevKey.DeviceId = 0x9054; // PLX Device ID is 0x9054
	rc = PlxPci_DeviceFind(&DevKey, 0); // Looks for the first PLX device defined by the system
	if (rc != ApiSuccess)
	{
	    sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Can't find any PLX devices, PlxPci_DeviceFind rc=%d", rc);
	    Pixie_Print_MSG(ErrMSG);
	    return(-4);
	}
	
	ClassCodeReg = 0;
	k = 0;
	while (k < MAX_PCI_DEV && ClassCodeReg != 0x604) {  // Looking for a PCI-to-PCI bridge on the bus of the first PLX device
	    ClassCodeReg = PlxPci_PciRegisterRead_BypassOS(DevKey.bus, (U8)k, 0, 0x8, &rc) >> 16;


	 	switch (rc) {		// more return code checking, 
			case ApiSuccess:
				break;

			case ApiNoActiveDriver:
				sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): PlxPci_PciRegisterRead_BypassOS() - no driver");
				Pixie_Print_MSG(ErrMSG);
				return(-5);

			case ApiUnsupportedFunction:
				sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): PlxPci_PciRegisterRead_BypassOS() - unsupported");
				Pixie_Print_MSG(ErrMSG);
				return(-5);

			default:
//				sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): PlxPci_PciRegisterRead_BypassOS() - 0x%x  ?", rc);
//				Pixie_Print_MSG(ErrMSG);
//				return(-5);
				break;

                }
	 

//	    if (rc != ApiSuccess)  {
//			sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to read register #1");
//			Pixie_Print_MSG(ErrMSG);
//			return(-5);
//	    }

	    k++;
	}

	if(PRINT_DEBUG_MSG){
		sprintf(ErrMSG, "*INFO* (Pixie_Scan_Crate_Slots): Scanning for busses done");
		Pixie_Print_MSG(ErrMSG);
	}
	
	if (Max_Number_of_Modules == 7 && k == MAX_PCI_DEV) BusNumOffset = DevKey.bus - 1; // No more bridges exist on the bus of the 8-slot crate
	if (Max_Number_of_Modules == 7 && k != MAX_PCI_DEV) { // Otherwise we got a problem
	    sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to find bus number offset");
	    Pixie_Print_MSG(ErrMSG);
	    return(-5);
	}
	if (Max_Number_of_Modules == 62 && k == MAX_PCI_DEV) BusNumOffset = DevKey.bus - 1; // No more bridges exist on the bus of the 8-slot crate
	if (Max_Number_of_Modules == 62 && k != MAX_PCI_DEV) { // Otherwise we got a problem
	    sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to find bus number offset");
	    Pixie_Print_MSG(ErrMSG);
	    return(-5);
	}
	if (Max_Number_of_Modules == 13 && k == MAX_PCI_DEV) BusNumOffset = DevKey.bus - 2; // If no bridges exist on the bus of the first PLX device in a 14-slot crate then the device is in the second bus of the crate
	if (Max_Number_of_Modules == 13 && k != MAX_PCI_DEV) BusNumOffset = DevKey.bus - 1; // Otherwise the device is in the first bus of the crate
	
	if (Max_Number_of_Modules == 17 && k == MAX_PCI_DEV) BusNumOffset = DevKey.bus - 3; // If no bridges exist on the bus of the first PLX device in a 18-slot crate then the device is in the third bus of the crate
	if (Max_Number_of_Modules == 17 && k != MAX_PCI_DEV) { // Otherwise more checking is necessary
	    ClassCodeReg = 0;
	    k = 0;
	    while (k < MAX_PCI_DEV && ClassCodeReg != 0x604) {  // Looking for a PCI-to-PCI bridge on the bus next to the one containing the first PLX device
		ClassCodeReg = PlxPci_PciRegisterRead_BypassOS((U8)(DevKey.bus + 1), (U8)k, 0, 0x8, &rc) >> 16;
		if (rc != ApiSuccess)
		{
		    sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to read register #2");
		    Pixie_Print_MSG(ErrMSG);
		    return(-5);
		}
		k++;
	    }
	    if (k == MAX_PCI_DEV) BusNumOffset = DevKey.bus - 2; // If none is found the device is in the second bus
	    else                  BusNumOffset = DevKey.bus - 1; // Otherwise the device is in the first bus
	}


	// Open PCI devices for all modules
	for(k = 0; k < (U16)NumModules; k++)
	{
		// Clear key structure
		memset(&DevKey, PCI_FIELD_IGNORE, sizeof(PLX_DEVICE_KEY));
		DevKey.VendorId = 0x10b5; // PLX Vendor ID is 0x10b5
		DevKey.DeviceId = 0x9054; // PLX Device ID is 0x9054
		DevKey.bus  = PCIBusNum   [PXISlotMap[k]] + (U8)BusNumOffset;		// BusNumOffset is >=0 by logic above
		DevKey.slot = PCIDeviceNum[PXISlotMap[k]];
		
		// Use the previously mapped PlxModIndex as the Device Number in the search
		rc = PlxPci_DeviceFind(&DevKey, 0);
		if (rc == ApiSuccess)
		{
			if(PRINT_DEBUG_MSG){
				sprintf(ErrMSG, "System Bus = %i  Local bus  = %i", DevKey.bus, PCIBusNum[PXISlotMap[k]]);                         // Physical device location
				Pixie_Print_MSG(ErrMSG);
				sprintf(ErrMSG, "Device Number = %i  Crate Slot Number = %i", DevKey.slot, PXISlotMap[k]);
				Pixie_Print_MSG(ErrMSG);
			}
		
			rc = PlxPci_DeviceClose(&Sys_hDevice[k]);
			if (rc != ApiSuccess && rc != 0x206)
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to close the PLX device for module %d; rc=%d", k, rc);
				Pixie_Print_MSG(ErrMSG);
				return(-2);
			}
			rc = PlxPci_DeviceOpen(&DevKey, &Sys_hDevice[k]);
			if(rc != ApiSuccess) // Print error if failure
			{
				sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Could not open PCI Device Number (%d) at Bus Number %d; rc=%d", PCIDeviceNum[PXISlotMap[k]], PCIBusNum[PXISlotMap[k]] + BusNumOffset, rc);
				Pixie_Print_MSG(ErrMSG);
			
				// Before return, we need to close those PCI devices that are already opened
				for(m=0; m<k; m++)
				{
					rc = PlxPci_DeviceClose(&Sys_hDevice[m]);
					if (rc != ApiSuccess)
					{
						sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to close the PLX device for module %d; rc=%d", k, rc);
						Pixie_Print_MSG(ErrMSG);
						return(-2);
					}
				}
			
				return(-6);
			}
			else
			{
				// Map a PCI BAR into user virtual space and return the virtual address
				// for the opened PCI device. For PLX 9054, Space 0 is at PCI BAR 2.
				rc = PlxPci_PciBarMap(&Sys_hDevice[k], 2, (VOID**)&VAddr[k]);
				if(rc != ApiSuccess)
				{
					sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to map a PCI BAR and obtain a virtual address for Module=%d; rc=%d", k, rc);
					Pixie_Print_MSG(ErrMSG);
					
					// Before return, we need to close those PCI devices that are already opened
					for(m=0; m<k; m++)
					{
						rc = PlxPci_DeviceClose(&Sys_hDevice[m]);
						if (rc != ApiSuccess)
						{
							sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to close the PLX device for module %d; rc=%d", k, rc);
							Pixie_Print_MSG(ErrMSG);
							return(-2);
						}
					}
					return(-7);
				}
				else
				{
					if(PRINT_DEBUG_MSG){
						sprintf(ErrMSG, "VAddr[%d][%d]=0x%lx", PCIBusNum[PXISlotMap[k]] + BusNumOffset, k, VAddr[k]);
						Pixie_Print_MSG(ErrMSG);
					}
				}

				// Get Revision and Serial Number
				Pixie_ReadVersion((U8)k, &BoardInfo); // read from Xilinx PROM
				Pixie_Devices[k].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)]=(double)(BoardInfo & 0xFFFF);
				
				/* Read from Microchip EEPROM
				   EEPROM contents		Pixie-4		Pixie-500	Notes
				   word 0: revision		0,2,3,4		0,1,2		0,1,2, ... = Rev A, B, C ..., except P4 Rev B is "0" 
				   word 1: S/N			0-250		0-250		add 250 for P4 Rev D to get true S/N
																add 450 for P4 Rev E to get true S/N
				*/
				BoardRevision = 255; // initialize to bad value
				retval = I2C24LC16B_Read_One_Byte((U8)k, 0x0, &ByteValue);
				if(retval < 0)
				{
					sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Failed to read I2C board revision in Module %d", k);
					Pixie_Print_MSG(ErrMSG);
				}
				else
				{
                	if( (BoardInfo & 0x0F00) == 0x700 )	// Pixie-4
					{
						if(ByteValue == 0)
						{
							BoardRevision = 1;			// Rev/ B
						}
						else if( (ByteValue >= 2) & (ByteValue <= 4) )
						{
							BoardRevision = ByteValue;  // Rev. C & D & E
						}
					}		
					else if( (BoardInfo & 0x0F00) == 0x300 ) // P500
					{
						if(ByteValue <= 1)
						{
							BoardRevision = ByteValue;  // Rev A and B
						}
					}
					if(PRINT_DEBUG_MSG){
						sprintf(ErrMSG, "*INFO* (Pixie_Scan_Crate_Slots): Xilinx: %X I2C: %d", BoardInfo, ByteValue );
						Pixie_Print_MSG(ErrMSG);
					}
					
				    if( (BoardInfo & 0xF ) != (BoardRevision) )
				    {
                    	sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Board revision mismatch in Module %d", k);
						Pixie_Print_MSG(ErrMSG);
						// But since we use the Xilinx PROM value, reading a mismatch value from the EEPROM is not fatal
				    }
				}
				
				
				retval = I2C24LC16B_Read_One_Byte((U8)k, 0x1, &ByteValue);
				if(retval < 0)
				{
					sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Failed to read serial number in Module %d", k);
					Pixie_Print_MSG(ErrMSG);
				}
				else
				{
				    if( (BoardInfo & 0x0F0F ) == 0x703)
				    {
						Pixie_Devices[k].Module_Parameter_Values[Find_Xact_Match("SERIAL_NUMBER", Module_Parameter_Names, N_MODULE_PAR)]=(double)(ByteValue) +250.0;	// Pixie-4 Rev. D serial numbers start at 250, but are stored as S/N-250
				    }
					else if ( (BoardInfo & 0x0F0F ) == 0x704)
					{
						Pixie_Devices[k].Module_Parameter_Values[Find_Xact_Match("SERIAL_NUMBER", Module_Parameter_Names, N_MODULE_PAR)]=(double)(ByteValue) +450.0;	// Pixie-4 Rev. E serial numbers start at 450, but are stored as S/N-450
				    }
				    else
				    {
	                	Pixie_Devices[k].Module_Parameter_Values[Find_Xact_Match("SERIAL_NUMBER", Module_Parameter_Names, N_MODULE_PAR)]=(double)(ByteValue);
				    }
				}
				if(PRINT_DEBUG_MSG){
					sprintf(ErrMSG, "(Pixie_Scan_Crate_Slots): Module # %i SERIAL NUMBER = %i", k, (U8)ByteValue);
					Pixie_Print_MSG(ErrMSG);
				}
			}
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): PlxPci_DeviceFind failed for PCI Device Number (%d) at Bus Number %d; rc=%d", PCIDeviceNum[PXISlotMap[k]], PCIBusNum[PXISlotMap[k]] + BusNumOffset, rc);
			Pixie_Print_MSG(ErrMSG);
			
			// Before return, we need to close those PCI devices that are already opened
			for(m=0; m<k; m++)
			{
				rc = PlxPci_DeviceClose(&Sys_hDevice[m]);
				if (rc != ApiSuccess)
				{
					sprintf(ErrMSG, "*ERROR* (Pixie_Scan_Crate_Slots): Unable to close the PLX device for module %d; rc=%d", k, rc);
					Pixie_Print_MSG(ErrMSG);
					return(-2);
				}
			}
			
			return(-8);
		}
	}

	if(PRINT_DEBUG_MSG){
		sprintf(ErrMSG, "(Pixie_Scan_Crate_Slots) Finished scanning all modules");
		Pixie_Print_MSG(ErrMSG);
	}
	
	return(0);
}



/****************************************************************
 *	Pixie_Close_PCI_Devices function:
 *		Unmap PCI BAR and close PLX PCI devices.
 *
 *		Return Value:
 *			 0 - successful
 *			-1 - Unable to unmap the PCI BAR
 *			-2 - Unable to close the PCI device
 *
 ****************************************************************/

S32 Pixie_Close_PCI_Devices (
			U16 ModNum )		// Pixie module number
{

	PLX_STATUS	rc;

	/* Unmaps a previously mapped PCI BAR from user virtual space */
	rc = PlxPci_PciBarUnmap(&Sys_hDevice[ModNum], (VOID **)&VAddr[ModNum]);
	if(rc != ApiSuccess)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Close_PCI_Devices): Unable to unmap the PCI BAR; rc=%d", rc);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	/* Release the PLX device */
	rc = PlxPci_DeviceClose(&Sys_hDevice[ModNum]);
	if (rc != ApiSuccess)
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Close_PCI_Devices): Unable to close the PLX device; rc=%d", rc);
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}

	return(0);
}


/****************************************************************
 *	Pixie_Init_Globals: 
 *		Initialize some frequently used globals. 
 * 
 *		Return Value: 
 *			0  - successful 
 ****************************************************************/

S32 Pixie_Init_Globals(void) {

	U8     i;
	S8     str[128];
	U16    idx;
   U16    version;
	double value;

	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale = P4_LTSCALE;			// The scaling factor for live time counters


	/* Set frequently used indices */
	Run_Task_Index=Find_Xact_Match("RUNTASK", DSP_Parameter_Names, N_DSP_PAR);
	Control_Task_Index=Find_Xact_Match("CONTROLTASK", DSP_Parameter_Names, N_DSP_PAR);
	Resume_Index=Find_Xact_Match("RESUME", DSP_Parameter_Names, N_DSP_PAR);
	RUNTIME_Index=Find_Xact_Match("RUNTIMEA", DSP_Parameter_Names, N_DSP_PAR);
	NUMEVENTS_Index=Find_Xact_Match("NUMEVENTSA", DSP_Parameter_Names, N_DSP_PAR);
	SYNCHDONE_Index=Find_Xact_Match("SYNCHDONE", DSP_Parameter_Names, N_DSP_PAR);

	for(i=0; i<NUMBER_OF_CHANNELS; i++)
	{
		sprintf(str,"FASTPEAKSA%d",i);
		FASTPEAKS_Index[i]=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);

		sprintf(str,"LIVETIMEA%d",i);
		LIVETIME_Index[i]=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	}

	/* Make SGA gain table */
	Make_SGA_Gain_Table();

	/* Initialize energy filter interval */
	for(i=0; i<Number_Modules; i++)
	{
		// Define clock constants according to BoardRevision 
		Pixie_Define_Clocks (i,0,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale,&ThisADCclkMHz );

		
		idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
		value=(double)Pixie_Devices[i].DSP_Parameter_Values[idx];

		Filter_Int[i]=pow(2.0, value)/FILTER_CLOCK_MHZ;

	}

   // Check if this is ZDT code
   idx=Find_Xact_Match("DSPBUILD", DSP_Parameter_Names, N_DSP_PAR);
	version=Pixie_Devices[0].DSP_Parameter_Values[idx];
   if( (version & 0x0F00) == 0x0A00)
      ZDT = 1;
   else
      ZDT = 0;



	return(0);
}








