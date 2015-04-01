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
 *      Utilities.c
 *
 * Description:
 *
 *      This file contains all the boot, control and other utilities functions
 *		for Pixie.
 *
 * Revision:
 *
 *		12-1-2004
 *
 * Member functions:
 *	1) Run control functions:
 *		Check_Run_Status, Control_Task_Run, End_Run, Get_Traces, Run_Manager,
 *		Start_Run
 *
 *	2) Pixie memory I/O functions:
 *		Pixie_IODM, Pixie_IOEM, Read_List_Mode_Data,
 *		Read_Spectrum_File, Write_List_Mode_File, Write_Spectrum_File, 

 *		Pixie_Register_IO, Pixie_RdWrdCnt, Pixie_ReadCSR , Pixie_WrtCSR, Pixie_ReadVersion.
 *
 *	3) Pixie User Parameter I/O functions used not only in ua_par_io:

 *	    BLcut_Finder, Make_SGA_Gain_Table, Pixie_CopyExtractSettings :		 
 *		
 *	4) Pixie parse functions:
 *		Pixie_Locate_List_Mode_Traces, Pixie_Parse_List_Mode_Events, Pixie_Read_Energies,
 *		Pixie_Read_List_Mode_Traces, Pixie_Read_Event_PSA, Pixie_Read_Long_Event_PSA, 
 *	    Pixie_Locate_List_Mode_Events, Pixie_Read_List_Mode_Events 
 *
 *	5) Pixie automatic optimization functions:
 *		Phi_Value, Linear_Fit, RandomSwap, Tau_Finder,
 *		Tau_Fit, Thresh_Finder, Adjust_Offsets, Adjust_AllOffsets 
 *
 *	6) Utility functions:
 *		ClrBit, Find_Xact_Match, RoundOff
 *		SetBit, TglBit, TstBit, Pixie_Sleep	
 *
 *  7) Functions that apply differences for different board types and variants
 *     Pixie_Define_Clocks
 *
******************************************************************************/


#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef XIA_WINDOZE
	#include <io.h>
#endif
#ifdef XIA_LINUX
	#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "PlxTypes.h"
#include "PciTypes.h"
#include "Plx.h"

#include "globals.h"
#include "sharedfiles.h"
#include "utilities.h"

#include "Reg9054.h"
#include "PexApi.h"


#ifdef COMPILE_IGOR_XOP
	#include <XOPStandardHeaders.h>
#endif

/****************************************************************
 *	Start_Run function:
 *		Starts a new run or resumes a run in a Pixie module or multiple
 *		modules.
 *
 *		Don't check Run Task or Control Task for validity.
 *		The DSP code takes care of that. There are two legal
 *		values for Type: NEW_RUN and RESUME_RUN.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to stop a previous run
 *
 ****************************************************************/

S32 Start_Run (
		U8  ModNum,			// Pixie module number
		U8  Type,			// run type (NEW_RUN or RESUME_RUN)
		U16 Run_Task,		// run task
		U16 Control_Task )	// control task
{

	S32 retval;
	U32 CSR, address, value;
	U32 buffer[MAX_HISTOGRAM_LENGTH*NUMBER_OF_CHANNELS]={0};

	U8  k;

	if(ModNum == Number_Modules)  /* Start run in all modules */
	{
		/* Prepare for a run: ending a previous run, clear external memory, set parameters */
		for(k = 0; k < Number_Modules; k ++)
		{
			/* Check if there is a run in progresss; if so, end it */
			retval = Check_Run_Status(k);
			if(retval == 1)
			{
				retval = End_Run(k);
				/* Check if End_Run returned without errors */
				if(retval < 0)
				{
					sprintf(ErrMSG, "*ERROR* (Start_Run): End_Run failed in Module %d; return value=%d", k, retval);
					Pixie_Print_MSG(ErrMSG);
					return(-1);
				}
			}

			/* Clear external memory first before starting a new data acquisition run */
			if((Type == NEW_RUN) && (Run_Task != 0) && (Control_Task == 0))
			{
				Pixie_IOEM(k, 0, MOD_WRITE, MAX_HISTOGRAM_LENGTH*NUMBER_OF_CHANNELS, buffer);
			}

			if(Type == NEW_RUN)
			{
				/* Set RunTask */
				address=Run_Task_Index+DATA_MEMORY_ADDRESS;
				value=(U32)Run_Task;
				Pixie_IODM(k, address, MOD_WRITE, 1, &value);

				/* Set ControlTask */
				address=Control_Task_Index+DATA_MEMORY_ADDRESS;
				value=(U32)Control_Task;
				Pixie_IODM(k, address, MOD_WRITE, 1, &value);
			}

			/* Set RESUME */
			address=Resume_Index+DATA_MEMORY_ADDRESS;
			value=(U32)Type;
			Pixie_IODM(k, address, MOD_WRITE, 1, &value);
		}

		/* Set CSR to start run in all modules */
		for(k = 0; k < Number_Modules; k ++)
		{
			/* Read CSR */
			Pixie_ReadCSR(k, &CSR);
			CSR=(U32)SetBit(0, (U16)CSR);	/* Set bit 0 of CSR to enable run */
			Pixie_WrtCSR(k, CSR);
		}
	}
	else if(ModNum < Number_Modules)  /* Start run in one module only */		
	{
		/* Check if there is a run in progresss; if so, end it */
		retval = Check_Run_Status(ModNum);
		if(retval == 1)
		{
			retval = End_Run(ModNum);
			/* Check if End_Run returned without errors */
			if(retval < 0)
			{
				sprintf(ErrMSG, "*ERROR* (Start_Run): End_Run failed in Module %d; return value=%d", ModNum, retval);
				Pixie_Print_MSG(ErrMSG);
				return(-1);
			}
		}

		/* Clear external memory first before starting a new data acquisition run */
		if((Type == NEW_RUN) && (Run_Task != 0) && (Control_Task == 0))
		{
			Pixie_IOEM(ModNum, 0, MOD_WRITE, MAX_HISTOGRAM_LENGTH*NUMBER_OF_CHANNELS, buffer);
		}

		if(Type == NEW_RUN)
		{
			/* Set RunTask */
			address=Run_Task_Index+DATA_MEMORY_ADDRESS;
			value=(U32)Run_Task;
			Pixie_IODM(ModNum, address, MOD_WRITE, 1, &value);
			
			/* Set ControlTask */
			address=Control_Task_Index+DATA_MEMORY_ADDRESS;
			value=(U32)Control_Task;
			Pixie_IODM(ModNum, address, MOD_WRITE, 1, &value);
		}

		/* Set RESUME */
		address=Resume_Index+DATA_MEMORY_ADDRESS;
		value=(U32)Type;
		Pixie_IODM(ModNum, address, MOD_WRITE, 1, &value);

		/* Read CSR */
		Pixie_ReadCSR(ModNum, &CSR);
		CSR=(U32)SetBit(0, (U16)CSR);	/* Set bit 0 of CSR to enable run */
		Pixie_WrtCSR(ModNum, CSR);
	}

	return(0);
}


/****************************************************************
 *	End_Run function:
 *		Ends run in a single Pixie module or multiple modules.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to end the run
 *
 ****************************************************************/

