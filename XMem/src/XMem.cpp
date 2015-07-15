/*
error Codes:
        0 = success
        1 = invalid pane
        2 = invalid page
        3 = not initialized
        5 = SPI hardware failure
 26JUL2014 Operational       
 
 */

#include <SPI.h>
#include <XMem.h>

XMem::XMem(void){   //constructor
initVars();
_flags=0;
}

XMem::~XMem(void){ //destructor
//move heap back to internal ram
disable();
}

void XMem::initVars(){
_CS=0;
_Addr=0;
_panes[0] = 0xC0;
for(uint8_t i=1;i<7;i++){
  _panes[i]=0xFF; // current pane to page mapping
  }
_frozen=0; // highest pane number for frozen heap  
}

void XMem::disable(){
if(_frozen&7){// heap in external ram 
  __malloc_heap_end   = _h.__malloc_heap_end;
  __malloc_heap_start = _h.__malloc_heap_start;
  __brkval            = _h.__brkval;
  __flp               = _h.__flp;  
  _frozen=0;
  }
// Disable Paging hardware
if(_CS){
  MCP23S17_write_word(_CS,_Addr,0x12,0xFFFF); //load latches with 255
  MCP23S17_write_byte(_CS,_Addr,0x13,0xC0);      // latch it, Disable Default
  _CS=0;
  }
initVars(); // init variables
XMCRA=0; // disable External Memory;  
}	
void XMem::setWait(uint8_t wait){ // call before init to set wait states
_flags = (_flags &0xFC)|(wait &3);
}	

void XMem::setHighHeap(bool hi){ 
/* call before init to set to use pages 64++ for heap
   defaults to use pages 0++ for heap
	 */
_flags = (_flags &0xFB);
if(hi) _flags = _flags | 4;
}

uint8_t XMem::init(uint8_t CS,uint8_t SPIAddr,uint8_t highestFrozenPane){ // 0..7, 8k pages used for heap
  // 0 means heap stays in internal mem
if(_CS){ // re-initing
  disable();
}
_CS = CS;
_Addr = (SPIAddr&0x27);

for(uint8_t i=0;i<7;i++){
  _panes[i]=255; // current pane to page mapping
  }
_frozen=(highestFrozenPane&7); // highest pane number for frozen heap
/* need to 
1. setup expander hardware
2. if Frozen >0 Move heap to frozen area.
enable mapping, assign pages 0..(frozen-1) to panes [1..frozen]
3. disable all panes above frozen (set page to 0xff)
4. save heap variables
5. change heap variables to correspond to frozen area
*/
if(_CS){
  digitalWrite(_CS,HIGH);
  pinMode(_CS,OUTPUT);
  SPI.begin();
  uint8_t er= MCP23S17_init(_CS,_Addr,0,0xFFFF,true); //
	if(!er){
		er = !((MCP23S17_read_byte(_CS,_Addr,0x12)==0xff)&&(MCP23S17_read_byte(_CS,_Addr,0x13)==0xff));
	}
  if(er){
//    Serial.print("MCP Init Failed=");Serial.println(er,DEC);Serial.flush();delay(1000);
    _CS=0;
    return 5;
    }
  MCP23S17_write_byte(_CS,_Addr,0x13,0xC0);// Latch 0xff into all latches
     // no default in pane 1
  uint8_t i=0;
  while(i<(_frozen&7)){
		if(_flags&4) setRam(i+1,fixPage(i+64)); // pages 64+ as heap
		else setRam(i+1,fixPage(i)); // pages 0+ as heap
		//map pages to panes for heap, enable HW if needed.
  	i++;
    }
  if((_frozen&7)>0){ // heap in xmem
 //   Serial.println("moving Heap to Xmem");Serial.flush();
    _h.__malloc_heap_end   = __malloc_heap_end;
    _h.__malloc_heap_start = __malloc_heap_start;
    _h.__brkval            = __brkval;
    _h.__flp               = __flp;  
    uint16_t max=(paneToAddr((_frozen&7))&0xe000)+0x2000; // max heap_end must be > heap_start, if all
		   // panes allocated max =0xffff, not 0x0000,  0x0000 means heap is below stack.
		if(max==0){// wrapped around 
		  max=0xffff;
		}
    __malloc_heap_end=(char*)max;
    __malloc_heap_start=(char*)0x2200;
    __brkval=NULL;
    __flp=NULL;
//		Serial.print("heap Start=0x");Serial.println((uint16_t)__malloc_heap_start,HEX);
//		Serial.print("heap End  =0x");Serial.println((uint16_t)__malloc_heap_end,HEX);
    }
  }
XMCRB=0x00; // all 64k 
XMCRA=B10000000 | ((_flags&3) << 2)| (_flags&3);// enable Xmem, waitstates.
return 0;
}

