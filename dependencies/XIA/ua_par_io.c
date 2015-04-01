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

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef XIA_WINDOZE
	#include <io.h>
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




/*************************************************************************************************
 ************************************ SUBFUNCTIONS ***********************************************
 *************************************************************************************************/

S32 Compute_PileUp(U8 ModNum, U8 ChanNum, U16 SL, U16 SG, double *User_Par_Values, U16 offset);

S32 ComputeFIFO (U8 ModNum,	// Pixie module number
		 U8 ChanNum );	// Pixie channel number


U16 Pixie_MinCoincWait(U8 ModNum);

U16 Pixie_ComputeMaxEvents(
			U8  ModNum,			// Pixie module number
			U16 RunType );		// data run type

S32 Select_SGA_Gain(double gain);

S32 Pixie_Broadcast (
			S8 *str,				// variable name whose value is to be broadcasted
			U8 SourceModule );		// the source module number

S32 UA_SYSTEM_PAR_IO (	double *User_Par_Values,			// user parameters to be transferred
			S8     *user_variable_name,			// parameter name (string)
			U16    direction);				// Read or Write

S32 UA_MODULE_PAR_IO (	double *User_Par_Values,			// user parameters to be transferred
			S8     *user_variable_name,			// parameter name (string)
			U16    direction,				// Read or Write
			U8     ModNum,					// Pixie module number
			U8     ChanNum );				// Pixie channel number

S32 UA_CHANNEL_PAR_IO (	double *User_Par_Values,			// user parameters to be transferred
			S8     *user_variable_name,			// parameter name (string)
			U16    direction,				// Read or Write
			U8     ModNum,					// Pixie module number
			U8     ChanNum );				// Pixie channel number



/****************************************************************
 *	UA_PAR_IO function:
 *		This is the User Value I/O function.
 *
 *		for direction==put (0)
 *			1)	Compute all new DSP variables
 *			2)	Download the new variables
 *			3)	Apply where needed (Set_DACs, Program_FIPPI,
 *				Adjust_Offsets)
 *
 *		for direction==get (1)
 *			1)	Read all DSP variables from the Module
 *			2)	Calculate all new User Values
 *			3)	Upload all User Values to the Host
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid system parameter name
 *			-2 - invalid module parameter name
 *			-3 - invalid channel parameter name
 *                      -4 - invalid direction
 *                      -5 - invalid variable type
 *
 ****************************************************************/

S32 UA_PAR_IO (	double *User_Par_Values,			// user parameters to be transferred
		S8     *user_variable_name,			// parameter name (string)
		S8     *user_variable_type,			// parameter type (string)
		U16    direction,				// Read or Write
		U8     ModNum,					// Pixie module number
		U8     ChanNum )				// Pixie channel number
{


	U8      SYSTEM      = (U8)(strcmp(user_variable_type,                "SYSTEM")  == 0);
	U8      MODULE      = (U8)(strcmp(user_variable_type,                "MODULE")  == 0);
	U8      CHANNEL     = (U8)(strcmp(user_variable_type,                "CHANNEL") == 0);
	U8      ALLCHANNEL  = (U8)(strcmp(user_variable_name,  "ALL_CHANNEL_PARAMETERS") == 0);
	U8      CHANNELSTAT = (U8)(strcmp(user_variable_name, "CHANNEL_RUN_STATISTICS") == 0);
	S32     retval      = -1;
	
	if (direction > 1)
	{
	    sprintf(ErrMSG, "*ERROR* (UA_PAR_IO): invalid direction %hu", direction);
	    Pixie_Print_MSG(ErrMSG);
	    return(-4);
	}
	
	if (SYSTEM) return UA_SYSTEM_PAR_IO (User_Par_Values, user_variable_name, direction);
	if (MODULE) return UA_MODULE_PAR_IO (User_Par_Values, user_variable_name, direction, ModNum, ChanNum);
	if (CHANNEL && (ALLCHANNEL || CHANNELSTAT)) {
	    ChanNum = 0;
	    while (ChanNum < NUMBER_OF_CHANNELS && (retval = UA_CHANNEL_PAR_IO (User_Par_Values, user_variable_name, direction, ModNum, ChanNum)) == 0) {
		ChanNum++;
	    }
	    return retval;
	}	
	if (CHANNEL) return UA_CHANNEL_PAR_IO (User_Par_Values, user_variable_name, direction, ModNum, ChanNum);
	sprintf(ErrMSG, "*ERROR* (UA_PAR_IO): invalid variable type %s", user_variable_type);
	Pixie_Print_MSG(ErrMSG);
	return(-5);
}


/****************************************************************
 *	UA_SYSTEM_PAR_IO function:
 *		This is the User Value System Parameter I/O function.
 *
 *		for direction==put (0)
 *			1)	Compute all new DSP variables
 *			2)	Download the new variables
 *			3)	Apply where needed (Set_DACs, Program_FIPPI,
 *				Adjust_Offsets)
 *
 *		for direction==get (1)
 *			1)	Read all DSP variables from the Module
 *			2)	Calculate all new User Values
 *			3)	Upload all User Values to the Host
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid system parameter name
 *			-2 - invalid module parameter name
 *			-3 - invalid channel parameter name
 *                      -4 - invalid direction
 *
 ****************************************************************/

S32 UA_SYSTEM_PAR_IO (	double *User_Par_Values,	// user parameters to be transferred
			S8     *user_variable_name,	// parameter name (string)
			U16    direction)		// Read or Write
{
	U8  ALLREAD = (strcmp(user_variable_name, "ALL_SYSTEM_PARAMETERS") == 0);
	U8 	READ    = (direction == 1);
	U8	WRITE   = (direction == 0);
	U16	idx   = 65535;
	U16 	k;

	
	/*************************************************************************************************
	 ************************************ SYSTEM PARAMETERS ******************************************
	 *************************************************************************************************/
	
	if(strcmp(user_variable_name,"NUMBER_MODULES") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("NUMBER_MODULES", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) Number_Modules         = (U8)    (System_Parameter_Values[idx] = (U16)User_Par_Values[idx]);
	    if (READ)  User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)Number_Modules);
	}
	
	if(strcmp(user_variable_name,"OFFLINE_ANALYSIS") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("OFFLINE_ANALYSIS", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) Offline                = (U8)    (System_Parameter_Values[idx] = (U16)User_Par_Values[idx]);
	    if (READ)  User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)Offline);
	}
	
	if(strcmp(user_variable_name,"AUTO_PROCESSLMDATA") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("AUTO_PROCESSLMDATA", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) AutoProcessLMData      = (U8)    (System_Parameter_Values[idx] = (U16)User_Par_Values[idx]);
	    if (READ)  User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)AutoProcessLMData);
	}
	
	if(strcmp(user_variable_name,"MAX_NUMBER_MODULES") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("MAX_NUMBER_MODULES", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) Max_Number_of_Modules  = (U8)    (System_Parameter_Values[idx] = (U16)User_Par_Values[idx]);
	    if (READ)  User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)Max_Number_of_Modules);
	}
	
	if(strcmp(user_variable_name,"SLOT_WAVE") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("SLOT_WAVE", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) for(k=0; k<Number_Modules; k++) Phy_Slot_Wave[k]         = (U8)    (System_Parameter_Values[idx+k] = (U16)User_Par_Values[idx+k]);
	    if (READ)  for(k=0; k<Number_Modules; k++) User_Par_Values[idx+k] = (double)(System_Parameter_Values[idx+k] = (U16)Phy_Slot_Wave[k]);
	}

	if(strcmp(user_variable_name,"C_LIBRARY_RELEASE") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("C_LIBRARY_RELEASE", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE)  return (-4); // Wrong direction
	    if (READ) User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)C_LIBRARY_RELEASE);
	}
	
	if(strcmp(user_variable_name,"C_LIBRARY_BUILD") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("C_LIBRARY_BUILD", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE)  return (-4); // Wrong direction
	    if (READ) User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)C_LIBRARY_BUILD);
	}

	if(strcmp(user_variable_name,"KEEP_CW") == 0 || ALLREAD)
	{
	    idx = Find_Xact_Match("KEEP_CW", System_Parameter_Names, N_SYSTEM_PAR);
	    if (WRITE) KeepCW  = (U8)    (System_Parameter_Values[idx] = (U16)User_Par_Values[idx]);
	    if (READ) User_Par_Values[idx] = (double)(System_Parameter_Values[idx] = (U16)KeepCW);
	}
	
	// Do not put new system variables beyond this line
	
	if(idx == 65535) // Keep it at the end of list
	{
	    sprintf(ErrMSG, "*ERROR* (UA_PAR_IO): invalid system parameter name %s", user_variable_name);
	    Pixie_Print_MSG(ErrMSG);
	    return(-1);
	}
	else {
//	    sprintf(ErrMSG, "*INFO* (UA_PAR_IO): Successfully transferred system parameter %s", user_variable_name);
//	    Pixie_Print_MSG(ErrMSG);
	    return (0);
	}
}	

/****************************************************************
 *	UA_MODULE_PAR_IO function:
 *		This is the User Value Module Parameter I/O function.
 *
 *		for direction==put (0)
 *			1)	Compute all new DSP variables
 *			2)	Download the new variables
 *			3)	Apply where needed (Set_DACs, Program_FIPPI,
 *				Adjust_Offsets)
 *
 *		for direction==get (1)
 *			1)	Read all DSP variables from the Module
 *			2)	Calculate all new User Values
 *			3)	Upload all User Values to the Host
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid system parameter name                                            
 *			-2 - invalid module parameter name
 *			-3 - invalid channel parameter name
 *                      -4 - invalid direction
 *
 ****************************************************************/

