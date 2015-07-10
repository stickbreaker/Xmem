#include <XMem.h>
#include <my_spi.h>

XMem X;
uint16_t mfgTest(uint8_t mode,bool errormsgs); // forward


void displayRam(byte * bp, uint16_t len){
uint16_t a=0;
while(a<len){
  if((a&0xf)==0){
    Serial.println();
    Serial.print((uint16_t)bp+a,HEX);
    Serial.print(": ");
    }
  uint8_t b=*(bp+a);
  if(b<16)Serial.print('0');
  Serial.print(b,HEX);
  Serial.print(' ');
  a++;
  }

}


void printRam(byte * bp){
if(((uint16_t)bp&0xF)==0){
  Serial.println();
  Serial.print((uint16_t)bp,HEX);
  Serial.print(": ");
}
uint8_t a=*bp;
if(a<16)Serial.print('0');
Serial.print(a,HEX);
Serial.print(' ');
a=*(bp+1);
if(a<16)Serial.print('0');
Serial.print(a,HEX);
Serial.print(' ');

}
void readExpander(){
for(uint8_t i=0;i<0x16;i++){
  uint8_t b=SPI.MCP23S17_read_byte  (SS, 0x27, i);
  if((i&0x7)==0){
    if(i<16) Serial.print("\n0");
    else Serial.println();
    Serial.print(i,HEX);
    Serial.print(" : ");
  }
  if (b<16) Serial.print('0');
  Serial.print(b,HEX);
  Serial.print(' ');
}

}

void reTest(uint8_t * bp){
uint8_t buff[256],diff[256];
for(uint16_t i=0;i<256;i++){
  buff[i]=bp[i];
  diff[i]=0;
}
displayRam((uint8_t*)&buff,0x100);
bool same=true;
do{
   for(uint16_t i=0;i<256;i++){
    uint8_t b= bp[i];
    if(b!=buff[i])diff[i]=b;
    same = same && (buff[i]==b);
  
   }
}while(same&&(!Serial.available()));
Serial.println();
if(!same) {
  displayRam((uint8_t*)&diff,0x100);
  Serial.print("\nbp=");
  Serial.println((uint16_t)bp,HEX);
  displayRam(bp,0x100);
  }
}   
void displaySector(uint8_t * bp){
displayRam((uint8_t*)((uint16_t)bp&0xFF00),0x100);
Serial.println();
}


void fillRamBytes(byte * bp,uint8_t page, bool msg){
if(msg){
  Serial.print("fill page with Bytes @0x");
  Serial.print((uint16_t)bp,HEX);
  Serial.print(" with 0x");
  Serial.print(page,HEX);
  Serial.print(", 0x");
  Serial.println((uint8_t)~page,HEX);
  }
byte * savebp = bp;
do{
	*bp++=page;
	*bp++=(uint8_t)~page;
	//printRam(bp-2);
	}while(((uint16_t)bp&0x1FFE)!=0);
if(msg)displaySector(savebp);
} 

void fillRamWords(byte * bp,uint8_t page, bool msg){
uint16_t word= page | (((uint8_t)~page)<<8);
if(msg){
  Serial.print("fill page with words @0x");
  Serial.print((uint16_t)bp,HEX);
  Serial.print(" with 0x");
  Serial.println((uint16_t)word,HEX);
  }
uint16_t * wordptr=(uint16_t*)bp;
do{
*wordptr++=word;
//printRam(bp-2);
}while(((uint16_t)wordptr&0x1FFE)!=0);
if(msg)displaySector(bp);
} 

void fillRamLong(byte * bp,uint8_t page, bool msg){
uint32_t lword= page | (((uint16_t)~page)<<8)|((uint32_t)page<<16)|(((uint32_t)~page)<<24);
if(msg){
  Serial.print("fill page with Long words @0x");
  Serial.print((uint16_t)bp,HEX);
  Serial.print(" with 0x");
  Serial.println((uint32_t)lword,HEX);
  displaySector(bp);
  }
uint32_t * lwordptr=(uint32_t*)bp;
do{
	*lwordptr++=lword;
	//printRam(bp-2);
	}while(((uint16_t)lwordptr&0x1FFC)!=0);
if(msg)displaySector(bp);
} 