uint8_t XMem::init(void){ // 0..7, 8k pages 
return init(SS,0x27,0);
}

uint8_t XMem::init(uint8_t highestFrozenPane){ // 0..7, 8k pages
return init(SS,0x27,highestFrozenPane);
} 

uint8_t XMem::fixPage(uint8_t page){
uint8_t inpage = 0xFE; // setup bad page error
if (page<128){  
   inpage = (page & 0x3f )| 0xC0;
   if(page&0x40) inpage = inpage & 0x7f;
   else inpage = inpage & 0xBF;
}else if(page==0xff) inpage=page;
return inpage;  
}

uint8_t XMem::unFixPage(uint8_t page){
uint8_t outpage = page & 0xC0;
if(outpage==0xC0) outpage = 0xff;
else if (outpage==0x80) outpage = page &0x3f;
else if( outpage==0x40) outpage = outpage + (page&0x3f);
else outpage = 0xFE;
return outpage;
}

uint8_t XMem::setPane(uint8_t pane, uint8_t page){ // pane=1..7, page=0..127,255= disable
char ch[50];
// sprintf(ch,"sP pane=%u page=%X",(uint8_t)pane,(uint8_t)page);
// Serial.println(ch);

if(_CS){
  if((pane>(_frozen&0x7))&&(pane<8)){
    uint8_t inpage = fixPage(page);
 //   sprintf(ch,"sR pane=%u fixpage=%X",(uint8_t)pane,(uint8_t)inpage);
 //   Serial.println(ch);

    if (inpage!=254){
      return setRam(pane,inpage);
      }
    else return 2; // bad page
    }
    else return 1;// bad pane number
  }
else return 3;
}

uint16_t XMem::pushPane(uint8_t pane, uint8_t page){
uint16_t result = 0;
if(_CS){
  if ((pane>(_frozen&0x7))&&(pane<8)){
    page = fixPage(page);
    if(page!=254){
      if(setRam(pane,page)==0) result = ((uint16_t)pane<<8)+_panes[pane-1]| 0xA000;
      
      }
    else result = 2; // bad page
    }
  else result = 1; // bad pane
  }
else result = 3; // not initialized
return result;
}

uint8_t XMem::popPane(uint16_t pushstack){
if(_CS){
  if(((pushstack&0xA000)==0xA000)&&((pushstack &&0x00B0)>0)){
    return setRam(((pushstack&0x0f00)>>8),(pushstack&0xff));
    }
  else return 1;
  }
else return 3;
}

uint8_t XMem::setWindow(uint8_t window[]){ // window is array of byte [pane1,pane2,...]
if(_CS){
  bool good=true;
  uint8_t i=_frozen;
  while((good)&&(i<7)){
    good = good && (fixPage(window[i])!=254);
    if(i==0){
      good = good && ((window[0]==0)|(window[0]==64)|(window[0]==255));
    }
    i++;
  }
  if(!good) return 2; // bad page number
  i = _frozen;
  uint8_t lastError=0;
  while((lastError==0)&&(i<7)){
     lastError = setRam(i+1,fixPage(window[i]));
     i++;
     }
  return lastError;
  }
else return 3;
}

uint8_t XMem::getWindow(uint8_t window[]){ // return array of current pane mapping
uint8_t result=0;
if(_CS){
	for(uint8_t i=0;i<7;i++){
    window[i]=unFixPage(_panes[i]);
    }
  }
else result=3;
return result;
}

uint16_t XMem::paneToAddr(uint8_t pane){ //calculate memory pointer for direct access

pane=pane&7;
uint16_t result;
result = pane;
result = result <<13;
if(pane==1) result = result + 0x200; // pane 1 is smaller, by 512 bytes because of CPU internals
return result;
}

