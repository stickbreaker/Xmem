/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 
 Modified by Chuck Todd to allow interrupt driven Block Transfers.
 
 Added Read, write to allow overlapped accesses, about 12% faster 
  
 08JUN14 Problem
   if buffer size is more than 64 bytes, maybe 128 wierd stuff happens, 
   buffer.count stalls at buffersize-1 ?
   
  08JUN14 ReWrote to allow bigger buffers and multi block transfers 
  09JUN14 Fixed bug in MULTI block transfer logic
     Works with big buffers, 256+
	 Nolonger needs Static buffer, or Struc Buffer, Just char c[x]
	
  10JUN14 
     1. readVerify() to compare bpptr to read stream without destroying bpptr
	 2. added modes, 
	    writeonly : ignore the input stream, leave bpptr data alone
		multi     : don't release CS, and execute registered callback on completion if possible
		blocking  : ****to be implemented*****  hang until transfer completed.
		nonblocking : *** to be implemented *** quick return
		
  12JUN14
     1. added verifyWriteable() to validate EPPROM overwrite.  
	 2. readVerify and verifyWriteable abort comparison when  nonwriteable byte is encountered
	    if writeOnly is set, else transfer runs to completion.
	 3. beginTransfer now hangs if previous transfer has not completed 
	 
 */

#ifndef _MY_SPI_H_INCLUDED
#define _MY_SPI_H_INCLUDED

#include <stdio.h>
#include <Arduino.h>
#include <avr/pgmspace.h>

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06
//#define SPI_CLOCK_DIV64 0x07

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

/*

Read and Write will hang forever if no SPI transmission in progress or completed.
This method results in about a 12% speed improvement over Standard transfer method. if anything else can be done between calls to 
write or read the preformance will increase.

standard usage  individual uint8_t transfers.
   SPI.begin(); // enable SPI interface;
   SPI.setBitOrder(!LSBORDER);  // Set msb out first
   SPI.setDataMode(SPI_MODE0);   // set data valid on Rising edge of clock, Positive Clock
   SPI.setClockDivider(SPI_CLOCK_DIV4);  // Standard Fast Clock rate, about 4Mb on 16Mhz clock

   digitalwrite(SS,LOW); // this sequence set a 23LC512 for Sequencial access 
   SPI.write(0x01,0); // first call 
   SPI.write(0x40); //all subsiquence calls
   
   while(!(SPSR&_BV(SPIF))); // wait for last SPI.write to complete
   digitalWrite(SS,HIGH); // end write to Mode flags

   digitalWrite(SS,LOW);
   SPI.write(0x03,0); // read command for 23LC512
   SPI.write(0x00);   // MSB for 16bit address
   SPI.write(0x00);   // LSB for 16bit address
   SPI.write(0);	// transfer requested uint8_t to SPDR
   a=SPI.read(0);  // 'a' returns value received during previous write transfer, and starts next transfer to load SPDR for next read.
                   // read is always one transfer behind.
   //don't need to wait for transfer to complete, the current transfer is for the next uint8_t
   digitalWrite(SS,HIGH);  // Release Slave, complete read command.
   SPI.end();  // Shutdown SPI interface  

Buffered block tranfer method

   #define spibufsize 256 // max 65535
   char spibuff[spibufsize]
  
  //Fill spibuffer with data to be sent, content will be replace with data from device
  // transfersize is total number of bytes to exchange, Read and Write, spibuff must be at least as 
  // large as transfersize or bad thing will happen.
  // cs_pin is arduino pin used as !cs control for SPI device
  // the last parameter is false for single block transfers.
  // if the last parameter is true, a callback must have been registered to support multi block transfers
  // or the programmer must use finished() to manually append additional blocks. the final block must
  // have this parameter set to false to cause cs_pin to be brought HIGH after the transmission completes.
  
   
  SPI.beginTransfer( spibuff,transfersize,cs_pin,false); assign transfer buffer for Interrupt driven transfer
  
  // do other stuff while SPI Transfer happens in the back ground.
  // before accessing spibuf, test for transfer completion

  while(!SPI.finished()) ; // wait until transfer complete
  
  // spibuf now contains received data, access buf directly.  
      	  
	  The SPI hardware is simultaniously sending and receiving to the save buffer.
	  the buffer offsets for valid data will depend on each Specific Device, Read The Device SpecSheet for detail.
	  as an example, a MicroChip 23LC512 SPI Ram will be accessed with the following sequence.

Buffered block tranfer method

  buffer consisting of the following structure
 
 #define bufsize 20
  char spibuff[bufsize];
  
  // this is an example of how to write the 50 bytes of zeros at address 1234 (0x4D2) of a Microchip 23LC512 SPI RAM
  SPI.setBitOrder(!LSBORDER);  		// Set msb out first
  SPI.setDataMode(SPI_MODE0);   	// set data valid on Rising edge of clock, Positive Clock
  SPI.setClockDivider(SPI_CLOCK_DIV4);  // Standard Fast Clock rate, about 4Mb on 16Mhz clock

  spibuff.buf[0] = 1; 				// mode cmd
  spibuff.buf[1] = 0x40; 			// sequencial access
 
  SPI.beginTransfer(spibuff,2,SS_23LC512,false);
 
 // do other processing if possible

  while(!SPI.finished()) ;			// hang in loop waiting for Transfer to complete
 
 uint16_t adr=1234;
  spibuff[0]=2;					// write command for SPI Ram
  spibuff[1]=highByte(adr);		// spi ram expect high address uint8_t first
  spibuff[2]=lowByte(adr);
  for(uint8_t i=0;i<16;i++){
     spibuff[3+i]=0;
	 }
  
  SPI.registerCallback(void);  // deregister callback, just leave CS LOW after final byte transfers.
  
  SPI.beginTransfer(spibuff,19,SS_23LC512,true); //send write cmd +data, multi tranfer
 	 
  while(!SPI.finished()) ;
  for(uint8_t i=0;i<20;i++){
   spibuff[i]=0;
   }  
   
  SPI.beginTransfer(spibuff,20,SS_23LC512,true); // send next 20 bytes for a total of 36 bytes sent, multi transfer
  
  while(!SPI.finished()) ;
  for(uint8_t i=0;i<20;i++){
   spibuff[i]=0;
   }  
  
  SPI.beginTransfer(spibuff,14,SS_23LC512,false); //a total of 50 bytes send, last transfer, complete transaction
  
  while(!SPI.finished());			// wait for tranfer to complete.
	  
  SPI.end();  						// detach interrupt and powerdown SPI Hardware.
  
  // This transaction was broken into 3 pieces because of my self imposed 20 byte transaction buffer size.
  //  if the buffer was large enough for the complete transaction.  It could have been completed in 2 
  //  operations. 
  //	1. the sequencial mode command
  //	2. the write command
	   
	    
  // The SPI Routine beginTranser handles the SS signal, The last call to beginTransfer must have the MULTI parameter
  // set false to complete the transaction.
   
   */
   