S32 End_Run (
		U8 ModNum )		// Pixie module number
{
	U32 CSR, retval, tcount;
	U32 address, value;
	U16 sumActive, k;
	U16 Active[PRESET_MAX_MODULES];
	
	/* Set RESUME in DSP to 2 to signal a run-stop in all cards */
	for(k = 0; k < Number_Modules; k++)
	{
		address = Resume_Index + DATA_MEMORY_ADDRESS;
		value   = 2;
		Pixie_IODM((U8)k, address, MOD_WRITE, 1, &value);
	}

	/* Set SYNCHDONE in DSP to 1 to cause a synch-loop break-up in all cards */
	for(k = 0; k < Number_Modules; k++)
	{
		address = SYNCHDONE_Index + DATA_MEMORY_ADDRESS;
		value   = 1;
		Pixie_IODM((U8)k, address, MOD_WRITE, 1, &value);
	}

	if(ModNum == Number_Modules)  /* Stop run in all modules */
	{
		for(k = 0; k < Number_Modules; k ++)
		{
			/* Read CSR */
			Pixie_ReadCSR((U8)k, &CSR);
			CSR = ClrBit(0, (U16)CSR);  /* Clear bit 0 of CSR to disable run */
			Pixie_WrtCSR((U8)k, CSR);

			Active[k] = 1;  /* To be used for checking run status below */ 
		}

		/* Check if the run has been really ended */
		tcount = 0;
		sumActive = Number_Modules;

		do
		{
			for(k = 0; k < Number_Modules; k ++)
			{
				if(Active[k] == 1)
				{
					retval = Check_Run_Status((U8)k);
					if( retval == 0)
					{
						Active[k] = 0;	/* Clear Active for a module whose run is done */
						sumActive --;
					}
				}
			}

			if(sumActive == 0)
			{
				break;		/* Run in all modules is done */
			}

			Pixie_Sleep(1);
			tcount++;

		} while(tcount < 100); /* TimeOut = 100 ms */

		/* Check if there is any module whose run has not ended */
		for(k = 0; k < Number_Modules; k ++)
		{
			if(Active[k] == 1)
			{
				sprintf(ErrMSG, "*ERROR* (End_Run): Module %d failed to stop its run", k);
				Pixie_Print_MSG(ErrMSG);
			}
		}

		if(sumActive != 0)
		{
			return(-1);		/* Not all modules stopped the run successfully */
		}
		else
		{
			return(0);		/* All modules stopped the run successfully */
		}
	}
	else if(ModNum < Number_Modules)  /* Stop run in one module only */
	{		
		/* Read CSR */
		Pixie_ReadCSR(ModNum, &CSR);
		CSR = ClrBit(0, (U16)CSR);	/* Clear bit 0 of CSR to disable run */
		Pixie_WrtCSR(ModNum, CSR);

		/* Check if the run has been really ended. */
		tcount=0;
		do
		{
			retval = Check_Run_Status(ModNum);
			if( retval == 0)
			{
				break;
			}

			Pixie_Sleep(1);
			tcount++;
		} while(tcount < 100); /* TimeOut = 100 ms */

		if(tcount == 100)  /* Timed out */
		{
			sprintf(ErrMSG, "*ERROR* (End_Run): Module %d failed to stop its run", ModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}
		else
		{
			return(0);		/* The module stopped the run successfully */
		}
	}

	return(0);
}

/****************************************************************
 *	Read_Resume_Run function:
 *		Reads data and resums run in Pixie modules after memory buffers are filled.
 *
 *	An attempt to minimize readout dead time, currently not fully tested
 *		Return Value:
 *			 0 - success
 *			-1 - failure to end the run
 *			-2 - invalid word count
 *
 ****************************************************************/

S32 Read_Resume_Run (
			U8  ModNum,			// Pixie module number
			U8  Stop,		    // If 0, do not stop and resume if stopped (poll in any spill but last) 
								// if 1, do not stop but do not resume if stopped (last spill)
								// If 2, stop, then resume (spill timeout)
								// If 3, stop and do not resume (run timeout, new file). 
			S8  *FileName    )  // list mode file name
{
	
	U8    k;
	U32   CSR, RunActive, DataReady;
	S32	  retval=0;

/**** End/Poll run part ******************************************************************/
	if(Stop>=2)
	{
		End_Run(Number_Modules);
	}

	Pixie_ReadCSR(ModNum, &CSR);
	RunActive = ((CSR & 0x2000) >0);
	DataReady = ((CSR & 0x4000) >0);
	
	
/**** Check end run part ***************************************************************/
	
	/* If stopped, check if the run has really ended in all modules */
	if(RunActive==0)
	{
		for(k = 0; k < Number_Modules; k++)
		{   	  
			if(Check_Run_Status((U8)k) != 0)  /* run not stopped */
			{
				sprintf(ErrMSG, "*ERROR* (Read_Resume_Run): Module %d failed to stop its run", k);
				Pixie_Print_MSG(ErrMSG);
				return(-3);
			}
		}
	}
	

/**** Write LMF part *******************************************************************/
	if( (RunActive==0) | (DataReady ==1))
	{
		retval=Write_List_Mode_File(FileName);
		if(retval<0)
		{
			return(retval);
		}
		retval=1;		// set to 1 indicating one spill has been read
	}

	
/**** Resume run part ******************************************************************/

	/* Resume run in all modules */
	if( ((Stop==2) | (Stop==0)) & (RunActive ==0))
	{
		for(k = 0; k < Number_Modules; k++)
		{
			/* Read CSR */
			Pixie_ReadCSR(k, &CSR);
			CSR = (U32)SetBit(0, (U16)CSR);	/* Set bit 0 of CSR to enable run */
			Pixie_WrtCSR(k, CSR);
		}
	}

/***** Return info ************************************************************************/


	return(retval);		//0 = polled ok, no read, 1 = polled and read one spill
}


/****************************************************************
 *	Check_Run_Status function:
 *		Check if a run is in progress.
 *
 *		Return Value:
 *			0:	no run in progress
 *			1:	a run in progress
 *
 ****************************************************************/

S32 Check_Run_Status (
			U8 ModNum )			// Pixie module number
{

	U32 CSR;

	Pixie_ReadCSR(ModNum, &CSR);
//		sprintf(ErrMSG, "*TEST* (Check_Run_Status): CSR =  %d", CSR);
//		Pixie_Print_MSG(ErrMSG);

	return( ((U16)CSR & 0x2000) >> 13 );
}


/****************************************************************
 *	Control_Task_Run function:
 *		Perform a control task run.
 *
 *		Return Value:
 *			 0 - successful
 *			-1 - failed to start the control task run
 *			-2 - control task run timed out
 *
 ****************************************************************/

S32 Control_Task_Run (
		U8 ModNum,			// Pixie module number
		U8 ControlTask,		// Control task number
		U32 Max_Poll )		// Timeout control in unit of ms for control task run
{
	S32 retval;
	U32 count;

	if (Offline == 1) {
		sprintf(ErrMSG, "(Control_Task_Run): Offline mode. No I/O operations possible");
		Pixie_Print_MSG(ErrMSG);
		return (0);
	}

	/* Start control task run: NEW_RUN and RunTask = 0 */
	retval = Start_Run(ModNum, NEW_RUN, 0, ControlTask);
	if(retval < 0)
	{
		sprintf(ErrMSG, "*ERROR* (Control_Task_Run): Failure to start control task %d in Module %d", ControlTask, ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	/* A short wait before polling the run status */
	Pixie_Sleep(1);

	count=0;
	while(Check_Run_Status(ModNum) && (count<Max_Poll))
	{
		count++; /* The maximal waiting time is set by Max_Poll */ 
		Pixie_Sleep(1);
	}

	if(count>=Max_Poll)
	{
		sprintf(ErrMSG, "*ERROR* (Control_Task_Run): Control task %d in Module %d timed out", ControlTask, ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-2); /* Time Out */
	}
	else
	{
		return(0); /* Normal finish */
	}
}


/****************************************************************
 *	Pixie_Register_IO function:
 *		Used for single-word I/O communications between Host
 *		and Pixie registers.
 *
 ****************************************************************/

S32 Pixie_Register_IO (
			U16 ModNum,	// the Pixie module to communicate to
			U32 address,	// register address
			U16 direction,	// either MOD_READ or MOD_WRITE
			U32 *value )	// holds or receives the data
{
	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	if (VAddr[ModNum] == 0) {            
	  sprintf(ErrMSG, "*ERROR* (Pixie_Register_IO) - module address not mapped. Module=%d", ModNum);
	  Pixie_Print_MSG(ErrMSG);
	  return(-6);
	  }


	/* Write to and read from register */
	if(direction == MOD_WRITE) *(U32*)(VAddr[ModNum]+address) = *value;
	if(direction == MOD_READ ) *value = *(U32*)(VAddr[ModNum]+address);

	return(0);
}


/****************************************************************
 *	Pixie_RdWrdCnt:
 *		Read word count register of the selected Pixie module to
 *		obtain the number of 16-bit words in the linear I/O buffer.
 *
 ****************************************************************/

S32 Pixie_RdWrdCnt (
			U8 ModNum,			// Pixie module number
			U32 *WordCount )	// word count value
{
	Pixie_Register_IO(ModNum, PCI_WCR, MOD_READ, WordCount);
	return(0);
}


/****************************************************************
 *	Pixie_ReadCSR:
 *		Read Control Status Register (CSR) of the selected Pixie module.
 *
 ****************************************************************/

S32 Pixie_ReadCSR (
			U8 ModNum,			// Pixie module number
			U32 *CSR )			// CSR value
{
	Pixie_Register_IO(ModNum, PCI_CSR, MOD_READ, CSR);
	return(0);
}


/****************************************************************
 *	Pixie_WrtCSR:
 *		Write Control Status Register (CSR) to the selected Pixie module.
 *
 ****************************************************************/

S32 Pixie_WrtCSR (
			U8 ModNum,			// Pixie module number
			U32 CSR )			// CSR value
{
	Pixie_Register_IO(ModNum, PCI_CSR, MOD_WRITE, &CSR);
	return(0);
}


/****************************************************************
 *	Pixie_ReadVersion:
 *		Read hardware version of the selected Pixie module.
 *
 ****************************************************************/

S32 Pixie_ReadVersion (
			U8 ModNum,			// Pixie module number
			U32 *Version )		// Version number
{
	Pixie_Register_IO(ModNum, PCI_VERSION, MOD_READ, Version);
	return(0);
}




/****************************************************************
 *	Pixie_IODM function:
 *		Used for all I/O communications between Host and Pixie data memory.
 *
 ****************************************************************/

S32 Pixie_IODM (
			U8  ModNum,	// the Pixie module to communicate to
			U32 address,	// data memory address
			U8  direction,	// either MOD_READ or MOD_WRITE
			U16 nWords,	// the number of 32-bit words to be transferred
			U32 *buffer )	// holds or receives the data
{
	U16 k = 0;

	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	if (VAddr[ModNum] == 0) {		// rlk
	  sprintf(ErrMSG, "*ERROR* (Pixie_IODM) - module address not mapped. Module=%d", ModNum);
	  Pixie_Print_MSG(ErrMSG);
	  return(-6);
	  }


	/* Set initial address to talk to */
	*(U32*)(VAddr[ModNum]+PCI_IDMAADDR) = address;
	/* Write or read to data memory */
	if (direction == MOD_WRITE) for (k = 0; k < nWords; k++) *(U32*)(VAddr[ModNum] + PCI_IDMADATA) = buffer[k];
	if (direction == MOD_READ ) for (k = 0; k < nWords; k++) buffer[k]                             = *(U32*)(VAddr[ModNum] + PCI_IDMADATA);
	/* Done */
	return(0);
}


/****************************************************************
 *	Pixie_IOEM function:
 *		Pixie external memory I/O communication (burst Read/Write)
 *
 *		Return Value:
 *			 0 - success or offline analysis
 *			-1 - external memory locked by DSP 
 *			-2 - unable to open DMA channel
 *			-3 - unable to read dummy words
 *			-4 - unable to read external memory data
 *			-5 - unable to close DMA channel
 *			-6 - module address not mapped
 *
 ****************************************************************/

S32 Pixie_IOEM (
			U8  ModNum,			// the Pixie module to communicate to
			U32 address,		// external memory address
			U8  direction,		// either MOD_READ or MOD_WRITE
			U32 nWords,			// the number of 32-bit words to be transferred
			U32 *buffer )		// holds or receives the data
{
	U32            k; 
	U32			   dummy[5]={5*0};
	U32			   count;
	U32            CSR;
	PLX_STATUS     rc = ApiSuccess;
	PLX_DMA_PROP   DmaDesc;
	PLX_DMA_PARAMS DmaData;
	//PLX_PHYSICAL_MEM  PciBuffer;
	S32            retval, error;
	
	/* Returns immediately for offline analysis */
	if(Offline == 1) return(0);

	if (VAddr[ModNum] == 0)  {		// rlk
	  sprintf(ErrMSG, "*ERROR* (Pixie_IOEM) - module address not mapped. Module=%d", ModNum);
	  Pixie_Print_MSG(ErrMSG);
	  return(-6);
	 }

	error = 0;
	/* Set bit 2 of CSR to indicate that PCI wants to access the external memory */
	Pixie_ReadCSR(ModNum, &CSR);
	CSR = SetBit(2, (U16)CSR);
	/* Write back CSR */
	Pixie_WrtCSR(ModNum, (U16)CSR);

	/* Read CSR and check if DSP is still busy with the external memory */
	count = 10000;	/* Check DSPWRTRACE 10000 times */
	do
	{
		Pixie_ReadCSR(ModNum, &CSR);
		retval = CSR & 0x0080;	/* Check bit 7 of CSR (DSPWRTRACE) */
		count --;
	} while ((retval == 0x80) && (count > 0));

	if(count == 0)  {  /* Timed out */
		sprintf(ErrMSG, "*ERROR* (Pixie_IOEM) - External memory locked by DSP in Module %d", ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	if(direction==MOD_WRITE) {  /* Write to external memory */
		*(U32*)(VAddr[ModNum] + PCI_EMADDR) = address;  
		for (k=0; k<nWords; k++) *(U32*)(VAddr[ModNum] + PCI_EMDATA) = *buffer++;
	}
	else if(direction==MOD_READ)  {  /* Read from external memory */
		/* Clear DMA descriptor structure */
		memset(&DmaDesc, 0, sizeof(PLX_DMA_PROP));
		/* Set up DMA configuration structure */
		DmaDesc.ReadyInput 	      = 1;
		DmaDesc.Burst             = 1;
		DmaDesc.BurstInfinite	  = 1;
		DmaDesc.FastTerminateMode = 0;
		DmaDesc.LocalBusWidth     = 2; /* 32 bit bus */

		rc = PlxPci_DmaChannelOpen(&Sys_hDevice[ModNum], 0, &DmaDesc);    
		if (rc != ApiSuccess) {
			sprintf(ErrMSG, "*ERROR* (Pixie_IOEM) - Unable to open DMA in Module %d, rc=%d", ModNum, rc);
			Pixie_Print_MSG(ErrMSG);
			error = -2;
		}
	
		if(error>=0) {
			memset(&DmaData, 0, sizeof(PLX_DMA_PARAMS));
			DmaData.UserVa	     = (PLX_UINT_PTR)dummy;
			DmaData.LocalAddr  = PCI_EMDATA;
			DmaData.ByteCount	 = 3*4;	/* Read three dummy words; each word is 32-bt (4 bytes) */
			DmaData.Direction    = PLX_DMA_LOC_TO_PCI;

			*(U32*)(VAddr[ModNum] + PCI_EMADDR) = address;  
			rc = PlxPci_DmaTransferUserBuffer(&Sys_hDevice[ModNum], 0, &DmaData, DMATRANSFER_TIMEOUT);
			if (rc != ApiSuccess) {	
				sprintf(ErrMSG, "*ERROR* (Pixie_IOEM) - Unable to read dummy words in Module %d, rc=%d", ModNum, rc);
				Pixie_Print_MSG(ErrMSG);
				error = -3;
			}
		}
		
		if(error>=0) {
			memset(&DmaData, 0, sizeof(PLX_DMA_PARAMS));
			DmaData.UserVa			= (PLX_UINT_PTR)buffer;
			DmaData.LocalAddr     = PCI_EMDATA;
			DmaData.ByteCount		= nWords*4;	/* Read external memory data */
			DmaData.Direction       = PLX_DMA_LOC_TO_PCI;

			rc = PlxPci_DmaTransferUserBuffer(&Sys_hDevice[ModNum], 0, &DmaData, DMATRANSFER_TIMEOUT);
			if (rc != ApiSuccess) {
				sprintf(ErrMSG, "*ERROR* (Pixie_IOEM) - Unable to read external memory data in Module %d, rc=%d", ModNum, rc);
				Pixie_Print_MSG(ErrMSG);
				error = -4;
			}
		}

		/* ALWAYS close when opened successfully */
		if(error!=-2) {
			rc = PlxPci_DmaChannelClose(&Sys_hDevice[ModNum], 0); 
			if (rc != ApiSuccess) {
				error = -5;
				/* But try to reset the device if a DMA is in-progress */
				if (rc == ApiDmaInProgress) {
					PlxPci_DeviceReset(&Sys_hDevice[ModNum]);
					/* Attempt to close again */
					rc = PlxPci_DmaChannelClose(&Sys_hDevice[ModNum], 0);
					if (rc == ApiSuccess) error = 0;
				}

				if(error == -5) {
					sprintf(ErrMSG, "ERROR (Pixie_IOEM) - Unable to close the DMA channel in Module %d, rc=%d", ModNum, rc);
					Pixie_Print_MSG(ErrMSG);
				}
			}

		}
	}

	/* ALWAYS Clear bit 2 of CSR to release PCI's control of external memory */
	CSR &= 0xFFFB;
	*(U32*)(VAddr[ModNum] + PCI_CSR) = CSR;

	return(error);
}


/****************************************************************
 *	Find_Xact_Match function:
 *		Looks for an exact match between str and a name in the
 *		array Names. It uses Names_Array_Len to know how many names
 *		there are. Note that all elements in Names have all-uppercase
 *		names.
 *
 *		Return Value:
 *			index of str in Names if found
 *			Names_Array_Len-1 if not found
 *
 ****************************************************************/

U16 Find_Xact_Match (S8 *str,				// the string to be searched
		     S8 Names[][MAX_PAR_NAME_LENGTH],	// the array which contains the string
		     U16 Names_Array_Len )			// the length of the array
{
    U16	k = 0;

    while (strcmp(str, Names[k]) != 0 && k < Names_Array_Len) { k++; }
    
    if (k == Names_Array_Len) 
    {
	/* If str is not found in Names */
	 sprintf(ErrMSG, "*ERROR* (Find_Xact_Match): %s was not found", str);
	 Pixie_Print_MSG(ErrMSG);
	 return (Names_Array_Len - 1);
    }
    else return (k);
    
}

/****************************************************************
 *	Write_Spectrum_File function:
 *		Read histogram data from each Pixie module then save it into a
 *		32-bit integer binary file. The data from Module #0 is saved first,
 *		then the data from Module #1 is appended to the end of the file,
 *		and so on.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to open MCA spectrum file
 *
 ****************************************************************/

S32 Write_Spectrum_File (
			S8 *FileName )		// histogram data file name
{
	U16  i;
	FILE *specFile = NULL;
	U32  specdata[HISTOGRAM_MEMORY_LENGTH];
	S32  retval;

	specFile = fopen(FileName, "ab"); /* Append to a binary file */
	if(specFile != NULL)
	{
		for(i=0; i<Number_Modules; i++)
		{
			retval = Pixie_IOEM((U8)i, HISTOGRAM_MEMORY_ADDRESS, MOD_READ, HISTOGRAM_MEMORY_LENGTH, specdata);
			if(retval < 0)  /* Check if reading is successful */
			{
				sprintf(ErrMSG, "*ERROR* (Write_Spectrum_File): failure to read histogram data from Module %d", i);
				Pixie_Print_MSG(ErrMSG);
			}
			else  /* Write to file */
			{
				fwrite(specdata, 4, HISTOGRAM_MEMORY_LENGTH, specFile);
			}
		}

		fclose(specFile);
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Write_Spectrum_File): can't open MCA spectrum file %s", FileName);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	
	return(0);
}


/****************************************************************
 *	Write_List_Mode_File function:
 *		Read list mode data from each Pixie module then append the data to
 *		a 16-bit integer binary file. The data from Module #0 is saved first,
 *		then the data from Module #1 is appended to the end of the file,
 *		and so on.
 *		The assumption is that all modules are in the same mode, i.e. 32x buffer, ping
 *      pong or single buffer mode, and that they take data synchronously, i.e they
 *      have the same number of buffers.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *			-2 - invalid word count
 *
 ****************************************************************/

S32 Write_List_Mode_File (
			S8  *FileName )		// List mode data file name
{
	U16 i, j, MCSRA, MCSRA_index, EMwords_index, shortbuffer[IO_BUFFER_LENGTH];
	U16 EMwords2_index, DblBufCSR_index, DblBufCSR;
	U32 Aoffset[2], WordCountPP[2];
	U32 WordCount, NumWordsToRead, buffer[LIST_MEMORY_LENGTH], CSR;
	U32 dsp_word[2];
	U32 WCRs[PRESET_MAX_MODULES], CSRs[PRESET_MAX_MODULES];
	FILE *listFile = NULL;

	listFile = fopen(FileName, "ab");  /* Append to a binary file */
	if(listFile != NULL)
	{
		/* Locate DSP variables MODCSRA and EMWORDS */
        
		MCSRA_index = Find_Xact_Match("MODCSRA", DSP_Parameter_Names, N_DSP_PAR);
		EMwords_index = Find_Xact_Match("EMWORDS", DSP_Parameter_Names, N_DSP_PAR);
		EMwords2_index = Find_Xact_Match("EMWORDS2", DSP_Parameter_Names, N_DSP_PAR);
		DblBufCSR_index = Find_Xact_Match("DBLBUFCSR", DSP_Parameter_Names, N_DSP_PAR);

		Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + MCSRA_index, MOD_READ, 1, dsp_word);
		MCSRA = (U16)dsp_word[0];
		Pixie_IODM(0, (U16)DATA_MEMORY_ADDRESS + DblBufCSR_index, MOD_READ, 1, dsp_word);
		DblBufCSR = (U16)dsp_word[0];

		// ----------- single buffer mode, List mode data in DM ------------------------------

		if( (TstBit(MODCSRA_EMWORDS, MCSRA) == 0) & (TstBit(DBLBUFCSR_ENABLE, DblBufCSR) == 0) )	
		{
			// Read out list mode data module by module 
			for(i=0; i<Number_Modules; i++)
			{
				
				// Read Pixie's word count register => the number of 16-bit words to read 
				Pixie_RdWrdCnt((U8)i, &WordCount);
	
				if(WordCount > IO_BUFFER_LENGTH)
				{
					sprintf(ErrMSG, "*ERROR* (Write_List_Mode_File):invalid word count %d", WordCount);
					Pixie_Print_MSG(ErrMSG);
					return(-2);
				}
				
				/* Read out the list mode data */
				Pixie_IODM((U8)i, IO_BUFFER_ADDRESS, MOD_READ, (U16)WordCount, buffer);
				
				for(j=0; j<WordCount; j++)
				{
					shortbuffer[j] = (U16)buffer[j];
				}
			
				/* Append to the file */
				fwrite(shortbuffer, 2, WordCount, listFile);
				
			}

			fclose(listFile);
			return(0);

		}
		
		// ----------- 32x buffer mode, List mode data in EM ------------------------------
	
		if( (TstBit(MODCSRA_EMWORDS, MCSRA) == 1) & (TstBit(DBLBUFCSR_ENABLE, DblBufCSR) == 0) )	
		{
			// Read out list mode data module by module 
			for(i=0; i<Number_Modules; i++)
			{

				// A dummy read of Pixie's word count register 
				Pixie_RdWrdCnt((U8)i, &WordCount);

				// The number of 16-bit words to read is in EMwords 
				Pixie_IODM((U8)i, (U16)DATA_MEMORY_ADDRESS + EMwords_index, MOD_READ, 2, dsp_word);
				WordCount = dsp_word[0] * 65536 + dsp_word[1];

				// Check if it is an odd or even number 
				if(fmod(WordCount, 2.0) == 0.0)
				{
					NumWordsToRead = WordCount / 2;
				}
				else
				{
					NumWordsToRead = WordCount / 2 + 1;
				}
			
				if(NumWordsToRead > LIST_MEMORY_LENGTH)
				{
					sprintf(ErrMSG, "*ERROR* (Write_List_Mode_File):invalid word count %d", NumWordsToRead);
					Pixie_Print_MSG(ErrMSG);
					return(-2);
				}
			
				// Read out the list mode data 
				Pixie_IOEM((U8)i, LIST_MEMORY_ADDRESS, MOD_READ, NumWordsToRead, buffer);
			
				fwrite(buffer, 2, WordCount, listFile);

			}

			fclose(listFile);
			return(0);

		}

		// ----------- double buffer mode, List mode data in EM ------------------------------

		else
		{
		
			Aoffset[0] = 0;
			Aoffset[1] = LM_DBLBUF_BLOCK_LENGTH;
			
			for(i=0; i<Number_Modules; i++)
			{
				// read the CSR
				Pixie_ReadCSR((U8)i, &CSR);
				CSRs[i] = CSR;

				// A read of Pixie's word count register 
				// This also indicates to the DSP that a readout has begun 
				Pixie_RdWrdCnt((U8)i, &WordCount);
				WCRs[i] = WordCount;
			}	// CSR for loop

			// Read out list mode data module by module 
			for(i=0; i<Number_Modules; i++)
			{

				// The number of 16-bit words to read is in EMwords or EMwords2
				Pixie_IODM((U8)i, (U16)DATA_MEMORY_ADDRESS + EMwords_index, MOD_READ, 2, dsp_word);
				WordCountPP[0] = dsp_word[0] * 65536 + dsp_word[1];
				Pixie_IODM((U8)i, (U16)DATA_MEMORY_ADDRESS + EMwords2_index, MOD_READ, 2, dsp_word);
				WordCountPP[1] = dsp_word[0] * 65536 + dsp_word[1];

						
				if(TstBit(CSR_128K_FIRST, (U16)CSRs[i]) == 1) 
				{
					j=0;			
				}
				else		// block at 128K+64K was first
				{
					j=1;
				}
		
				if  (TstBit(CSR_DATAREADY, (U16)CSRs[i]) == 0 )		
				// function called after a readout that cleared WCR => run stopped => read other block
				{
					j=1-j;			
					sprintf(ErrMSG, "*INFO* (Write_List_Mode_File): Module %d: Both memory blocks full (block %d older). Run paused (or finished).",i,1-j);
					Pixie_Print_MSG(ErrMSG);
				}

//sprintf(ErrMSG, "*INFO* (Write_List_Mode_File): Module %d: CSR = %x, j=%d",i,CSRs[i],j );
//Pixie_Print_MSG(ErrMSG);
				
				if (WordCountPP[j] >0)
				{
					// Check if it is an odd or even number 
					if(fmod(WordCountPP[j], 2.0) == 0.0)
					{
						NumWordsToRead = WordCountPP[j] / 2;
					}
					else
					{
						NumWordsToRead = WordCountPP[j] / 2 + 1;
					}
		
					if(NumWordsToRead > LIST_MEMORY_LENGTH)
					{
						sprintf(ErrMSG, "*ERROR* (Write_List_Mode_File):invalid word count %d", NumWordsToRead);
						Pixie_Print_MSG(ErrMSG);
						return(-2);
					}
		
					// Read out the list mode data 
					Pixie_IOEM((U8)i, LIST_MEMORY_ADDRESS+Aoffset[j], MOD_READ, NumWordsToRead, buffer);
		
					// save to file
					fwrite(buffer, 2, WordCountPP[j], listFile);

				}
							
			}	// readout for loop
		
		
			fclose(listFile);

			for(i=0; i<Number_Modules; i++)
			{
				// A second read of Pixie's word count register to clear the DBUF_1FULL bit
				// indicating to the DSP that the read is complete
				Pixie_RdWrdCnt((U8)i, &WordCount);
			}	// third for loop
	
			return(0);
		}
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Write_List_Mode_File): can't open list mode file %s", FileName);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
}



/****************************************************************
 *	Read_Spectrum_File function:
 *		Read MCA spectrum data from a previously-saved binary file.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open the MCA spectrum file
 *			-2 - the spectrum file doesn't contain data for this module
 *
 ****************************************************************/

S32 Read_Spectrum_File (
			U8  ModNum,			// Pixie module number
			U32 *Data,			// Receives histogram data
			S8  *FileName )		// previously-saved histogram file
{
	U32 offset, TotalBytes;

	FILE *specFile = NULL;

	specFile = fopen(FileName, "rb"); /* Read a binary file */
	if(specFile != NULL)
	{
		/* Get the file length */
		fseek(specFile, 0, SEEK_END);
		TotalBytes = ftell(specFile) + 1;

		/* Calculate the offset to point the file pointer to the module histogram data */
		offset = ModNum*HISTOGRAM_MEMORY_LENGTH*4;
		
		/* Check offset to make sure histogram data is available for this Pixie module */
		if(offset < (TotalBytes - HISTOGRAM_MEMORY_LENGTH*4))
		{
			fseek(specFile, offset, SEEK_SET);
			fread(Data, 4, HISTOGRAM_MEMORY_LENGTH, specFile);
			fclose(specFile);
			return(0);
		}
		else
		{
			sprintf(ErrMSG, "*ERROR* (Read_Spectrum_File): the spectrum file %s doesn't contain data for Module %d", FileName, ModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-2);
		}
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Read_Spectrum_File): can't open MCA spectrum file %s", FileName);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
}


/****************************************************************
 *	Get_Traces function:
 *		Acquire ADC traces for one channel of a Pixie module.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to start the GET_TRACES run
 *			-2 - GET_TRACES run timed out
 *			
 ****************************************************************/

S32 Get_Traces (
			U32 *Trace_Buffer,		// ADC trace data
			U8  ModNum,				// Pixie module number
			U8  ChanNum )			// Pixie channel number
{

	U16 idx, ch, Wcount, BoardRevision;
	U32 value;
	S32 retval;

	/* Check whether only only one channel or all channels are requested to acquring traces */
	if(ChanNum == NUMBER_OF_CHANNELS)
	{
		for (ch=0; ch<NUMBER_OF_CHANNELS; ch++ )
		{
			/* Set DSP parameter CHANNUM (channel number) */
			idx = Find_Xact_Match("CHANNUM", DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = ch;
			/* Download to the data memory */
			value = (U32)Pixie_Devices[ModNum].DSP_Parameter_Values[idx] ;
			Pixie_IODM(ModNum, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

			/* Start GET_TRACES run to get ADC traces */
			retval = Start_Run(ModNum, NEW_RUN, 0, GET_TRACES);
			if(retval < 0) {
				sprintf(ErrMSG, "*ERROR* (Get_Traces): failure to start GET_TRACES run in Module %d, Channel %d", ModNum, ch);
				Pixie_Print_MSG(ErrMSG);
				return(-1);
			}

			BoardRevision = (U16)( 0x0F0F & (U32)Pixie_Devices[ModNum].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)]);
			if (BoardRevision == 0x0701)	// Pixie-4 Rev B
			{
				Pixie_Sleep(100);
			}
			/* Check Run Status */
			Wcount=0;
			do{
				Pixie_Sleep(1);
				if(Check_Run_Status(ModNum)==0)
				{
					break;
				}

				Wcount++;
			} while(Wcount<10000); /* The maximum allowed waiting time is 10 s */

			if(Wcount>=10000)
			{
				sprintf(ErrMSG, "*ERROR* (Get_Traces): Acquiring ADC traces in Module %d Channel %d timed out", ModNum, ch);
				Pixie_Print_MSG(ErrMSG);
				return(-2); /* Time Out */
			}

			/* Read out the I/O buffer */
			Pixie_IODM (ModNum, IO_BUFFER_ADDRESS, MOD_READ, IO_BUFFER_LENGTH, Trace_Buffer);

			/* Update pointer *Trace_Buffer */
			Trace_Buffer += IO_BUFFER_LENGTH;
		}
	}
	else if(ChanNum < NUMBER_OF_CHANNELS)
	{
		/* Set DSP parameter CHANNUM (channel number) */
		idx = Find_Xact_Match("CHANNUM", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = ChanNum;
		/* Download to the data memory */
		value = (U32)Pixie_Devices[ModNum].DSP_Parameter_Values[idx] ;
		Pixie_IODM(ModNum, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

		/* Start GET_TRACES run to get ADC traces */
		retval = Start_Run(ModNum, NEW_RUN, 0, GET_TRACES);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Get_Traces): failure to start GET_TRACES run in Module %d Channel %d", ModNum, ChanNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}

		/* Check Run Status */
		Wcount=0;
		do
		{
			Pixie_Sleep(1);
			if(Check_Run_Status(ModNum)==0)
			{
				break;
			}

			Wcount++;
		} while(Wcount<10000); /* The maximum allowed waiting time is 10 s */

		if(Wcount>=10000)
		{
			sprintf(ErrMSG, "*ERROR* (Get_Traces): Acquiring ADC traces in Module %d Channel %d timed out", ModNum, ChanNum);
			Pixie_Print_MSG(ErrMSG);
			return(-2); /* Time Out */
		}

		/* Read out the I/O buffer */
		Pixie_IODM (ModNum, IO_BUFFER_ADDRESS, MOD_READ, IO_BUFFER_LENGTH, Trace_Buffer);
	}

	return(0);
}


/****************************************************************
 *	Adjust_Offsets function:
 *      if called with a module number < current number of modules:
 *			Adjust the DC-offset of all channels of a Pixie module.
 *		if called with current number of modules:
 *			Adjust the DC-offset of all channels of all Pixie modules.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to start RampTrackDAC run
 *			-2 - RampTrackDAC run timed out
 *			-3 - failure to set DACs
 *			-4 - failure to program Fippi
 *
 ****************************************************************/

S32 Adjust_Offsets (
			U8 ModNum )		// module number
{

	U32    Wcount, j, CurrentModNum, Twait, MNstart, MNend;
	S32    retval;
	U32    sTDACwave[IO_BUFFER_LENGTH], IOBuffer[DSP_IO_BORDER];
	double TDACwave[IO_BUFFER_LENGTH], DACcenter, DACfifty;
	double a, b, abdiff, abmid, coeff[2], low, high, ADCtarget,baselinepercent;
	U16    adcMax, TDACwaveLen, TDACStepSize, TRACKDAC, idx, ChanCSRA, BoardRevision;
	S8     str[256];

	/* Initialize several constants */
	adcMax = 4096;	/* The Fippi still reports 12-bit ADC numbers */
	TDACwaveLen = 2048;	/* 8192 words shared by 4 channels */
	TDACStepSize = 32;	/* 32 Ramp TrackDAC steps; each step covers 2048 TDAC values */


	if(ModNum == Number_Modules)
	{
		MNstart = 0;
		MNend = Number_Modules;
	}
	else
	{
		MNstart = ModNum;
		MNend = ModNum+1;
	}

	// start ramping
	for(CurrentModNum = MNstart; CurrentModNum < MNend ; CurrentModNum ++)
	{
		/* Start to ramp TRACK DACs */
		retval = Start_Run((U8)CurrentModNum, NEW_RUN, 0, RAMP_TRACKDAC);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): failure to start RampTrackDAC run in Module %d", CurrentModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}
	}
	
	// wait and poll
	Twait=0;
	for(CurrentModNum = MNstart; CurrentModNum < MNend ; CurrentModNum ++)
	{
		BoardRevision = (U16)( 0x0F0F & (U32)Pixie_Devices[CurrentModNum].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)]);

		if (BoardRevision ==0x0701)		// Pixie-4 Rev B
		{
			if (Twait<500)
			{
				Wcount=0;
				do
				{
					Pixie_Sleep(10);
					Wcount++;
				}while(Wcount<500); /* fixed waiting time of 5 s */
				Twait = Wcount;	/* but only once for all modules */
			}

		}
		else
		{
			
		/* Check run status */
			Wcount=0;
			do
			{
				Pixie_Sleep(1);
				if(Check_Run_Status((U8)CurrentModNum)==0)		
				{
					break;
				}

				Wcount++;
			}while(Wcount<10000); /* The maximum allowed waiting time is 10 s */

			
			if(Wcount>=10000)
			{
				sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): RampTrackDACs timed out in Module %d", CurrentModNum);
				Pixie_Print_MSG(ErrMSG);
				return(-2); /* Time Out */
			}
		}

	}

	//readout and calculate offsets
	for(CurrentModNum = MNstart; CurrentModNum < MNend ; CurrentModNum ++)
	{
		/* Read out the I/O buffer */
		Pixie_IODM ((U8)CurrentModNum, IO_BUFFER_ADDRESS, MOD_READ, IO_BUFFER_LENGTH, sTDACwave);
		
		/* cast the tdac data into doubles */
		for (j = 0; j < IO_BUFFER_LENGTH; j++) 
		{
			TDACwave[j] = (double) sTDACwave[j];
		}

		/*	clean up the array by removing entries that are out of bounds, or on 
			a constant level */
		for (j = (IO_BUFFER_LENGTH - 1); j > 0; j --)
		{
			if ((TDACwave[j] > (adcMax-1)) || (TDACwave[j] == TDACwave[j-1]))
			{
				TDACwave[j] = 0;
			}
		}

		/* take care of the 0th element last, always lose it */
		TDACwave[0] = 0.0;

		/*	Another pass through the array, removing any element that is 
			surrounded by ZEROs */
		for(j = 1; j < (IO_BUFFER_LENGTH - 1); j ++)
		{
			if(TDACwave[j] != 0)  /* remove out-of-range points and failed measurements */
			{
					if ((TDACwave[j - 1] == 0) && (TDACwave[j + 1] == 0))
					{
						TDACwave[j] = 0;
					}
			}
		}

		for( j = 0; j < NUMBER_OF_CHANNELS; j ++ )
		{
			/* Get DSP parameter CHANCSRA */
			sprintf(str,"CHANCSRA%d",j);
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			ChanCSRA=Pixie_Devices[CurrentModNum].DSP_Parameter_Values[idx];
			if( TstBit(2, ChanCSRA) )  /* Check if this is a good channel */
			{
				/* Perform a linear fit to these data */
				low = j*TDACwaveLen;
				coeff[0] = low;
				high = low+TDACwaveLen-1;
				coeff[1] = high;
				retval = Linear_Fit(TDACwave, coeff);
				if ( retval == 0 )
				{
					a = -coeff[0] / coeff[1];
					a = MIN(MAX(low, a), high);
					b = (adcMax - coeff[0]) / coeff[1];
					b = MIN(MAX(low, b), high);
					abdiff = (double) (fabs(b - a) / 2.);
					abmid = (b + a) / 2.;
					a = (double) (ceil(abmid - (0.25 * abdiff)));
					b = (double) (floor(abmid + (0.25 * abdiff)));
					coeff[0] = a;
					coeff[1] = b;

					retval = Linear_Fit(TDACwave, coeff);
					if ( retval == 0 )
					{
						a = -coeff[0] / coeff[1];
						a = MIN(MAX(low, a), high);
						b = (adcMax - coeff[0]) / coeff[1];
						b = MIN(MAX(low, b), high);
						abdiff = (double) (fabs(b - a) / 2.);
						abmid = (b + a) / 2.;
						a = (double) (ceil(abmid - (0.9 * abdiff)));
						b = (double) (floor(abmid + (0.9 * abdiff)));
						coeff[0] = a;
						coeff[1] = b;
						
						retval = Linear_Fit(TDACwave, coeff);
						if( retval == 0 )
						{
							DACfifty = (adcMax/2 - coeff[0]) / coeff[1];
							DACcenter = (DACfifty - low) * TDACStepSize;

							/* Find baseline percent and calculate ADCtarget */
							sprintf(str,"BASELINEPERCENT%d",j);
							idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
							baselinepercent=(double)Pixie_Devices[CurrentModNum].DSP_Parameter_Values[idx];
							ADCtarget=adcMax*baselinepercent/100.0;

							TRACKDAC = (U16)RoundOff(DACcenter+TDACStepSize*(ADCtarget-adcMax/2)/(coeff[1]));
				
							/* Update DSP parameter TRACKDAC */
							sprintf(str,"TRACKDAC%d", j);
							idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
							Pixie_Devices[CurrentModNum].DSP_Parameter_Values[idx]=TRACKDAC;
						}
						else
						{
							sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): linear fit error in Module %d Channel %d", CurrentModNum, j);
							Pixie_Print_MSG(ErrMSG);
						}
					}
					else
					{
						sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): linear fit error in Module %d Channel %d", CurrentModNum, j);
						Pixie_Print_MSG(ErrMSG);
					}
				}
				else
				{
					sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): linear fit error in Module %d Channel %d", CurrentModNum, j);
					Pixie_Print_MSG(ErrMSG);
				}
			}
		}
	}

	for(CurrentModNum = MNstart; CurrentModNum < MNend ; CurrentModNum ++)
	{


		/* Download to the data memory */
		for(j=0; j<DSP_IO_BORDER; j++)
		{
			IOBuffer[j] = (U32)Pixie_Devices[CurrentModNum].DSP_Parameter_Values[j];
		}

		Pixie_IODM((U8)CurrentModNum, DATA_MEMORY_ADDRESS, MOD_WRITE, DSP_IO_BORDER, IOBuffer);

		/* Set DACs */
		retval = Control_Task_Run((U8)CurrentModNum, SET_DACS, 10000);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): failure to set DACs in Module %d", CurrentModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-3);
		}

		/* Program signal processing FPGAs */
		retval = Control_Task_Run((U8)CurrentModNum, PROGRAM_FIPPI, 1000);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Adjust_AllOffsets): failure to program Fippi in Module %d", CurrentModNum);
			Pixie_Print_MSG(ErrMSG);
			return(-4);
		}
	}

	return(0);
}