uint8_t XMem::setRam(uint8_t pane, uint8_t inpage){  // map page of Ram to pane
char ch[50];
if((pane<8)&&(pane>1)){
//  sprintf(ch,"sR pane=%u page=%X",(uint8_t)pane,(uint8_t)page);
//  Serial.println(ch);
  if((inpage&0xC0)==0) return 2; // invalid page, can't have both CS's Low at the same time!
  _panes[pane-1]=inpage;
  uint8_t sel=_BV(pane-2) | _panes[0]; // set latch high and keep default for pane 1 active.
  MCP23S17_write_byte(_CS,_Addr,0x13,sel); //select latch
  MCP23S17_write_byte(_CS,_Addr,0x12,inpage); //load value
 // sprintf(ch,"strobe sel=%X",(uint8_t)sel);
 // Serial.println(ch);Serial.flush();
//  sel=((_frozen&0x10)>>4); // flag to enable 3:8 decoder hardware
  sel = _panes[0];
  MCP23S17_write_byte(_CS,_Addr,0x13,sel); // latch it
//  sprintf(ch,"sel=%X",(uint8_t)sel);
//  Serial.println(ch);
//  Serial.flush();
  return 0;
  }
else if(pane==1){
//  sprintf(ch,"sR pane=%u page=%X",(uint8_t)pane,(uint8_t)page);
 // Serial.println(ch);
  if(inpage==0x80) _panes[0] = 0x80; // default to page 0, bank0:0
  else if(inpage==0x40) _panes[0] = 0x40; // default to page 64, bank1:0
  else if(inpage==0xFF) _panes[0] = 0xC0; // default to nothing, buss floats
  else return 2; // invalid page for pane 1;
  uint8_t sel=_panes[0];
  MCP23S17_write_byte(_CS,_Addr,0x13,sel);
//  sprintf(ch,"sel=%X",(uint8_t)sel);
//  Serial.println(ch);
//  Serial.flush();
  return 0;
  }
else return 1; // invalid pane;
}

uint8_t XMem::clearRam(uint8_t pane){ // disable this pane
if((pane>(_frozen&7))&&(pane<8)){
  setRam(pane,255);
  return 0;
  }
else return 1; // invalid pane
}  


uint16_t XMem::heapUnAllocated(){
  uint16_t v=0;
  if((uint16_t)&v<(uint16_t)__malloc_heap_start){// in Expansion ram
	  v= (uint16_t)__malloc_heap_end - ((uint16_t)__brkval==0 ? (uint16_t)__malloc_heap_start : (uint16_t)__brkval);
	}
  else {	
    v= (uint16_t) &v - ((uint16_t)__brkval == 0 ? (uint16_t)__malloc_heap_start : (uint16_t) __brkval); 
   }
   return v;
}
 
 uint16_t XMem::heapMaxFree(){ // find biggest block, unallocated or in freelist, return size
  uint16_t v=heapUnAllocated(); // size of unallocated heap;
  // run free list
  struct __freelist * myfreeptr;
	myfreeptr = __flp;
	while(myfreeptr){ // non zero
    if(myfreeptr->sz>v) v=myfreeptr->sz;
	  myfreeptr = myfreeptr->nx;
	  }		
	return v;
}

uint16_t XMem::heapFree(){ // sum of all heap, including freelist
  uint16_t v=heapUnAllocated(); // size of unallocated heap;
  // run free list
  struct __freelist *myfreeptr;
	myfreeptr = __flp;
	while(myfreeptr){ // non zero
    v += myfreeptr->sz;
		myfreeptr = myfreeptr->nx;
	  }		
	return v;
}

uint8_t XMem::MCP23S17_write_byte(uint8_t cs, uint8_t spiaddr, uint8_t addr,uint8_t din){
spiaddr &= 0x27;
spiaddr = (spiaddr <<1); // write
digitalWrite(cs,LOW);
uint8_t x;
x=SPI.transfer(spiaddr);
x=SPI.transfer(addr);
x=SPI.transfer(din);
digitalWrite(cs,HIGH);
return 1; //bytes transfered
}

uint8_t XMem::MCP23S17_write_word(uint8_t cs, uint8_t spiaddr, uint8_t addr,uint16_t din){
spiaddr &= 0x27;
spiaddr = (spiaddr <<1); // write
digitalWrite(cs,LOW);
SPI.transfer(spiaddr);
SPI.transfer(addr);
SPI.transfer(lowByte(din));
SPI.transfer(highByte(din));
digitalWrite(cs,HIGH);
return 2; //bytes transfered
}

uint8_t XMem::MCP23S17_write_block(uint8_t cs,uint8_t spiaddr, uint8_t addr, char * din, uint8_t len){
spiaddr =(spiaddr&0x27)<<1; // write
digitalWrite(cs,LOW);
SPI.transfer(spiaddr);
SPI.transfer(addr);
for(uint8_t i=0;i<len;i++){
  SPI.transfer(din[i]);
  }
digitalWrite(cs,HIGH);
return len;
}
 
uint8_t XMem::MCP23S17_init(uint8_t cs, uint8_t spiaddr, uint16_t iodir, uint16_t initval, bool sequential){
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

uint8_t XMem::MCP23S17_read_byte(uint8_t cs, uint8_t spiaddr, uint8_t addr){
spiaddr &= 0x27;
spiaddr = (spiaddr <<1) +1; // read
uint8_t x;
digitalWrite(cs,LOW);
x=SPI.transfer(spiaddr);
x=SPI.transfer(addr);
x=SPI.transfer(0);
digitalWrite(cs,HIGH);
return x;
}
