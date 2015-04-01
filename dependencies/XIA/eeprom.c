#include <stdio.h>
#include <math.h>
#include <time.h>

#ifdef XIA_LINUX
#include <stdlib.h>
#include <unistd.h>
#include <sys/times.h>
#endif

#include "PlxApi.h"
#include "PciTypes.h"
#include "Reg9054.h"
#include "Plx.h"
#include "PexApi.h"

#include "globals.h"
#include "sharedfiles.h"


/* ----------------------------------------------------- */
/* wait_for_a_short_time:                                */
/*   A shot wait (the time for each cycle = ns_per_cycle)*/
/* ----------------------------------------------------- */

void wait_for_a_short_time(S32 t)
{
    S32 cycles = (S32)((double)t / One_Cycle_Time);
    while (cycles >= 0) cycles--;
}


/* ----------------------------------------------------- */
/* get_ns_per_cycle:                                     */
/*   Measure how many ns for each cycle                  */
/* ----------------------------------------------------- */

#ifdef XIA_WINDOZE
S32 get_ns_per_cycle(double *ns_per_cycle)
{
	U32 NumCycles, count;
	clock_t start, finish;

	count = NumCycles = 100000000;
	if ((start  = clock()) < 0) return (-1);
	while (count > 0) count--;
	if ((finish = clock()) < 0) return (-1);
	*ns_per_cycle = 10.0 * (double)(finish - start) / (double)CLOCKS_PER_SEC;

	return(0);
}
#endif

#ifdef XIA_LINUX
S32 get_ns_per_cycle(double *ns_per_cycle)
{
clock_t tn1;
clock_t tn2;
struct tms tm1;
struct tms tm2;

U32 NumCycles;
S32 count;
double duration;
long clk_res;

  NumCycles = 100000000;
  count = NumCycles;

  tn1 = times(&tm1);
  do {
    count--;
    } while (count >= 0);

  tn2 = times(&tm2);
  duration = tn2 - tn1;
  clk_res = sysconf(_SC_CLK_TCK);
  duration = duration * (1.0/clk_res);
  duration = duration / (double)NumCycles;
  duration *= 1.0e9;

//  *ns_per_cycle = ceil(duration);		// this makes the delay too short 'cause it rounds upward
  *ns_per_cycle = floor(duration);		// this rounds downward so if anything the delay will be too long

  return(0);
}
#endif


/* ----------------------------------------------------- */
/* I2C24LC16B_init:                                      */
/*   Bus master initializes 24LC16B                      */
/* ----------------------------------------------------- */