void testRam(byte * bp,uint8_t page){
bool good=true;
do{
//printRam(bp);

good = good&&(*bp++==page)&&(*(bp++)=(uint8_t)~page);
}while((((uint16_t)bp&0x1FFE)!=0)&&good);
if(!good){
  Serial.print("RAM test Failed at ");
  Serial.println((uint16_t)bp,HEX);
  }
} 
uint8_t * bp=0;
uint8_t page=0;
uint8_t pane=1;
uint8_t b=0;

void dispHeap(const char msg[]){

extern int * __brkval;		// first location not yet allocated 
extern struct __freelist * __flp; // freelist pointer (head of freelist) 
extern char * __malloc_heap_start;
extern char * __malloc_heap_end;
extern int __heap_start; // compiled constant value for where heap started
extern int __heap_end;   // compiled constant for where heap ended, always 0 
Serial.println(msg);
Serial.print("Heap Structure\n__brkval=0x");
Serial.println((uint16_t)__brkval,HEX);
Serial.print("__flp =0x");
Serial.println((uint16_t)__flp,HEX);
Serial.print("__malloc_heap_start =0x");
Serial.println((uint16_t)__malloc_heap_start,HEX);
Serial.print("__malloc_heap_end   =0x");
Serial.println((uint16_t)__malloc_heap_end,HEX);
Serial.print("&__heap_Start       =0x");
Serial.print((uint16_t)&__heap_start,HEX);
Serial.print("&__heap_End         =0x");
Serial.print((uint16_t)&__heap_end,HEX);

}

void setup(){
Serial.begin(115200);
SPI.begin();

bp=(byte *)X.paneToAddr(1);
Serial.print("addr of pane 1=");
Serial.println((uint16_t)bp,HEX);
  
}


uint8_t readTest(uint8_t * bp){
uint8_t b;

  b = *bp;

return b;
}

void writeTest( uint8_t * bp,uint8_t b){

  *bp = b;

}
unsigned long timeout;

void eatSerial(){
timeout=millis()+100;
do{
  if(Serial.available()){
     if((Serial.peek()=='\n')||(Serial.peek()=='\r')){
       Serial.read();
       timeout=millis()+50;
       }
     else timeout==millis();
  }
  }while(millis()<timeout);
}

bool eol(char term[],char ch){
bool result = false;
while((*term!=0)&&(!result)){
  result =(*term==ch);
  term++;
}
return result;
}

uint16_t getNum(const char msg[]){
eatSerial();
if(!Serial.available())
	Serial.print(msg);
bool done=false;
uint16_t b=0;
bool hexflag=false;
while(!done){
  if(Serial.available()){
    char ch=Serial.read();
    //Serial.print(ch);
    if((ch>='0')&&(ch<='9')){
       if(hexflag) b=b*16;
       else b=b*10;
       b= b +(ch-48);
       }
    else if ((ch>='A')&&(ch<='F')){
       hexflag = true;
       b=b*16;
       b= b+(ch-55);
    }
    else if ((ch>='a')&&(ch<='f')){
       hexflag = true;
       b=b*16;
       b= b+(ch-87);
    }else if((ch=='x')||(ch=='X')||(ch=='#'))
      hexflag=true;
        
    done = eol("\n\r,/",ch);
    }
  }
  return b;
}


