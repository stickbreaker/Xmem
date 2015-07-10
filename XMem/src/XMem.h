/*
External memory routines for Version 2 Main Board
25JUL2014 
  x1 compiles
  x2 needs testing


error Return Codes:
        0 = success
        1 = invalid pane
        2 = invalid page
        3 = not initialized
 09JUN2015
   fix problem with __malloc_heap_start in init, prior value was 0x8200 changed to 0x2200.
   adding heapFree , not done.
 30JUN2015
   Change for compatibility with Memory Panes V2
 07JUL2015
   first Publish to Github 
   
*/

#ifndef XMem_h
#define XMem_h

#include "Arduino.h"

extern int * __brkval;		// first location not yet allocated 
extern struct __freelist * __flp; // freelist pointer (head of freelist) 
extern char * __malloc_heap_start;
extern char * __malloc_heap_end;

struct heapState{
  char* __malloc_heap_start;
  char* __malloc_heap_end;
  int* __brkval;
  __freelist* __flp;
};

struct __freelist {
	uint16_t sz;
	struct __freelist *nx;
};
class XMem {
public:
  XMem(void);   //constructor
  ~XMem(void); //destructor, disables all panes, reconfigures CPU shutting off XMEM function (AD0..AD15,_WR,_RD,ALE)
  uint8_t init(void); // CS=SS, SPIAddr =0x27, frozen=0
  uint8_t init(uint8_t highestFrozenPane); // CS=SS SPIAddr =0x27
  uint8_t init(uint8_t CS, uint8_t SPIAddr,uint8_t highestFrozenPane); // 1..7, 8k pages used for heap
    // if highestFrozenPane >0 these panes are assigned as HEAP
  uint8_t  setPane(uint8_t pane, uint8_t page); // pane=1..7, page=0..127,255=disable
     // pane=1 can only accept page: {0,64,255}
  uint16_t pushPane(uint8_t pane, uint8_t page); // used to temporarily change a pane mapping 
  uint8_t  popPane(uint16_t pushstack);  // returns pane to previous (pushstack) mapping
  uint8_t  setWindow(uint8_t []); // window is array of byte [pane1,pane2,...,pane7]
  uint8_t  getWindow(uint8_t [] ); // return a 7byte array of current pane mapping 255=disabled
  uint16_t paneToAddr(uint8_t pane); //calculate memory pointer for direct access
  uint16_t heapFree(void); // report heap avail.
	uint16_t heapUnAllocated(void); // size of block between top of heap and current highest allocated
	uint16_t heapMaxFree(void); // size of largest block available on heap.
	void setWait( uint8_t wait); // 0..3 call before Init() to set memory waitstates. Defaults to 0
	void setHighHeap(bool hi); // call before init() to use page 64+ as heap 
private:
  uint8_t _panes[7]; // current pane to page mapping, pane1 .. pane7
  uint8_t _frozen; // highest pane that is frozen as HEAP, Default 0 (internal RAM) 
  uint8_t _CS;     // Hardware pin _CS for MCP23S17, Defaults to SS(53)
  uint8_t _Addr;   // SPI sub address for MCP23S17, Defaults 0x27 SPI I/O Expander used to update Pane Latches
  uint8_t setRam(uint8_t pane, uint8_t page);  // map page of Ram to pane
  uint8_t clearRam(uint8_t pane); // disable this pane  
  uint8_t fixPage(uint8_t); // convert from page(0..127) to Internal Format 254=error
  uint8_t unFixPage(uint8_t);; //convert from internal Format to logical page (0..127,255) or 254=error
	void    disable(); // shutdown hardware
	void    initVars(); // enable hardware
  heapState _h;
	uint8_t _flags; // waitStates and High Heap
};
#endif