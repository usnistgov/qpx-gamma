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


/*
 * utilities.h
 *
 * Header files for utility functions.
 *
 */

#ifndef UTILITIES_H
#define UTILITIES_H

/* If this is compiled by a C++ compiler, make it clear that these are C routines */
#ifdef __cplusplus
extern "C" {
#endif

/* Global data structure used for the list mode file reader */
extern LMR_t LMA;

/*
*             Defines MIN and MAX macros.
*/

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

/************************************/
/*		Function prototypes			*/
/************************************/

S32 Tau_Finder (
			U8 ModNum,			// Pixie module number
			U8 ChanNum,			// Pixie channel number
			double *Tau );		// Tau value




double Tau_Fit (
			U32 *Trace,		// ADC trace data
			U32 kmin,		// lower end of fitting range
			U32 kmax,		// uuper end of fitting range
			double dt );		// sampling interval of ADC trace data


double Phi_Value (
			U32 *ydat,		// source data for search
			double qq,		// search parameter
			U32 kmin,		// search lower limit
			U32 kmax );		// search upper limit

double Thresh_Finder (
			U32 *Trace,			// ADC trace data
			double *Tau,		// Tau value
			double *FF,			// return values for fast filter
			double *FF2,		// return values for fast filter
			U32 FL,				// fast length
			U32 FG,				// fast gap
			U8  ModNum,			// Pixie module number
			U8  ChanNum );		// Pixie channel number

S32 Adjust_Offsets (
			U8 ModNum );	// module number

S32 Linear_Fit (
			double *data,			// source data for linear fit
			double coeff[2] );		// coefficients

S32 Pixie_Sleep (
		    double ms );			// time in milliseconds



//******************************************************/
//		%%% List mode data parse functions %%% */
//******************************************************/

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
S32 Pixie_User_List_Mode_Reader (
			S8  *filename,		// the list mode data file name (with complete path)
			U32 *UserData);	


#ifdef __cplusplus
}
#endif	/* End of notice for C++ compilers */

#endif	/* End of utilities.h */