void setPage(){
pane = getNum("\nEnter Pane? ");
Serial.println(pane,DEC);
bp=(uint8_t *)X.paneToAddr(pane);
page = getNum("Enter Page?" );
Serial.println(page,DEC);
uint8_t er=X.setPane(pane,page);
if (er){
  Serial.print("XMem.setPane failed=0x");
  Serial.println(er,HEX);
}
}
char getch( const char choice[]){
  bool done = false;
  char ch;
  do{
    if(Serial.available()){
      ch=Serial.read();
      if((ch>96)&&(ch<123))ch=ch-32;
      for(uint8_t i=0;i<strlen(choice);i++){
        done=(ch==choice[i]);
        if(done) break;
        }
    }
  }while(!done);
return ch;
}

char getch(const char msg[], const char choice[]){
Serial.print(msg);
return getch(choice);
}

void continous(){
Serial.print("\n Continous Test until Failure or keypress\n");
Serial.print(" M - Manufacturing \n H - Heap");
Serial.print("\n>");
eatSerial();
char ch = getch("MHS\n\r");
Serial.print(ch);
uint16_t res=0;
uint8_t mode=2;
bool success = true;
unsigned long count=0;
switch(ch){
  case 'M' : eatSerial();
    do{
			mode++;	
			mode = mode % 3;
			Serial.print("\n MFG Test mode=");
			Serial.print(mode,DEC);
			res = mfgTest(mode,false);
      count ++;
      if(res>0){
        Serial.print("Test failed after ");
        Serial.print(count,DEC);
        Serial.println(" iterations\n Enter to Continue");
        getch("\n\r");
        }
      if(Serial.available()){
        eatSerial();
        res=1;
      }
    }while(res==0);
    break;
	case 'H' : eatSerial();
	  do{
			if(!success){
				do{
					success = heapTest(true);
					Serial.print("fail Heap Test=");
					if(success)Serial.println("PASS");
					else Serial.println("FAIL");
					}while(success||(!Serial.available()));
				res=1;
			}else {
				res = X.heapFree();
  			success = heapTest(false);
  			Serial.print("Heap Test =");
				if(success)Serial.println("PASS");
				else Serial.println("FAIL");
        if((res==X.heapFree())&&success) res=0;
				count ++;
				}
      if(res>0){
        Serial.print("Test failed after ");
        Serial.print(count,DEC);
        Serial.println(" iterations\n Enter to Continue");
        getch("\n\r");
        }
      if(Serial.available()){
        eatSerial();
        res=1;
      }
    }while(res==0);
    Serial.print(count,DEC);
    Serial.println(" iterations");
		
		break;
			
   case 'S' : ;
   default : ;
  }

}

struct MEMUSER{
	uint16_t sz;
	uint16_t count;
	struct MEMUSER * nx, *px;
};
void printLink(const char ch[],struct MEMUSER * m){
Serial.print(ch);
Serial.print((uint16_t)m,HEX);
if(m){
	Serial.print(" nx=");
	Serial.print((uint16_t)m->nx,HEX);
	Serial.print(" px=");
	Serial.println((uint16_t)m->px,HEX);
	}
}

bool checkLink( struct MEMUSER * m, struct MEMUSER head, bool msg){
bool result = true;
if(msg)printLink("+- ",m);
if(m->nx){
	if(m->nx->px!=m){
		result=false;
		if(msg){
			Serial.println("next broken");
		  printLink("m   ",m);
		  printLink(" nx ",m->nx);
		  }
		}
	}
else{
	if(head.nx!=m){
		result = false;
		if(msg){
			Serial.println("Head last broken");
			printLink("m   ",m);
			printLink("hd  ",&head);
		}
	}
}
if(m->px){
	if(m->px->nx!=m){
		result=false;
		if(msg){
			Serial.print("\nPrev Broken\n");
			printLink("m   ",m);
			printLink(" px ",m->px);
			}
		}
	}
else{
	if(head.px!=m){
		result = false;
		if(msg){
			Serial.println("Head First Broken");
			printLink("m   ",m);
			printLink("hd  ",&head);
			}
		}
	}
return result;
}

