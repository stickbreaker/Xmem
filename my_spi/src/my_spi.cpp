/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.

 * Modified by Chuck Todd for interrupt driven transfers and Overlapping transfers 2013

 */

#include "pins_arduino.h"
#include "my_spi.h"

SPIClass SPI;

char * SPIBP;						// storage for point to user supplied buffer.
volatile uint16_t SPIPOS;
volatile uint16_t SPICOUNT; 
volatile bool SPIMULTI;
volatile uint8_t SPIPIN;
volatile bool SPIVERIFY;
volatile uint16_t SPIVERIFYPOS;
volatile bool SPIWRITEONLY;
volatile uint16_t SPIVERIFYWRITEPOS;

void SPIClass::beginTransfer( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode){
//SPIClass::begin();
//delay(250);
//Serial.print("SPI ^=");Serial.print((word)bpptr,HEX);Serial.print(' ');Serial.print(bsize,HEX);
//Serial.print(' ');Serial.println(mode);
while(!finished()); // wait for prior call to complete

SPIBP = bpptr;
SPIPOS = 0;
SPICOUNT = bsize;
SPIPIN = pin;
SPIMULTI = (mode & SPIClass::multi)==SPIClass::multi;
SPIWRITEONLY = (mode & SPIClass::writeonly)==SPIClass::writeonly;
if(SPICOUNT>1){ //start actual transfer
  digitalWrite(SPIPIN,LOW);
  SPDR=SPIBP[SPIPOS];
  SPICOUNT--;
  SPI.attachInterrupt();
  }
 else if(SPICOUNT==1){
  digitalWrite(SPIPIN,LOW);
  uint8_t b=transfer(SPIBP[0]);
  if(SPIVERIFY){
    if(!(b==SPIBP[SPIPOS])){
      if(!SPIVERIFYPOS) SPIVERIFYPOS=SPIPOS+1;
	  if(!(SPIBP[SPIPOS]==(b&SPIBP[SPIPOS]))) if(!SPIVERIFYWRITEPOS) SPIVERIFYWRITEPOS=SPIPOS+1;
	  }
	}
  if(!SPIWRITEONLY)SPIBP[0]=b;
  SPICOUNT=0;
  if(!SPIMULTI){
    digitalWrite(SPIPIN,HIGH);
	}
  else if(SPIMULTI_callback) SPIMULTI_callback();
 }
}

 
uint16_t SPIClass::readVerify( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode){
    // returns:
    // 0 : complete match
    // 1..bsize : (pos+1) of miss match
	// hangs until complete
SPIVERIFYPOS =0;
SPIVERIFYWRITEPOS=0;
SPIVERIFY=true;	
void (*SPIMULTI_callback_save)(void);
SPIMULTI_callback_save = SPIMULTI_callback;
registerCallback(NULL);

beginTransfer( bpptr, bsize, pin, mode);
while(!finished());
uint16_t possave=SPIVERIFYPOS;
if((multi)&&(SPIMULTI_callback_save)) SPIMULTI_callback_save();
registerCallback(SPIMULTI_callback_save);
SPIVERIFY=false;
return possave;
}

uint16_t SPIClass::verifyWriteable( char * bpptr, uint16_t bsize, uint8_t pin, uint8_t mode){
    // returns:
    // 0 : complete match
    // 1..bsize : (pos+1) of miss match
	// hangs until complete
SPIVERIFYPOS =0;
SPIVERIFYWRITEPOS=0;
SPIVERIFY=true;	
//SPIWRITEONLY=true;
void (*SPIMULTI_callback_save)(void);
SPIMULTI_callback_save = SPIMULTI_callback;
registerCallback(NULL);
beginTransfer( bpptr, bsize, pin, mode);
while(!finished());
uint16_t possave=SPIVERIFYWRITEPOS;
if((multi)&&(SPIMULTI_callback_save)) SPIMULTI_callback_save();
registerCallback(SPIMULTI_callback_save);
SPIVERIFY=false;
//Serial.print("SPI::vW=");Serial.println(SPIVERIFYWRITEPOS);
return possave;
}