/****************************************************************
 *	Linear_Fit function:
 *		This routine performs a linear fit to positive definite
 *		data, any negative or zero point will be ignored!
 *
 *		Return Value:
 *			 0 - fit ok
 *			-1 - no fit
 *
 ****************************************************************/

S32 Linear_Fit (
			double *data,			// source data for linear fit
			double coeff[2] )		// coefficients
{
	U32 i;
	U32 ndata  = 0;
	double sxx = 0;
	double sx  = 0;
	double sy  = 0;
	double syx = 0;

	for(i = (U32)coeff[0]; i < (U32)coeff[1]; i++)
	{
		if(data[i] <= 0)
		{
			continue;
		}

		sx  += i;
		sxx += i*i;
		sy  += data[i];
		syx += data[i]*i;
		ndata++;
	}

	if(ndata > 1)
	{
		coeff[1] = (syx - ((sx * sy) / ((double) ndata))) / (sxx - ((sx * sx) / ((double) ndata)));
		coeff[0] = (sy - (coeff[1] * sx)) / ((double) ndata);
		return(0); /* fit ok */
	}
	else
	{
		return(-1); /* no fit */
	}
}


/****************************************************************
 *	Pixie_Sleep function:
 *		This routine serves as a wrapper for a system dependent
 *              sleep-like function
 *
 *		Return Value:
 *			 0 - good sleep 
 *		      
 *
 ****************************************************************/

S32 Pixie_Sleep (
		    double ms )			// Time in milliseconds
{
    
#ifdef XIA_WINDOZE
    Sleep((U32)ms);
#endif
#ifdef XIA_LINUX
    usleep((unsigned long)(ms * 1000));
#endif
    
    return(0);
}


/****************************************************************
 *	Get_Slow_Traces function:
 *		Acquire slow ADC traces for one channel of a Pixie module.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to start the  run
 *			-2 - run timed out
 *			-3 - no file found
 *			
 ****************************************************************/

S32 Get_Slow_Traces (
			U32 *UserData,			// input data
			U8  ModNum,				// Pixie module number
			S8 *FileName )			// data file name
{



	U16 Wcount, shortbuffer[4096];
	U32 buffer[4096], k,j, CSR, WCR;
	S32 retval;
	FILE *listFile = NULL;

	sprintf(ErrMSG, "*MESSAGE* (Get_Slow_Traces): starting, please wait");
	Pixie_Print_MSG(ErrMSG);

	listFile = fopen(FileName, "ab");  /* Append to a binary file */
	if(listFile != NULL)
	{
	
		/* Start GET_SLOW_TRACES run to get ADC traces */
	
		retval = Start_Run(ModNum, NEW_RUN, 0, 24);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Get_Slow_Traces): failure to start run");
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}
		
		

		for(k=0;k<UserData[0];k+=1)
		{
			
			/* Check Run Status */
			Wcount=0;
			do
			{
				//Pixie_Sleep(1); don't sleep, do dummy reads to keep up stealing cycles from DSP
				Pixie_IODM (ModNum, IO_BUFFER_ADDRESS-4096, MOD_READ, 1024, buffer);
				
				//poll
				Pixie_ReadCSR(ModNum, &CSR);
				if ((CSR & 0x4000)>0)		// poll LAM bit
				{
					break;
				}

				Wcount++;
			} while(Wcount<10000); /* The maximum allowed waiting time is 10 s */

			if(Wcount>=10000)
			{
				sprintf(ErrMSG, "*ERROR* (Get_Slow_Traces): Acquiring ADC traces timed out");
				Pixie_Print_MSG(ErrMSG);
				return(-2); /* Time Out */
			}

			Pixie_RdWrdCnt(ModNum, &WCR);	// WCR is 0 or 1, indicating DM block


			if(WCR==0)
			{
				Pixie_IODM (ModNum, IO_BUFFER_ADDRESS, MOD_READ, 4096, buffer);
			}
			else
			{
				Pixie_IODM (ModNum, IO_BUFFER_ADDRESS+4096, MOD_READ, 4096, buffer);
			}

			for(j=0; j<4096; j++)
			{
				shortbuffer[j] = (U16)buffer[j];
			}

			/* Append to a file */
			fwrite(shortbuffer, 2, 4096, listFile);

		}


		retval=End_Run(ModNum);  /* Stop the run */
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Get_Slow_Traces): failure to end the run, retval=%d", retval);
			Pixie_Print_MSG(ErrMSG);
			Pixie_Sleep(5000);
			return(-4);
		}
	}
	else
	{
		sprintf(ErrMSG, "*ERROR* (Get_Slow_Traces): Could not open output file");
		Pixie_Print_MSG(ErrMSG);
		return(-3); /* No file  */
	}

	fclose(listFile);

	sprintf(ErrMSG, "*MESSAGE* (Get_Slow_Traces): done taking traces");
	Pixie_Print_MSG(ErrMSG);

	return(0);
}