bool runLinks(struct MEMUSER head, bool msg){
	bool result=true;
struct MEMUSER * m;
if(msg)printLink("run head=",&head);
m=head.px; // first of chain
uint16_t xcnt=0,xsz=0;
while((m)&&(xcnt<head.count)&&result){
	result=result && checkLink(m,head,msg);
  xcnt++;
	xsz += m->sz;
  m=m->nx;
	}
if(m&&result){
	result=false;
	if(msg){
		Serial.print("too long forward\n");
		checkLink(m,head,msg);
		}
	}
if(xcnt!=head.count){
	result=false;
	if(msg){
		Serial.print(" Count Mismatch xcnt=");
		Serial.print(xcnt);
		Serial.print(" != ");
		Serial.println(head.count);
	}
}
if(xsz!=head.sz) {
	result=false;
	if(msg){
		Serial.print(" size Mismatch ");
		Serial.print(xsz);
		Serial.print(" != ");
		Serial.println(head.sz);
		}
	}
m=head.nx; // last of chain
xcnt=0;
xsz=0;
while((m)&&(xcnt<head.count)){
	result=result && checkLink(m,head,msg);
  xcnt++;
	xsz += m->sz;
  m=m->px;
	}
if(m){
	result=false;
	if(msg){
		Serial.print("too long backward ");
		checkLink(m,head,msg);
		}
	}
if(xcnt!=head.count){
	result = false;
	if(msg){
		Serial.print(" Count Mismatch xcnt=");
		Serial.print(xcnt);
		Serial.print(" != ");
		Serial.println(head.count);
		}
	}
if(xsz!=head.sz) {
	result = false;
	if(msg){
		Serial.print(" size Mismatch ");
		Serial.print(xsz);
		Serial.print(" != ");
		Serial.println(head.sz);
		}
	}
if(msg){
	Serial.print(" count =");
	Serial.print(xcnt);
	Serial.print(" size =");
	Serial.print(xsz);
	Serial.print(" head sz=");Serial.print(head.sz);
	Serial.print(" head cnt=");Serial.println(head.count);
	}
return result;
}

bool fixBrokenLinks(struct MEMUSER head, bool msg){
bool success = true,found=false;
struct MEMUSER * m, *n;
uint16_t cnt=0;
m=head.px; // first
while((m)&&(cnt<head.count)&&!found){
	cnt++;
  found = !checkLink(m,head,false);
  if(!found) m= m->nx;
  }
if(msg){
	if(found)	printLink(" running forward broken ",m );
}
n=head.nx; // last
cnt=0;
found=false;
while((n)&&(cnt<head.count)&&!found){	
	cnt++;
  found = !checkLink(n,head,false);
  if(!found) n= n->px;
  }
if(msg){
	if(found) printLink(" running Backward broken ",n);
}
if(m&&n){
	if((m->nx==n)||(n->px=m)){
		if(m->nx==n) {
			Serial.print("fixing n->px\n");
			n->px=m;
		  }
		if(n->px==m){
			Serial.print("fixing m->nx\n");
			m->nx=n;
			}
		if(msg){
			printLink(" fixed m->nx =",m);
			printLink(" fixed n->px ",n);
			}
		}
	else {
		success = false;
		if(msg){
			Serial.print(" double broken\n");
			printLink(" failed m=",m);
			printLink(" failed n=",m);
			}
		}
	}
else {
	success = false;
	if(msg){
		Serial.print(" NULL link\n");
		printLink(" failed m=",m);
		printLink(" failed n=",m);
		}
	}
return success;
}