void SPIClass::registerCallback( void (*callback)(void)){
SPIMULTI_callback = callback; 
}

bool SPIClass::finished(void){
bool f;
f=(SPIPOS==0)&&(SPICOUNT==0);
return f;
}


ISR(SPI_STC_vect){
byte b;
b=SPDR;
if(SPIVERIFY){
  if(!((byte)b==(byte)SPIBP[SPIPOS])) {
    if(!SPIVERIFYPOS) SPIVERIFYPOS=SPIPOS+1;
    if(!((byte)SPIBP[SPIPOS]==((byte)b&(byte)SPIBP[SPIPOS]))){
      if(!SPIVERIFYWRITEPOS) SPIVERIFYWRITEPOS=SPIPOS+1;
	  //because of verify mode, and only writeable to here abort remainder of transfer, wasted effort
	  if(SPIWRITEONLY) SPICOUNT=0; // terminate tranfer if Write only, (buffer is not affected);
	  }
	}
  }
if(!SPIWRITEONLY)SPIBP[SPIPOS]=b; // Save Previous Received

if(SPICOUNT>0){// more to send
  SPIPOS++;
  SPDR = SPIBP[SPIPOS]; // send next uint8_t
  SPICOUNT--;  // decrease count
  }
else{
  SPIPOS = 0;
  SPI.detachInterrupt();
  if(!SPIMULTI){// transfer done, complete transaction.
    digitalWrite(SPIPIN,HIGH);
	} 
  else{
    if (SPIMULTI_callback)SPIMULTI_callback();
    }
  }
}

void SPIClass::begin() {
  // Set direction register for SCK and MOSI pin.
  // MISO pin automatically overrides to INPUT.
  // When the SS pin is set as OUTPUT, it can be used as
  // a general purpose output port (it doesn't influence
  // SPI operations).
  SPIMULTI_callback = NULL;
  SPIVERIFY = false;
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SS, OUTPUT);
  
  digitalWrite(SCK, LOW);
  digitalWrite(MOSI, LOW);
  digitalWrite(SS, HIGH);

  // Warning: if the SS pin ever becomes a LOW INPUT then SPI 
  // automatically switches to Slave, so the data direction of 
  // the SS pin MUST be kept as OUTPUT.
  SPCR |= _BV(MSTR);
  SPCR |= _BV(SPE);
}

void SPIClass::end() {
  SPCR &= ~_BV(SPE);
  SPI.detachInterrupt();
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
  if(bitOrder == LSBFIRST) {
    SPCR |= _BV(DORD);
  } else {
    SPCR &= ~(_BV(DORD));
  }
}