/****************************************************************
 *	Tau_Finder function:
 *		Find the exponential decay constant of the detector/preamplifier
 *		signal connected to one channle of a Pixie module.
 *			
 *		Tau is both an input and output parameter: it is used as the
 *		initial guess of Tau, then used for returning the new Tau value.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - failure to acquire ADC traces
 *
 ****************************************************************/

S32 Tau_Finder (
			U8 ModNum,			// Pixie module number
			U8 ChanNum,			// Pixie channel number
			double *Tau )		// Tau value
{
 
	U32 Trace[IO_BUFFER_LENGTH];
	U16 idx, FL, FG, Xwait, TFcount; /* fast filter times are set here */
	U16 ndat, k, kmin, kmax, n, tcount, MaxTimeIndex;
	U16 Trig[IO_BUFFER_LENGTH];
	S8  str[256];
	double localAmplitude, s1, s0; /* used to determine which tau fit was best */
	double dt; /* dt is the time between Trace samples */
	double threshold, t0, t1, TriggerLevelShift, avg, MaxTimeDiff, fitted_tau;
	double FF[IO_BUFFER_LENGTH], FF2[IO_BUFFER_LENGTH], TimeStamp[IO_BUFFER_LENGTH/4];
    double input_tau;
	S32 retval;

	U16 SYSTEM_CLOCK_MHZ = 75;
	U16 FILTER_CLOCK_MHZ = 75;
	U16	ADC_CLOCK_MHZ = 75;
	U16	ThisADCclkMHz = 75;
	U16 LTscale = 16;

	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );



    /* Save input Tau value */
    input_tau=*Tau;

	/* Generate random indices */
	RandomSwap();

	/* Get DSP parameters FL, FG and XWAIT */
	sprintf(str,"FASTLENGTH%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	FL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	sprintf(str,"FASTGAP%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	FG=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
	
	sprintf(str,"XWAIT%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Xwait=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
	dt=(double)Xwait/(double)SYSTEM_CLOCK_MHZ*1.0e-6;

	ndat=IO_BUFFER_LENGTH;
	localAmplitude=0;

    TFcount=0;  /* Initialize TFcount */
    do
    {
		/* get ADC-trace */
		retval = Get_Traces(Trace, ModNum, ChanNum);
		if(retval < 0)
		{
			sprintf(ErrMSG, "*ERROR* (Tau_Finder): failure to get ADC traces in Module %d Channel %d", ModNum, ChanNum);
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}
  
		/* Find threshold */
		threshold=Thresh_Finder(Trace, Tau, FF, FF2, FL, FG, ModNum, ChanNum);

		kmin=2*FL+FG;

		for(k=0;k<kmin;k+=1)
		{
			Trig[k]= 0;
		}

		/* Find average FF shift */
		avg=0.0;
		n=0;
		for(k=kmin;k<(ndat-1);k+=1)
		{
			if((FF[k+1]-FF[k])<threshold)
			{
				avg+=FF[k];
				n+=1;
			}
		}

		avg/=n;
		for(k=kmin;k<(ndat-1);k+=1)
		{
			FF[k]-=avg;
		}

		for(k=kmin;k<(ndat-1);k+=1)  /* look for rising edges */
		{
			Trig[k]= (FF[k]>threshold)?1:0;
		}

		tcount=0;
		for(k=kmin;k<(ndat-1);k+=1)  /* record trigger times */
		{
			if((Trig[k+1]-Trig[k])==1)
			{
				TimeStamp[tcount++]=k+2;  /* there are tcount triggers */
			}
        }
		if(tcount>2)
		{
			TriggerLevelShift=0.0;
			for(n=0; n<(tcount-1); n+=1)
			{
				avg=0.0;
				kmin=(U16)(TimeStamp[n]+2*FL+FG);
				kmax=(U16)(TimeStamp[n+1]-1);
				if((kmax-kmin)>0)
				{
					for(k=kmin;k<kmax;k+=1)
					{
						avg+=FF2[k];
					}
				}
				TriggerLevelShift+=avg/(kmax-kmin);
			}
	    	TriggerLevelShift/=tcount;
		}

		switch(tcount)
		{
			case 0:
                /* Increament TFcount */
                TFcount ++;
                continue;
			case 1:
				t0=TimeStamp[0]+2*FL+FG;
				t1=ndat-2;
				break;
			default:
				MaxTimeDiff=0.0;
				for(k=0;k<(tcount-1);k+=1)
				{
					if((TimeStamp[k+1]-TimeStamp[k])>MaxTimeDiff)
					{
						MaxTimeDiff=TimeStamp[k+1]-TimeStamp[k];
						MaxTimeIndex=k;
					}
				}

				if((ndat-TimeStamp[tcount-1])<MaxTimeDiff)
				{
					t0=TimeStamp[MaxTimeIndex]+2*FL+FG;
					t1=TimeStamp[MaxTimeIndex+1]-1;
				}
				else
				{
					t0=TimeStamp[tcount-1]+2*FL+FG;
					t1=ndat-2;
				}

				break;
		}

		if(((t1-t0)*dt)<3*(*Tau))
		{
            /* Increament TFcount */
            TFcount ++;
			continue;
		}

		t1=MIN(t1,(t0+RoundOff(6*(*Tau)/dt+4)));

		s0=0;	s1=0;
		kmin=(U16)t0-(2*FL+FG)-FL-1;
		for(k=0;k<FL;k++)
		{
			s0+=Trace[kmin+k];
			s1+=Trace[(U16)(t0+k)];
		}
		if((s1-s0)/FL > localAmplitude)
		{
			fitted_tau=Tau_Fit(Trace, (U16)t0, (U16)t1, dt);
    		if(fitted_tau > 0)	/* Check if returned Tau value is valid */
			{
				*Tau=fitted_tau;
			}

			localAmplitude=(s1-s0)/FL;
		}

        /* Increament TFcount */
        TFcount ++;

    } while((*Tau == input_tau) && (TFcount < 10)); /* Try 10 times at most to get a valid Tau value */

	return(0);
  
}


/****************************************************************
 *	Tau_Fit function:
 *		Exponential fit of the ADC trace.
 *
 *		Return Value:
 *			Tau value if successful
 *			-1 - Geometric search did not find an enclosing interval
 *			-2 - Binary search could not find small enough interval
 *
 ****************************************************************/

double Tau_Fit (
			U32 *Trace,		// ADC trace data
			U32 kmin,		// lower end of fitting range
			U32 kmax,		// uuper end of fitting range
			double dt )		// sampling interval of ADC trace data
{
	double mutop,mubot,valtop,valbot,eps,dmu,mumid,valmid;
	U32 count;

	eps=1e-3;
	mutop=10e6; /* begin the search at tau=100ns (=1/10e6) */
	valtop=Phi_Value(Trace,exp(-mutop*dt),kmin,kmax);
	mubot=mutop;
	count=0;
	do  /* geometric progression search */
	{
		mubot=mubot/2.0;
		valbot=Phi_Value(Trace,exp(-mubot*dt),kmin,kmax);
		count+=1;
		if(count>20)
		{
			sprintf(ErrMSG, "*ERROR* (Tau_Fit): geometric search did not find an enclosing interval");
			Pixie_Print_MSG(ErrMSG);
			return(-1);
		}	/* Geometric search did not find an enclosing interval */ 
	} while(valbot>0);	/* tau exceeded 100ms */

	mutop=mubot*2.0;
	valtop=Phi_Value(Trace,exp(-mutop*dt),kmin,kmax);
	count=0;
	do  /* binary search */
	{
		mumid=(mutop+mubot)/2.0;
		valmid=Phi_Value(Trace,exp(-mumid*dt),kmin,kmax);
		if(valmid>0)
		{
			mutop=mumid;
		}
		else
		{
			mubot=mumid;
		}

		dmu=mutop-mubot;
		count+=1;
		if(count>20)
		{
			sprintf(ErrMSG, "*ERROR* (Tau_Fit): Binary search could not find small enough interval");
			Pixie_Print_MSG(ErrMSG);
			return(-2);  /* Binary search could not find small enough interval */
		}
  
	} while(fabs(dmu/mubot) > eps);

	return(1/mutop);  /* success */
}


/****************************************************************
 *	Phi_Value function:
 *		geometric progression search.
 *
 *		Return Value:
 *			search result
 *
 ****************************************************************/

double Phi_Value (
			U32 *ydat,		// source data for search
			double qq,		// search parameter
			U32 kmin,		// search lower limit
			U32 kmax )		// search upper limit
{
	S32 ndat;
	double s0,s1,s2,qp;
	double A,B,Fk,F2k,Dk,Ek,val;
	U32 k;

	ndat=kmax-kmin+1;
	s0=0; s1=0; s2=0;
	qp=1;

	for(k=kmin;k<=kmax;k+=1)
	{
		s0+=ydat[k];
		s1+=qp*ydat[k];
		s2+=qp*ydat[k]*(k-kmin)/qq;
		qp*=qq;
	}

	Fk=(1-pow(qq,ndat))/(1-qq);
	F2k=(1-pow(qq,(2*ndat)))/(1-qq*qq);
	Dk=-(ndat-1)*pow(qq,(2*ndat-1))/(1-qq*qq)+qq*(1-pow(qq,(2*ndat-2)))/pow((1-qq*qq),2);
	Ek=-(ndat-1)*pow(qq,(ndat-1))/(1-qq)+(1-pow(qq,(ndat-1)))/pow((1-qq),2);
	A=(ndat*s1-Fk*s0)/(ndat*F2k-Fk*Fk) ;
	B=(s0-A*Fk)/ndat;

	val=s2-A*Dk-B*Ek;

	return(val);

} 

/****************************************************************
 *	Thresh_Finder function:
 *		Threshold finder used for Tau Finder function.
 *
 *		Return Value:
 *			Threshold
 *
 ****************************************************************/

double Thresh_Finder (
			U32 *Trace,			// ADC trace data
			double *Tau,		// Tau value
			double *FF,			// return values for fast filter
			double *FF2,		// return values for fast filter
			U32 FL,				// fast length
			U32 FG,				// fast gap
			U8  ModNum,			// Pixie module number
			U8  ChanNum )		// Pixie channel number
{

	U32 ndat,kmin,k,idx,ndev,n,m;
	double Xwait,dt,xx,c0,sum0,sum1,deviation,threshold;
	S8  str[256];

	U16 SYSTEM_CLOCK_MHZ = 75;
	U16 FILTER_CLOCK_MHZ = 75;
	U16	ADC_CLOCK_MHZ = 75;
	U16	ThisADCclkMHz = 75;
	U16 LTscale = 16;

	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );


	ndev=8;		/* threshold will be 8 times sigma */
	ndat=IO_BUFFER_LENGTH;

	sprintf(str,"XWAIT%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Xwait=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	dt=Xwait/SYSTEM_CLOCK_MHZ*1e-6;
	xx=dt/(*Tau);
	c0=exp(-xx*(FL+FG));

	kmin=2*FL+FG;
	/* zero out the initial part,where the true filter values are unknown */
	for(k=0;k<kmin;k+=1)
	{
		FF[k]=0;
	}

    for(k=kmin;k<ndat;k+=1)
	{
		sum0=0;	sum1=0;
		for(n=0;n<FL;n++)
		{
			sum0+=Trace[k-kmin+n];
			sum1+=Trace[k-kmin+FL+FG+n];
		}
		FF[k]=sum1-sum0*c0;
	}

	/* zero out the initial part,where the true filter values are unknown */
	for(k=0;k<kmin;k+=1)
	{
		FF2[k]=0;
	}

    for(k=kmin;k<ndat;k+=1)
	{
		sum0=0;	sum1=0;
		for(n=0;n<FL;n++)
		{
			sum0+=Trace[k-kmin+n];
			sum1+=Trace[k-kmin+FL+FG+n];
		}
		FF2[k]=(sum0-sum1)/FL;
	}

	deviation=0;
	for(k=0;k<ndat;k+=2)
	{
		deviation+=fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]]);
	}

	deviation/=(ndat/2);
	threshold=ndev/2*deviation/2;

	m=0; deviation=0;
	for(k=0;k<ndat;k+=2)
	{
		if(fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]])<threshold)
		{
			m+=1;
			deviation+=fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]]);
		}
	}
	deviation/=m;
	deviation*=sqrt(PI)/2;
	threshold=ndev*deviation;
	
	m=0; deviation=0;
	for(k=0;k<ndat;k+=2)
	{
		if(fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]])<threshold)
		{
			m+=1;
			deviation+=fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]]);
		}
	}

	deviation/=m;
	deviation*=sqrt(PI)/2;
	threshold=ndev*deviation;
	
	m=0; deviation=0;
	for(k=0;k<ndat;k+=2)
	{
		if(fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]])<threshold)
		{
			m+=1;
			deviation+=fabs(FF[Random_Set[k]]-FF[Random_Set[k+1]]);
		}
	}
	deviation/=m;
	deviation*=sqrt(PI)/2;
	threshold=ndev*deviation;

    return(threshold);
}


/****************************************************************
 *	BLcut_Finder function:
 *		Find the BLcut value for the selected channel and return it
 *		using pointer *BLcut.
 *
 *		Return Value:
 *			 0 - successful
 *			-1 - failed to start collecting baselines
 *			-2 - baseline collection timed out
 *
 ****************************************************************/