bool heapTest(bool msg){
bool result = true; // success
randomSeed(millis());
struct MEMUSER head, *m;
uint16_t size;
head.px=NULL;
head.nx = NULL;
head.sz = 0; // size of mem allocated
head.count = 0; //number of nodes
if(msg){
	Serial.print("Free before,");
	Serial.println(X.heapFree());
}
bool done=false;
while((X.heapFree()>512)&&(!done)){
	size=random(512)+sizeof(struct MEMUSER);
	if(msg){
		Serial.print("tot,");
		Serial.print(head.sz);
		Serial.print(",size,");
		Serial.print(size);
		}
	m=(struct MEMUSER*)malloc(size);
	if(m){
		memset(m,0xff,size);
		if(msg){
			Serial.print(",memfree,");
			Serial.print(X.heapFree());
			Serial.print(",@0x");
			Serial.println((uint16_t)m,HEX);
			}
		m->sz=size;
		m->count=head.count;
		m->nx = NULL;
		m->px = NULL;
		head.count++;
		head.sz += m->sz;
		if(head.nx==NULL){
			head.nx = m;
			m->px = NULL; //redundant, first in chain
		}else{// splice into list
			m->px = head.nx;
			m->px->nx = m;
			head.nx = m;
			}
		if(head.px==NULL){
			head.px = m; 
		}			
	}
	else {
		done = true; // malloc failed
	  }
	}
if(msg){
	Serial.print(" freeing\n count,");
	Serial.print(head.count);
	Serial.print(",size,");
	Serial.print(head.sz);
  Serial.print(",freeMem,");
	Serial.println(X.heapFree());
  }
done=false;	
while((head.count > 0)&& result&&!done){
  if(!runLinks(head,false)){
		if(msg)runLinks(head,true);
	  fixBrokenLinks(head,true);
		if(msg)runLinks(head,true);
	  }
  size = random(head.count); // number of hops to run down
	if(msg){
		Serial.print("hop,");
		Serial.print(size);
//	  Serial.print(" starting @ ");
		}
	m=head.px;
	while((size>0)&&(m)){
//		Serial.println((uint16_t)m,HEX);
		size--;
		if(!checkLink(m,head,false)&&msg) checkLink(m,head,true);
		m=m->nx;
	  }
//	Serial.print("found =");
//	Serial.print((uint16_t)m,HEX);
	if(m){
		if(msg){
			Serial.print(",cnt,");Serial.print(head.count);Serial.print(",sz,");Serial.print(head.sz);
			Serial.print(",freeing,");Serial.print(m->sz);
  		Serial.print(",@0x");
			Serial.print((uint16_t)m,HEX);
			Serial.print(",px=0x");
			Serial.print((uint16_t)m->px,HEX);
			Serial.print(",nx=0x");
			Serial.print((uint16_t)m->nx,HEX);
		}
		if(m->sz <512+sizeof(struct MEMUSER)){
			if(m->px)
				 m->px->nx = m->nx;
			else 
				head.px = m->nx;
			if(m->nx)
				m->nx->px = m->px;
			else
				head.nx = m->px;
			head.count--;
			head.sz -= m->sz;
			free(m);
			if(msg){
				Serial.print(",heap free,");Serial.print((uint16_t)X.heapFree());
				Serial.print(",UnAlloc free,");Serial.print((uint16_t)X.heapUnAllocated());
				Serial.print(",Max free,");Serial.println((uint16_t)X.heapMaxFree());
				}
			}
		else { // invalid size,
			done = true;
			if(msg)Serial.println("invalid size value");
			result = false; 
			}
		}
 
	else {
		if(msg){
			Serial.println(" broken link Failure ");
			Serial.print(" cnt=");Serial.print(head.count);Serial.print("sz=");Serial.println(head.sz);
			}
		result=false;
		}
	}
if(msg){
	Serial.println("Free After=");Serial.println(X.heapFree());
	}
if((head.count!=0)||(head.sz!=0)) result = false;
return result;
}

