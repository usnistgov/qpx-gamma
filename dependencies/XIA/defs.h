#ifndef __DEFS_H
#define __DEFS_H


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
 *     Defs.h
 *
 * Description:
 *
 *     Constant definitions
 *
 * Revision:
 *
 *     11-30-2004
 *
 ******************************************************************************/


/* If this is compiled by a C++ compiler, make it */
/* clear that these are C routines.               */
#ifdef __cplusplus
extern "C" {
#endif

//***********************************************************
//		Definition for COMPILE_IGOR_XOP
//***********************************************************
// Set COMPILE_IGOR_XOP if compiling Igor XOP code
// defined in make file of project file
//***********************************************************
//		End of definition for COMPILE_IGOR_XOP
//***********************************************************

// Set XIA_WINDOZE for Windows
// Set XIA_LINUX for Linux. Example code only, not tested/supported by XIA
// defined in make file of project file

//***********************************************************
//		Basic data type definitions
//***********************************************************

#ifndef PCI_CODE
	#define PCI_CODE
#endif

#ifndef LITTLE_ENDIAN			// LITTLE_ENDIAN: least significant byte stored first
	#define LITTLE_ENDIAN		// BIG_ENDIAN:    most significant byte stored first
#endif

#ifndef PI
	#define PI					3.14159265358979
#endif


//***********************************************************
//		C Library version information
//***********************************************************

#define C_LIBRARY_RELEASE		0x260	// C Library release number
#define C_LIBRARY_BUILD			0x7		// C Library build number


//***********************************************************
//		Module specifications
//***********************************************************

#define PRESET_MAX_MODULES		17			// Preset total number of modules allowed in the system
#define PRESET_MAX_PXI_SLOTS	18			// Preset maximum number of PXI slots for one PXI chassis
#define NUMBER_OF_CHANNELS		4			// Number of channels for each module

#define GAIN_FACTOR				1			// Pixie voltage gain adjust factor
#define V_OFFSET_MAX			2.5			// Pixie voltage offset maximum
#define APA_V_GAIN				11.25		// Fixed voltage gain for APA Pixie-4 boards
#define	P500_HIGH_GAIN			2.9			// high/low gain for Pixie-500 Rev B boards
#define	P500_HIGH_SGA			0x0001
#define	P500_LOW_GAIN			1.0			// high/low gain for Pixie-500 Rev B boards
#define	P500_LOW_SGA			0x0000
#define DEFAULT_GAIN			1.0
#define	DEFAULT_SGA				0x0000

//***********************************************************
//		Number of variables and parameters
//***********************************************************

#define DSP_IO_BORDER			256		// Number of DSP I/O parameters
#define N_DSP_PAR				416		// Number of all DSP parameters
#define N_MEM_PAR				16384	// Number of DSP internal memory parameters
#define N_CHANNEL_PAR			64		// Number of channel dependent parameters
#define N_MODULE_PAR			64		// Number of module dependent parameters
#define N_SYSTEM_PAR			64		// Number of system dependent parameters
#define N_SGA_GAIN				128		// Number of SGA gains
#define N_BOOT_FILES			7		// Number of boot files


//***********************************************************
//		Data memory, circular and output buffer, histogram
//***********************************************************

#define DATA_MEMORY_ADDRESS		0x4000	// DSP data memory address
#define USER_MEMORY_ADDRESS		0x41A0	// Address of user data block in DSP data memory 
#define DATA_MEMORY_LENGTH		16384	// DSP data memory length
#define USER_MEMORY_LENGTH		1100	// Length of user data block in DSP data memory 
#define EVENT_BUFFER_LENGTH		4060	// Circular buffer length
#define IO_BUFFER_ADDRESS		24540//20476	// Address of I/O output buffer
#define IO_BUFFER_LENGTH		8192	// Length of I/O output buffer
#define BUFFER_HEAD_LENGTH		6		// Output buffer header length
#define EVENT_HEAD_LENGTH		3		// Event header length
#define MAX_CHAN_HEAD_LENGTH		9		// Largest channel header length
#define MAX_HISTOGRAM_LENGTH	32768	// Maximum MCA histogram length

#define HISTOGRAM_MEMORY_LENGTH   131072	// external histogram memory length (32bit wide)
#define LIST_MEMORY_LENGTH        131072	// external list mode memory length (32-bit wide)
#define HISTOGRAM_MEMORY_ADDRESS  0x0		// histogram memory buffer in external memory   	
#define LIST_MEMORY_ADDRESS       0x20000	// list mode buffer in external memory
#define LM_DBLBUF_BLOCK_LENGTH    65536  	// lenght of one block in external memory in double buffer mode


#define PCI_CFDATA				  0x00		// PCI address for Data Register in Config FPGA (write only)
#define PCI_CFCTRL				  0x04		// PCI address for Control register in Config FPGA (write only)
#define PCI_CFSTATUS			  0x08		// PCI address for Status Register in Config FPGA (read only)
#define PCI_VERSION				  0x0C		// PCI address for Version Register in Config FPGA/PROM)
#define PCI_I2C				      0x10		// PCI address for I2C I/O and test bits
#define PCI_PULLUP				  0x14		// PCI address for register controlling backplane pullups (write only)
#define PCI_CSR					  0x48		// PCI address for Cnotrol/Status Register
#define PCI_WCR					  0x4C		// PCI address for Word Count Register
#define PCI_IDMAADDR			  0x80		// PCI address for IDMA address write (prior to read/write from/to DSP memory)
#define PCI_IDMADATA			  0x84		// PCI address for IDMA data (read/write from/to DSP memory)
#define PCI_SP1				      0x88		// PCI address reserved for special purposes
#define PCI_EMADDR				  0xC0		// PCI address for external memory address write (prior to read/write from/to EM)
#define PCI_EMDATA				  0x100000	// PCI address for external memory data (read/write from/to EM)


//***********************************************************
//		Length of communication FPGA, Fippi, and DSP configurations
//***********************************************************

#define N_COMFPGA_BYTES			166980	// Communication FPGA file
#define N_FIPPI_BYTES			166980	// FIPPI file
#define N_DSP_CODE_BYTES		65536	// DSP code file
#define EEPROM_MEMORY_SIZE		2048	// Memory size in bytes of EEPROM chip 
#define N_VIRTEX_BYTES			977488	// Fippi (P500)



//***********************************************************
//		Limits of parameter and file name length
//***********************************************************

#define MAX_FILE_NAME_LENGTH	1024	// Maximum length of file names
#define MAX_PAR_NAME_LENGTH		65		// Maximum length of parameter names

//***********************************************************
//		Switches for downloading configurations
//***********************************************************

#define FIPPI_CONFIG			1		// FIPPI configuration
#define DSP_CODE				2		// DSP code
#define DSP_PARA_VAL			3		// DSP parameter values
#define COM_FPGA_CONFIG_REV_B	4		// Communication FPGA configuration (Rev. B)
#define COM_FPGA_CONFIG_REV_C	5		// Communication FPGA configuration (Rev. C)
#define VIRTEX_CONFIG			6		// P500 Virtex configuration

#define DSP_PARA_NAM			0		// DSP parameter names
//#define DSP_MEM_NAM			1		// DSP internal memory parameter names


//***********************************************************
//		Data transfer direction
//***********************************************************

#define DMATRANSFER_TIMEOUT		5*1000		// PLX DMA transfer timeout limit in ms
#define MOD_READ				1		// Host read from modules
#define MOD_WRITE				0		// Host write to modules  


//***********************************************************
//		Frequently used Control Tasks
//***********************************************************

#define SET_DACS					0	// Set DACs
#define ENABLE_INPUT					1	// Enable detector signal input
#define RAMP_TRACKDAC					3	// Ramp TrackDAC
#define GET_TRACES					4	// Acquire ADC traces
#define PROGRAM_FIPPI					5	// Program FIPPIs
#define COLLECT_BASES					6	// Collect baselines
#define GET_BLCUT					0x80	// Compute BLcut (C function compared to other codes referring to DSP functions)
#define FIND_TAU					0x81	// Compute tau
#define ADJUST_OFFSETS					0x83	// Adjust module offsets
#define ACQUIRE_ADC_TRACES				0x84	// Acquire ADC traces from the module
#define READ_EEPROM_MEMORY				0x100	// Read the entire contents of EEPROM memory
#define WRITE_EEPROM_MEMORY				0x101	// Write the entire contents of EEPROM memory
#define WRITE_EEPROM_MEMORY_SHORT		0x102	// Write only 64 bytes to EEPROM memory
#define READ_NEC_RUNSTATS				0x110	// Read a section of DSP memory and return 8x run statistics	

//***********************************************************
//		Run Type and Polling
//***********************************************************

#define NEW_RUN					1		// New data run
#define RESUME_RUN				0		// Resume run

#define NO_POLL					0		// No polling
#define AUTO_POLL				1		// Auto polling


//***********************************************************
//		MODULE CSRA, CSR and DBLBUFCSR bit masks
//***********************************************************

#define MODCSRA_EMWORDS			1		// Write list mode data to EM
#define MODCSRA_PULLUP			2		// Enable pullups on the trigger line
#define DBLBUFCSR_ENABLE		0		// Enable double buffer mode
#define DBLBUFCSR_READ          0x0002	// OR in this to notify DSP host has read
#define DBLBUFCSR_128K_FIRST    3		// Set by DSP to indicate whick block to read first
#define CSR_128K_FIRST			10		// Set by DSP to indicate whick block to read first in CSR
#define CSR_ODD_WORD			11		// Set by DSP to indicate odd number of words in EM
#define CSR_ACTIVE				13		// Set by DSP to indicate run in progress
#define CSR_DATAREADY			14		// Set by DSP to indicate data is ready. Cleared by reading WCR

//***********************************************************
//		Pixie-4/P500 variant definitions
//***********************************************************

#define P4_LTSCALE				16			// The scaling factor for live time counters
#define P4_SYSTEM_CLOCK_MHZ		75			// System, DSP clock in MHz
#define P4_FILTER_CLOCK_MHZ		75			// clock for pulse processing in FPGA in MHz
#define P4_ADC_CLOCK_MHZ		75			// digitization clock in ADC. 
#define P500_LTSCALE			32			// The scaling factor for live time counters
#define P500_SYSTEM_CLOCK_MHZ	75			// System, DSP clock in MHz
#define P500_FILTER_CLOCK_MHZ	125			// clock for pulse processing in FPGA in MHz
#define P500_ADC_CLOCK_MHZ		500			// digitization clock in ADC. 
#define P400_LTSCALE			32			// The scaling factor for live time counters
#define P400_SYSTEM_CLOCK_MHZ	75			// System, DSP clock in MHz
#define P400_FILTER_CLOCK_MHZ	100			// clock for pulse processing in FPGA in MHz
#define P400_ADC_CLOCK_MHZ		400			// digitization clock in ADC. 



#ifdef __cplusplus
}
#endif

#endif	/* End of defs.h */