class SPIClass {
public:
  enum  SPImode { multi=1, writeonly=2, blocking=4, nonblocking=8}; 
   // blocking,nonblocking not implemented yet 11jun14
  
  static void begin(); // Default initialize hardware, and MOSI, MOSI, SCK and SS
  static void end();   // Poweroff SPI Hardware, Release interrupt.
  
  void beginTransfer( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode=0);
   //if writeonly is set then discard received SPDR, don't overWrite bpptr buffer data
   // waits for previous transfer to completed before starting.
  
  uint16_t readVerify( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode=4); 
    // returns:
    // 0 : complete match
    // 1..bsize : (pos+1) of miss match
	// hangs until complete
	// if mode is not writeonly, bpptr is changed to the input stream. Just like beginTransfer.
	
  uint16_t verifyWriteable( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode=4);
	// hangs until complete
	
  bool finished(void );

  void registerCallback( void (*callback)(void));  // use registerCallback(NULL); to disable this feature.
  
  inline static uint8_t transfer(uint8_t _data);	// single uint8_t tranfer, sends uint8_t and receives a uint8_t.
 
 // overlapped read/write routine, 
  uint8_t read(uint8_t _data, uint8_t _pending=1); 	// _pending=0 for first uint8_t
                                            // _pending=1 for overlapped
 
  void write(uint8_t _data, uint8_t _pending=1);

    // SPI Configuration methods
  
  inline static void attachInterrupt();
  inline static void detachInterrupt(); // Default
  
  static void setBitOrder(uint8_t);
  static void setDataMode(uint8_t);
  static void setClockDivider(uint8_t);
  
  // helper routines for MicroChip SPI I/O Expander
  
  uint8_t  MCP23S17_init(uint8_t cs, uint8_t spiaddr, uint16_t iodir, uint16_t initval, bool sequential);
  uint16_t MCP23S17_read_word  (uint8_t cs, uint8_t spiaddr, uint8_t addr);
  uint8_t  MCP23S17_read_byte  (uint8_t cs, uint8_t spiaddr, uint8_t addr);
  uint8_t  MCP23S17_write_word (uint8_t cs, uint8_t spiaddr, uint8_t addr, uint16_t din);
  uint8_t  MCP23S17_write_byte (uint8_t cs, uint8_t spiaddr, uint8_t addr, uint8_t  din);
  uint8_t  MCP23S17_write_block(uint8_t cs, uint8_t spiaddr, uint8_t addr, char *   din, uint8_t len);
  
  
  };

extern SPIClass SPI;
extern char * SPIBP;						// storage for point to user supplied buffer.
extern volatile uint16_t SPIPOS;
extern volatile uint16_t SPICOUNT; 
extern volatile bool SPIMULTI;
extern volatile uint8_t SPIPIN;
extern volatile bool SPIVERIFY;
extern volatile uint16_t SPIVERIFYPOS;
static void (*SPIMULTI_callback)(void);
extern volatile bool SPIWRITEONLY;
extern volatile uint16_t SPIVERIFYWRITEPOS;

#endif