void stat(){
  Serial.print("\n pane=0x");
  Serial.print(pane,HEX);
  Serial.print(" page=0x");
  Serial.print(page,HEX);
  Serial.print(" addr=0x");
  Serial.print((uint16_t)bp,HEX);
  Serial.print(" byte=0x");
  Serial.println(b,HEX);
  Serial.print(" HEAP_start = 0x");
	Serial.println((uint16_t)__malloc_heap_start,HEX);

  Serial.print(" HEAPFree = 0x");
	Serial.println(X.heapFree(),HEX);
  Serial.print(" HEAPMaxFree = 0x");
	Serial.println(X.heapMaxFree(),HEX);
  Serial.print(" HEAPUnAllocated = 0x");
	Serial.println(X.heapUnAllocated(),HEX);
  uint8_t win[7];
  uint8_t er=X.getWindow(win);
  if(er){
    Serial.print(" getWindow failed =");
    Serial.print(er,DEC);
  }
  Serial.print("Panes : ");
  for (uint8_t i=0;i<7;i++){
    Serial.print((uint8_t)(i+1),DEC);
    Serial.print("   ");
  }
 
  Serial.print("\nPages :");
  for (uint8_t i=0;i<7;i++){
    if (win[i]<100) Serial.print ('0');
    if(win[i]<10) Serial.print('0');
    Serial.print(win[i],DEC);
    Serial.print(' ');
  }
  Serial.println();
}
void choices(){
  stat();
  Serial.print("\nR - Continous Read\nW - Continous Write\nN - set pane/page\nH - HeapTest\n"
    "Annnn - Addr\nBnn - byte\nT - Test page/pane\nY - reTest\nF - Fill pane/page\n"
    "D - disp\nP - Pause\nM - MFG Test\nX - Exit\n");
}