S32 BLcut_Finder (
			U8 ModNum,			// Pixie module number
			U8 ChanNum,			// Pixie channel number
			double *BLcut )		// BLcut return value
{

	U16 idx, KeepLog, FilterRange, SL, SG, localBlCut, KeepChanNum, k, l;
	U32 BadBaselines, value, Wcount, buffer[IO_BUFFER_LENGTH];
	S8  str[256];
	double tim, tau, Bnorm, BLave, BLsigma, ExpFactor, b0, b1, b2, b3, baseline[IO_BUFFER_LENGTH/6];
	S32 retval;

	U16 SYSTEM_CLOCK_MHZ = 75;
	U16 FILTER_CLOCK_MHZ = 75;
	U16	ADC_CLOCK_MHZ = 75;
	U16	ThisADCclkMHz = 75;
	U16 LTscale = 16;

	if (Offline == 1) {
		sprintf(ErrMSG, "(BLcut_Finder): Offline mode. No I/O operations possible");
		Pixie_Print_MSG(ErrMSG);
		return (0);
	}

	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );



	/*****************************************************
	 *
	 *	Set proper DSP parameters for collecting baselines 
	 *
	 *****************************************************/

	/* Store the DSP parameter Log2BWeight value */
	sprintf(str,"LOG2BWEIGHT%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	KeepLog=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
	/* Temporarily set Log2BWeight to 0 */
	value=0;
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Get the values of DSP parameters FilterRange, SlowLength, SlowGap and BLCut */
	idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
	FilterRange=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	sprintf(str,"SLOWLENGTH%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	SL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	sprintf(str,"SLOWGAP%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	SG=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	/* Temporarily set BLcut to 0 */
	sprintf(str,"BLCUT%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	localBlCut=0;
	value=(U32)localBlCut;
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Set the DSP parameter ChanNum */
	idx=Find_Xact_Match("CHANNUM", DSP_Parameter_Names, N_DSP_PAR);
	KeepChanNum=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=ChanNum;
	value=(U32)ChanNum;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/*****************************************************
	 *
	 *	Start to collect baselines 
	 *
	 *****************************************************/

	tim=(double)(SL+SG)*pow(2.0, (double)FilterRange);

	/* Get DSP parameter PreampTau */
	sprintf(str,"PREAMPTAUA%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	tau=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	sprintf(str,"PREAMPTAUB%d", ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	tau+=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx]/65536;

	/* Calculate necessary bit shift for the baseline sums */
	/* baseline sums are truncated (FilterRange-2) bits in FPGA if FilterRange is */
	/* greater than 2; for all FilterRanges, baseline sums are upshifted by 6 bits. */

	Bnorm = pow(2.0, (double)(MAX(0,(FilterRange-2))-6))/(SL*pow(2.0, (double)(FilterRange+1)));
	
	/* Start Control Task COLLECT_BASES to collect 1365 baselines */
	retval=Start_Run(ModNum, NEW_RUN, 0, COLLECT_BASES);
	if(retval < 0)
	{
	    sprintf(ErrMSG, "BLcut_Finder: failed to collect baselines in Module %d", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-1); /* failed to start collecting baselines */
	}
	
	/* Check Run Status */
	Wcount=0;
	Pixie_Sleep(1); /* A short wait before polling */
	while(Check_Run_Status(ModNum) && (Wcount<10000))
	{
	    Pixie_Sleep(1);
	    Wcount++; /* The maximal waiting time is 10 s */ 
	}
	if(Wcount>=10000)
	{
	    sprintf(ErrMSG, "BLcut_Finder: Module %d timed out", ModNum);
	    Pixie_Print_MSG(ErrMSG);
	    return(-2); /* Time Out */
	}

	/* Read the data memory */
	Pixie_IODM(ModNum, IO_BUFFER_ADDRESS, MOD_READ, IO_BUFFER_LENGTH, buffer);

	/* Calculate BLsigma */
	BadBaselines = 0;
	l            = 0;
	BLsigma      = 0; // Baseline sigma^2
	BLave        = 0; // Baseline average
	ExpFactor    = exp(-tim / (tau * FILTER_CLOCK_MHZ));
	for(k = 0; k < IO_BUFFER_LENGTH-6; k += 6)
	{
	    b0 = (double)buffer[k];
	    b1 = (double)buffer[k+1];
	    b2 = (double)buffer[k+2];
	    b3 = (double)buffer[k+3];
	    baseline[l]        = ((b2 + b3 * 65536.0) - ExpFactor * (b0 + b1 * 65536.0)) * Bnorm;
	    if (l > 0) BLsigma = (BLsigma * (1.0 - 1.0 / (double)l) + 1.0 / (1.0 + (double)l) * (baseline[l] - BLave) * (baseline[l] - BLave));
	    BLave              = (baseline[l] / (1.0 + (double)l) + (double)l / (1.0 + (double)l) * BLave);
	    if (fabs(baseline[l] - BLave) > 4 * sqrt(BLsigma)) BadBaselines++;
	    l++;
	}
	BLsigma = sqrt(BLsigma); // sigma = sqrt(sigma^2)
	
	/* Calculate BLcut */
	localBlCut = (U16)floor(4.0 * BLsigma);
	if(localBlCut < 15)		//New in 123: ensure min. BLcut
	{
		localBlCut = 15;
	}
		if(localBlCut > 32767)		//New in 123: ensure min. BLcut
	{
		localBlCut = 32767;
	}
	/* Report progress or an alarming channel */
	if (BadBaselines > IO_BUFFER_LENGTH / 12) { // More than 600
	    sprintf(ErrMSG, "*WARNING* Mod. %d Chan.: %d  BLave: %e BLsigma: %e BLcut: %u Bad Baselines: %u", ModNum, ChanNum, BLave, BLsigma, localBlCut, BadBaselines);
	    Pixie_Print_MSG(ErrMSG);
	}
	else {
//	    sprintf(ErrMSG, "Mod. %d Chan.: %d  BLave: %e BLsigma: %e BLcut: %u Bad Baselines: %u", ModNum, ChanNum, BLave, BLsigma, localBlCut, BadBaselines);
//	    Pixie_Print_MSG(ErrMSG);
	}
	/*****************************************************
	 *
	 *	Set BLcut in DSP and restore several parameters
	 *
	 *****************************************************/

	/* Set DSP parameter BLcut */
	sprintf(str,"BLCUT%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=localBlCut;
	/* Download BLcut to the DSP data memory */
	value = (U32)localBlCut;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Restore DSP parameter Log2BWeight */
	sprintf(str,"LOG2BWEIGHT%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=KeepLog;
	/* Download Log2BWeight to the DSP data memory */
	value = (U32)KeepLog;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Restore DSP parameter ChanNum */
	idx=Find_Xact_Match("CHANNUM", DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=KeepChanNum;
	/* Download HostIO to the DSP data memory */
	value = (U32)KeepChanNum;
	Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Update user value BLCUT */
	idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
	Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=localBlCut;

	/* Update BLcut return value */
	*BLcut = (double)localBlCut;

	return(0);
}


/****************************************************************
 *	RandomSwap function:
 *		Generate a random set. The size of the set is IO_BUFFER_LENGTH.
 *
 ****************************************************************/

S32 RandomSwap(void)
{
	
	U32 rshift,Ncards;
	U32 k,MixLevel,imin,imax;
	U16 a;

	for(k=0; k<IO_BUFFER_LENGTH; k++) Random_Set[k]=(U16)k;

	Ncards=IO_BUFFER_LENGTH;
	rshift= (U32)(log(((double)RAND_MAX+1.0)/(double)IO_BUFFER_LENGTH)/log(2.0));
	MixLevel=5;
	
	for(k=0; k<MixLevel*Ncards; k++)
	{
	    imin=(rand()>>rshift); 
	    imax=(rand()>>rshift);
	    a=Random_Set[imax];
    	    Random_Set[imax]=Random_Set[imin];
    	    Random_Set[imin]=a;
	}

	return(0);

}




/****************************************************************
 *	RoundOff function:
 *		Round a floating point number to the nearest integer.
 *
 *		Return Value:
 *			rounded 32-bit integer
 *
 ****************************************************************/

U32 RoundOff(double x) { return((U32)floor(x+0.5)); }

/****************************************************************
 *	SetBit function:
 *		Set Bit function (for 16-bit words only).
 *
 *		Return Value:
 *			16-bit integer after setting the bit
 *
 ****************************************************************/

U16 SetBit(U16 bit, U16 value)
{
	return(value | (U16)(pow(2,bit)));
}

/****************************************************************
 *	ClrBit function:
 *		Clear Bit function (for 16-bit words only).
 *
 *		Return Value:
 *			16-bit integer after clearing the bit
 *
 ****************************************************************/

U16 ClrBit(U16 bit, U16 value)
{
	value=SetBit(bit, value);
	return(value ^ (U16)(pow(2,bit)));
}

/****************************************************************
 *	TglBit function:
 *		Toggle Bit function (for 16-bit words only).
 *
 *		Return Value:
 *			16-bit integer after toggling the bit
 *
 ****************************************************************/

U16 TglBit(U16 bit, U16 value)
{
	return(value ^ (U16)(pow(2,bit)));
}

/****************************************************************
 *	TstBit function:
 *		Test Bit function (for 16-bit words only).
 *
 *		Return Value:
 *			bit value
 *
 ****************************************************************/

U16 TstBit(U16 bit, U16 value)
{
	return(((value & (U16)(pow(2,bit))) >> bit));
}


/****************************************************************
 *	PixieListModeReader function:
 *		Parse list mode data file using analysis logic 
 *              defined in six functions. These functions contain 
 *              logic to be applied:
 *              1) before the file processing begins;
 *              2) after reading buffer header;
 *              3) after reading event header;
 *              4) after reading channel data (header + trace if applicable);
 *              5) for a channel not in the read pattern;
 *              6) after the file processing is finished.
 *
 *              The reader uses a data structure (LMA) globally defined in 
 *              globals.c and typedefed in utilities.h. This structure contains all
 *              the necessary variables to read list mode files as well as function
 *              and void pointers for the user analysis functions to be linked in and 
 *              additional user parameters to be exchanged between the functions.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *                      -2 - buffer length zero; corrupted file
 *                      -3 - wrong run type
 *
 ****************************************************************/

int PixieListModeReader (LMR_t LMA)
{

	U16 RunType                = 0;
	U16 ModuleType = 0;
	U16 ChannelHeaderLengths[] = {MAX_CHAN_HEAD_LENGTH, MAX_CHAN_HEAD_LENGTH, 4, 2};
	U16 MaxChanLen = 0;
	U16 MaxBufLen = 8192;
	S32 ReturnValue            = 0;

	/* Open the list mode file if exists */
	if(!(LMA->ListModeFile = fopen(LMA->ListModeFileName, "rb"))) { 
		sprintf(ErrMSG, "*ERROR* (PixieListModeReader): can't open list mode data file %s", LMA->ListModeFileName);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	/* Read the first buffer header to glean initial information about the type of the list mode file */
	fread (LMA->BufferHeader, sizeof(U16), BUFFER_HEAD_LENGTH, LMA->ListModeFile);
	/* User requests only first header for identification if LMA->ReadFirstBufferHeader is set */
	if (LMA->ReadFirstBufferHeader) {
		fclose(LMA->ListModeFile);
		return (0);
	}
	/* Otherwise rewind to the beginning of the file */
	rewind(LMA->ListModeFile);
	/* Determine the module type and the coresponding channel length limitation */
	/* The upper 4 bits of the record indicate the module type */
	ModuleType = LMA->BufferHeader[2] & 0xF000;
	if (ModuleType == 2) MaxChanLen = 1024;
	else MaxChanLen = 8192;
	/* Determine the run type and coresponding channel header length */
	/* The lower 12 bits of the record indicate the run task number */
	RunType = LMA->BufferHeader[2] & 0x0F0F; 
	if (RunType < 0x100 || RunType > 0x103) { /* If run type is bogus then finish processing */
		fclose(LMA->ListModeFile);
		sprintf(ErrMSG, "*ERROR* (PixieListModeReader): wrong run type");
		Pixie_Print_MSG(ErrMSG);
		return (-3);
	}
	/* Chose the right channel header length from the list */
	LMA->ChanHeadLen = ChannelHeaderLengths[RunType-0x100];
	/* If defined call pre-analysis user logic function */
	if (LMA->PreAnalysisAction) LMA->PreAnalysisAction(LMA);
	/* Read the list mode file and do the processing */
	while (!feof(LMA->ListModeFile)) { /* Loop over buffer headers */
		fread (LMA->BufferHeader, sizeof(U16), BUFFER_HEAD_LENGTH, LMA->ListModeFile);
		/* If buffer header length is zero, things went awry */
		if (!LMA->BufferHeader[0] || LMA->BufferHeader[0] > MaxBufLen) break;
		/* If defined call buffer-level user logic function */
		if (LMA->BufferLevelAction) LMA->BufferLevelAction(LMA);
		LMA->BufferHeader[0] -= BUFFER_HEAD_LENGTH;
		while (LMA->BufferHeader[0]) {/* Loop over event headers */
			fread (LMA->EventHeader, sizeof(U16), EVENT_HEAD_LENGTH, LMA->ListModeFile);
			LMA->BufferHeader[0] -= EVENT_HEAD_LENGTH;
			/* If defined call event-level user logic function */
			if (LMA->EventLevelAction) LMA->EventLevelAction(LMA);
			for (LMA->Channel = 0; LMA->Channel < NUMBER_OF_CHANNELS; LMA->Channel++) { /* Loop over channel headers and traces if present */
				if (LMA->EventHeader[0] & (0x1 << LMA->Channel)) { /* Only go over channels defined in read pattern */
					fread (LMA->ChannelHeader, sizeof(U16), LMA->ChanHeadLen, LMA->ListModeFile);
					if (LMA->ChannelHeader[0] > MaxChanLen && RunType < 0x102) {
						sprintf(ErrMSG, "*ERROR* (PixieListModeReader): channel header corruption encountered");
						Pixie_Print_MSG(ErrMSG);
						fclose(LMA->ListModeFile);
						return (-2);
					}
					if (RunType == 0x100) { /* Read trace if it is run 0x100 */
						fread (LMA->Trace, sizeof(U16), LMA->ChannelHeader[0] - LMA->ChanHeadLen, LMA->ListModeFile);
						LMA->BufferHeader[0] -= LMA->ChannelHeader[0];
					}
					else LMA->BufferHeader[0] -= LMA->ChanHeadLen;
					/* If defined call channel-level user logic function */
					if (LMA->ChannelLevelAction) LMA->ChannelLevelAction(LMA);
					LMA->Traces[LMA->BufferHeader[1]]++; /* Count traces in each module */
					LMA->TotalTraces++; /* Count all traces */
				}
				else { /* If the channel is not in the read pattern some action may still be needed */
					/* If defined call auxiliary channel action function */
					if (LMA->AuxChannelLevelAction) LMA->AuxChannelLevelAction(LMA);
				}
			}
			LMA->Events[LMA->BufferHeader[1]]++; /* Count events in each module */
			LMA->TotalEvents++;  /* Count all events */
		}
		LMA->Buffers[LMA->BufferHeader[1]]++;    /* Count buffers in each module */
		LMA->TotalBuffers++;  /* Count all buffers */
	}
	/* Check for premature EOF due to an error */
	if (!feof(LMA->ListModeFile)) {
		sprintf(ErrMSG, "*ERROR* (PixieListModeReader): buffer header corruption encountered");
		Pixie_Print_MSG(ErrMSG);
		ReturnValue = -2; /* If so return a corresponding error code */
	}
	/* If defined call post-analysis user logic function */
	if (LMA->PostAnalysisAction) LMA->PostAnalysisAction(LMA);
	/* Close list mode file */
	fclose(LMA->ListModeFile);
	/* Done */
	return(ReturnValue);

}

/****************************************************************
 *	Pixie_Parse_List_Mode_Events function:
 *		Parse the list mode events from the list mode data file,
 *		write values of time stamp, energy, XIA_PSA, user_PSA
 *		of each event into a text file, and send the number of events
 *		and traces in each module to the host.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - NULL pointer *ModuleEvents
 *			-2 - can't open list mode data file
 *			-3 - can't open output file
 *			-4 - found BufNdata = 0
 *
 ****************************************************************/

void *allocate_array (size_t drow, size_t dcol, char   type)
{
	/* Create an array of different types */

	char **m        = NULL;
	char **mfirst   = NULL;
	char  *auxptr   = NULL;
	size_t dcolsize = 0;
	size_t ptrsize  = sizeof(char *);
	size_t varsize  = 0;

	if (type == 'c' ) dcolsize = dcol * (varsize = sizeof(char));
	if (type == 's' ) dcolsize = dcol * (varsize = sizeof(unsigned short));
	if (type == 'u' ) dcolsize = dcol * (varsize = sizeof(unsigned int));
	if (type == 'f' ) dcolsize = dcol * (varsize = sizeof(float));
	if (type == 'd' ) dcolsize = dcol * (varsize = sizeof(double));
	if (varsize == 0) return NULL; // Wrong type specified

	mfirst = m = calloc(drow, ptrsize);
	if (m)
	{
		auxptr = *m = calloc(drow, dcolsize);
		if (*m)
		{
			while (--drow) *(++m) = (auxptr += dcolsize);
			return (void *)mfirst;
		}
		free(m);
	}

	return NULL;
}

void free_array (void **a)
{
    /* Destroy a previously created array */

    free (*a);
    free (a);

}

S32 PixieParseListModeEventsBufferLevel (LMR_t LMA)
{ 

	/* This is a buffer-level user function for the list mode reader */
	U16 Module           = LMA->BufferHeader[1];
	U16 ModuleID         = LMA->BufferHeader[2] & 0xF000; /* The upper 4 bits of the record indicate the module type: DGF, Pixie-4, P500 */
	U16 System_Clock_MHz = 0;

	if (ModuleID == 0x2000) System_Clock_MHz = P4_SYSTEM_CLOCK_MHZ;
	if (ModuleID == 0x4000) System_Clock_MHz = P500_SYSTEM_CLOCK_MHZ;

	if(AutoProcessLMData > 0 && LMA->Buffers[Module] == 0) { /* Print out the Module header if first buffer */
		FILE  *OutputFilePerModule = ((FILE **)LMA->par1)[Module];
		FILE *OutputFile                    =   (FILE *)LMA->par0;
		U16    RunType    = LMA->BufferHeader[2] & 0x0FFF;
		double RunStartTime = ((double)LMA->BufferHeader[3] * 65536.0 * 65536.0 + (double)LMA->BufferHeader[4] * 65536.0 + (double)LMA->BufferHeader[5]) / (double)System_Clock_MHz * 1.0e-6;
		if(AutoProcessLMData != 4) fprintf(OutputFilePerModule, "\nModule:\t%d\n",         Module);
		if(AutoProcessLMData != 4) fprintf(OutputFilePerModule, "Run Type:\t%d\n",         RunType);
		if(AutoProcessLMData != 4) fprintf(OutputFilePerModule, "Run Start Time:\t%f\n\n", RunStartTime);
		if(AutoProcessLMData == 1) {
			if (RunType < 0x102) fprintf(OutputFilePerModule, "Event No\tChannel No\tEnergy\tTrig Time\tXIA_PSA\tUser_PSA\n");
			if (RunType > 0x101) fprintf(OutputFilePerModule, "Event No\tChannel No\tEnergy\tTrig Time\n");
		}
		if(AutoProcessLMData == 2) fprintf(OutputFilePerModule, "Event No\tChannel No\tHit Pattern\tEvent_Time_A\tEvent_Time_B\tEvent_Time_C\tEnergy\tTrig Time\tXIA_PSA\tUser_PSA\n");
		if(AutoProcessLMData == 3) fprintf(OutputFilePerModule, "Event\tChannel\tTimeStamp\tEnergy\tRT\tApeak\tBsumC\tCsum\tPsum\tPSAval\n");
		if(AutoProcessLMData == 4 && Module == 0) fprintf(OutputFile, "Event\tMulti\tEa\tEb\tEc\tTa\tTb\tTc\tChanA\tChanB\tChanC\tMa\tMb\tMc\n");
	}

	return (0);
}

S32 PixieParseListModeEventsChannelLevel (LMR_t LMA) 
{
    
    /* This is a channel-level user function for the list mode reader */
	if(AutoProcessLMData > 0) {
		U8 *(*Format)[5]          = LMA->par2;
		S32 **DT4Data		  = (U32 **)LMA->par3;
		U16   Module              = LMA->BufferHeader[1];
		U16   RunType             = LMA->BufferHeader[2] & 0x0FFF;
		U16   chl                 = LMA->ChanHeadLen;
		U16   Channel             = LMA->Channel;
		U16   TrigTime            = 0;
		U16   Energy              = 0;
		U16   XIA_PSA             = LMA->ChannelHeader[3];    
		U16   User_PSA            = LMA->ChannelHeader[4];
		U16   HT                  = LMA->ChannelHeader[8];
		U16   EvtPattern          = LMA->EventHeader[0];
		U16   ETH                 = LMA->EventHeader[1];    
		U16   ETL                 = LMA->EventHeader[2];
		U32   NumEvents           = LMA->Events[Module];
		FILE *OutputFilePerModule = ((FILE **)LMA->par1)[Module];

		if (RunType < 0x102) {
			TrigTime            = LMA->ChannelHeader[1];
			Energy              = LMA->ChannelHeader[2];
		}
		if (RunType > 0x101) {
			TrigTime            = LMA->ChannelHeader[0];
			Energy              = LMA->ChannelHeader[1];
		}
		if(chl == 4) HT = 0;
		if(chl == 2) HT = XIA_PSA = User_PSA = 0;
		if(AutoProcessLMData == 1) fprintf(OutputFilePerModule, Format[chl][1], NumEvents, Channel, Energy, TrigTime, XIA_PSA, User_PSA);
		if(AutoProcessLMData == 2) fprintf(OutputFilePerModule, Format[chl][2], NumEvents, Channel, EvtPattern, HT, ETH, ETL, Energy, TrigTime, XIA_PSA, User_PSA); 
		if(AutoProcessLMData == 3) fprintf(OutputFilePerModule, 
			"%d   %d   %d   %d   %d   %d   %d   %d  %d  %d\n", 
			NumEvents, 
			Channel,
			LMA->ChannelHeader[1],
			LMA->ChannelHeader[2],
			LMA->ChannelHeader[3],
			LMA->ChannelHeader[4],
			LMA->ChannelHeader[5],
			LMA->ChannelHeader[6],
         LMA->ChannelHeader[7],
			LMA->ChannelHeader[8]);
		if (AutoProcessLMData == 4) {
			/* If multiplicity is >=3 and event numbers coincide fill up the third record */
			if (DT4Data[NumEvents][1] >= 3 && Energy>0) 
				DT4Data[NumEvents][1] ++ ; /* Multiplicity becomes two */			

			 /* If multiplicity is two and event numbers coincide fill up the third record */
			if (DT4Data[NumEvents][1] == 2 && Energy>0 ) {
					DT4Data[NumEvents][1] = 3; /* Multiplicity becomes 3 */
					DT4Data[NumEvents][4] = Energy; /* Energy C */
					DT4Data[NumEvents][7] = ETL; /* Time C */
					DT4Data[NumEvents][10] = Channel + 4 * Module; /* Channel C */
					DT4Data[NumEvents][13] = Module; /* Module C */ 

				if ( (abs(DT4Data[NumEvents][5]-ETL)>10) && (abs(DT4Data[NumEvents][6]-ETL)<=10) ) {
					// close to B and different from A -> move A to C, store new in A
					DT4Data[NumEvents][4] = DT4Data[NumEvents][2]; /* Energy C */
					DT4Data[NumEvents][7] = DT4Data[NumEvents][5]; /* Time C */
					DT4Data[NumEvents][10] = DT4Data[NumEvents][8]; /* Channel C */
					DT4Data[NumEvents][13] = DT4Data[NumEvents][11]; /* Module C */ 

					DT4Data[NumEvents][2] = Energy; /* Energy A */
					DT4Data[NumEvents][5] = ETL; /* Time A */
					DT4Data[NumEvents][8] = Channel + 4 * Module; /* Channel A */
					DT4Data[NumEvents][11] = Module; /* Module A */
				}
				if ( (abs(DT4Data[NumEvents][5]-ETL)<=10) && (abs(DT4Data[NumEvents][6]-ETL)>10) ) {
					// close to A and different from B -> move B to C, store new in B
					DT4Data[NumEvents][4] = DT4Data[NumEvents][3]; /* Energy C */
					DT4Data[NumEvents][7] = DT4Data[NumEvents][6]; /* Time C */
					DT4Data[NumEvents][10] = DT4Data[NumEvents][9]; /* Channel C */
					DT4Data[NumEvents][13] = DT4Data[NumEvents][12]; /* Module C */ 

					DT4Data[NumEvents][3] = Energy; /* Energy B */
					DT4Data[NumEvents][6] = ETL; /* Time B */
					DT4Data[NumEvents][9] = Channel + 4 * Module; /* Channel B */
					DT4Data[NumEvents][12] = Module; /* Module B */
				}
			

			}
			/* If multiplicity is one and event numbers coincide fill up the second record */
			if (DT4Data[NumEvents][1] == 1 && Energy>0 ) {
				if(DT4Data[NumEvents][5]<=ETL){
					DT4Data[NumEvents][1] = 2; /* Multiplicity becomes two */
					DT4Data[NumEvents][3] = Energy; /* Energy B */
					DT4Data[NumEvents][6] = ETL; /* Time B */
					DT4Data[NumEvents][9] = Channel + 4 * Module; /* Channel B */
					DT4Data[NumEvents][12] = Module; /* Module B */
				}
				else{
					DT4Data[NumEvents][1] = 2; /* Multiplicity becomes two */
					DT4Data[NumEvents][3] = DT4Data[NumEvents][2]; /* Energy B */
					DT4Data[NumEvents][6] = DT4Data[NumEvents][5]; /* Time B */
					DT4Data[NumEvents][9] = DT4Data[NumEvents][8]; /* Channel B */
					DT4Data[NumEvents][12] = DT4Data[NumEvents][11]; /* Module B */

					DT4Data[NumEvents][2] = Energy; /* Energy A */
					DT4Data[NumEvents][5] = ETL; /* Time A */
					DT4Data[NumEvents][8] = Channel + 4 * Module; /* Channel A */
					DT4Data[NumEvents][11] = Module; /* Module A */
				}
			}
			 /* If multiplicity is zero fill up the first record */
			if (!DT4Data[NumEvents][1] && Energy>0 ) {
				DT4Data[NumEvents][0] = NumEvents;
				DT4Data[NumEvents][1] = 1; /* Multiplicity becomes one */
				DT4Data[NumEvents][2] = Energy; /* Energy A */
				DT4Data[NumEvents][5] = ETL; /* Time A */
				DT4Data[NumEvents][8] = Channel + 4 * Module; /* Channel A */
				DT4Data[NumEvents][11] = Module; /* Module A */
			}
		}
	}
	return (0);
}


S32 PixieParseListModeEventsPostAnalysisLevel (LMR_t LMA) 
{
    
	if(AutoProcessLMData == 4) {
		S32 **DT4Data		  = (U32 **)LMA->par3;
		U16   Module              = LMA->BufferHeader[1];
		U16   Channel             = LMA->Channel;
		U16   TrigTime            = 0;
		U16   Energy              = 0;
		U32   NumEvents           = LMA->Events[Module];
		U32 i, j;
		FILE *OutputFile                    =   (FILE *)LMA->par0;

		for (i = 0; i < NumEvents; i++) {
			if ((DT4Data[i][1] == 2 && /* If multiplicity is two */
				abs(DT4Data[i][5]-DT4Data[i][6]) < 10)  /* If time stamps match */
				|| /* or */ 
				(DT4Data[i][1] >= 3 && /* If multiplicity is three */
					(abs(DT4Data[i][5]-DT4Data[i][6]) < 10 ||	/* If time stamps match */
					abs(DT4Data[i][7]-DT4Data[i][6]) < 10 ||
					abs(DT4Data[i][5]-DT4Data[i][7]) < 10))  
				) {
						for (j = 0; j < 14; j++) {
							fprintf(OutputFile, "%d\t", DT4Data[i][j]);
						}
						fprintf(OutputFile, "\n");
			}
		}
	
	}
	return (0);
}


S32 Pixie_Parse_List_Mode_Events (S8  *filename,      /* the list mode data file name (with complete path) */
			          U32 *ModuleEvents ) /* receives number of events & traces for each module */
{
    U8   			  *Format[10][5] = {NULL};
    U8                 OutputFileName[256]           = {'\0'};
    U8                 TempFileName  [256]           = {'\0'};
	S32				**DT4Data							= NULL;
    U16                i												=  0;
    FILE              *OutputFile						=  NULL;
    FILE              *TempFiles[PRESET_MAX_MODULES] = {NULL};
    
    /* Check if pointer *ModuleEvents has been initialized */
	if(!ModuleEvents) {
		sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Events): NULL pointer *ModuleEvents");
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
    
	//sprintf(ErrMSG, "*INFO* (Pixie_Parse_List_Mode_Events): Auto variable: %d", AutoProcessLMData);
	//Pixie_Print_MSG(ErrMSG);

	Format[9][1] = "%-9d%-12d%-9d%-15d%-9d%-6d\n";
    Format[9][2] = "%d\t %d\t %X\t %d\t %d\t %d\t %d\t %d\t %d\t %d\n";
    Format[4][1] = Format[9][1];
    Format[4][2] = "%-9d%-12d%-12x%-12d%-15d%-15d%-9d%-12d%-9d%-6d\n";
    Format[2][1] = "%-9d%-12d%-9d%-15d\n";
    Format[2][2] = Format[4][2];
	
    /* Zero the list mode reader arguments structure */ 
    memset(LMA, 0, sizeof(*LMA)); 
    /* If a record file is requested */
	if (AutoProcessLMData > 0) {
		if ( AutoProcessLMData == 4) { /* For AutoProcessLMData = 4 determine the number of recorded traces*/
			LMA->ListModeFileName = filename;
			/* Do the processing */
			if (PixieListModeReader(LMA) == -1) {
				sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Traces): Cannot open list mode file");
				Pixie_Print_MSG(ErrMSG);
				return (-2); /* Error means "cannot open file" */
			}
			sprintf(ErrMSG, "*INFO* (Pixie_Parse_List_Mode_Events): Traces: %d", LMA->Events[0]);
			Pixie_Print_MSG(ErrMSG);
			/* Create a 2D array of run data */
			if (!(DT4Data = allocate_array (LMA->Events[0]+100, 14, 'u'))) {
				sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Traces): Cannot allocate 2D array");
				Pixie_Print_MSG(ErrMSG);
				return (-3);
			}
			/* Zero the list mode reader arguments structure */ 
			memset(LMA, 0, sizeof(*LMA));
		}
		/* Determine the name of the output file */
		strcpy(OutputFileName, filename); 
		/* Check if the user requests outputing parsed data to a file */
		if(AutoProcessLMData == 1) strcpy(&OutputFileName[strlen(OutputFileName)-3], "dat");
		/* Long output data file with full timestamp and hit pattern */
		if(AutoProcessLMData == 2) strcpy(&OutputFileName[strlen(OutputFileName)-3], "dt2");
		/* Long output data file with Energy, Time and various PSA values*/
		if(AutoProcessLMData == 3) strcpy(&OutputFileName[strlen(OutputFileName)-3], "dt3");
		/* Long output data file with Xe coincidence event data */
		if(AutoProcessLMData == 4) strcpy(&OutputFileName[strlen(OutputFileName)-3], "dt4");
		if(!(OutputFile = fopen(OutputFileName, "w"))) {
			sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Events): can't open output file", OutputFileName);
			Pixie_Print_MSG(ErrMSG);
			if (AutoProcessLMData == 4) free_array (DT4Data);
			return(-3);
		}
		/* Open temporary files for each module for writing if not AutoProcess variant 4*/
		if (AutoProcessLMData != 4) {
			for(i = 0; i < PRESET_MAX_MODULES; i++) {
				sprintf(TempFileName, "_TempFile_Module_%hu", i);
				if(!(TempFiles[i] = fopen(TempFileName, "w"))) {
					sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Events): can't open output temporary file", TempFileName);
					Pixie_Print_MSG(ErrMSG);
				}
			}
		}
		/* Populate data structure for AutoProcessLMData > 0 */
		LMA->par0               = OutputFile;
		LMA->par1               = TempFiles;
		LMA->par2               = Format;
		LMA->par3               = DT4Data;
		LMA->BufferLevelAction  = PixieParseListModeEventsBufferLevel;
		LMA->ChannelLevelAction = PixieParseListModeEventsChannelLevel;
		LMA->PostAnalysisAction = PixieParseListModeEventsPostAnalysisLevel;
	}
    
    /* Add file name to the data structure */
    LMA->ListModeFileName = filename;
    
    /* Do the processing */
	if (PixieListModeReader(LMA) == -1) {
		sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Traces): Cannot open list mode file");
		Pixie_Print_MSG(ErrMSG);
		if (AutoProcessLMData == 4) free_array (DT4Data);
		return (-2); /* Error means "cannot open file" */
	}

    /* Fill up ModeluEvents. If requested: close temp files for writing; */ 
    /* combine temp files into one output file; close and delete temp files */
	for(i = 0; i < PRESET_MAX_MODULES; i++) {
		MODULE_EVENTS[i]                    = ModuleEvents[i]                    = LMA->Events[i];
		MODULE_EVENTS[i+PRESET_MAX_MODULES] = ModuleEvents[i+PRESET_MAX_MODULES] = LMA->Traces[i];
		if (AutoProcessLMData > 0) {
			if (AutoProcessLMData != 4) {
				fclose(TempFiles[i]);
				sprintf(TempFileName, "_TempFile_Module_%hu", i);
				if(!(TempFiles[i] = fopen(TempFileName, "rb"))) {
					sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Events): can't open temporary file", TempFileName);
					Pixie_Print_MSG(ErrMSG);
				}
				while (!feof(TempFiles[i])) putc(getc(TempFiles[i]), OutputFile);
				fclose(TempFiles[i]);
				remove(TempFileName);
			}
		}
	}
    
    /* Close output file */
    if (AutoProcessLMData > 0) fclose(OutputFile);
	if (AutoProcessLMData == 4) free_array (DT4Data);
    
    return(0);
    
}

/****************************************************************
 *	Pixie_Locate_List_Mode_Traces function:
 *		Parse the list mode file and locate the starting point,
 *		length, energy of each trace.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL pointer *ModuleTraces
 *			-4 - Run type is not 0x100
 *
 ****************************************************************/

S32 PixieLocateListModeTracesChannelLevel (LMR_t LMA) 
{
    
    /* This is a channel-level user function for the list mode reader */
    U16  ModNum         = LMA->BufferHeader[1];
    U32 *ModuleTraces   = (U32*)LMA->par0;
    U32 *ShiftFromStart = (U32*)LMA->par1;
    U32  TraceLen       = (U32)(LMA->ChannelHeader[0] - LMA->ChanHeadLen);
    U32  Energy         = (U32) LMA->ChannelHeader[2];
    U32  TracePos       = (U32)(ftell(LMA->ListModeFile) + 1) / 2 - TraceLen;

    ModuleTraces[3*(ShiftFromStart[ModNum]+LMA->Traces[ModNum])+0] = TracePos;
    ModuleTraces[3*(ShiftFromStart[ModNum]+LMA->Traces[ModNum])+1] = TraceLen; 
    ModuleTraces[3*(ShiftFromStart[ModNum]+LMA->Traces[ModNum])+2] = Energy;
    
    return (0);
    
}

S32 Pixie_Locate_List_Mode_Traces (S8  *filename,	// the list mode data file name (with complete path)
			           U32 *ModuleTraces)	// receives trace length and location for each module
{
    
    S32                status                             =  0;
    S32                i                                  =  0;
    U32                ShiftFromStart[PRESET_MAX_MODULES] = {0};
    U32                TotalShift                         =  0;
    
    /* Check if pointer *ModuleEvents has been initialized */
    if(!ModuleTraces) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Traces): NULL pointer *ModuleTraces");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
    
    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA)); 
    
    /* Prepare the array of module-dependent shifts */
    for(i = 1; i < PRESET_MAX_MODULES; i++) {
	ShiftFromStart[i] = (TotalShift += MODULE_EVENTS[i+PRESET_MAX_MODULES-1]);
    }
    
    /* Determine the run type reading only the first buffer */
    LMA->ListModeFileName      = filename;
    LMA->ReadFirstBufferHeader = 1;
    if (PixieListModeReader(LMA) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Traces): problems opening list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    
    /* If run type is not 0x100 then there is no reason to do any processing */
    if ((LMA->BufferHeader[2] & 0x0FFF) != 0x100) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Parse_List_Mode_Traces): wrong run type");
	Pixie_Print_MSG(ErrMSG);
	return (-4);
    }

    /* Do the processing */
    LMA->ReadFirstBufferHeader = 0;
    LMA->par0                  = ModuleTraces;
    LMA->par1                  = ShiftFromStart;
    LMA->ChannelLevelAction    = PixieLocateListModeTracesChannelLevel;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Traces): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Traces): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }
    
    return(0);
    
}