void SPIClass::setDataMode(uint8_t mode)
{
  SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

void SPIClass::setClockDivider(uint8_t rate)
{
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

uint8_t SPIClass::read(uint8_t _data, uint8_t _pending){
if(!(_pending==0)){
 while (!(SPSR & _BV(SPIF)))
   ; // wait for Transfer to complete
 }
SPDR = _data;
return SPDR;  // read previous uint8_t
}

void SPIClass::write(uint8_t _data, uint8_t _pending){ //if (_pending ==0) send _data else wait for previous uint8_t to be transfered then send _data. 
if(!(_pending==0)){
  while (!(SPSR & _BV(SPIF)))
    ;  // Wait for current transfer to complete
 }
 SPDR = _data;  // next uint8_t out
}


uint8_t SPIClass::transfer(uint8_t _data) {
  SPDR = _data;
  while (!(SPSR & _BV(SPIF)))
    ;
  return SPDR;
}

void SPIClass::attachInterrupt() {
  SPCR |= _BV(SPIE);
}

void SPIClass::detachInterrupt() {
  SPCR &= ~_BV(SPIE);
}

uint16_t SPIClass::MCP23S17_read_word(uint8_t cs, uint8_t spiaddr, uint8_t addr){
uint16_t bf[2];
spiaddr &= 0x27;
spiaddr = (spiaddr <<1) +1; // read
bf[0]=(uint16_t)spiaddr+((uint16_t)addr<<8);
beginTransfer((char*)&bf, 4, cs);
while(!finished());
return bf[1];
}

uint8_t SPIClass::MCP23S17_read_byte(uint8_t cs, uint8_t spiaddr, uint8_t addr){
char bf[3];
spiaddr &= 0x27;
spiaddr = (spiaddr <<1) +1; // read
bf[0]=(char)spiaddr;
bf[1]=(char)addr;
beginTransfer(bf, 3, cs);
while(!finished());
return (uint8_t)bf[2];
}

uint8_t SPIClass::MCP23S17_write_word(uint8_t cs, uint8_t spiaddr, uint8_t addr, uint16_t din){
char bf[4];
spiaddr &= 0x27;
spiaddr = (spiaddr <<1); // write
bf[0]=(char)spiaddr;
bf[1]=(char)addr;
bf[2]=(char)(din & 0xff);
bf[3]=(char)((din>>8) & 0xff);
/*
Serial.print("ww ");
for(uint8_t i=0;i<4;i++){Serial.print((uint8_t)bf[i],HEX);
  Serial.print(' ');
  }
Serial.println();  
*/
beginTransfer(bf, 4, cs);
while(!finished());
return 2; //count of bytes transfered
}

uint8_t SPIClass::MCP23S17_write_byte(uint8_t cs, uint8_t spiaddr, uint8_t addr,uint8_t din){
char bf[3];
spiaddr &= 0x27;
spiaddr = (spiaddr <<1); // write
bf[0]=(char)spiaddr;
bf[1]=(char)addr;
bf[2] =(char)din;
/*Serial.print("wb ");
for(uint8_t i=0;i<3;i++){Serial.print((uint8_t)bf[i],HEX);
  Serial.print(' ');
  }
Serial.println();  
*/
beginTransfer(bf, 3, cs);
while(!finished());
return 1; //bytes transfered
}

uint8_t SPIClass::MCP23S17_write_block(uint8_t cs,uint8_t spiaddr, uint8_t addr, char * din, uint8_t len){
char ch[2];
ch[0] =(char)((spiaddr&0x27)<<1);
ch[1] =(char)addr;
beginTransfer(ch,2,cs,SPIClass::multi);
beginTransfer(din,len,cs);
while(!finished());
return len;
}

uint8_t SPIClass::MCP23S17_init(uint8_t cs, uint8_t spiaddr, uint16_t iodir, uint16_t initval, bool sequential){
// cs = arduino pin for CS
// addr =  Address of chip
// iodir = io direction for pins, 1 for input, 0 for output (byte)
// initvalue = initial value for GPIO
// interrupts disabled, OC, Active Low, Pullups off
// returns 0 = success;
//         1 = data too large for buffer
//         2 = NACK during addresses
//         3 = NACK during DATA
//         4 = Other error
//         254 = No Response from Device, Device not Present
//         255 = invalid address

uint8_t success = 255; // unknown error
  if(spiaddr<0x20||spiaddr>0x27) return success; // out of possible Address range
  if(MCP23S17_write_byte(cs,spiaddr,0x0A,12)!=1)return 254;
  // Controller, Configuration Register,(hardware address enable, Sequencial, INT OC, INT Active Low)
  // sequential int the complete chip.
  char ch[20]={0xff,0xff,0,0,0,0,0,0,0,0,12,12,0,0,0,0,0,0,0,0};
  ch[18]=(char)(initval&0xff);
  ch[19]=(char)((initval>>8)&0xff);
  success=MCP23S17_write_block(cs,spiaddr,0,ch,20);
  // Now set the Pin Directions and change to NON-Sequencial access (every register must be explicitly addressed)
  if(success!=20) return 4; // error out
  else success=0;
  if(MCP23S17_write_word(cs,spiaddr,0,iodir)!=2) return 254; //  Controller,IO direction,defaults to all input
  if(sequential) return success; // don't need to change to no sequencial access
  if(MCP23S17_write_byte(cs,spiaddr,0xA,0x2C)!=1) return 254; // Controller,configuration register.[NON-Sequencial access, INT OC, INT Active Low]

  return success;

}