S32 I2C24LC16B_init(U8 ModNum)
{
	U32 buffer[4]; 

	buffer[0] = 0x7;	/* SDA = 1; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	return(0);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_start:                                     */
/*   Bus master sends "START" to 24LC16B                 */
/* ----------------------------------------------------- */

S32 I2C24LC16B_start(U8 ModNum)
{
	U32 buffer[4]; 

	//***************************
	//	Set SCL to 1, SDA to 1 and CTRL to 1 (write)
	//***************************

	buffer[0] = 0x7;	/* SDA = 1; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	//***************************
	//	Set SDA to 0
	//***************************

	buffer[0] = 0x6;	/* SDA = 0; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);
	
	/* Wait for 600 ns */
	wait_for_a_short_time(600);

	return(0);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_stop:                                      */
/*   Bus master sends "STOP" to 24LC16B                  */
/* ----------------------------------------------------- */

S32 I2C24LC16B_stop(U8 ModNum)
{
	U32 buffer[4]; 

	//***************************
	//	Set SCL to 1
	//***************************

	buffer[0] = 0x6;	/* SDA = 0; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);
	
	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	//***************************
	//	Set SDA to 1
	//***************************

	buffer[0] = 0x7;	/* SDA = 1; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 600 ns */
	wait_for_a_short_time(600);

	return(0);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_byte_send:                                 */
/*   Bus master sends a byte to 24LC16B                  */
/* ----------------------------------------------------- */

S32 I2C24LC16B_byte_send(U8 ModNum, U8 ByteToSend)
{
	S16 i;
	U32 buffer[4];

	buffer[0] = 0x4;	/* SDA = 0; SCL = 0; CTRL = 1 */

	for(i = 7; i >= 0; i --)
	{
		//***************************
		//	Set SCL to 0
		//***************************

		buffer[0] = ClrBit(1, (U16)buffer[0]);
		Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

		/* Wait for 600 ns */
		wait_for_a_short_time(600);

		//***************************
		//	Send bit i
		//***************************
		
		if(TstBit((U8)i, ByteToSend) == 1)
		{
			buffer[0] = 0x5;	/* SDA = 1; SCL = 0; CTRL = 1 */
		}
		else
		{
			buffer[0] = 0x4;	/* SDA = 0; SCL = 0; CTRL = 1 */
		}
		Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

		//***************************
		//	Set SCL to 1
		//***************************
		buffer[0] = SetBit(1, (U16)buffer[0]);
		Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

		/* Wait for 1200 ns */
		wait_for_a_short_time(1200);

	}

	//************************************************************
	//	Set SCL to 0, also CTRL to release bus for acknowledge
	//***********************************************************

   	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	buffer[0] = (buffer[0] & 0x1);
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	return(0);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_byte_receive:                              */
/*   Bus master receives a byte from 24LC16B             */
/* ----------------------------------------------------- */

S32 I2C24LC16B_byte_receive(U8 ModNum, U8 *ByteToReceive)
{
	S16 i;
	U32 buffer[4];
	U8 ByteReceived;

	buffer[0] = 0x0;	/* SDA = 0; SCL = 0; CTRL = 0 */

	for(i = 7; i >= 0; i --)
	{
		//***************************
		//	Set SCL to 1
		//***************************

		buffer[0] = (U32)SetBit(1, (U16)buffer[0]);
		Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

        /* Wait for 600 ns */
		wait_for_a_short_time(600);

		//***************************
		//	Receive bit i
		//***************************
        
        buffer[0] = 0x0;

		Pixie_Register_IO(ModNum, PCI_I2C, MOD_READ, buffer);
		ByteReceived = (U8)buffer[0];

		if(TstBit((U8)0, ByteReceived) == 1)
		{
			*ByteToReceive = (U8)SetBit(i, *ByteToReceive);
		}
		else
		{
			*ByteToReceive = (U8)ClrBit(i, *ByteToReceive);
		}

		/* Wait for 200 ns */
		wait_for_a_short_time(200);

		//***************************
		//	Set SCL to 0
		//***************************
		buffer[0] = (U32)ClrBit(1, (U16)buffer[0]);
		Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

		/* Wait for 1200 ns */
		wait_for_a_short_time(1200);

	}

	return(0);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_getACK:                                    */
/*   Bus master receives ACKNOWLEDGE from 24LC16B        */
/* ----------------------------------------------------- */

U8 I2C24LC16B_getACK(U8 ModNum)
{
	U32 buffer[4];
	U8 retval;

	//***************************
	//	Set SCL to 1
	//***************************

	buffer[0] = 0x2;	/* SDA = 0; SCL = 1; CTRL = 0 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	//***************************
	//	Read SDA
	//***************************
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_READ, buffer);
	retval = (U8)(buffer[0] & 0x1);

	//***************************
	//	Set SCL to 1
	//***************************

	buffer[0] = 0x4;	/* SDA = 0; SCL = 1; CTRL = 1 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);
	
	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	return(retval);
}


/* ----------------------------------------------------- */
/* I2C24LC16B_getACK_before_read:                        */
/*   Bus master receives ACKNOWLEDGE from 24LC16B        */
/*   keep CTRL = 0 to leave bus to memory for reading    */
/* ----------------------------------------------------- */

U8 I2C24LC16B_getACK_before_read(U8 ModNum)
{
	U32 buffer[4];
	U8 retval;

	//***************************
	//	Set SCL to 1
	//***************************

	buffer[0] = 0x2;	/* SDA = 0; SCL = 1; CTRL = 0 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);

	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);

	//***************************
	//	Read SDA
	//***************************
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_READ, buffer);
	retval = (U8)(buffer[0] & 0x1);

	//***************************
	//	Set SCL to 0
	//***************************

	buffer[0] = 0x0;	/* SDA = 0; SCL = 0; CTRL = 0 */
	Pixie_Register_IO(ModNum, PCI_I2C, MOD_WRITE, buffer);
	
	/* Wait for 1200 ns */
	wait_for_a_short_time(1200);
   
	return(retval);
}


S32 I2C24LC16B_Read_One_Byte (
		U8 ModNum,             // Pixie module number
		U16 Address,            // The address to read this byte
		U8 *ByteValue )        // The byte value
{
	U8 IOByte;
	U8 ackvalue, blocknum, blockaddr;;

	// Check if CardNo is valid
	if(ModNum >= Number_Modules)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Read_One_Byte): invalid Pixie module number %d", ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	// Extract I2C24LC16B segment number
	blocknum = (U8)((float)Address / 256.0);
	blockaddr = (U8)(fmod((double)Address, 256.0));

	// Initialize I2C24LC16B
	I2C24LC16B_init(ModNum);

	// Send "START"
	I2C24LC16B_start(ModNum);

	// Send Control Byte
	IOByte = (U8)(0xA0 | (blocknum * 2));
	I2C24LC16B_byte_send(ModNum, IOByte);
	
	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Read_One_Byte): Failure to get Acknowledge after sending control byte");
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}

	// Send address
	IOByte = blockaddr;
	I2C24LC16B_byte_send(ModNum, IOByte);
	
	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Read_One_Byte): Failure to get Acknowledge after sending address");
		Pixie_Print_MSG(ErrMSG);
		return(-3);
	}

	// Send "START"
	I2C24LC16B_start(ModNum);

	// Send Control Byte
	IOByte = (U8)(0xA1 | (blocknum * 2));
	I2C24LC16B_byte_send(ModNum, IOByte);
	
	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK_before_read(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Read_One_Byte): Failure to get Acknowledge after sending second control byte");
		Pixie_Print_MSG(ErrMSG);
		return(-4);
	}

	// Receive one byte
	I2C24LC16B_byte_receive(ModNum, ByteValue);

	// Send "STOP"
	I2C24LC16B_stop(ModNum);

	return(0);
}