S32 UA_MODULE_PAR_IO (	double *User_Par_Values,	// user parameters to be transferred
			S8     *user_variable_name,	// parameter name (string)
			U16    direction,		// Read or Write
			U8     ModNum,			// Pixie module number
			U8     ChanNum )		// Pixie channel number
{
	U8	str[256];
	U8	ALLREAD = (U8)(strcmp(user_variable_name, "ALL_MODULE_PARAMETERS") == 0);
	U8	ALLSTAT = (U8)(strcmp(user_variable_name, "MODULE_RUN_STATISTICS") == 0);	
	U8	READ    = (U8)(direction == 1);
	U8	WRITE   = (U8)(direction == 0);
	U16	idx = 65535, idx2, idx3, idx4;
	U16	k;
	U16	RunType;
	U16	newMaxEvents;
	U16	LimitMaxEvents;
	U16	MaxEvents;
	U16	rta, rtb, rtc;
	U16	SL, SG;
	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize ot Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters

	U16	offset = ModNum * N_MODULE_PAR;
	U32	value, value2;
	U32	dsp_par[N_DSP_PAR];
	double	RunTime, TotalTime; 
	double	NumEvents;
	double  Rate;

	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale,&ThisADCclkMHz );

	
	
	/* if READ Read out all DSP parameters from the current Pixie module */
	if (READ) Pixie_IODM(ModNum, (U16)DATA_MEMORY_ADDRESS, MOD_READ, N_DSP_PAR, dsp_par);
	/* If running in Offline mode, we should copy DSP parameter values to dsp_par. */
	/* Otherwise, dsp_par will contain some random numbers.                        */
	if (READ && Offline == 1) for(k = 0; k < N_DSP_PAR; k++) dsp_par[k]=(U32)Pixie_Devices[ModNum].DSP_Parameter_Values[k];	
	
	/***************************************************************************************************
	 *********************************** MODULE PARAMETERS *********************************************
	 ***************************************************************************************************/	
	
	if(strcmp(user_variable_name,"MODULE_NUMBER") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/*----- Get MODULE_NUMBER -----*/
		idx=Find_Xact_Match("MODNUM", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULE_NUMBER", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"MODULE_CSRA") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx2 = Find_Xact_Match("MODULE_CSRA", Module_Parameter_Names, N_MODULE_PAR);
		idx  = Find_Xact_Match("MODCSRA",     DSP_Parameter_Names,    N_DSP_PAR);
        	/* Update the DSP parameter MODCSRA */
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = (U16)(Pixie_Devices[ModNum].Module_Parameter_Values[idx2] = User_Par_Values[idx2+offset]);
		/* bit 13 (signal to STAR) may not be set in slot 2 */
		idx3 = Find_Xact_Match("SLOT_WAVE", System_Parameter_Names, N_SYSTEM_PAR);
		if(System_Parameter_Values[idx3+ModNum] == 2) Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = ClrBit(13 ,(U16)(Pixie_Devices[ModNum].Module_Parameter_Values[idx2] = (double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx]));
		/* some settings affect all modules: */
		idx3 = Find_Xact_Match("DBLBUFCSR", Module_Parameter_Names, N_MODULE_PAR); // Igor index
		idx4 = Find_Xact_Match("DBLBUFCSR", DSP_Parameter_Names,    N_DSP_PAR);    // DSP index
		for(k = 0; k < Number_Modules; k++)
		{
		    /* Veto_Control code: bit 5 of ModCSRA controls external veto signal        */
		    /* If bit 5 is 1, veto output to the backplane is enabled; if it is 0,      */
		    /* veto output to the backplane is disabled. Only one module can be enabled.*/
		    /* clear all others _before_ setting in selected module						*/
		    if (TstBit(5, Pixie_Devices[ModNum].DSP_Parameter_Values[idx]) == 1 && k != ModNum) Pixie_Devices[k].DSP_Parameter_Values[idx] = ClrBit(5 ,Pixie_Devices[k].DSP_Parameter_Values[idx]);
		    /* Extended List mode runs: bit 1 of ModCSRA controls if 32 spills are moved to EM */
		    /* all modules must be the same */
		    /* if 32 buffers/spill is requested, clear double buffer request	*/
		    /* However, if bit 1 is zero, we don't know if to clear double buffer request, so user interface has to clear it */
		    if(TstBit(1, Pixie_Devices[ModNum].DSP_Parameter_Values[idx]) == 1)
		    {
		        Pixie_Devices[k].DSP_Parameter_Values[idx4] = ClrBit(0 ,Pixie_Devices[k].DSP_Parameter_Values[idx4]); // clear DblCSR
		        Pixie_Devices[k].DSP_Parameter_Values[idx]  = SetBit(1 ,Pixie_Devices[k].DSP_Parameter_Values[idx]);   // set bit 1 (in all modules for convenience)
		    }
		    else Pixie_Devices[k].DSP_Parameter_Values[idx] = ClrBit(1 ,Pixie_Devices[k].DSP_Parameter_Values[idx]);  // clear bit 1 (in all modules for convenience)
		    /* Update the Igor variables  */
		    value  = (U32)(User_Par_Values[idx2+offset] = Pixie_Devices[k].Module_Parameter_Values[idx2] = (double)Pixie_Devices[k].DSP_Parameter_Values[idx]);
		    value2 = (U32)(User_Par_Values[idx3+offset] = Pixie_Devices[k].Module_Parameter_Values[idx3] = (double)Pixie_Devices[k].DSP_Parameter_Values[idx4]);
		    /* Download to the  Pixie modules */
		    Pixie_IODM((U8)k, (U16)(DATA_MEMORY_ADDRESS+idx),  MOD_WRITE, 1, &value);
		    Pixie_IODM((U8)k, (U16)(DATA_MEMORY_ADDRESS+idx4), MOD_WRITE, 1, &value2);
		    /* Program FiPPI also applies ModCSRA settings to System FPGA */
		    Control_Task_Run((U8)k, PROGRAM_FIPPI, 1000);
		}
	    }
	    if (READ) {
		/* Get the DSP parameter MODCSRA */
		idx=Find_Xact_Match("MODCSRA", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULE_CSRA", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
				
	if(strcmp(user_variable_name,"MODULE_CSRB") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("MODULE_CSRB", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter MODCSRB */
		idx=Find_Xact_Match("MODCSRB", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		/* Get the DSP parameter MODCSRB */
		idx=Find_Xact_Match("MODCSRB", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULE_CSRB", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	    
	if(strcmp(user_variable_name,"MODULE_CSRC") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("MODULE_CSRC", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter MODCSRC */
		idx=Find_Xact_Match("MODCSRC", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		/* Get the DSP parameter MODCSRC */
		idx=Find_Xact_Match("MODCSRC", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULE_CSRC", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"MODULE_FORMAT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("MODULE_FORMAT", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter MODFORMAT */
		idx=Find_Xact_Match("MODFORMAT", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		/* Get the DSP parameter MODFORMAT */
		idx=Find_Xact_Match("MODFORMAT", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULE_FORMAT", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"RUN_TYPE") == 0)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("RUN_TYPE", Module_Parameter_Names, N_MODULE_PAR);
		RunType=(U16)User_Par_Values[idx+offset];
		/* If unknown run type force to MCA run */
		if (((RunType & 0x000F) > 3) || ((RunType & 0x0F00) >> 8) > 3) RunType = 0x301;
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Recompute MaxEvents */
		newMaxEvents=Pixie_ComputeMaxEvents(ModNum, RunType);
		/* Update module parameter MaxEvents */
		idx=Find_Xact_Match("MAX_EVENTS", Module_Parameter_Names, N_MODULE_PAR);
		User_Par_Values[idx+offset]=(double)newMaxEvents;
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)newMaxEvents;
		/* Update the DSP parameter MAXEVENTS */
		idx=Find_Xact_Match("MAXEVENTS", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=newMaxEvents;
		/* Download to the selected Pixie module */
		value=(U32)newMaxEvents;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) return (-4); // Wrong direction for this variable
	}
	
	if(strcmp(user_variable_name,"MAX_EVENTS") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get the Run Type */
		idx=Find_Xact_Match("RUN_TYPE", Module_Parameter_Names, N_MODULE_PAR);
		RunType = (U16)Pixie_Devices[ModNum].Module_Parameter_Values[idx];
		/* Recompute MaxEvents limit */
		LimitMaxEvents=Pixie_ComputeMaxEvents(ModNum, RunType);
		idx=Find_Xact_Match("MAX_EVENTS", Module_Parameter_Names, N_MODULE_PAR);
		if (User_Par_Values[idx+offset] <                      0) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > (double)LimitMaxEvents) User_Par_Values[idx+offset] = (double)LimitMaxEvents;
		MaxEvents=(U16)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter MAXEVENTS */
		idx=Find_Xact_Match("MAXEVENTS", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=MaxEvents;
		/* Download to the selected Pixie module */
		value=(U32)MaxEvents;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		/* Get the DSP parameter MAXEVENTS */
		idx=Find_Xact_Match("MAXEVENTS", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MAX_EVENTS", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"COINCIDENCE_PATTERN") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("COINCIDENCE_PATTERN", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter COINCPATTERN */
		idx=Find_Xact_Match("COINCPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes Coinc Pattern to System FPGA */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		/* Get the DSP parameter COINCPATTERN */
		idx=Find_Xact_Match("COINCPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("COINCIDENCE_PATTERN", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"MIN_COINCIDENCE_WAIT") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Calculate minimum coincidence wait */
    		value = Pixie_MinCoincWait(ModNum);
		idx=Find_Xact_Match("MIN_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value*1000.0/SYSTEM_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);
	    }
	}

	if(strcmp(user_variable_name,"ACTUAL_COINCIDENCE_WAIT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		//	idx=Find_Xact_Match("MIN_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		//	rta=(U16)(RoundOff(User_Par_Values[idx+offset]*SYSTEM_CLOCK_MHZ/1000.0));
		//	Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
			idx=Find_Xact_Match("ACTUAL_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
			rtb=(U16)(RoundOff(User_Par_Values[idx+offset]*SYSTEM_CLOCK_MHZ/1000.0));
			Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];

		/*	// Adjust ACW according to KeepCW option
			if(KeepCW==0) {	
				// if ACW < MCW, force ACW to be MCW; otherwise keep unchanged (assume user added on purpose) 
				if(rtb < rta)   rtb = rta;	
				if(rtb > 65531) rtb = 65531;	// enforce upper limit
			}
			if(KeepCW==1) {	
				// always force ACW to be MCW; (assume any increase is purely due to MinCoincWait computation and if MCW decreases, ACW should follow) 
				rtb = rta;
				if(rtb > 65531) rtb = 65531;	// enforce upper limit
			}
			if(KeepCW==2) {	
				// ACW disregards MCW; (assume channels are independent) 
				if(rtb < 1) rtb = 1;			// enforce lower limit 	
				if(rtb > 65531) rtb = 65531;	// enforce upper limit 				
			}
			*/
			if(rtb < 1) rtb = 1;			// enforce lower limit 	
			if(rtb > 65531) rtb = 65531;	// enforce upper limit 	

			/* Update ACTUAL_COINCIDENCE_WAIT */
			Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)rtb*1000.0/SYSTEM_CLOCK_MHZ;
			User_Par_Values[idx+offset]=(double)rtb*1000.0/SYSTEM_CLOCK_MHZ;
			/* Update the DSP parameter COINCWAIT */
			idx=Find_Xact_Match("COINCWAIT", DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=rtb;
			/* Download to the selected Pixie module */
			value=(U32)rtb;
			Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
			/* Program FiPPI also writes Coincwait to System FPGA */
			Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
			/* Get the DSP parameter COINCWAIT */
			idx=Find_Xact_Match("COINCWAIT", DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
			value=dsp_par[idx];
			idx=Find_Xact_Match("ACTUAL_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
			Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);
			User_Par_Values[idx+offset]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);
	    }
	}

	if(strcmp(user_variable_name,"SYNCH_WAIT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Update SYNCH_WAIT */
		idx=Find_Xact_Match("SYNCH_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		if (User_Par_Values[idx+offset] < 0) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 1) User_Par_Values[idx+offset] = 1;
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* When changing Synch_Wait in one Pixie module, we need to broadcast it to all other modules as well */
		Pixie_Broadcast("SYNCH_WAIT", ModNum);
	    }
	    if (READ) {
		/* Get the DSP parameter SYNCHWAIT */
		idx=Find_Xact_Match("SYNCHWAIT", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("SYNCH_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"IN_SYNCH") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Update IN_SYNCH */
		idx=Find_Xact_Match("IN_SYNCH", Module_Parameter_Names, N_MODULE_PAR);
		if (User_Par_Values[idx+offset] < 0) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 1) User_Par_Values[idx+offset] = 1;
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* When changing In_Synch in one Pixie module, we need to broadcast it to all other modules as well */
		Pixie_Broadcast("IN_SYNCH", ModNum);
	    }
	    if (READ) {
		/* Get the DSP parameter INSYNCH */
		idx=Find_Xact_Match("INSYNCH", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("IN_SYNCH", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"FILTER_RANGE") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Update FilterRange */
		idx=Find_Xact_Match("FILTER_RANGE", Module_Parameter_Names, N_MODULE_PAR);
		if (User_Par_Values[idx+offset] < 1) User_Par_Values[idx+offset] = 1;
		if (User_Par_Values[idx+offset] > 6) User_Par_Values[idx+offset] = 6;
		value = (U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update energy filter interval */
		Filter_Int[ModNum]=pow(2.0, (double)value)/FILTER_CLOCK_MHZ;
		/* Update the DSP parameter FILTERRANGE */
		idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		for (k = 0; k < NUMBER_OF_CHANNELS; k++) {
    		    sprintf(str, "SLOWLENGTH%d", k);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    SL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		    sprintf(str,"SLOWGAP%d", k);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    SG=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		    Compute_PileUp(ModNum, (U8)k, SL, SG, User_Par_Values, offset);
		}
	    }
	    if (READ) {
		/* Get the DSP parameter FILTERRANGE */
		idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("FILTER_RANGE", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"MODULEPATTERN") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("MODULEPATTERN", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter MODULEPATTERN */
		idx=Find_Xact_Match("MODULEPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		/* Get the DSP parameter MODULEPATTERN */
		idx=Find_Xact_Match("MODULEPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("MODULEPATTERN", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
        }
	
	if(strcmp(user_variable_name,"NNSHAREPATTERN") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("NNSHAREPATTERN", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter NNSHAREPATTERN */
		idx=Find_Xact_Match("NNSHAREPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes NNshare Pattern to System FPGA -> PDM */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		/* Get the DSP parameter NNSHAREPATTERN */
		idx=Find_Xact_Match("NNSHAREPATTERN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("NNSHAREPATTERN", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"DBLBUFCSR") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx3=Find_Xact_Match("DBLBUFCSR", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx3+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx3]=User_Par_Values[idx3+offset];
		/* Update the DSP parameter DBLBUFCSR */
		idx4=Find_Xact_Match("DBLBUFCSR", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx4]=(U16)value;
		/* some settings affect all modules: */
		idx2=Find_Xact_Match("MODULE_CSRA", Module_Parameter_Names, N_MODULE_PAR);
		idx=Find_Xact_Match("MODCSRA", DSP_Parameter_Names, N_DSP_PAR);
		for(k=0; k<Number_Modules; k++)
		{
		    /* bit 0 of DBLBUFCSR enables Double buffer List mode runs */
		    /* all modules must be the same                                     */
		    /* if double buffer is requested, clear 32 buffers/spill request 	*/
		    if(TstBit(0 ,Pixie_Devices[ModNum].DSP_Parameter_Values[idx4]) == 1)
		    {
			 Pixie_Devices[k].DSP_Parameter_Values[idx4] = SetBit(0 ,Pixie_Devices[k].DSP_Parameter_Values[idx4]);
			 Pixie_Devices[k].DSP_Parameter_Values[idx]  = ClrBit(1 ,Pixie_Devices[k].DSP_Parameter_Values[idx]);
		    }
		    else Pixie_Devices[k].DSP_Parameter_Values[idx4] = ClrBit(0 ,Pixie_Devices[k].DSP_Parameter_Values[idx4]);
		    /* Update the Igor variables  */
		    value2 = (U32)Pixie_Devices[k].DSP_Parameter_Values[idx];	
		    Pixie_Devices[k].Module_Parameter_Values[idx2]=(double)value2;
		    User_Par_Values[idx2+offset]=(double)value2;
		    /* Download to the  Pixie modules */
		    Pixie_IODM((U8)k, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value2);
		    /* Update the Igor variables  */
		    value2 = (U32)Pixie_Devices[k].DSP_Parameter_Values[idx4];	
		    Pixie_Devices[k].Module_Parameter_Values[idx3]=(double)value2;
		    User_Par_Values[idx3+offset]=(double)value2;
		    /* Download to the  Pixie modules */
		    Pixie_IODM((U8)k, (U16)(DATA_MEMORY_ADDRESS+idx4), MOD_WRITE, 1, &value2);
		    /* Program FiPPI also applies ModCSRA settings to System FPGA */
		    Control_Task_Run((U8)k, PROGRAM_FIPPI, 1000);		
		}
	    }
	    if (READ) {
		/* Get the DSP parameter DBLBUFCSR */
		idx=Find_Xact_Match("DBLBUFCSR", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("DBLBUFCSR", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"XET_DELAY") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("XET_DELAY", Module_Parameter_Names, N_MODULE_PAR);
	    rta=(U16)(RoundOff(User_Par_Values[idx+offset]*SYSTEM_CLOCK_MHZ/1000.0));
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update XET_DELAY */
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)rta*1000.0/SYSTEM_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)rta*1000.0/SYSTEM_CLOCK_MHZ;

		/* Update the DSP parameter XETDELAY */
		idx=Find_Xact_Match("XETDELAY", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=rta;
		/* Download to the selected Pixie module */
		value=(U32)rta;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes XETDELAY to System FPGA */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		/* Get the DSP parameter XETDELAY */
		idx=Find_Xact_Match("XETDELAY", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("XET_DELAY", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);
		User_Par_Values[idx+offset]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);
	    }
	}



	if(strcmp(user_variable_name,"PDM_MASKA") == 0 || ALLREAD)

	{

	    if (WRITE) {
		idx=Find_Xact_Match("PDM_MASKA", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter PDMMASKA */
		idx=Find_Xact_Match("PDMMASKA", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes PDM MASK to System FPGA -> PDM */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }

	    if (READ) {
		/* Get the DSP parameter PDMMASKA */
		idx=Find_Xact_Match("PDMMASKA", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("PDM_MASKA", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}



	if(strcmp(user_variable_name,"PDM_MASKB") == 0 || ALLREAD)

	{
	    if (WRITE) {
		idx=Find_Xact_Match("PDM_MASKB", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter PDMMASKB */
		idx=Find_Xact_Match("PDMMASKB", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes PDM MASK to System FPGA -> PDM */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }

	    if (READ) {
		/* Get the DSP parameter PDMMASKB */
		idx=Find_Xact_Match("PDMMASKB", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("PDM_MASKB", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}



	if(strcmp(user_variable_name,"PDM_MASKC") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("PDM_MASKC", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter PDMMASKC */
		idx=Find_Xact_Match("PDMMASKC", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value;
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI also writes PDM MASK to System FPGA -> PDM */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }

	    if (READ) {
		/* Get the DSP parameter PDMMASKC */
		idx=Find_Xact_Match("PDMMASKC", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("PDM_MASKC", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
        								
	if(strcmp(user_variable_name,"OUTPUT_BUFFER_LENGTH") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter LOUTBUFFER */
		idx=Find_Xact_Match("LOUTBUFFER", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("OUTPUT_BUFFER_LENGTH", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"BUFFER_HEAD_LENGTH") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter BUFHEADLEN */
		idx=Find_Xact_Match("BUFHEADLEN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("BUFFER_HEAD_LENGTH", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"EVENT_HEAD_LENGTH") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter EVENTHEADLEN */
		idx=Find_Xact_Match("EVENTHEADLEN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("EVENT_HEAD_LENGTH", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"CHANNEL_HEAD_LENGTH") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter CHANHEADLEN */
		idx=Find_Xact_Match("CHANHEADLEN", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("CHANNEL_HEAD_LENGTH", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"NUMBER_EVENTS") == 0 || ALLSTAT || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Number of events */
		Pixie_Devices[ModNum].DSP_Parameter_Values[NUMEVENTS_Index]=(U16)dsp_par[NUMEVENTS_Index];
		rta=(U16)dsp_par[NUMEVENTS_Index];
		Pixie_Devices[ModNum].DSP_Parameter_Values[NUMEVENTS_Index+1]=(U16)dsp_par[NUMEVENTS_Index+1];
		rtb=(U16)dsp_par[NUMEVENTS_Index+1];
		NumEvents=(double)(rta*65536.0+rtb);
		idx=Find_Xact_Match("NUMBER_EVENTS", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=NumEvents;
		User_Par_Values[idx+offset]=NumEvents;
	    }
	}
	
	if(strcmp(user_variable_name,"RUN_TIME") == 0 || ALLSTAT || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Run time */
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index]=(U16)dsp_par[RUNTIME_Index];
		rta=(U16)dsp_par[RUNTIME_Index];
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index+1]=(U16)dsp_par[RUNTIME_Index+1];
		rtb=(U16)dsp_par[RUNTIME_Index+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index+2]=(U16)dsp_par[RUNTIME_Index+2];
		rtc=(U16)dsp_par[RUNTIME_Index+2];
		RunTime=(rta*pow(65536.0,2.0)+rtb*65536.0+rtc)*1.0e-6/SYSTEM_CLOCK_MHZ;
		idx=Find_Xact_Match("RUN_TIME", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=RunTime;
		User_Par_Values[idx+offset]=RunTime;
	    }
	}
	
	if(strcmp(user_variable_name,"TOTAL_TIME") == 0 || ALLSTAT || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Run time */
		idx=Find_Xact_Match("TOTALTIMEA", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rta=(U16)dsp_par[idx];
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		rtb=(U16)dsp_par[idx+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx+2]=(U16)dsp_par[idx+2];
		rtc=(U16)dsp_par[idx+2];
		TotalTime=(rta*pow(65536.0,2.0)+rtb*65536.0+rtc)*1.0e-6/SYSTEM_CLOCK_MHZ;
		idx=Find_Xact_Match("TOTAL_TIME", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=TotalTime;
		User_Par_Values[idx+offset]=TotalTime;
	    }
	}
	
	if(strcmp(user_variable_name,"BOARD_VERSION") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		idx=Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Module_Parameter_Values[idx];
	    }
        }
	
	if(strcmp(user_variable_name,"SERIAL_NUMBER") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		idx=Find_Xact_Match("SERIAL_NUMBER", Module_Parameter_Names, N_MODULE_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Module_Parameter_Values[idx];
	    }
	}	

	if(strcmp(user_variable_name,"EVENT_RATE") == 0 || ALLSTAT || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Number of events */
		Pixie_Devices[ModNum].DSP_Parameter_Values[NUMEVENTS_Index]=(U16)dsp_par[NUMEVENTS_Index];
		rta=(U16)dsp_par[NUMEVENTS_Index];
		Pixie_Devices[ModNum].DSP_Parameter_Values[NUMEVENTS_Index+1]=(U16)dsp_par[NUMEVENTS_Index+1];
		rtb=(U16)dsp_par[NUMEVENTS_Index+1];
		NumEvents=(double)(rta*65536.0+rtb);
		/* Run time */
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index]=(U16)dsp_par[RUNTIME_Index];
		rta=(U16)dsp_par[RUNTIME_Index];
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index+1]=(U16)dsp_par[RUNTIME_Index+1];
		rtb=(U16)dsp_par[RUNTIME_Index+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[RUNTIME_Index+2]=(U16)dsp_par[RUNTIME_Index+2];
		rtc=(U16)dsp_par[RUNTIME_Index+2];
		RunTime=(rta*pow(65536.0,2.0)+rtb*65536.0+rtc)*1.0e-6/SYSTEM_CLOCK_MHZ;
		/* Update Event Rate */
		idx=Find_Xact_Match("EVENT_RATE", Module_Parameter_Names, N_MODULE_PAR);
		if (RunTime == 0) Rate = 0.0;
		else              Rate = NumEvents / RunTime;
		Pixie_Devices[ModNum].Module_Parameter_Values[idx] = User_Par_Values[idx+offset] = Rate;
	    }
	}

	if(strcmp(user_variable_name,"DSP_RELEASE") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter DSPRELEASE */
		idx=Find_Xact_Match("DSPRELEASE", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("DSP_RELEASE", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"DSP_BUILD") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter DSPBUILD */
		idx=Find_Xact_Match("DSPBUILD", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("DSP_BUILD", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"FIPPI_ID") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter FIPPIID */
		idx=Find_Xact_Match("FIPPIID", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("FIPPI_ID", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"SYSTEM_ID") == 0 || ALLREAD)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get the DSP parameter HARDWAREID */
		idx=Find_Xact_Match("HARDWAREID", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("SYSTEM_ID", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}	
	
	// Do not put new module variables beyond this line
	
	if(idx == 65535) // Keep it at the end of list	
	{
	    sprintf(ErrMSG, "*ERROR* (UA_PAR_IO): invalid module parameter name %s", user_variable_name);
	    Pixie_Print_MSG(ErrMSG);
	    return(-2);
	}
	else
	{
//	    sprintf(ErrMSG, "*INFO* (UA_PAR_IO): Successfully transferred module parameter %s", user_variable_name);
//	    Pixie_Print_MSG(ErrMSG);
	    return(0);
	}
}

/****************************************************************
 *	UA_CHANNEL_PAR_IO function:
 *		This is the User Value Channel Parameter I/O function.
 *
 *		for direction==put (0)
 *			1)	Compute all new DSP variables
 *			2)	Download the new variables
 *			3)	Apply where needed (Set_DACs, Program_FIPPI,
 *				Adjust_Offsets)
 *
 *		for direction==get (1)
 *			1)	Read all DSP variables from the Module
 *			2)	Calculate all new User Values
 *			3)	Upload all User Values to the Host
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid system parameter name
 *			-2 - invalid module parameter name
 *			-3 - invalid channel parameter name
 *                      -4 - invalid direction
 *
 ****************************************************************/

S32 UA_CHANNEL_PAR_IO (	double *User_Par_Values,	// user parameters to be transferred
			S8     *user_variable_name,	// parameter name (string)
			U16    direction,		// Read or Write
			U8     ModNum,			// Pixie module number
			U8     ChanNum )		// Pixie channel number
{
	U8	str[256];
	U8  ALLREAD = (strcmp(user_variable_name, "ALL_CHANNEL_PARAMETERS") == 0);
	U8  ALLSTAT = (strcmp(user_variable_name, "CHANNEL_RUN_STATISTICS") == 0);
	U8	READ    = (direction == 1);
	U8	WRITE   = (direction == 0);
	U16	idx   = 65535, idx1, idx2;
	U16	k,i; 
	U16	Xavg;
	U16	SGA;
	U16	FL, FG, SL, SG;
	U16	rta, rtb, rtc;
	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize ot Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters
	U16	DigGainInt;
	U16	TRACKDAC;
	U16	offset = ModNum*N_CHANNEL_PAR*NUMBER_OF_CHANNELS + ChanNum*N_CHANNEL_PAR;
	U16 TraceLength, FifoLength;
	U16	PSAStart, PSAEnd, BoardVersion;
	U32 buffer[USER_MEMORY_LENGTH];
	U32	value, value2, cwd, dsp_par[N_DSP_PAR];
	double	BLcut, DigGain, Vgain, Voffset, tau, TriggerRiseTime;
	double	TriggerFlatTop, EnergyRiseTime, EnergyFlatTop, TriggerThreshold;
	double	FastThresh, xdt, intdt, xwait, rate, temp, SLSG;
	double	baselinepercent, CFDthresh, EnergyLow, Log2EBin, LiveTime, FastPeaks, FTDT;
	double	ChanCSRA, ChanCSRB, ChanCSRC, Integrator, lastxdt, Dbl_value;
	double  GateWindow, GateDelay, BaselineCut, BaselineAVG, SGA_gain, CwDelay;
	double Efilter;
	double  result;

	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );
	
//	sprintf(ErrMSG, "*INFO* (UA_CHANNEL_PAR_IO): Filter_Clock_MHz %d", FILTER_CLOCK_MHZ);
//	Pixie_Print_MSG(ErrMSG);

	/* if READ Read out all DSP parameters from the current Pixie module */
	if (READ) Pixie_IODM(ModNum, (U16)DATA_MEMORY_ADDRESS, MOD_READ, N_DSP_PAR, dsp_par);
	/* If running in Offline mode, we should copy DSP parameter values to dsp_par. */
	/* Otherwise, dsp_par will contain some random numbers.                        */
	if (READ && Offline == 1) for(k = 0; k < N_DSP_PAR; k++) dsp_par[k]=(U32)Pixie_Devices[ModNum].DSP_Parameter_Values[k];	
	
	/***************************************************************************************************
	 *********************************** CHANNEL PARAMETERS ********************************************
	 ***************************************************************************************************/	


	if(strcmp(user_variable_name,"TRIGGER_RISETIME") == 0 || ALLREAD)
	{
	    if (WRITE) { 		
		/* Calculate fast length */
		idx=Find_Xact_Match("TRIGGER_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		TriggerRiseTime=User_Par_Values[idx+offset];
		FL=(U16)RoundOff(TriggerRiseTime*FILTER_CLOCK_MHZ);
		/* Check fast length limit */
		sprintf(str,"FASTGAP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		FG=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		if ( FL       <  2) FL = 2;
		if ((FL + FG) > 63) 
			if (FG<=61)   FL = 63 - FG;
			else {
				FL = 4;	// if both values are off, set to default
				FG = 4;
			}
		TriggerRiseTime=FL/FILTER_CLOCK_MHZ;
		/* Update trigger rise time */
		idx=Find_Xact_Match("TRIGGER_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=TriggerRiseTime;
		User_Par_Values[idx+offset]=TriggerRiseTime;
		/* Update DSP parameter FASTLENGTH */
		sprintf(str,"FASTLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=FL;
		/* Download to the selected Pixie module */
		value=(U32)FL;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);			
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"FASTLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("TRIGGER_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/FILTER_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)value/FILTER_CLOCK_MHZ;
	    }
	}
	
	if(strcmp(user_variable_name,"TRIGGER_FLATTOP") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Calculate fast gap */
		idx=Find_Xact_Match("TRIGGER_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
		TriggerFlatTop=User_Par_Values[idx+offset];
		FG=(U16)RoundOff(TriggerFlatTop*FILTER_CLOCK_MHZ);
		/* Check fast gap limit */
		sprintf(str,"FASTLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		FL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		if ( (FL + FG) > 63) 
			if (FL<=63)   FG = 63 - FL;
			else {
				FL = 4;	// if both values are off, set to default
				FG = 4;
			}
		TriggerFlatTop=FG/FILTER_CLOCK_MHZ;
		/* Update fast gap time */
		idx=Find_Xact_Match("TRIGGER_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=TriggerFlatTop;
		User_Par_Values[idx+offset]=TriggerFlatTop;
		/* Update DSP parameter FASTGAP */			 
		sprintf(str,"FASTGAP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=FG;
		/* Download to the selected Pixie module */
		value=(U32)FG;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"FASTGAP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("TRIGGER_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/FILTER_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)value/FILTER_CLOCK_MHZ;
	    }
	}
	
	if(strcmp(user_variable_name,"ENERGY_RISETIME") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Calculate SL */
		idx=Find_Xact_Match("ENERGY_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		EnergyRiseTime=User_Par_Values[idx+offset];
		SL=(U16)RoundOff(EnergyRiseTime/Filter_Int[ModNum]);
		sprintf(str,"SLOWGAP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		SG=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		/* always ensure limits */
		if( SL       < 2  ) SL = 2;
		if((SL + SG) > 127) 
			if( SG <=125) SL = 127 - SG;
			else{
				SL = 8;// if both values are off, set to default
				SG = 8;
			}
		Compute_PileUp(ModNum, ChanNum, SL, SG, User_Par_Values, offset);
	    }
	    if (READ) {
		sprintf(str,"SLOWLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("ENERGY_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value*Filter_Int[ModNum];
		User_Par_Values[idx+offset]=(double)value*Filter_Int[ModNum];
	    }
	}
	
	if(strcmp(user_variable_name,"ENERGY_FLATTOP") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Calculate SG */
		idx=Find_Xact_Match("ENERGY_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
		EnergyFlatTop=User_Par_Values[idx+offset];
		SG=(U16)RoundOff(EnergyFlatTop/Filter_Int[ModNum]);
		sprintf(str,"SLOWLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		SL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		/* always ensure limits */
		if( SG       < 3  ) SG = 3;
		if((SL + SG) > 127) 
			if( SL <=124) SG = 127 - SL;
			else{
				SL = 8;// if both values are off, set to default
				SG = 8;
			}
		Compute_PileUp(ModNum, ChanNum, SL, SG, User_Par_Values, offset);
	    }
	    if (READ) {
		sprintf(str,"SLOWGAP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("ENERGY_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value*Filter_Int[ModNum];
		User_Par_Values[idx+offset]=(double)value*Filter_Int[ModNum];
	    }
	}
	
	if(strcmp(user_variable_name,"TRIGGER_THRESHOLD") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("TRIGGER_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		TriggerThreshold=User_Par_Values[idx+offset];
		sprintf(str,"FASTLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		FL=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
		FastThresh=TriggerThreshold*FL;
		if(FastThresh >= 4095) FastThresh = (TriggerThreshold = RoundOff(4095 / FL - 0.5)) * FL; /* in ADC counts */		 
		/* Update DSP parameter FastThresh */
		sprintf(str,"FASTTHRESH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)FastThresh;
		/* Download to the selected Pixie module */
		value=(U32)FastThresh;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update user_value of trigger threshold */ 
		idx=Find_Xact_Match("TRIGGER_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=TriggerThreshold;
		User_Par_Values[idx+offset]=TriggerThreshold;
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"FASTTHRESH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rta=(U16)dsp_par[idx];
		sprintf(str,"FASTLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rtb=(U16)dsp_par[idx];
		if(rtb==0)	TriggerThreshold=0.0;
		else		TriggerThreshold=(double)rta/rtb;
		idx=Find_Xact_Match("TRIGGER_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=TriggerThreshold;
		User_Par_Values[idx+offset]=TriggerThreshold;
	    }
	}
	
	if(strcmp(user_variable_name,"VGAIN") == 0 || ALLREAD)
	{
		BoardVersion = (U16)Pixie_Devices[ModNum].Module_Parameter_Values[Find_Xact_Match("BOARD_VERSION", Module_Parameter_Names, N_MODULE_PAR)];
	    if (WRITE) {

		if((BoardVersion & 0x0FF0) == 0x710) //Pixie-4 APA fixed gain 
		{
		    // Update user_value of Vgain 
		    idx=Find_Xact_Match("VGAIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		    Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=APA_V_GAIN; /* Gain for APA boards is 11.25 */
		    User_Par_Values[idx+offset]=APA_V_GAIN;
		    
			// Force DSP parameter SGA to be 0 
		    sprintf(str,"SGA%d",ChanNum);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=DEFAULT_SGA;
		    
			// Download to the selected Pixie module 
		    value=DEFAULT_SGA;
		    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		    // Program FiPPI 
		    Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);				
		}
		else 
		{
		    idx=Find_Xact_Match("VGAIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		    Vgain=User_Par_Values[idx+offset];
		    // Convert to SGA gain 
		    Vgain /= GAIN_FACTOR;
		    if (Vgain <  0.1) Vgain = 0.1;
		    if (Vgain > 20.0) Vgain = 20;

			if((BoardVersion & 0x0F00) == 0x700)	// Pixie-4
			{
			    SGA=(U16)Select_SGA_Gain(Vgain);
				SGA_gain = SGA_Computed_Gain[SGA];
			}
			else if((BoardVersion & 0x0F0F) == 0x301) // Pixie-500 Rev B
			{
				if (Vgain > (P500_HIGH_GAIN+P500_LOW_GAIN)/2)	
				{
					SGA = P500_HIGH_SGA;			// high gain 2.9 for requested gain > 1.95
					SGA_gain = P500_HIGH_GAIN;
				}
				else	
				{
					SGA = P500_LOW_SGA;				// low  gain 1.0 for requested gain < 1.95
					SGA_gain = P500_LOW_GAIN;
				}
			}
			else
			{
				SGA = DEFAULT_SGA;
				SGA_gain = DEFAULT_GAIN;
			}

			// Set DSP parameter SGA 
		    sprintf(str,"SGA%d",ChanNum);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=SGA;
		    // Download to the selected Pixie module 
		    value=(U32)SGA;
		    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		    
			// update Digital gain adjustment 
		    DigGain = Vgain/SGA_gain-1.0;
		    //limit digital gain to 10 percent up or down
		    if (DigGain >  0.1) DigGain    =  0.1;
		    if (DigGain < -0.1) DigGain    = -0.1;
		    if (DigGain >= 0)   DigGainInt = (U16)(65535 * DigGain);
		    else                DigGainInt = (U16)(65536 + 65535 * DigGain);
		    // Set DSP parameter DigGain 
		    sprintf(str,"DIGGAIN%d",ChanNum);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=DigGainInt;
		    // Download to the selected Pixie module 
		    value=(U32)DigGainInt;
		    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		    
			// Set DACs 
		    Control_Task_Run(ModNum, SET_DACS, 10000);
        	 // Program FiPPI 
		    Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
		    
			// Find baselne cut value 
		    BLcut_Finder(ModNum, ChanNum, &BLcut);
		    //------------------------------------------------------------------------------
		    //	BLcut_Finder will change the value of BLCut. Here we update it.
		    //------------------------------------------------------------------------------
		    idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		    User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
		    
			// Update user_value of Vgain  
		    idx=Find_Xact_Match("VGAIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		    Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)(1.0+DigGain)*(SGA_gain*GAIN_FACTOR);
		    User_Par_Values[idx+offset]=(double)(1.0+DigGain)*(SGA_gain*GAIN_FACTOR);
		}
	    }	// end write

	    if (READ) {
		sprintf(str,"SGA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		SGA=(U16)dsp_par[idx];
		if (SGA >= N_SGA_GAIN)  SGA = 0;	// preventing bad index for SGA_ComputedGain below

		sprintf(str,"DIGGAIN%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		DigGain=(double)(dsp_par[idx] & 0xffff);  
		if(DigGain >= 32768) DigGain =- (65536 - DigGain);
		
		// Check board version 
		if((BoardVersion & 0x0FF0) == 0x710)	// Pixie-4 APA
		{
		    Vgain = APA_V_GAIN;
		}
		else if((BoardVersion & 0x0F00) == 0x700)	// Pixie-4
		{
			Vgain=SGA_Computed_Gain[SGA]*GAIN_FACTOR*(1+DigGain/65536);
		}
		else if((BoardVersion & 0x0F0F) == 0x301) // Pixie-500 Rev B
		{
			if(SGA==P500_HIGH_SGA)	Vgain = P500_HIGH_GAIN*GAIN_FACTOR*(1+DigGain/65536);
			else					Vgain = P500_LOW_GAIN*GAIN_FACTOR*(1+DigGain/65536);
		}
		else
		{
			Vgain = DEFAULT_GAIN*GAIN_FACTOR*(1+DigGain/65536);
		}

		// update parameters
		idx=Find_Xact_Match("VGAIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Vgain;
		User_Par_Values[idx+offset]=Vgain;

		} // end read
	}
	
	if(strcmp(user_variable_name,"VOFFSET") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("VOFFSET", Channel_Parameter_Names, N_CHANNEL_PAR);
		Voffset=User_Par_Values[idx+offset];
		Dbl_value = ((1 - Voffset / V_OFFSET_MAX) * 32768);		
		if(Dbl_value < 0)
		{
		    Dbl_value = 0;
		    Voffset   = V_OFFSET_MAX;
		}
		else if(Dbl_value > 65535)
		{
		    Dbl_value =  65535;
		    Voffset   = -V_OFFSET_MAX;
		}
		TRACKDAC = (U16)Dbl_value;				
		/* Update DSP parameter TRACKDAC */
		sprintf(str,"TRACKDAC%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=TRACKDAC;
		/* Download to the selected Pixie module */
		value=(U32)TRACKDAC;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Set DACs */
		Control_Task_Run(ModNum, SET_DACS, 10000);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
		/* Update user_value of Voffset */ 
		idx=Find_Xact_Match("VOFFSET", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Voffset;
		User_Par_Values[idx+offset]=Voffset;
	    }
	    if (READ) {
		sprintf(str,"TRACKDAC%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rta=(U16)dsp_par[idx];
		Voffset=((32768.0-rta)/32768.0)*V_OFFSET_MAX;
		idx=Find_Xact_Match("VOFFSET", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Voffset;
		User_Par_Values[idx+offset]=Voffset;
	    }
	}
	
	if(strcmp(user_variable_name,"TAU") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("TAU", Channel_Parameter_Names, N_CHANNEL_PAR);
		if (User_Par_Values[idx+offset] < 1.0 / 65536.0) User_Par_Values[idx+offset] = 1.0 / 65536.0;
		if (User_Par_Values[idx+offset] > 65535.0) User_Par_Values[idx+offset] = 65535.0;
		tau = User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"PREAMPTAUA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)floor(tau);
		/* Download to the selected Pixie module */
		value=(U32)floor(tau);
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		sprintf(str,"PREAMPTAUB%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)((tau-floor(tau))*65536);
		/* Download to the selected Pixie module */
		value=(U32)((tau-floor(tau))*65536);
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Find baselne cut value */
		BLcut_Finder(ModNum, ChanNum, &BLcut);
		//------------------------------------------------------------------------------
		//	BLcut_Finder will change the value of BLCut. Here we update it.
		//------------------------------------------------------------------------------
		idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
	    }
	    if (READ) {
		sprintf(str,"PREAMPTAUA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rta=(U16)dsp_par[idx];
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		rtb=(U16)dsp_par[idx+1];
		idx=Find_Xact_Match("TAU", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)(rta+rtb/65536.0);
		User_Par_Values[idx+offset]=(double)(rta+rtb/65536.0);
	    }				
	}
	
	if(strcmp(user_variable_name,"TRACE_LENGTH") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("FIFOLENGTH", DSP_Parameter_Names, N_DSP_PAR);
		FifoLength=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
      idx=Find_Xact_Match("ENERGY_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
      Efilter = User_Par_Values[idx+offset];
      idx=Find_Xact_Match("ENERGY_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
      Efilter += User_Par_Values[idx+offset];
		idx=Find_Xact_Match("TRACE_LENGTH", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Check if TraceLength exceeds FifoLength */
		if (User_Par_Values[idx+offset] < 0                                         ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > (double)FifoLength / (double)ThisADCclkMHz) User_Par_Values[idx+offset] = (double)FifoLength / (double)ThisADCclkMHz;
	   if (ZDT && (User_Par_Values[idx+offset] > Efilter)                          ) User_Par_Values[idx+offset] = Efilter;    // for ZDT, TL must be < E filter. maybe only Tracedealy??
      TraceLength=(U16)(RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz);
  		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];	
		/* Check if PSAStart and PSAEnd are in range */
		idx=Find_Xact_Match("PSA_START", Channel_Parameter_Names, N_CHANNEL_PAR);
		PSAStart=(U16)(RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz);
		if(PSAStart > TraceLength)
		{
		    PSAStart = 0;
		    User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = (double)PSAStart / ThisADCclkMHz;
		    /* Update DSP parameter PSAOFFSET */
		    sprintf(str,"PSAOFFSET%d",ChanNum);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = PSAStart);
		    /* Download to the selected Pixie module */
		    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		}
		idx=Find_Xact_Match("PSA_END", Channel_Parameter_Names, N_CHANNEL_PAR);
		PSAEnd=(U16)((RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz));
		if(PSAEnd > TraceLength)
		{
		    PSAEnd = TraceLength;
		    User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)PSAEnd/ThisADCclkMHz;
		    /* Update DSP parameter PSALENGTH */
		    sprintf(str,"PSALENGTH%d",ChanNum);
		    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		    value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx] = PSAEnd - PSAStart);
		    /* Download to the selected Pixie module */
		    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		}
		/* Update DSP parameter TRACELENGTH */
		sprintf(str,"TRACELENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);				
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=TraceLength;
		/* Download to the selected Pixie module */
		value=(U32)TraceLength;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		ComputeFIFO(ModNum, ChanNum);
		//------------------------------------------------------------------------------
		//	ComputeFIFO will possibly change the value of TraceDelay. 
		//      Here we update TraceDelay.
		//------------------------------------------------------------------------------
		idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"TRACELENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("TRACE_LENGTH", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)value/ThisADCclkMHz;
	    }
	}
	    
	if(strcmp(user_variable_name,"TRACE_DELAY") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		// The ranges test for TRACE_DELAY is applied in ComputeFIFO()
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		ComputeFIFO(ModNum, ChanNum);
		//------------------------------------------------------------------------------
		//	ComputeFIFO will possibly change the value of TraceDelay. 
		//      Here we update TraceDelay.
		//------------------------------------------------------------------------------
		idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"PAFLENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);	
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rta=(U16)dsp_par[idx];
		sprintf(str,"TRIGGERDELAY%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);	
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		rtb=(U16)dsp_par[idx];
		idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)(rta-rtb)/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)(rta-rtb)/ThisADCclkMHz;
	    }
	}
	
	if((strcmp(user_variable_name,"PSA_START") == 0) || (strcmp(user_variable_name,"PSA_END") == 0) || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("TRACE_LENGTH", Channel_Parameter_Names, N_CHANNEL_PAR);
		TraceLength=(U16)((RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz));
		idx=Find_Xact_Match("PSA_START", Channel_Parameter_Names, N_CHANNEL_PAR);
		PSAStart=(U16)((RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz));
		idx=Find_Xact_Match("PSA_END", Channel_Parameter_Names, N_CHANNEL_PAR);
		PSAEnd=(U16)((RoundOff)(User_Par_Values[idx+offset]*ThisADCclkMHz));
		/* Enforce limits for PSA Start */
		//if((PSAStart >= TraceLength) || (PSAStart < 0)) PSAStart = 0;
		if(PSAStart >= TraceLength) PSAStart = 0;
		/* Enforce limits for PSA End */
		if(((PSAEnd <= PSAStart) && (PSAStart != 0)) || (PSAEnd > TraceLength)) PSAEnd = TraceLength;
		/* Update DSP parameter PSAOFFSET */
		sprintf(str,"PSAOFFSET%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=PSAStart;
		/* Download to the selected Pixie module */
		value=(U32)PSAStart;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update DSP parameter PSALENGTH */
		sprintf(str,"PSALENGTH%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=PSAEnd-PSAStart;
		/* Download to the selected Pixie module */
		value=(U32)(PSAEnd-PSAStart);
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update PSAStart */
		idx=Find_Xact_Match("PSA_START", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)PSAStart/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)PSAStart/FILTER_CLOCK_MHZ;				
		idx=Find_Xact_Match("PSA_END", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)PSAEnd/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)PSAEnd/ThisADCclkMHz;				
	    }
	    if (READ) {
		/* Get PSA Start */ 
		sprintf(str,"PSAOFFSET%d",ChanNum);
		idx1=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx1]=(U16)dsp_par[idx1];
		value=dsp_par[idx1];
		idx=Find_Xact_Match("PSA_START", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)value/ThisADCclkMHz;
		/* Get PSA End */                    
		sprintf(str,"PSALENGTH%d",ChanNum);
		idx2=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx2]=(U16)dsp_par[idx2];
		value=dsp_par[idx1] + dsp_par[idx2];
		idx=Find_Xact_Match("PSA_END", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/ThisADCclkMHz;
		User_Par_Values[idx+offset]=(double)value/ThisADCclkMHz;
	    }
	}
	
	if(strcmp(user_variable_name,"XDT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("XDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		xdt = User_Par_Values[idx+offset];
        	/* Keep the original XDT value */
        	lastxdt=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
        	intdt = (double)RoundOff(xdt*(double)SYSTEM_CLOCK_MHZ);
		Xavg = 65535;
		// intdt less than or equal to 4
		if (intdt <= 4)  xwait = 4; // for 2 waitstates
		// 4 < intdt <= 13
		if (intdt > 4  && intdt <= 13) xwait = intdt;
        	if (intdt > 13 && intdt <= 65533) // intdt > 13
		{
		    // increase XDT
            	    if(xdt > lastxdt) xwait = 13.0 + ceil((intdt - 13.0) / 5.0) * 5.0;
		    // decrease XDT
            	    else              xwait = 13.0 + floor((intdt - 13.0) / 5.0) * 5.0;
		    Xavg  = (U16)floor(65536 / ((intdt - 3) / 5));
		}
		if (intdt > 65533) {
		    xwait = 65533;
		    Xavg  = 5;
		}
		xdt = xwait / ((double)SYSTEM_CLOCK_MHZ);
		/* Update the DSP parameter XWAIT */
		sprintf(str,"XWAIT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)xwait;
		/* Download to the selected Pixie module */
		value=(U32)xwait;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update the DSP parameter XAVG */
		sprintf(str,"XAVG%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=Xavg;
		/* Download to the selected Pixie module */
		value=(U32)Xavg;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update xdt */ 
		idx=Find_Xact_Match("XDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=xdt;
		User_Par_Values[idx+offset]=xdt;
	    }
	    if (READ) {
		sprintf(str,"XWAIT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("XDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/SYSTEM_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)value/SYSTEM_CLOCK_MHZ;
	    }
	}
	
	if(strcmp(user_variable_name,"BASELINE_PERCENT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("BASELINE_PERCENT", Channel_Parameter_Names, N_CHANNEL_PAR);
		if (User_Par_Values[idx+offset] < 0.0  ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 100.0) User_Par_Values[idx+offset] = 100;
		baselinepercent = User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		/* Update the DSP parameter BASELINEPERCENT */
		sprintf(str,"BASELINEPERCENT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)baselinepercent;
		/* Download to the selected Pixie module */
		value=(U32)baselinepercent;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
	    }
	    if (READ) {
		sprintf(str,"BASELINEPERCENT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("BASELINE_PERCENT", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"CHANNEL_CSRA") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("CHANNEL_CSRA", Channel_Parameter_Names, N_CHANNEL_PAR);
		ChanCSRA=User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"CHANCSRA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)ChanCSRA;
		/* Download to the selected Pixie module */
		value=(U32)ChanCSRA;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);				
		
		/*
		// Update coincidence wait in case a channel is changed from "good" to "bad" and vice versa 
		// Compute minimum coincidence wait  
		value = Pixie_MinCoincWait(ModNum); // value is Minimum Coincidence Wait
		// Update minimum coincidence wait 
		idx=Find_Xact_Match("MIN_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)(value*1000.0/SYSTEM_CLOCK_MHZ);				
		// Update actual coincidence wait if necessary 
		idx=Find_Xact_Match("ACTUAL_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		value2=(U16)(Pixie_Devices[ModNum].Module_Parameter_Values[idx]*SYSTEM_CLOCK_MHZ/1000.0); // value2 is Actual Coincidence Wait				
	
		// Adjust ACW according to KeepCW option
		if(KeepCW==0) {	
			// if ACW < MCW, force ACW to be MCW; otherwise keep unchanged (assume user added on purpose) 
			if(value2 < value) value2 = value;	
			if(value2 > 65531) value2 = 65531;	// enforce upper limit
		}
		if(KeepCW==1) {	
			// always force ACW to be MCW; (assume any increase is purely due to MinCoincWait computation and if MCW decreases, ACW should follow) 
			value2 = value;
			if(value2 > 65531) value2 = 65531;	// enforce upper limit
		}
		if(KeepCW==2) {	
			// ACW disregards MCW; (assume channels are independent) 
			if(value2 <     1) value2 = 1;		// enforce lower limit 	
			if(value2 > 65531) value2 = 65531;	// enforce upper limit 				
		}
		
		Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)(value2*1000/SYSTEM_CLOCK_MHZ);
		// Update the DSP parameter COINCWAIT 
		idx=Find_Xact_Match("COINCWAIT", DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)value2;
		// Download to the selected Pixie module 
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value2);	
		*/
					
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"CHANCSRA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("CHANNEL_CSRA", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"CHANNEL_CSRB") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("CHANNEL_CSRB", Channel_Parameter_Names, N_CHANNEL_PAR);
		ChanCSRB=User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"CHANCSRB%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)ChanCSRB;
		/* Download to the selected Pixie module */
		value=(U32)ChanCSRB;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"CHANCSRB%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("CHANNEL_CSRB", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	} 
	
	if(strcmp(user_variable_name,"CHANNEL_CSRC") == 0 || ALLREAD)
	{
	    if (WRITE) {
		idx=Find_Xact_Match("CHANNEL_CSRC", Channel_Parameter_Names, N_CHANNEL_PAR);
		ChanCSRC=User_Par_Values[idx+offset];
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"CHANCSRC%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)ChanCSRC;
		/* Download to the selected Pixie module */
		value=(U32)ChanCSRC;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
	    }
	    if (READ) {
		sprintf(str,"CHANCSRC%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("CHANNEL_CSRC", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"CFD_THRESHOLD") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("CFD_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0  ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 99) User_Par_Values[idx+offset] = 99;
		CFDthresh = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"CFDTHR%d",ChanNum);
		idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)(CFDthresh*655.36));
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);				
		/* Update value */ 
		idx=Find_Xact_Match("CFD_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = CFDthresh;
	    }
	    if (READ) {
		sprintf(str,"CFDTHR%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("CFD_THRESHOLD", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)RoundOff(value / 655.36);
		User_Par_Values[idx+offset]=(double)RoundOff(value / 655.36);
	    }
	}
	
	if(strcmp(user_variable_name,"EMIN") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("EMIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0    ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 65535) User_Par_Values[idx+offset] = 65535;
		EnergyLow=User_Par_Values[idx+offset];				
		/* Update DSP parameters */
		sprintf(str,"ENERGYLOW%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)EnergyLow);
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update value */ 
		idx=Find_Xact_Match("EMIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=EnergyLow;
	    }
	    if (READ) {
		sprintf(str,"ENERGYLOW%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("EMIN", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"BINFACTOR") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("BINFACTOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0 ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 16) User_Par_Values[idx+offset] = 16;
		Log2EBin = User_Par_Values[idx+offset];
		if (Log2EBin < 1) Dbl_value = 0;
		else              Dbl_value = (65536 - Log2EBin);
		/* Update DSP parameters */
		sprintf(str,"LOG2EBIN%d",ChanNum);
		idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)(Dbl_value));
		/* Download to the selected Pixie module */
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update value */ 
		idx=Find_Xact_Match("BINFACTOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Log2EBin;
	    }
	    if (READ) {
		sprintf(str,"LOG2EBIN%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("BINFACTOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)(65536 - value);
		User_Par_Values[idx+offset]=(double)(65536 - value);
	    }
	}
	
	if(strcmp(user_variable_name,"INTEGRATOR") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("INTEGRATOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0 ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 5) User_Par_Values[idx+offset] = 5;
		Integrator = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"INTEGRATOR%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)Integrator;
		/* Download to the selected Pixie module */
		value=(U32)Integrator;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update value */ 
		idx=Find_Xact_Match("INTEGRATOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Integrator;
	    }
	    if (READ) {
		sprintf(str,"INTEGRATOR%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("INTEGRATOR", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}
	
	if(strcmp(user_variable_name,"GATE_WINDOW") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx = Find_Xact_Match("GATE_WINDOW", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 1.0   / (double)FILTER_CLOCK_MHZ) User_Par_Values[idx+offset] = 1.0   / (double)FILTER_CLOCK_MHZ;
		if (User_Par_Values[idx+offset] > 255.0 / (double)FILTER_CLOCK_MHZ) User_Par_Values[idx+offset] = 255.0 / (double)FILTER_CLOCK_MHZ;
		GateWindow = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"GATEWINDOW%d",ChanNum);
		idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(GateWindow*(double)FILTER_CLOCK_MHZ);
		/* Download to the selected Pixie module */
		value = (U32)RoundOff(GateWindow*(double)FILTER_CLOCK_MHZ);
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
		/* Update value */ 
		idx=Find_Xact_Match("GATE_WINDOW", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=GateWindow;
	    }
	    if (READ) {
		/* Get Gate Window */
		sprintf(str,"GATEWINDOW%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("GATE_WINDOW", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/(double)FILTER_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)value/(double)FILTER_CLOCK_MHZ;
	    }
	}
	
	if(strcmp(user_variable_name,"GATE_DELAY") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("GATE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 1.0   / (double)FILTER_CLOCK_MHZ) User_Par_Values[idx+offset] = 1.0   / (double)FILTER_CLOCK_MHZ;
		if (User_Par_Values[idx+offset] > 255.0 / (double)FILTER_CLOCK_MHZ) User_Par_Values[idx+offset] = 255.0 / (double)FILTER_CLOCK_MHZ;
		GateDelay = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"GATEDELAY%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(GateDelay*(double)FILTER_CLOCK_MHZ);
		/* Download to the selected Pixie module */
		value=(U32)RoundOff(GateDelay*(double)FILTER_CLOCK_MHZ);
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Program FiPPI */
		Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
		/* Update value */ 
		idx=Find_Xact_Match("GATE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=GateDelay;
	    }
	    if (READ) {
		sprintf(str,"GATEDELAY%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("GATE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/(double)FILTER_CLOCK_MHZ;
		User_Par_Values[idx+offset]=(double)value/(double)FILTER_CLOCK_MHZ;
	    }
	}
	
	if(strcmp(user_variable_name,"BLCUT") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0    ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 32767) User_Par_Values[idx+offset] = 32767;
		BaselineCut = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"BLCUT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)BaselineCut;
		/* Download to the selected Pixie module */
		value=(U32)BaselineCut;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update value */ 
		idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=BaselineCut;
	    }
	    if (READ) {
		sprintf(str,"BLCUT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"COINC_DELAY") == 0 || ALLREAD)
	{
	    if (WRITE) {
		if(KeepCW==0) {
			// if we do not adjust for E filter, Channel coinc delay is set by the user
			// else user can not change it
			// Get value 
			idx=Find_Xact_Match("COINC_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
			// Apply restrictions 
			if (User_Par_Values[idx+offset] < 0.0     / (double)SYSTEM_CLOCK_MHZ*1000.0) User_Par_Values[idx+offset] = 0.0   / (double)SYSTEM_CLOCK_MHZ*1000.0;
			if (User_Par_Values[idx+offset] > 65533.0 / (double)SYSTEM_CLOCK_MHZ*1000.0) User_Par_Values[idx+offset] = 65533.0 / (double)SYSTEM_CLOCK_MHZ*1000.0;
			CwDelay = User_Par_Values[idx+offset]; // in us
			
			// Update DSP parameters 
			sprintf(str,"COINCDELAY%d",ChanNum);
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(CwDelay*(double)SYSTEM_CLOCK_MHZ/1000.0);
			// Download to the selected Pixie module 
			value=(U32)RoundOff(CwDelay*(double)SYSTEM_CLOCK_MHZ/1000.0);
			Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
			
			temp = MAX(29,RoundOff(CwDelay*(double)FILTER_CLOCK_MHZ/1000.0+4.0));
			temp = MIN(temp, 65535);							// RESETDELAY should be a bit longer than COINCDELAY
			sprintf(str,"RESETDELAY%d",ChanNum);				// RESETDELAY is counted in Fippi, so use FILTER_CLOCK_MHZ
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)temp;
			// Download to the selected Pixie module 
			value=(U32)temp;
			Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

			// Program FiPPI 
			Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
			// Update value 
			idx=Find_Xact_Match("COINC_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
			Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=CwDelay;
			}
		else {
			idx = 65000;	// change idx so final check knows parameter has been recognized 
			}
		}

	    if (READ) {
		sprintf(str,"COINCDELAY%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("COINC_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value/(double)SYSTEM_CLOCK_MHZ*1000.0;
		User_Par_Values[idx+offset]=(double)value/(double)SYSTEM_CLOCK_MHZ*1000.0;
	    }
	}

	if(strcmp(user_variable_name,"BLAVG") == 0 || ALLREAD)
	{
	    if (WRITE) {
		/* Get value */
		idx=Find_Xact_Match("BLAVG", Channel_Parameter_Names, N_CHANNEL_PAR);
		/* Apply restrictions */
		if (User_Par_Values[idx+offset] < 0    ) User_Par_Values[idx+offset] = 0;
		if (User_Par_Values[idx+offset] > 65535) User_Par_Values[idx+offset] = 65535;
		BaselineAVG = User_Par_Values[idx+offset];
		/* Update DSP parameters */
		sprintf(str,"LOG2BWEIGHT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)BaselineAVG;
		/* Download to the selected Pixie module */
		value=(U32)BaselineAVG;
		Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
		/* Update value */ 
		idx=Find_Xact_Match("BLAVG", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=BaselineAVG;
	    }
	    if (READ) {
		sprintf(str,"LOG2BWEIGHT%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
		value=dsp_par[idx];
		idx=Find_Xact_Match("BLAVG", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
		User_Par_Values[idx+offset]=(double)value;
	    }
	}

	if(strcmp(user_variable_name,"LIVE_TIME") == 0 || ALLREAD || ALLSTAT)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]]=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		rta=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+1]=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		rtb=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+2]=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		rtc=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		LiveTime=((double)rta * pow(65536.0,2.0) + (double)rtb * 65536.0 + (double)rtc) * LTscale * 1.0e-6/FILTER_CLOCK_MHZ;
		idx=Find_Xact_Match("LIVE_TIME", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=LiveTime;
		User_Par_Values[idx+offset] = LiveTime;
	    }
	}
	
	if(strcmp(user_variable_name,"FAST_PEAKS") == 0 || ALLREAD || ALLSTAT)
	{
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		Pixie_Devices[ModNum].DSP_Parameter_Values[FASTPEAKS_Index[ChanNum]]=(U16)dsp_par[FASTPEAKS_Index[ChanNum]];
		rta=(U16)dsp_par[FASTPEAKS_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[FASTPEAKS_Index[ChanNum]+1]=(U16)dsp_par[FASTPEAKS_Index[ChanNum]+1];
		rtb=(U16)dsp_par[FASTPEAKS_Index[ChanNum]+1];
		FastPeaks = (double)rta * 65536.0 + (double)rtb;
		idx=Find_Xact_Match("FAST_PEAKS", Channel_Parameter_Names, N_CHANNEL_PAR);
		Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=FastPeaks;
		User_Par_Values[idx+offset]=FastPeaks;
	    }
	}
	
	if(strcmp(user_variable_name,"FTDT") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get Fast Trigger Dead Time (FTDT) */
		sprintf(str,"FTDTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0] = (U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1] = (U16)dsp_par[idx+1];
		rtc = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+2] = (U16)dsp_par[idx+2];
		FTDT=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*1.0e-6/FILTER_CLOCK_MHZ;
		idx=Find_Xact_Match("FTDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = FTDT;
	    }
	}
	
	if(strcmp(user_variable_name,"NOUT") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get Number of (channel) output counts */
		sprintf(str,"NOUTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0] = (U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1] = (U16)dsp_par[idx+1];
		Dbl_value = (double)rta * 65536.0 + (double)rtb;
		idx=Find_Xact_Match("NOUT", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }
	}
	
	if(strcmp(user_variable_name,"OUTPUT_COUNT_RATE") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Live time */
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]]=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		rta=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+1]=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		rtb=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+2]=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		rtc=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		LiveTime=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
		/* Get Number of (channel) output counts */
		sprintf(str,"NOUTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0] = (U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1] = (U16)dsp_par[idx+1];
		Dbl_value = (double)rta * 65536.0 + (double)rtb;
		/* Compute output count rate (channel) */
		idx=Find_Xact_Match("OUTPUT_COUNT_RATE", Channel_Parameter_Names, N_CHANNEL_PAR);
		if (LiveTime == 0) rate = 0.0;
		else               rate = Dbl_value / LiveTime;
		User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = rate;
	    }
	}
	
	if(strcmp(user_variable_name,"INPUT_COUNT_RATE") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* FTDT */
		sprintf(str,"FTDTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0] = (U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1] = (U16)dsp_par[idx+1];
		rtc = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+2] = (U16)dsp_par[idx+2];
		FTDT=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*1.0e-6/FILTER_CLOCK_MHZ;
		/* Fast peaks */
		Pixie_Devices[ModNum].DSP_Parameter_Values[FASTPEAKS_Index[ChanNum]]=(U16)dsp_par[FASTPEAKS_Index[ChanNum]];
		rta=(U16)dsp_par[FASTPEAKS_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[FASTPEAKS_Index[ChanNum]+1]=(U16)dsp_par[FASTPEAKS_Index[ChanNum]+1];
		rtb=(U16)dsp_par[FASTPEAKS_Index[ChanNum]+1];
		FastPeaks = (double)rta * 65536.0 + (double)rtb;
		/* Live time */
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]]=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		rta=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+1]=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		rtb=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+2]=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		rtc=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		LiveTime=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
		/* Compute input count rate (channel) */
		idx=Find_Xact_Match("INPUT_COUNT_RATE", Channel_Parameter_Names, N_CHANNEL_PAR);
		if ((LiveTime - FTDT) == 0) rate = 0.0;
		else                        rate = FastPeaks/(LiveTime - FTDT);
		User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = rate;
	    }
	}
	
	if(strcmp(user_variable_name,"GATE_COUNTS") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Get Number of Gate counts */
		sprintf(str,"GCOUNTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		Dbl_value = (double)rta * 65536.0 + (double)rtb;
		idx=Find_Xact_Match("GATE_COUNTS", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }
	}
	
	if(strcmp(user_variable_name,"GATE_RATE") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
		/* Live time */
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]]=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		rta=(U16)dsp_par[LIVETIME_Index[ChanNum]];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+1]=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		rtb=(U16)dsp_par[LIVETIME_Index[ChanNum]+1];
		Pixie_Devices[ModNum].DSP_Parameter_Values[LIVETIME_Index[ChanNum]+2]=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		rtc=(U16)dsp_par[LIVETIME_Index[ChanNum]+2];
		LiveTime=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
		/* Number of Gate counts */
		sprintf(str,"GCOUNTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		Dbl_value = (double)rta * 65536.0 + (double)rtb;
		/* Compute gate rate */
		idx=Find_Xact_Match("GATE_RATE", Channel_Parameter_Names, N_CHANNEL_PAR);
		if (LiveTime == 0) rate = 0.0;
		else               rate = Dbl_value / LiveTime;
		User_Par_Values[idx+offset] = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx] = rate;
	    }
	}
	
	if(strcmp(user_variable_name,"SFDT") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {	
		/* Get Slow Filter Dead Time (SFDT) */
		sprintf(str,"SFDTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		rtc = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+2]=(U16)dsp_par[idx+2];
		Dbl_value=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
		idx=Find_Xact_Match("SFDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }			
	}
	
	if(strcmp(user_variable_name,"GDT") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {	
		/* Get Gate Dead Time (GDT) */
		sprintf(str,"GDTA%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		rtb = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+1]=(U16)dsp_par[idx+1];
		rtc = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+2]=(U16)dsp_par[idx+2];
		Dbl_value=((double)rta*pow(65536.0,2.0)+(double)rtb*65536.0+(double)rtc)*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
		idx=Find_Xact_Match("GDT", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }			
	}

	if(strcmp(user_variable_name,"CURRENT_ICR") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {	
		/* Get current ICR  */
		sprintf(str,"ICR%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		Dbl_value=(double)rta/(65536.0*32*1.0e-6/FILTER_CLOCK_MHZ);
		idx=Find_Xact_Match("CURRENT_ICR", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }			
	}

		if(strcmp(user_variable_name,"CURRENT_OORF") == 0 || ALLREAD || ALLSTAT)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {	
		/* Get current ICR  */
		sprintf(str,"OORF%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		rta = Pixie_Devices[ModNum].DSP_Parameter_Values[idx+0]=(U16)dsp_par[idx+0];
		Dbl_value=(double)rta*100.0/(65536.0);
		idx=Find_Xact_Match("CURRENT_OORF", Channel_Parameter_Names, N_CHANNEL_PAR);
		User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=Dbl_value;
	    }			
	}



	if(strcmp(user_variable_name,"NEC_RUN_STATS") == 0)
        {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) 
		{	
		/* Get runstats from memory  */
			Pixie_IODM(ModNum, USER_MEMORY_ADDRESS, MOD_READ, 384, buffer);
			idx = 0;
			// compute live times in s 
			// memory contains 3 values (low/mid/high) of livetime data for each of 
			// Isotope ID000 ch0.1.2.3; ID001 ch0.1.2.3; etc -- total 8 x 4 values
			for (i = 0; i < 32; i++) 
			{
				result=((double)buffer[3*i+2]*pow(65536.0,2.0)+(double)buffer[3*i+1]*65536.0+(double)buffer[3*i])*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
				User_Par_Values[idx]= result;
				idx+=1;
			}
			// compute sfdt in s 
			// memory contains 3 values (low/mid/high) of sfdt data for each of 
			// Isotope ID000 ch0.1.2.3; ID001 ch0.1.2.3; etc -- total 8 x 4 values
			for (i = 32; i < 64; i++) 
			{
				result=((double)buffer[3*i+2]*pow(65536.0,2.0)+(double)buffer[3*i+1]*65536.0+(double)buffer[3*i])*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
				User_Par_Values[idx]= result;
				idx+=1;
			}
			// compute ftdt in s 
			// memory contains 3 values (low/mid/high) of ftdt data for each of 
			// Isotope ID000 ch0.1.2.3; ID001 ch0.1.2.3; etc -- total 8 x 4 values
			for (i = 64; i < 96; i++) 
			{
				result=((double)buffer[3*i+2]*pow(65536.0,2.0)+(double)buffer[3*i+1]*65536.0+(double)buffer[3*i])*1.0e-6/FILTER_CLOCK_MHZ;
				User_Par_Values[idx]= result;
				idx+=1;
			}
			// compute gdt in s 
			// memory contains 3 values (low/mid/high) of gdt data for each of 
			// Isotope ID000 ; ID001 ; ID010; etc -- total 8 values (input signals are the same for each channel)
			for (i = 96; i < 104; i++) 
			{
				result=((double)buffer[3*i+2]*pow(65536.0,2.0)+(double)buffer[3*i+1]*65536.0+(double)buffer[3*i])*LTscale*1.0e-6/FILTER_CLOCK_MHZ;
				User_Par_Values[idx]= result;
				idx+=1;
			}
			// copy AUX counter  
			// memory contains 1 value of data for each of 
			// Isotope ID000 ; ID001 ; ID010; etc -- total 8 values (input signals are the same for each channel)
			for (i = 312; i < 320; i++) 
			{
				result=((double)buffer[i]);
				User_Par_Values[idx]= result;
				idx+=1;
			}
		

	    }			

	}


	/* BAA Specific Variables */

	if(strcmp(user_variable_name,"PSM_GAIN_AVG_LEN") == 0 || ALLREAD) {
			if (WRITE) {
				/* Get value */
				idx=Find_Xact_Match("PSM_GAIN_AVG_LEN", Channel_Parameter_Names, N_CHANNEL_PAR);
				/* Apply restrictions */
				if (User_Par_Values[idx+offset] < 2    ) User_Par_Values[idx+offset] = 2;
				if (User_Par_Values[idx+offset] > 32768) User_Par_Values[idx+offset] = 32768;
				/* Update DSP parameters */
				sprintf(str,"LOGPSMGAINAVGLEN%d",ChanNum);
				idx2=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
				value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx2]=(U16)(log(User_Par_Values[idx+offset]) / log(2)));
				/* Download to the selected Pixie module */
				Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx2), MOD_WRITE, 1, &value);
				/* Update value */
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=pow(2, (double)value);
			}
			if (READ) {
				sprintf(str,"LOGPSMGAINAVGLEN%d",ChanNum);
				idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
				Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
				value=dsp_par[idx];
				idx=Find_Xact_Match("PSM_GAIN_AVG_LEN", Channel_Parameter_Names, N_CHANNEL_PAR);
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=pow(2, (double)value);
				User_Par_Values[idx+offset]=pow(2, (double)value);
			}
	}

	if(strcmp(user_variable_name,"PSM_TEMP_AVG_LEN") == 0 || ALLREAD) {
		if (WRITE) {
				/* Get value */
				idx=Find_Xact_Match("PSM_TEMP_AVG_LEN", Channel_Parameter_Names, N_CHANNEL_PAR);
				/* Apply restrictions */
				if (User_Par_Values[idx+offset] < 2 ) User_Par_Values[idx+offset] = 2;
				if (User_Par_Values[idx+offset] > 16) User_Par_Values[idx+offset] = 16;
				/* Update DSP parameters */
				sprintf(str,"LOGPSMTEMPAVGLEN%d",ChanNum);
				idx2=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
				value = (U32)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx2]=(U16)(log(User_Par_Values[idx+offset]) / log(2)));
				/* Download to the selected Pixie module */
				Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx2), MOD_WRITE, 1, &value);
				/* Update value */ 
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=pow(2, (double)value);
			}
			if (READ) {
				sprintf(str,"LOGPSMTEMPAVGLEN%d",ChanNum);
				idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
				Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
				value=dsp_par[idx];
				idx=Find_Xact_Match("PSM_TEMP_AVG_LEN", Channel_Parameter_Names, N_CHANNEL_PAR);
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=pow(2, (double)value);
				User_Par_Values[idx+offset]=pow(2, (double)value);
			}
	}

	if(strcmp(user_variable_name,"PSM_GAIN_CORR") == 0 || ALLREAD) {
			if (WRITE) {
				idx=Find_Xact_Match("PSM_GAIN_CORR", Channel_Parameter_Names, N_CHANNEL_PAR);
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=10;
			}
			if (READ) {
				idx=Find_Xact_Match("PSM_GAIN_CORR", Channel_Parameter_Names, N_CHANNEL_PAR);
				Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)10;
				User_Par_Values[idx+offset]=(double)10;
			}
	}

	if(strcmp(user_variable_name,"PSM_GAIN_AVG") == 0 || ALLREAD || ALLSTAT) {
	    if (WRITE) return (-4); // Wrong direction for this variable
		if (READ) {
			sprintf(str,"PSMGAINAVG%d",ChanNum);
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
			value=dsp_par[idx];
			idx=Find_Xact_Match("PSM_GAIN_AVG", Channel_Parameter_Names, N_CHANNEL_PAR);
			Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=(double)value;
			User_Par_Values[idx+offset]=(double)value;
		}			
	}						 

	if(strcmp(user_variable_name,"PSM_TEMP_AVG") == 0 || ALLREAD || ALLSTAT) {
	    if (WRITE) return (-4); // Wrong direction for this variable
	    if (READ) {
			sprintf(str,"PSMTEMPAVG%d",3);		// currently ch.3's variable is used for all
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)dsp_par[idx];
			value=dsp_par[idx];
			idx=Find_Xact_Match("PSM_TEMP_AVG", Channel_Parameter_Names, N_CHANNEL_PAR);
			Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=0.0625 * (double)value;
			User_Par_Values[idx+offset]=0.0625 * (double)value;
	    }			
	}			

	/* End of BAA Specific Variables */
	
	
	if(idx == 65535) // Keep it at the end of list	
	{
	    sprintf(ErrMSG, "*ERROR* (UA_PAR_IO): invalid channel parameter name %s", user_variable_name);
	    Pixie_Print_MSG(ErrMSG);
	    return(-3);
	}
	else
	{
//	    sprintf(ErrMSG, "*INFO* (UA_PAR_IO): Successfully transferred channel parameter %s for channel %hu", user_variable_name, ChanNum);
//	    Pixie_Print_MSG(ErrMSG);
	    return(0);
	}		
				
}			


S32 Compute_PileUp(U8 ModNum, U8 ChanNum, U16 SL, U16 SG, double *User_Par_Values, U16 offset) 
{

    char   str[256];
    U16	   idx, FilterRange, PeakSample, PeakSep, ACW, MCW;
 	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize to Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters
	U32 value, cwd;
    double EnergyRiseTime, EnergyFlatTop, BLcut, CwDelay;


	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );


    EnergyRiseTime = Filter_Int[ModNum]*SL;
    EnergyFlatTop  = Filter_Int[ModNum]*SG;
    /* Update energy peaking time */
    idx=Find_Xact_Match("ENERGY_RISETIME", Channel_Parameter_Names, N_CHANNEL_PAR);
    Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=EnergyRiseTime;
    User_Par_Values[idx+offset]=EnergyRiseTime;
    /* Update energy gap time */
    idx=Find_Xact_Match("ENERGY_FLATTOP", Channel_Parameter_Names, N_CHANNEL_PAR);
    Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=EnergyFlatTop;
    User_Par_Values[idx+offset]=EnergyFlatTop;
    /* Update DSP parameter SLOWLENGTH */
    sprintf(str,"SLOWLENGTH%d",ChanNum);
    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=SL;
    /* Download to the selected Pixie module */
    value=(U32)SL;
    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
    /* Update DSP parameter SLOWGAP */			 
    sprintf(str,"SLOWGAP%d",ChanNum);
    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=SG;
    /* Download to the selected Pixie module */
    value=(U32)SG;
    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
    /* Get the values of DSP parameters FilterRange, SlowLength, SlowGap and BLCut */
    idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
    FilterRange=Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
    if (FilterRange == 0)             PeakSep = (PeakSample = MAX(0, SL + SG - 7)) + 5; /* keep it greater than or equal to 0 */
    if (FilterRange == 1)             PeakSep = (PeakSample = MAX(2, SL + SG - 4)) + 5; /* keep it greater than 1 */
    if (FilterRange == 2)             PeakSep = (PeakSample =        SL + SG - 2 ) + 5;
    if (FilterRange == 3)             PeakSep = (PeakSample =        SL + SG - 1 ) + 5;
    if (FilterRange == 4)             PeakSep = (PeakSample =        SL + SG - 1 ) + 5;
    if (FilterRange >= 5)             PeakSep = (PeakSample =        SL + SG - 1 ) + 5;
    if ( PeakSep               > 128) PeakSep =  PeakSample + 1;
    if ((PeakSep - PeakSample) >   7) PeakSep =  PeakSample + 7; // This never ever happens. Insured by the above settings
    /* Update DSP parameter PEAKSAMPLE */
    sprintf(str,"PEAKSAMPLE%d",ChanNum);
    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=PeakSample;
    /* Download to the selected Pixie module */
    value=(U32)PeakSample;
    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
    /* Update DSP parameter PEAKSEP */
    sprintf(str,"PEAKSEP%d",ChanNum);
    idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
    Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=PeakSep;
    /* Download to the selected Pixie module */
    value=(U32)PeakSep;
    Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
    ComputeFIFO(ModNum, ChanNum);
    //------------------------------------------------------------------------------
    //	ComputeFIFO will possibly change the value of TraceDelay.
    //  Here we update TraceDelay.
    //------------------------------------------------------------------------------
    idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
    User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];
    /* Find baselne cut value */
    BLcut_Finder(ModNum, ChanNum, &BLcut);
    //------------------------------------------------------------------------------
    //	BLcut_Finder will change the value of BLCut. Here we update it.
    //------------------------------------------------------------------------------
    idx=Find_Xact_Match("BLCUT", Channel_Parameter_Names, N_CHANNEL_PAR);
    User_Par_Values[idx+offset]=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx];


	//------------------------------------------------------------------------------
    //	Changing filter settings affects the minimum coincidence wait. 
	//  Compute (and apply channel changes) in subfunction and update host value here
    //------------------------------------------------------------------------------

    MCW = Pixie_MinCoincWait(ModNum);
	idx=Find_Xact_Match("MIN_COINCIDENCE_WAIT", Module_Parameter_Names, N_MODULE_PAR);
	Pixie_Devices[ModNum].Module_Parameter_Values[idx]=(double)(MCW*1000.0/SYSTEM_CLOCK_MHZ);
	User_Par_Values[idx+offset]=(double)(MCW*1000.0/SYSTEM_CLOCK_MHZ);



    /* Program FiPPI */
    Control_Task_Run(ModNum, PROGRAM_FIPPI, 1000);
    return (0);	
}

/****************************************************************
 *	ComputeFIFO function:
 *		Update FIFO settings.
 *
 *		Return Value:
 *			0 - success
 *
 ****************************************************************/

S32 ComputeFIFO (
			U8 ModNum,		// Pixie module number
			U8 ChanNum )	// Pixie channel number
{
	U16 idx;
	double FilterRange, PeakSep, TraceLength, FifoLength;
	double TriggerDelay, PAFLength, TraceDelay;
	U32 value;
	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize ot Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters
	S8  str[50];


	// Define clock constants according to BoardRevision 
	Pixie_Define_Clocks (ModNum,ChanNum,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );

		
 
	/* Get the current TraceDelay */
	idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
	TraceDelay=Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]*ThisADCclkMHz;

	/* Get the current TraceLength */
	idx=Find_Xact_Match("TRACE_LENGTH", Channel_Parameter_Names, N_CHANNEL_PAR);
	TraceLength = Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]*ThisADCclkMHz;

	/* Get the current FIFO Length */
	idx=Find_Xact_Match("FIFOLENGTH", DSP_Parameter_Names, N_DSP_PAR);
	FifoLength=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

	/* Ensure TraceDelay is not larger than TraceLength (for P500 -32 pipeline delay applied by DSP)*/
	if(ThisADCclkMHz == P4_ADC_CLOCK_MHZ)
		TraceDelay = MIN(TraceDelay, (TraceLength));
	else
	{
      if(ZDT)
         TraceDelay = MIN(TraceDelay, 2048);       // in ZDT mode, TraceDelay < 2K
		TraceDelay = MIN(TraceDelay, (TraceLength-32));
		TraceDelay = MAX(TraceDelay, 0);
	}

	if(TraceLength > 0)
	{
		/* Get the DSP parameter FILTERRANGE */
		idx=Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);
		FilterRange=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

		/* Get the DSP parameter PEAKSEP */
		sprintf(str,"PEAKSEP%d",ChanNum);
		idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
		PeakSep=(double)Pixie_Devices[ModNum].DSP_Parameter_Values[idx];

		/* Calculate TriggerDelay */
		TriggerDelay=(PeakSep-1.0)*pow(2.0, FilterRange);
	}
	/* For MCA run, TraceLength is set to be 0 */
	else    TriggerDelay = 2;

	
	PAFLength=TriggerDelay+TraceDelay;
	if(PAFLength > FifoLength)  /* PAFLength must be not larger than FifoLength */
	{
		PAFLength = FifoLength - 1; /* Keep TraceDelay while reducing TriggerDelay */
		TriggerDelay = PAFLength - TraceDelay;
	}
   
	/* Update the DSP parameter TRIGGERDELAY */
	sprintf(str,"TRIGGERDELAY%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(TriggerDelay);

	/* Download to the selected Pixie module */
	value=(U32)RoundOff(TriggerDelay);
	Pixie_IODM(Chosen_Module, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Update the DSP parameter PAFLENGTH */
	sprintf(str,"PAFLENGTH%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(PAFLength);

	/* Download to the selected Pixie module */
	value=(U32)RoundOff(PAFLength);
	Pixie_IODM(Chosen_Module, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Update the DSP parameter USERDELAY */
	sprintf(str,"USERDELAY%d",ChanNum);
	idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RoundOff(TraceDelay);

	/* Download to the selected Pixie module */
	value=(U32)RoundOff(TraceDelay);
	Pixie_IODM(Chosen_Module, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);

	/* Update user value trace delay */
	idx=Find_Xact_Match("TRACE_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
	Pixie_Devices[ModNum].Channel_Parameter_Values[ChanNum][idx]=TraceDelay/ThisADCclkMHz;

	return(0);
}

/****************************************************************
 *	Pixie_ComputeMaxEvents function:
 *		Compute the maximum allowed list mode events for the
 *		Pixie module.
 *
 *		Return Value:
 *			MaxEvents
 *
 ****************************************************************/

U16 Pixie_ComputeMaxEvents(
			U8  ModNum,			/* Pixie module number */
			U16 RunType )		/* data run type */
{	
	U16 idx, k, bhl, ehl, chl;
	U32 MaxEvents, leventbuffer, loutputbuffer, lengthin, lengthout;
	S8  str[256];

	// Check InputEventSize for list modes (0x101, 0x102, 0x103)  
	// and must not exceed EventBufferSize; Check OutputEventSize 
	// for all list modes.	
   // in ZDT mode, every channel has its own event header, so add ehl for every good channel

	bhl = BUFFER_HEAD_LENGTH;				// Buffer Head Length 
	ehl = EVENT_HEAD_LENGTH;				// Event Head Length 
	leventbuffer = EVENT_BUFFER_LENGTH;		// Event Buffer Size 
	loutputbuffer = IO_BUFFER_LENGTH;		//	Output Buffer Size 

	if(RunType != 0x301)  /* All list modes */
	{		
		if((RunType == 0x101) || (RunType == 0x102) || (RunType == 0x103))
		{
         if(ZDT)
		    	lengthin = bhl;
         else
            lengthin = bhl + ehl;
			for(k = 0; k < NUMBER_OF_CHANNELS; k ++)
			{
				sprintf(str,"CHANCSRA%d",k);
				idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
				if(TstBit(2,Pixie_Devices[ModNum].DSP_Parameter_Values[idx]))
				{
					sprintf(str,"TRACELENGTH%d",k);
					idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
               if(ZDT)
					   lengthin += (MAX_CHAN_HEAD_LENGTH + ehl + Pixie_Devices[ModNum].DSP_Parameter_Values[idx]);
               else
                  lengthin += (MAX_CHAN_HEAD_LENGTH + Pixie_Devices[ModNum].DSP_Parameter_Values[idx]);
				}
			}
			if (lengthin > leventbuffer)
			{
				sprintf(ErrMSG, "*ERROR* (ComputeMaxEvents): The event is too large for compressed run modes (%d). Please shorten traces.",lengthin);
				Pixie_Print_MSG(ErrMSG);
				return(0);
			}
			
			if (lengthin > (U32)(bhl + 4*ehl) )
			{
				sprintf(ErrMSG, "*INFO* (ComputeMaxEvents): Runtype 0x101-103 - Tracelength should be reduced to zero unless DSP is performing PSA");
				Pixie_Print_MSG(ErrMSG);
			}
		}

		/* Check OutputEventSize for list modes and fast list modes; */
		/* must not exceed OutBufferSize; if successful, calculate   */
		/* MaxEvents.												 */

		/* Set Channel Head Length */
		switch(RunType)
		{
			case 0x100:
			case 0x101:
				chl = 9; break;
			case 0x102:
				chl = 4; break;
			case 0x103:
				chl = 2; break;
			default:     // invalid Run Type
				sprintf(ErrMSG,"*ERROR* (ComputeMaxEvents): %d is an invalid Run Type",RunType);
				Pixie_Print_MSG(ErrMSG);
				return(0);
		}

      if(ZDT)  
	     	lengthout = 0;
      else
        	lengthout = ehl;
 
		for(k = 0; k < NUMBER_OF_CHANNELS; k ++)
		{
			sprintf(str,"CHANCSRA%d",k);
			idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			if(TstBit(2,Pixie_Devices[ModNum].DSP_Parameter_Values[idx]))
			{
				if(RunType == 0x100)  /* capture traces only in list mode 0x100 */
				{
					sprintf(str,"TRACELENGTH%d",k);
					idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
               if(ZDT)  
					    lengthout += (chl + ehl + Pixie_Devices[ModNum].DSP_Parameter_Values[idx]);
               else
                   lengthout += (chl + Pixie_Devices[ModNum].DSP_Parameter_Values[idx]);
				}
				else
				{
                if(ZDT)  
					    lengthout += (chl + ehl);
                else
                    lengthout += chl;

				}
			}
		}

		if(lengthout > (loutputbuffer-bhl))
		{
			sprintf(ErrMSG, "*ERROR* (ComputeMaxEvents): The event is too large (%d) for the output buffer. Please shorten traces.",lengthout);
			Pixie_Print_MSG(ErrMSG);
			return(0);
		}

		/* Calculate MaxEvents */
		MaxEvents = (U32)floor((loutputbuffer-bhl)/lengthout);
	}				
	else  /* No need to check MCA run mode 0x301 */
	{
		MaxEvents=0;
	}

	/* Update the DSP parameter MAXEVENTS */
	idx=Find_Xact_Match("MAXEVENTS", DSP_Parameter_Names, N_DSP_PAR);
	Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)MaxEvents;
	Pixie_IODM(ModNum, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &MaxEvents);

	return((U16)MaxEvents);
}

/****************************************************************
 *	Pixie_MinCoincWait:
 *		This routine calculates the minimum coincidence wait time.
 *
 *		DSP triggers (and hit pattern) come out at <peaksep>
 *      (plus maybe some constant delay) after the fast trigger.
 *      If the same event occurs in all channels, a channel has to be delayed
 *		by its difference to the maximum <peaksep> of all 4 channels to make sure it
 *		falls into the coincidence window. 
 *
 *
 *		Return Value:
 *			the maximum <peaksep> of all 4 channels (in clock cycles)
 *
 *		must be followed by ProgrammFippi in parent function
 *
 ****************************************************************/

U16 Pixie_MinCoincWait (U8 ModNum)		/* Pixie module number */
{

	U16 k, FilterRange, idx, CW, CWmin, CWmax, PSP, CWait;
	U32 CWD, RSTDL, temp;
	S8  str[256];
	U16	offset;
	
	U16 SYSTEM_CLOCK_MHZ = P4_SYSTEM_CLOCK_MHZ;	// initialize ot Pixie-4 default
	U16 FILTER_CLOCK_MHZ = P4_FILTER_CLOCK_MHZ;
	U16	ADC_CLOCK_MHZ = P4_ADC_CLOCK_MHZ;
	U16	ThisADCclkMHz = P4_ADC_CLOCK_MHZ;
	U16	LTscale =P4_LTSCALE;			// The scaling factor for live time counters
	
	/* Get value of FilterRange */
	idx = (U16)Find_Xact_Match("FILTERRANGE", DSP_Parameter_Names, N_DSP_PAR);				
	FilterRange = Pixie_Devices[ModNum].DSP_Parameter_Values[idx];
	
	/* Calculate CWait */
	CWmin=(U16)(132 * pow(2.0, FilterRange)); /* max Peaksep (SL+SG+4) in clock cycles */
	CWmax=0; /* min Peaksep in clock cycles */
	
	for(k = 0; k < NUMBER_OF_CHANNELS; k++)
	{
	    /* Get each "GOOD" channel's PeakSep, express in clock cycles */
	    sprintf(str,"CHANCSRA%d",k);
	    idx = Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
	    if(TstBit(2,Pixie_Devices[ModNum].DSP_Parameter_Values[idx])) { // Only "GOOD" channels participate
			sprintf(str,"PEAKSEP%d", k);
			idx=(U16)Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);				
			PSP = (U16)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx] * pow(2.0, FilterRange));
			/* find min and max of all channels */
			CWmax=MAX(PSP,CWmax);
			CWmin=MIN(PSP,CWmin);
	    }
	}
	
	CWait = MAX(CWmax,1);		// return longest peaksep of all channels in a module (in filter clock cycles)


	// now update COINCDELAY and RESETDELAY for all channels (slowest may have changed)
	if(KeepCW==1) {
		// if we adjust for E filter, Channel COINC_DELAY makes up difference to maximum peaksep of the 4 channels
		// if not, COINC_DELAY is unchanged
		for(k = 0; k < NUMBER_OF_CHANNELS; k++)
		{
			// Define clock constants according to BoardRevision 
			Pixie_Define_Clocks (ModNum,k,&SYSTEM_CLOCK_MHZ,&FILTER_CLOCK_MHZ,&ADC_CLOCK_MHZ,&LTscale, &ThisADCclkMHz );

			// get PEAKSEP (in filter clock cycles)
			sprintf(str,"PEAKSEP%d", k);
			idx=(U16)Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);				
			PSP = (U16)(Pixie_Devices[ModNum].DSP_Parameter_Values[idx] * pow(2.0, FilterRange));

			// COINCDELAY compensates this channels PEAKSEP to longest PEAKSEP
			temp = (U32)(CWait - PSP);	
			CWD  = RoundOff(temp /(double)FILTER_CLOCK_MHZ * (double)SYSTEM_CLOCK_MHZ);  		// Coincdelay lives in system FPGA, so adjust time units
			CWD  = MAX(0,CWD);																	// COINCDELAY is increased by 1 in DSP 
			CWD  = MIN(CWD, 65535);
			RSTDL = MAX(29,temp+4);																// RESETDELAY should accommodate COINC_DELAY; counts in Fippi 
			RSTDL = MIN(RSTDL, 65535);															// past default was 29

			// Update DSP parameters 
			sprintf(str,"COINCDELAY%d",k);
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)CWD;
			// Download to the selected Pixie module 
			Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &CWD);
			sprintf(str,"RESETDELAY%d",k);
			idx=Find_Xact_Match(str, DSP_Parameter_Names, N_DSP_PAR);
			Pixie_Devices[ModNum].DSP_Parameter_Values[idx]=(U16)RSTDL;
			// Download to the selected Pixie module 
			Pixie_IODM(ModNum, (U16)(DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &RSTDL);

			// Update host values 
			idx=Find_Xact_Match("COINC_DELAY", Channel_Parameter_Names, N_CHANNEL_PAR);
			Pixie_Devices[ModNum].Channel_Parameter_Values[k][idx]=(double)CWD/(double)SYSTEM_CLOCK_MHZ*1000.0;
		//	offset = ModNum*N_CHANNEL_PAR*NUMBER_OF_CHANNELS + k*N_CHANNEL_PAR;
		//	User_Par_Values[idx+offset]=(double)CWD/(double)SYSTEM_CLOCK_MHZ*1000.0;
		//  User_Par_Values are not accessible here. May not matter, Igor updates all from Pixie_Devices after parameter change

		}
	}

	return((U16)CWait);
}

/****************************************************************
 *	Select_SGA_Gain function:
 *		This routine returns the SGA value that produces the gain closest
 *		to the requested one
 *
 ****************************************************************/


S32 Select_SGA_Gain (
			double gain )		/* requested gain */
{
	
	U32 k, minindex;
	double mindiff,diff,cgain;
	
	minindex=0;
	mindiff=1e6;
	for(k=0; k<N_SGA_GAIN; k++)
	{
		cgain=SGA_Computed_Gain[k];
		if(cgain>0)
		{
			diff=fabs(cgain/gain-1.0);
			if(diff<mindiff)
			{
				mindiff=diff;
				minindex=k;
			}
		}
	}
	
	return(minindex);
}

/****************************************************************
 *	Pixie_Broadcast function:
 *		Broadcast certain global parameters to all Pixie modules.
 *
 *		Return Value:
 *			 0 - success
 *			-1 - invalid parameter to be broadcasted
 *
 ****************************************************************/


S32 Pixie_Broadcast (
			S8 *str,				/* variable name whose value is to be broadcasted */
			U8 SourceModule )		/* the source module number */
{

	U32  idx, value;
	U16  i;
	
	if(strcmp(str,"SYNCH_WAIT")==0)
	{		
		/* Get SYNCHWAIT */
		idx=Find_Xact_Match("SYNCH_WAIT", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)Pixie_Devices[SourceModule].Module_Parameter_Values[idx];

		/* Update module parameters in all modules */
		for(i=0; i < PRESET_MAX_MODULES; i++)
		{			
			Pixie_Devices[i].Module_Parameter_Values[idx]=Pixie_Devices[SourceModule].Module_Parameter_Values[idx];
		}

		/* Send SYNCHWAIT to all modules */
		idx=Find_Xact_Match("SYNCHWAIT", DSP_Parameter_Names, N_DSP_PAR);
		for(i=0; i < PRESET_MAX_MODULES; i++)
		{
			
			Pixie_Devices[i].DSP_Parameter_Values[idx]=(U16)value;
			
			if(i < Number_Modules)	/* Download it to the modules which are present in the system */
			{
				Pixie_IODM((U8)i, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
			}
		}
	}
	
	else if(strcmp(str,"IN_SYNCH")==0)
	{		
		/* Get INSYNCH */
		idx=Find_Xact_Match("IN_SYNCH", Module_Parameter_Names, N_MODULE_PAR);
		value=(U32)Pixie_Devices[SourceModule].Module_Parameter_Values[idx];

		/* Update module parameters in all modules */
		for(i=0; i < PRESET_MAX_MODULES; i++)
		{			
			Pixie_Devices[i].Module_Parameter_Values[idx]=Pixie_Devices[SourceModule].Module_Parameter_Values[idx];
		}
		
		/* Send INSYNCH to all modules */
		idx=Find_Xact_Match("INSYNCH", DSP_Parameter_Names, N_DSP_PAR);
		for(i=0; i < PRESET_MAX_MODULES; i++)
		{			
			Pixie_Devices[i].DSP_Parameter_Values[idx]=(U16)value;
			
			if(i < Number_Modules)	/* Download it to the modules which are present in the system */
			{
				Pixie_IODM((U8)i, (DATA_MEMORY_ADDRESS+idx), MOD_WRITE, 1, &value);
			}
		}
	}
	
	else
	{
		sprintf(ErrMSG, "*ERROR* (Pixie_Broadcast): invalid global parameter %s", str);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}
	
	return(0);
}