uint16_t mfgTest(uint8_t mode, bool errormsgs){
uint16_t pageResult[129];
uint16_t paneResult[7];
uint8_t win[7];
uint8_t er=0;
win[0]=64;
uint8_t minpane =0;
X.getWindow(win);
while((minpane<7)&&(X.setPane(minpane+1,win[minpane])))minpane++;
if (minpane>6) {
	if(errormsgs) Serial.print(" All Panes Frozen to heap");
	return 1;
}
if((minpane>0)&&(errormsgs)){
	Serial.print(" Frozen panes, only using ");
	Serial.print(minpane+1,DEC);
	Serial.print(" .. 7\n");
}

if(errormsgs){
	switch(mode){
		case 0 : Serial.print(" Byte Write/Read\n");
		  break;
		case 1: Serial.print(" Word Write/Byte Read\n");
		  break;
		case 2: Serial.print(" Long Write/Byte Read\n");
		default :;
	}
}

for(uint8_t i=0;i<129;i++){
  pageResult[i]=0;
  }
for(uint8_t i=0;i<7;i++){
  paneResult[i]=0;
  }
for(uint8_t j = 0;j<128;j++){ // Write to all windows
  if((j&63)==0)
    if(errormsgs)Serial.println();
  if(errormsgs)Serial.print('W');
  for(uint8_t i=minpane;i<7;i++){
    win[i] = (i+j)%128;
    }
	if(minpane==0){
    if(win[0]==64) win[0]=0;
    else win[0]=64;  
	  }
  er = X.setWindow(win);
  if (er==0) {
    for(uint8_t i=minpane;i<7;i++){
      uint8_t * ptr = (uint8_t *)X.paneToAddr(i+1);
      switch(mode){
		case 0 :fillRamBytes(ptr,win[i],false);
		   break;
		case 1 :fillRamWords(ptr,win[i],false);
		   break;
		case 2 : fillRamLong(ptr,win[i],false);
           break;
		default : ;
	    }
	  }
    }
  else if(errormsgs){
    Serial.print("setWindow Fail=");
    Serial.println(er,DEC);
    }
  else pageResult[128]=er;
  }
for(uint8_t j = 0;j<128;j++){ // read from all windows
  if(errormsgs){
    if((j&63)==0)Serial.println();
    Serial.print('R');
    }
  for(uint8_t i=minpane;i<7;i++){
    win[i] = (i+j)%128;
    }
	if(minpane==0){
    if(win[0]==64) win[0]=0;
    else win[0]=64;
		}
  er = X.setWindow(win);
  if (er==0) {
    for(uint8_t i=minpane;i<7;i++){
      uint8_t * ptr = (uint8_t *)X.paneToAddr(i+1);
      do{
        if(*ptr++!=(uint8_t)win[i]){ 
          if(errormsgs){
            Serial.print("read Error @=0x");
            Serial.print((uint16_t)(ptr-1),HEX);
            Serial.print(" 0x");Serial.print(*(ptr-1),HEX);
            Serial.print(" != 0x");Serial.println(win[i],HEX);
            Serial.print("pane =");Serial.print((i+1),DEC);Serial.print(" page=");
            Serial.println((uint8_t)win[i],DEC);
            }
          pageResult[win[i]]++;
          paneResult[i]++;
					ptr = (uint8_t *)X.paneToAddr(i+1);
					switch(mode){
						case 0 :fillRamBytes(ptr,win[i],false);
							break;
						case 1 :fillRamWords(ptr,win[i],false);
							break;
						case 2 : fillRamLong(ptr,win[i],false);
							break;
						default : ;
						}	
					break;
					}
        
        if(*ptr++!=(uint8_t)(~win[i])){ 
          if(errormsgs){
            Serial.print("read Error @=0x");
            Serial.print((uint16_t)(ptr-1),HEX);
            Serial.print(" 0x");Serial.print(*(ptr-1),HEX);
            Serial.print(" != 0x");Serial.println((uint8_t)~win[i],HEX);
            Serial.print("pane =");Serial.print(i,DEC);Serial.print(" page=");
            Serial.println((uint8_t)win[i],DEC);
            }
          pageResult[win[i]]++;
          paneResult[i]++;
					ptr = (uint8_t *)X.paneToAddr(i+1);
					switch(mode){
						case 0 :fillRamBytes(ptr,win[i],false);
							break;
						case 1 :fillRamWords(ptr,win[i],false);
							break;
						case 2 : fillRamLong(ptr,win[i],false);
							break;
						default : ;
						}	
          break;
          }
        }while(((uint16_t)ptr&0x1fff)!=0);  
      }
		}
  else if(errormsgs){
    Serial.print("setWindow Fail=");
    Serial.println(er,DEC);
    }
  else pageResult[128]=er;
  }
unsigned long result = 0;
for(uint8_t i=0;i<129;i++){
  result += pageResult[i];
	}
if(errormsgs||(result>0)){ 
  Serial.println();
  if (result >0){
    Serial.print(" Failures Detected\nFailue Count(hex) by Page\n     ");
  
    for(uint8_t i=0;i<8;i++){
      Serial.print("   ");
      Serial.print(i,DEC);
      Serial.print(' ');
      }
    for(uint8_t i=0;i<128;i++){
      if((i&7)==0){
        Serial.print('\n');
        if(i<100)Serial.print(' ');
        if(i<10)Serial.print(' ');
        Serial.print(i,DEC);
        Serial.print(":");
        }
      Serial.print(" ");
      if(pageResult[i]<0x1000) Serial.print(' ');
      if(pageResult[i]<0x0100) Serial.print(' ');
      if(pageResult[i]<0x0010) Serial.print(' ');
      if(pageResult[i]>0)      Serial.print(pageResult[i],HEX); 
      else Serial.print(' ');
      }
    Serial.print("\nFailue Count(HEX) by Pane\n   ");
    for(uint8_t i=0;i<7;i++){
      Serial.print("   ");
      Serial.print(i+1,DEC);
      Serial.print(' ');
      }
    for(uint8_t i=0;i<7;i++){
      if((i&7)==0){
        Serial.print('\n');
        Serial.print(i,DEC);
        Serial.print(":");
        }
      Serial.print(" ");
      if(paneResult[i]<0x1000) Serial.print(' ');
      if(paneResult[i]<0x0100) Serial.print(' ');
      if(paneResult[i]<0x0010) Serial.print(' ');
      if(paneResult[i]>0)      Serial.print(paneResult[i],HEX); 
      else Serial.print(' ');
      }
    Serial.println();
    }
  }
return result;
}