/****************************************************************
 *	Pixie_Read_List_Mode_Traces function:
 *		Read specfic trace events from the list mode file.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *
 ****************************************************************/

S32 Pixie_Read_List_Mode_Traces (S8  *filename,		// the list mode data file name (with complete path)
				 U32 *ListModeTraces )	// receives list mode trace data
{
	U16 i, idx;
	U32 j;
	FILE  *ListModeFile = NULL;
	
	/* Open the list mode file */
	ListModeFile = fopen(filename, "rb");
	if(ListModeFile != NULL) {
		idx = NUMBER_OF_CHANNELS * 2;
		/* Read list mode traces from the file */
		for( i = 0; i < NUMBER_OF_CHANNELS; i ++ ) {
			if( ( ListModeTraces[i*2] != 0 ) && (ListModeTraces[i*2+1] != 0 ) ) {
				/* Position ListModeFile to the requested trace location */
				fseek(ListModeFile, ListModeTraces[i*2]*2, SEEK_SET);
				/* Read trace */
				for(j=0; j<ListModeTraces[i*2+1]; j++) {
					fread(&ListModeTraces[idx++], 2, 1, ListModeFile);
				}
			}
		}
		fclose(ListModeFile);
	}
	else {
		sprintf(ErrMSG, "*ERROR* (Pixie_Read_List_Mode_Traces): can't open list mode file %s", filename);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	return(0);
}


/****************************************************************
 *	Pixie_Read_Energies function:
 *		Read energy values from the list mode file for a Pixie module.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL EventEnergies pointer
 *
 ****************************************************************/
 
S32 PixieReadEnergiesChannelLevel (LMR_t LMA) 
{
    /* This is a channel-level user function for the list mode reader */
    U8  *ModNum        = (U8*) LMA->par1;
    U16  RunTask       =       LMA->BufferHeader[2] & 0x0FFF;
    U32 *EventEnergies = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    if (RunTask > 0x101) EventEnergies[TraceNum] = LMA->ChannelHeader[1];
    else                 EventEnergies[TraceNum] = LMA->ChannelHeader[2];
    
    return (0);  
} 

S32 PixieReadEnergiesAuxChannelLevel (LMR_t LMA) 
{

    /* This is an auxiliary channel-level user function for the list mode reader */
    U8  *ModNum        = (U8*) LMA->par1;
    U32 *EventEnergies = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    EventEnergies[TraceNum] = 0;
    
    return (0);
    
} 

S32 Pixie_Read_Energies (S8  *filename,			// list mode data file name
			 U32 *EventEnergies,		// receive event energy values
			 U8   ModNum )			// Pixie module number
{

    int status = 0;
    
    /* Check if pointer *ModuleEvents has been initialized */
    if(!EventEnergies) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Energies): NULL pointer *EventEnergies");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
    
    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA)); 

    /* Do the processing */
    LMA->ListModeFileName   = filename;
    LMA->par0               = EventEnergies;
    LMA->par1               = &ModNum;
    LMA->ChannelLevelAction = PixieReadEnergiesChannelLevel;
	LMA->AuxChannelLevelAction = PixieReadEnergiesAuxChannelLevel;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Energies): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Energies): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }

    return (0);
    
}