S32 I2C24LC16B_Write_One_Byte (
				U8 ModNum,             // Pixie card number
				U16 Address,            // The address to write this byte
				U8 *ByteValue )        // The byte value
{
	U8 IOByte;
	U8 ackvalue, blocknum, blockaddr;

	// Check if ModNum is valid
	if(ModNum >= Number_Modules)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Write_One_Byte): Invalid Pixie card number %d", ModNum);
		Pixie_Print_MSG(ErrMSG);
		return(-1);
	}

	// Extract I2C24LC16B segment number
	blocknum = (U8)((float)Address / 256.0);
	blockaddr = (U8)(fmod((double)Address, 256.0));

	// Initialize I2C24LC16B
	I2C24LC16B_init(ModNum);

	// Send "START"
	I2C24LC16B_start(ModNum);

	// Send Control Byte
	IOByte = 0xA0 | (blocknum * 2);
	I2C24LC16B_byte_send(ModNum, IOByte);
		
	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Write_One_Byte): Failure to get Acknowledge after sending control byte");
		Pixie_Print_MSG(ErrMSG);
		return(-2);
	}

	// Send address
	IOByte = blockaddr;
	I2C24LC16B_byte_send(ModNum, IOByte);

	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Write_One_Byte): Failure to get Acknowledge after sending address");
		Pixie_Print_MSG(ErrMSG);
		return(-3);
	}

	// Send byte value
	IOByte = *ByteValue;
	I2C24LC16B_byte_send(ModNum, IOByte);

	// Get Acknowledge
	ackvalue = I2C24LC16B_getACK(ModNum);
	if(ackvalue != 0)
	{
		sprintf(ErrMSG, "*Error* (I2C24LC16B_Write_One_Byte): Failure to get Acknowledge after sending byte value");
		Pixie_Print_MSG(ErrMSG);
		return(-4);
	}
	
	// Send "STOP"
	I2C24LC16B_stop(ModNum);

	return(0);
}