void menu(){
eatSerial();
timeout=millis();
char ch = ' ';
char repCh = 0;
uint16_t res;
do{
if(millis()>timeout){
  Serial.print("cmd> ");
  timeout = millis()+300000;
 }
if (Serial.available()){
  ch = Serial.read();
  if((ch>96)&&(ch<123))ch=ch-32;
  if(ch>32) Serial.print(ch);
  switch(ch){
    case 'A' : 
       bp=(uint8_t*)getNum("\nEnter Addr? ");
       Serial.print(" 0x");
       Serial.println((uint16_t)bp,HEX);
       break;
    case 'R' :
       eatSerial();
       b = readTest(bp);
       stat();
       break;
		case 'H' :
		   heapTest(true);
			 break;
    case 'W' : 
       eatSerial();
       writeTest(bp,b);
       stat();
       break;
    case 'N' : 
       setPage();
       break;
    case 'M' : 
			 eatSerial();
			 if(Serial.available()){
				ch=Serial.read();
				Serial.print(ch);
				}
			else ch='0';
			Serial.print("\nMFG Test ");
			switch(ch){
				case '2' : res=mfgTest(2,true);
				  break;
				case '1' : res=mfgTest(1,true);
					break;
				default : res=mfgTest(0,true);
				}
       if (res) Serial.print("test Failed with ");Serial.print(res,DEC);Serial.println(" Errors.");
       break;
    case 'B' : 
       b=getNum("\n Enter Byte? ");
       Serial.print("0x");
       Serial.println(b,HEX);
       break;
    case 'T' : 
       stat();
       testRam(bp,b);
       break;
    case 'F' : eatSerial();
		   if(Serial.available()){
				ch=Serial.read();
			  }
			else ch ='0';
			switch(ch){
				case '2' :fillRamLong(bp,b,true);
				  break;
				case '1' :fillRamWords(bp,b,true);
					break;
				default : fillRamBytes(bp,b,true);
			}
      break;
    case 'D' : displaySector(bp);
       repCh ='D';
			 break;
    case 'Y' : eatSerial();
      stat();
      reTest(bp);
      break;
    case 'P' : eatSerial();
      Serial.println("\nEnter to Continue");
      while(!Serial.available());   
      break;
    case 'S' : readExpander();
      break;    
		case '?' : choices();
       timeout=millis();
       break;
    case 'C' : //continouts next
      continous();
      break;      
    case '\n' : if (repCh=='D'){
      bp+=(uint16_t)0x100;
      displaySector(bp);
			}       
    default : break;
    }
  eatSerial();
 
  }
  }while(ch!='X');

}

void loop(){
uint8_t win[7],err;
uint8_t heapsize=0;
char ch=getch(" heapsize (0..7)\n> ","0123456789\n\r");
if((ch>=48)&&(ch<=57)){
	heapsize = ch-48;
	Serial.print(ch);
	eatSerial();
	if(heapsize>0){
		ch=getch("\nHigh Heap(y,n)\n> ","YN\n\r");
		if(ch=='Y'){
			Serial.print("Selecting High Heap\n");
			X.setHighHeap(true);
			}
		else 	X.setHighHeap(false);
		eatSerial();
		}
  ch=getch("\nWait States (0,1,2,3)\n> ","0123\n\r");
	if((ch>='1')&&(ch<='3')){
		Serial.print("\nSelected ");Serial.print(ch);
		Serial.print(" waitStates\n");
		X.setWait(ch-48);
	}
	else X.setWait(0);
			
	Serial.print("\niniting XMEM=");
	err=X.init(SS,0x27,heapsize);
	Serial.println(err,DEC);
	if (err==0){
		for(err=0;err<7;err++){
			win[err]=err;
		}
		err=X.setWindow(win);
		Serial.print(" setWindow=");
		Serial.println(err,DEC);
	  }
  }

menu();
}