/****************************************************************
 *	Pixie_Read_Event_PSA function:
 *		Read event PSA values from the list mode file for a Pixie module.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can not open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL EventPSA array pointer
 *			-4 - wrong run type
 *
 ****************************************************************/
 
S32 PixieReadEventPSAChannelLevel (LMR_t LMA) 
{
    /* This is a channel-level user function for the list mode reader */
    U8  *ModNum   = (U8*) LMA->par1;
    U16  RunTask  =       LMA->BufferHeader[2] & 0x0FFF;
    U32 *EventPSA = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    if (RunTask == 0x100 || RunTask == 101) {
	EventPSA[2*TraceNum+0] = LMA->ChannelHeader[3]; /* XIA PSA */
	EventPSA[2*TraceNum+1] = LMA->ChannelHeader[4]; /* User PSA */
    }
    if (RunTask == 0x102) {
	EventPSA[2*TraceNum+0] = LMA->ChannelHeader[2]; /* XIA PSA */
	EventPSA[2*TraceNum+1] = LMA->ChannelHeader[3]; /* User PSA */
    }
    
    return (0);    
} 

S32 PixieReadEventPSAAuxChannelLevel (LMR_t LMA) 
{
    /* This is a channel-level user function for the list mode reader */
    U8  *ModNum   = (U8*) LMA->par1;
    U32 *EventPSA = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    EventPSA[2*TraceNum+0] = 0; /* XIA PSA */
	EventPSA[2*TraceNum+1] = 0; /* User PSA */
    
    return (0);   
} 

S32 Pixie_Read_Event_PSA (S8  *filename,	// list mode data file name
			  U32 *EventPSA,	// receive event PSA values
			  U8   ModNum )		// Pixie module number
{

    int status;
    
    /* Check if pointer *ModuleEvents has been initialized */
    if(!EventPSA) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Event_PSA): NULL pointer *EventPSA");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
    
    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA));

    /* Determine the run type reading only the first buffer */
    LMA->ListModeFileName      = filename;
    LMA->ReadFirstBufferHeader = 1;
    if (PixieListModeReader(LMA) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Event_PSA): problem opening list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    
    /* If run type is higher than 0x102 then there is no reason to do any processing */
    if ((LMA->BufferHeader[2] & 0x0FFF) > 0x102) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Event_PSA): wrong run type");
	Pixie_Print_MSG(ErrMSG);
	return (-4);
    }

    /* Do the processing */
    LMA->ReadFirstBufferHeader = 0;
    LMA->par0                  = EventPSA;
    LMA->par1                  = &ModNum;
    LMA->ChannelLevelAction    = PixieReadEventPSAChannelLevel;
	LMA->AuxChannelLevelAction = PixieReadEventPSAAuxChannelLevel;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Event_PSA): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Event_PSA): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }

    return (0);
    
}

/****************************************************************
 *	Pixie_Read_Long_Event_PSA function:
 *		Read the extended event PSA values from the list mode file for a Pixie module:
 *		- timestamp
 *      - energy
 *      - XIA_PSA
 *		- User_PSA
 *      - User_2
 *		- User_3
 *      - User_4
 *		- User_5
 *	 
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can not open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL EventPSA array pointer
 *			-4 - wrong run type
 *
 ****************************************************************/

S32 PixieReadLongEventPSAChannelLevel (LMR_t LMA) 
{
    /* This is a channel-level user function for the list mode reader */
	U8  *ModNum   = (U8*) LMA->par1;
    U32 *EventPSA = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    EventPSA[8*TraceNum+0] = LMA->ChannelHeader[1]; /* Time stamp */
    EventPSA[8*TraceNum+1] = LMA->ChannelHeader[2]; /* Energy */
    EventPSA[8*TraceNum+2] = LMA->ChannelHeader[3]; /* XIA PSA */
    EventPSA[8*TraceNum+3] = LMA->ChannelHeader[4]; /* User PSA */
    EventPSA[8*TraceNum+4] = LMA->ChannelHeader[5]; /* User 2 */
    EventPSA[8*TraceNum+5] = LMA->ChannelHeader[6]; /* User 3 */
    EventPSA[8*TraceNum+6] = LMA->ChannelHeader[7]; /* User 4 */
    EventPSA[8*TraceNum+7] = LMA->ChannelHeader[8]; /* User 5 */
	
    return (0);
} 

S32 PixieReadLongEventPSAAuxChannelLevel (LMR_t LMA) 
{
    /* This is an auxiliary channel-level user function for the list mode reader */
    U8  *ModNum   = (U8*) LMA->par1;
    U32 *EventPSA = (U32*)LMA->par0;
	U32  TraceNum = 4*LMA->Events[*ModNum] + (U32)LMA->Channel;
    
    EventPSA[8*TraceNum+0] = 0; /* Time stamp */
    EventPSA[8*TraceNum+1] = 0; /* Energy */
    EventPSA[8*TraceNum+2] = 0; /* XIA PSA */
    EventPSA[8*TraceNum+3] = 0; /* User PSA */
    EventPSA[8*TraceNum+4] = 0; /* User 2 */
    EventPSA[8*TraceNum+5] = 0; /* User 3 */
    EventPSA[8*TraceNum+6] = 0; /* User 4 */
    EventPSA[8*TraceNum+7] = 0; /* User 5 */

    return (0);   
} 

S32 Pixie_Read_Long_Event_PSA (S8  *filename,			// list mode data file name
			       U32 *EventPSA,			// receive event PSA values
			       U8   ModNum )			// Pixie module number
{

    int status = 0;
    
    /* Check if pointer *ModuleEvents has been initialized */
    if(!EventPSA) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Long_Event_PSA): NULL pointer *EventPSA");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }

    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA)); 

    /* Determine the run type reading only the first buffer */
    LMA->ListModeFileName      = filename;
    LMA->ReadFirstBufferHeader = 1;
    if (PixieListModeReader(LMA) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Long_Event_PSA): problem opening list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    
    /* If run type is not 0x100 or 0x101 then there is no reason to do any processing */
    if ((LMA->BufferHeader[2] & 0x0FFF) > 0x101) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Long_Event_PSA): wrong run type");
	Pixie_Print_MSG(ErrMSG);
	return (-4);
    }

    /* Do the processing */
    LMA->ReadFirstBufferHeader = 0;
    LMA->par0                  = EventPSA;
    LMA->par1                  = &ModNum;
    LMA->ChannelLevelAction    = PixieReadLongEventPSAChannelLevel;
	LMA->AuxChannelLevelAction = PixieReadLongEventPSAAuxChannelLevel;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Long_Event_PSA): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Read_Long_Event_PSA): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }

    return (0);
    
}

/****************************************************************
 *	Pixie_Locate_List_Mode_Events function:
 *		Parse the list mode file and find 
 *                              every event start, 
 *                              corresponding buffer start and 
 *                              number of words in the event
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL ModuleTraces array pointer
 *
 ****************************************************************/

S32 PixieLocateListModeEventsBufferLevel (LMR_t LMA) 
{
    
    /* This is a buffer-level user function for the list mode reader */
    /* Buffer start */
    *(S32*)LMA->par2 = (ftell(LMA->ListModeFile) + 1) / 2 - BUFFER_HEAD_LENGTH;
    
    return (0);
    
} 

S32 PixieLocateListModeEventsEventLevel (LMR_t LMA) 
{
    
    /* This is a event-level user function for the list mode reader */
    U16  ModNum         =        LMA->BufferHeader[1];
    U16 *LastChannel    =  (U16*)LMA->par4;
    U32 *ModuleEvents   =  (U32*)LMA->par0;
    U32 *ShiftFromStart =  (U32*)LMA->par1;
    U32 *EventLen       =  (U32*)LMA->par3;
    S32  BufferPos      = *(S32*)LMA->par2;
    S32  EventPos       =       (ftell(LMA->ListModeFile) + 1) / 2 - EVENT_HEAD_LENGTH;
    
    if (LMA->EventHeader[0] & 0x1) *LastChannel = 0;
    if (LMA->EventHeader[0] & 0x2) *LastChannel = 1;
    if (LMA->EventHeader[0] & 0x4) *LastChannel = 2;
    if (LMA->EventHeader[0] & 0x8) *LastChannel = 3;
    
    *EventLen = EVENT_HEAD_LENGTH; /* Initialize EventLen to event buffer length */
    ModuleEvents[3*(ShiftFromStart[ModNum]+LMA->Events[ModNum])+0] = (U32)EventPos;
    ModuleEvents[3*(ShiftFromStart[ModNum]+LMA->Events[ModNum])+1] = (U32)BufferPos; 
    
    return (0);
    
} 

S32 PixieLocateListModeEventsChannelLevel (LMR_t LMA) 
{
    
    /* This is a channel-level user function for the list mode reader */
    U16  ModNum         =        LMA->BufferHeader[1];
    U16  RunType        =        LMA->BufferHeader[2] & 0x0F0F;
    U16  LastChannel    = *(U16*)LMA->par4;
    U32 *ModuleEvents   =  (U32*)LMA->par0;
    U32 *ShiftFromStart =  (U32*)LMA->par1;
    U32 *EventLen       =  (U32*)LMA->par3;
    
    if (RunType == 0x100) *EventLen += (U32)LMA->ChannelHeader[0]; /* Sum channel lengths to get event length */
    else                  *EventLen += (U32)LMA->ChanHeadLen;
    if (LMA->Channel == LastChannel) {              /* If no more channels to sum report event length */
	ModuleEvents[3*(ShiftFromStart[ModNum]+LMA->Events[ModNum])+2] = *EventLen;
    }
    
    return (0);
    
} 

S32 Pixie_Locate_List_Mode_Events (S8  *filename,		// the list mode data file name (with complete path)
			           U32 *ModuleEvents)	// receives event length and location for each module
{

    U16                LastChannel                        =  0;
    U32                EventLen                           =  0;
    U32                ShiftFromStart[PRESET_MAX_MODULES] = {0};
    U32                TotalShift                         =  0;
    S32                BufferPos                          =  0;
    S32                status                             =  0;
    S32                i                                  =  0;
    
    /* Check if pointer *ModuleEvents has been initialized */
    if(!ModuleEvents) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Events): NULL pointer *EventPSA");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
    
    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA)); 
    
    /* Prepare the array of module-dependent shifts */
    for(i = 1; i < PRESET_MAX_MODULES; i++) ShiftFromStart[i] = (TotalShift += MODULE_EVENTS[i-1]);

    /* Do the processing */
    LMA->ListModeFileName      = filename;
    LMA->par0                  = ModuleEvents;
    LMA->par1                  = ShiftFromStart;
    LMA->par2                  = &BufferPos;
    LMA->par3                  = &EventLen;
    LMA->par4                  = &LastChannel;
    LMA->BufferLevelAction     = PixieLocateListModeEventsBufferLevel;
    LMA->EventLevelAction      = PixieLocateListModeEventsEventLevel;
    LMA->ChannelLevelAction    = PixieLocateListModeEventsChannelLevel;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Events): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Events): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }

    return (0);
    
}


/****************************************************************
 *	Pixie_User_List_Mode_Reader function:
 *		Parse the list mode file and conduct 
 *      any user defined analysis using six action  
 *      functions of the list mode reader. The user is 
 *      responsible to populate the six functions as well as 
 *      the calling function with the code and recompile.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *			-2 - found BufNdata = 0
 *			-3 - NULL UserData array pointer
 *
 ****************************************************************/

S32 PixieUserListModeReaderPreProcess (LMR_t LMA) 
{
	/* This function can be used to conduct any computation before a
	 * list mode file parsing begins. In this example the array UserData 
	 * is initialized to zero */
	 
	U32 *UserData     =  (U32*)LMA->par2;
	U32  UserDataSize = *(U32*)LMA->par3 ;
	memset(UserData, 0, sizeof(U32) * UserDataSize);
    return (0);
} 

S32 PixieUserListModeReaderBufferLevel (LMR_t LMA) 
{
	/* This function can be used to conduct any computation right after
	 * a buffer header has been read into LMA->BufferHeader[]. In this 
	 * example the position of the buffer start is determined and kept 
	 * in the fourth parameter */
	 
	*(S32*)LMA->par4 = (ftell(LMA->ListModeFile) + 1) / 2 - BUFFER_HEAD_LENGTH;
    return (0);
} 

S32 PixieUserListModeReaderEventLevel (LMR_t LMA) 
{
	/* This function can be used to conduct any computation right after
	 * an event header has been read into LMA->EventHeader[]. In this 
	 * example UserVariable is incremented by one if all channels trigger
	 * simultaneously */
	
	U16    HitPattern   = LMA->EventHeader[0];
	double UserVariable = *(double *)LMA->par1;
	if (HitPattern & 0x0F00) UserVariable += 1.0;
    return (0);
} 

S32 PixieUserListModeReaderChannelLevel (LMR_t LMA) 
{
	/* This function can be used to conduct any computation right after
	 * a channel header has been read into LMA->ChannelHeader[]. Only 
	 * channels defined in the read pattern of an event (bits 3-0 of LMA->EventHeader[0])
	 * are considered. If a certain action is required for a channel not in the
     * read pattern	the user must use the function PixieUserListModeReaderAuxChannelLevel().
	 * In this example the first element of the UserData array is populated with 
	 * different channel header values depending on the run type */
	
	U16  RunType  =       LMA->BufferHeader[2] & 0x0FFF;
    U32 *UserData = (U32*)LMA->par2;
    if (RunType == 0x100) UserData[0] = (U32)LMA->ChannelHeader[3]; /* XIA PSA */
    if (RunType == 0x102) UserData[0] = (U32)LMA->ChannelHeader[2]; /* XIA PSA */
    return (0);
} 

S32 PixieUserListModeReaderAuxChannelLevel (LMR_t LMA) 
{
	/* This function can be used to conduct any computation for channels
	 * which are not defined in the read pattern. In this example the first 
	 * element of the UserData array is initiated to zero */
	
	U32 *UserData = (U32*)LMA->par2;
    UserData[0] = 0;
    return (0);
} 

S32 PixieUserListModeReaderPostProcess (LMR_t LMA)
{
	/* This function can be used to conduct any computation after
	 * a list mode file parsing is over. In this example total event 
	 * and trace tallies are printed to the screen */
	 
	S8 MSG[256] = {'\0'};
	sprintf(MSG, "Total Number of Events: %u\n", LMA->TotalEvents);
	Pixie_Print_MSG(ErrMSG);
	sprintf(MSG, "Total Number of Traces: %u\n", LMA->TotalTraces);
	Pixie_Print_MSG(ErrMSG);
    return (0);
} 

S32 Pixie_User_List_Mode_Reader (S8  *filename,		// the list mode data file name (with complete path)
			           U32 *UserData)	// receives event length and location for each module
{
	U32    UserDataSize = *UserData; /* The user must provide the size of the array in the zeroth element */
	S32    status       = 0;
    double UserVariable = 2.718281828459045;
	double UserArray[5] = {3.141592653589793};
	
    /* Check if pointer has been initialized */
    if(!UserData) {
	sprintf(ErrMSG, "*ERROR* (Pixie_User_List_Mode_Reader): NULL pointer *UserData");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
    /* Make sure the size of the user array is not zero */
    if(!UserDataSize) {
	sprintf(ErrMSG, "*ERROR* (Pixie_User_List_Mode_Reader): UserDataSize is zero");
	Pixie_Print_MSG(ErrMSG);
	return(-3);
    }
	
    /* Zero the list mode reader arguments structure */
    memset(LMA, 0, sizeof(*LMA)); 

    /* Do the processing */
    LMA->ListModeFileName      = filename;
    LMA->par0                  = UserArray;
	LMA->par1                  = &UserVariable;
	LMA->par2                  = UserData;
	LMA->par3                  = &UserDataSize;
	LMA->PreAnalysisAction     = PixieUserListModeReaderPreProcess;
    LMA->BufferLevelAction     = PixieUserListModeReaderBufferLevel;
    LMA->EventLevelAction      = PixieUserListModeReaderEventLevel;
    LMA->ChannelLevelAction    = PixieUserListModeReaderChannelLevel;
	LMA->AuxChannelLevelAction = PixieUserListModeReaderAuxChannelLevel;
	LMA->PostAnalysisAction    = PixieUserListModeReaderPostProcess;
    if ((status = PixieListModeReader(LMA)) == -1) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Events): Cannot open list mode file");
	Pixie_Print_MSG(ErrMSG);
	return (-1);
    }
    if (status == -2) {
	sprintf(ErrMSG, "*ERROR* (Pixie_Locate_List_Mode_Events): Corrupted file: module# %hu, buffer# %u", LMA->BufferHeader[1], LMA->Buffers[LMA->BufferHeader[1]]);
	Pixie_Print_MSG(ErrMSG);
	return (-2);
    }

    return (0);   
}








/****************************************************************
 *	Pixie_Read_List_Mode_Events function:
 *		Read specfic event header, channel headers and trace from the list mode file.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - can't open list mode data file
 *
 ****************************************************************/

S32 Pixie_Read_List_Mode_Events (
			S8  *filename,		// the list mode data file name (with complete path)
			U32 *ListModeTraces )	// receives list mode trace data
{						// first three words contain location of event, location of buffer, and length of event
	U16 idx, RunTask, traceindex;
	U16 EvtPattern, ChanNData, chl;
	U32 j,k;
	FILE  *ListModeFile = NULL;
	
	/* Open the list mode file */
	ListModeFile = fopen(filename, "rb");
	
	if(ListModeFile != NULL) {
	    /* Read list mode traces from the file */
	    if((ListModeTraces[0] != 0 ) && (ListModeTraces[2] != 0)) {
        	idx = 7;
                /* Position ListModeFile to the beginning of this buffer */
		fseek(ListModeFile, ListModeTraces[1]*2, SEEK_SET);
		/* Read trace */
		for(j=0; j<BUFFER_HEAD_LENGTH; j++) fread(&ListModeTraces[idx++], 2, 1, ListModeFile);
	        /* Determine Run Task */				
		RunTask = (U16)(ListModeTraces[9] & 0x0FFF);
		/* Determine Channel Header Length */
		chl=9;	//default
		switch(RunTask) {
			case 0x100:
			case 0x101:
			case 0x200:
			case 0x201:
			    chl = 9;
			    break;
			case 0x102:
			case 0x202:
			    chl = 4;
			    break;
			case 0x103:
			case 0x203:
			    chl = 2;
			    break;
			default:
			    break;
		}
        	/* Position ListModeFile to the requested event location */
		fseek(ListModeFile, ListModeTraces[0]*2, SEEK_SET);	
		/* Read event header */
		for(j=0; j<EVENT_HEAD_LENGTH; j++) fread(&ListModeTraces[idx++], 2, 1, ListModeFile);
        	EvtPattern = (U16)ListModeTraces[13];
                /* position trace write pointer to channel j header entry */
        	traceindex = 7+BUFFER_HEAD_LENGTH+EVENT_HEAD_LENGTH+9*NUMBER_OF_CHANNELS;
       		for(j = 0; j < NUMBER_OF_CHANNELS; j ++) {
		    if( TstBit((U16)j, EvtPattern) ) {
                    /*position write pointer to channel j header entry */
                    /* idx = 7+BUFFER_HEAD_LENGTH+EVENT_HEAD_LENGTH+9*j; */
			if( chl == 9 ) {
			    /* Read channel header */
		            for(k=0; k<chl; k++) fread(&ListModeTraces[idx++], 2, 1, ListModeFile);
                    	    ChanNData = (U16)ListModeTraces[idx-chl];
                    	    if ( ChanNData > chl) {
                        	/* store traces */
                        	for(k=0; k<(U16)(ChanNData - chl); k++) fread(&ListModeTraces[traceindex++], 2, 1, ListModeFile);
                        	/* store tracelength */
                        	ListModeTraces[3+j] = ChanNData-chl;
                            }
			}
			else {
                    	    /* for easier processing, store the channel header length */
                    	    ListModeTraces[idx++] = chl;  
	    		    /* Read channel header */
		            for(k=0; k<chl; k++) fread(&ListModeTraces[idx++], 2, 1, ListModeFile);
                    	    /* store tracelength */
                    	    ListModeTraces[3+j] = 0;			
                    	    idx += (9-1-chl);    
			}
		    }
		    else {
			/* XXX bug fixed 9/29/06 */
			idx += 9;
		    }
		}
	    }
	    fclose(ListModeFile);
	}
	else {
	    sprintf(ErrMSG, "*ERROR* (Pixie_Read_List_Mode_Events): can't open list mode file %s", filename);
	    Pixie_Print_MSG(ErrMSG);
	    return(-1);
	}

	return(0);
}



/****************************************************************
 *	Pixie_CopyExtractSettings function:
 *		Copy or extract settings to the specific modules and
 *		channels.
 *
 ****************************************************************/

S32 Pixie_CopyExtractSettings (
			U8 SourceChannel,			// source Pixie channel
			U16 BitMask,				// copy/extract bit mask pattern
			U8 DestinationModule,		// destination module number
			U8 DestinationChannel,		// destination channel number
			U16 *DSPSourceSettings )	// DSP settings of the source channel
{
	S8  str[256];
	U32 idxS, idxD, idx;
	U16 value;

	U16 SYSTEM_CLOCK_MHZ = 75;
	U16 FILTER_CLOCK_MHZ = 75;
	U16	ADC_CLOCK_MHZ = 75;
	U16	ThisADCclkMHz = 75;
	U16 LTscale = 16;

	/* Gain */
	if( TstBit(0, (U16)BitMask) )
	{
		/* Copy SGA */
		sprintf(str,"SGA%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"SGA%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy DIGGAIN */
		sprintf(str,"DIGGAIN%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"DIGGAIN%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* Offset */
	if( TstBit(1, (U16)BitMask) )
	{
		/* Copy TRACKDAC */
		sprintf(str,"TRACKDAC%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"TRACKDAC%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy BASELINE_PERCENT */
		sprintf(str,"BASELINEPERCENT%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"BASELINEPERCENT%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
		
	}

	/* Filter */
	if( TstBit(2, (U16)BitMask) )
	{
		/* Copy FILTERRANGE */
		idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Update energy filter interval */
		Pixie_Define_Clocks ((U8)DestinationModule,(U8)0,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale,&ThisADCclkMHz );
        Filter_Int[DestinationModule]=pow(2.0, (double)Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx])/FILTER_CLOCK_MHZ;
		

		/* Copy SLOWLENGTH */
		sprintf(str,"SLOWLENGTH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"SLOWLENGTH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy SLOWGAP */
		sprintf(str,"SLOWGAP%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"SLOWGAP%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy PEAKSAMPLE */
		sprintf(str,"PEAKSAMPLE%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PEAKSAMPLE%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy PEAKSEP */
		sprintf(str,"PEAKSEP%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PEAKSEP%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy LOG2BWEIGHT */
		sprintf(str,"LOG2BWEIGHT%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"LOG2BWEIGHT%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy BLCUT */
		sprintf(str,"BLCUT%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"BLCUT%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* Trigger */
	if( TstBit(3, (U16)BitMask) )
	{
		/* Copy FASTLENGTH */
		sprintf(str,"FASTLENGTH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"FASTLENGTH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy FASTGAP */
		sprintf(str,"FASTGAP%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"FASTGAP%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy FASTTHRESH */
		sprintf(str,"FASTTHRESH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"FASTTHRESH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy CFDREG */
		sprintf(str,"CFDREG%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"CFDREG%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

	}

	/* FIFO */
	if( TstBit(4, (U16)BitMask) )
	{
		/* Copy PAFLENGTH */
		sprintf(str,"PAFLENGTH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PAFLENGTH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy TRIGGERDELAY */
		sprintf(str,"TRIGGERDELAY%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"TRIGGERDELAY%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy RESETDELAY */
		sprintf(str,"RESETDELAY%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"RESETDELAY%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy TRACELENGTH */
		sprintf(str,"TRACELENGTH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"TRACELENGTH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy XWAIT */
		sprintf(str,"XWAIT%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"XWAIT%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy XAVG */
		sprintf(str,"XAVG%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"XAVG%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy PSAOFFSET */
		sprintf(str,"PSAOFFSET%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PSAOFFSET%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy PSALENGTH */
		sprintf(str,"PSALENGTH%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PSALENGTH%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy CFDTHR */
		sprintf(str,"CFDTHR%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"CFDTHR%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy USERDELAY */
		sprintf(str,"USERDELAY%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"USERDELAY%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* CSR */
	if( TstBit(5, (U16)BitMask) )
	{
		/* Copy CHANCSRA */
		sprintf(str,"CHANCSRA%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"CHANCSRA%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy CHANCSRB */
		sprintf(str,"CHANCSRB%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"CHANCSRB%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];


		/* Copy CHANCSRC */
		sprintf(str,"CHANCSRC%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"CHANCSRC%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* Coincidence Pattern */
	if( TstBit(6, (U16)BitMask) )
	{
		/* Copy COINCPATTERN */
		idx=Find_Xact_Match("COINCPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Copy COINCWAIT */
		idx=Find_Xact_Match("COINCWAIT", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];
	}

	/* MCA */
	if( TstBit(7, (U16)BitMask) )
	{
		/* Copy ENERGYLOW */
		sprintf(str,"ENERGYLOW%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"ENERGYLOW%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy LOG2EBIN */
		sprintf(str,"LOG2EBIN%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"LOG2EBIN%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* TAU */
	if( TstBit(8, (U16)BitMask) )
	{
		/* Copy PREAMPTAUA */
		sprintf(str,"PREAMPTAUA%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PREAMPTAUA%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];

		/* Copy PREAMPTAUB */
		sprintf(str,"PREAMPTAUB%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"PREAMPTAUB%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* INTEGRATOR */
	if( TstBit(9, (U16)BitMask) )
	{
		/* Copy INTEGRATOR */
		sprintf(str,"INTEGRATOR%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"INTEGRATOR%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	/* ModCSR and ModulePattern */
	if( TstBit(10, (U16)BitMask) )
	{
		/* Copy MODCSRA */
		idx=Find_Xact_Match("MODCSRA", DSP_Parameter_Names, N_DSP_PAR);
		value=DSPSourceSettings[idx];
		value = ClrBit(5, value);		// clear the bit for front panel veto input, only one module can be enabled
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=value;
		
		/* Copy MODCSRB */
		idx=Find_Xact_Match("MODCSRB", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Copy MODCSRC */
		idx=Find_Xact_Match("MODCSRC", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Copy DBLBUFCSR */
		idx=Find_Xact_Match("DBLBUFCSR", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Copy ModulePattern */
		idx=Find_Xact_Match("MODULEPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];

		/* Copy NNSHAREPATTERN */
		idx=Find_Xact_Match("NNSHAREPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idx]=DSPSourceSettings[idx];
	}



	/* GATE */
	if( TstBit(11, (U16)BitMask) )

	{
		/* Copy GATEWINDOW */
		sprintf(str,"GATEWINDOW%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"GATEWINDOW%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];


		/* Copy GATEDELAY */
		sprintf(str,"GATEDELAY%d",SourceChannel);
		idxS=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		sprintf(str,"GATEDELAY%d",DestinationChannel);
		idxD=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[DestinationModule].DSP_Parameter_Values[idxD]=DSPSourceSettings[idxS];
	}

	return(0);
}




/****************************************************************
 *	Make_SGA_Gain_Table function:
 *		This routine generates the SGA gain table for Pixie module's
 *		gain setting.
 *
 ****************************************************************/


S32 Make_SGA_Gain_Table(void)
{

	U32 k;
	double RG,RF;
	
	for(k=0;k<N_SGA_GAIN;k++)
	{
		/* RF = 1200 + 120 (if bit 0 not set) + 270 (if bit 1 not set) + 560 (if bit 2 not set) */ 
		RF = 2150 - 120*((k>>0)&1) - 270*((k>>1)&1) - 560*((k>>2)&1);
		/* RG = 100 + 100 (if bit 3 not set) + 100 (if bit 4 not set) + 150 (if bit 5 not set) + 820 (if bit 6 not set) */ 
		RG =  1320 - 0*((k>>3)&1) - 100*((k>>4)&1) - 300*((k>>5)&1) - 820*((k>>6)&1);   
		SGA_Computed_Gain[k]=(1 + RF/RG)/2;
	}
	return(0);
}




/****************************************************************
 *	Pixie_Print_MSG function:
 *		This routine prints error message or other message
 *		either to Igor history window or a text file.
 *
 ****************************************************************/

S32 Pixie_Print_MSG (
			S8 *message )	/* message to be printed */
{
#ifdef COMPILE_IGOR_XOP
	/* Add carriage return character '\r' for XOPNotice */
	strcat(message, "\r");
	XOPNotice(message);
#else
	
	FILE *PIXIEmsg = NULL;
	
	PIXIEmsg = fopen("PIXIEmsg.txt", "a");
	if(PIXIEmsg != NULL)
	{
		/* Add new line character '\n' for printf */
		strcat(message, "\n");
		printf(message);
		
		fprintf(PIXIEmsg, "%s", message);
		fclose(PIXIEmsg);
	}
	
#endif
	
	return(0);
}


/****************************************************************
 *	Pixie_Define_Clocks function:
 *		Define clock constants according to BoardRevision .
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid BoardRevision
 *
 ****************************************************************/


S32 Pixie_Define_Clocks (
		
	U8  ModNum,				// Pixie module number
	U8  ChanNum,			// Pixie channel number
	U16 *SYSTEM_CLOCK_MHZ,	// system clock -- coincidence window, DSP time stamps
	U16 *FILTER_CLOCK_MHZ,	// filter/processing clock -- filter lengths, runs statistics
	U16	*ADC_CLOCK_MHZ,		// sampling clock of the ADC
	U16	*LTscale,			// The scaling factor for live time counters
	U16 *ThisADCclkMHz )		// for P500 Rev A: ch0 gets special treatment
{

	U16	BoardRevision;


	// Define clock constants according to BoardRevision 
	BoardRevision = (U16)Pixie_Devices[ModNum].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)];
	
//	sprintf(ErrMSG, "*INFO* (Pixie_Define_Clocks): BoardRevision (Xilinx) for Module %d is %X", ModNum,BoardRevision);
//	Pixie_Print_MSG(ErrMSG);

	if((BoardRevision & 0x0F00) == 0x0700) 
	{
		*SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ; // For Pixie-4
		*FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
		*ADC_CLOCK_MHZ    = P4_ADC_CLOCK_MHZ;
		*ThisADCclkMHz    = P4_ADC_CLOCK_MHZ;
		*LTscale		  = P4_LTSCALE;
	}
	else if((BoardRevision & 0x0FF0) == 0x0300) 
	{
		*SYSTEM_CLOCK_MHZ = P500_SYSTEM_CLOCK_MHZ; // For P500 
		*FILTER_CLOCK_MHZ = P500_FILTER_CLOCK_MHZ;
		*ADC_CLOCK_MHZ    = P500_ADC_CLOCK_MHZ;
		*LTscale		  = P500_LTSCALE;
		if((ChanNum==0) && ((BoardRevision & 0x000F)==0)  )
			*ThisADCclkMHz = P500_FILTER_CLOCK_MHZ;	// P500 Rev A ch0: 125 MHz
		else
			*ThisADCclkMHz = P500_ADC_CLOCK_MHZ;		// P500 Rev A ch1-3, Rev B all ch: 500 MHz
	}
	else if((BoardRevision & 0x0FF0) == 0x0310) 
	{
		*SYSTEM_CLOCK_MHZ = P400_SYSTEM_CLOCK_MHZ; // For P500 (400 MHz version)
		*FILTER_CLOCK_MHZ = P400_FILTER_CLOCK_MHZ;
		*ADC_CLOCK_MHZ    = P400_ADC_CLOCK_MHZ;
		*LTscale		  = P400_LTSCALE;
		*ThisADCclkMHz	  = P400_ADC_CLOCK_MHZ;	//
	}
	else
	{
		sprintf(ErrMSG, "*Error* (Pixie_Define_Clocks): Unknown BoardRevision, time constants defaulting to Pixie-4");
		Pixie_Print_MSG(ErrMSG);
		*SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ; // For Pixie-4
		*FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
		*ADC_CLOCK_MHZ    = P4_ADC_CLOCK_MHZ;
		*ThisADCclkMHz	  = P4_ADC_CLOCK_MHZ;
		*LTscale		  = P4_LTSCALE;
		return(-1);
	}

//	sprintf(ErrMSG, "*INFO* (Pixie_Define_Clocks): Filter_Clock_MHz %d", *FILTER_CLOCK_MHZ);
//	Pixie_Print_MSG(ErrMSG);


	return(0);
}

