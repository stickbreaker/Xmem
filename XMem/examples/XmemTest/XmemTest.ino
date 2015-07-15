#include <XMem.h>
#include <SPI.h>

XMem X;
unsigned long timeout=0;
uint8_t pane,b,page;
uint8_t * bp;


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


void stat(){
  Serial.print("\n pane=0x");
  Serial.print(pane,HEX);
  Serial.print(" page=0x");
  Serial.print(page,HEX);
  Serial.print(" addr=0x");
  Serial.print((uint16_t)bp,HEX);
  Serial.print(" byte=0x");
  Serial.println(b,HEX);
  uint8_t win[7];
  uint8_t er=X.getWindow(win);
  if(er){
    Serial.print(" getWindow failed =");
    Serial.print(er,DEC);
  }
  Serial.print("Panes : ");
  for (uint8_t i=0;i<7;i++){
		if(X.setPane(i+1,win[i])) Serial.print('*');
			else Serial.print(' ');
    Serial.print((uint8_t)(i+1),DEC);
    Serial.print("  ");
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
  Serial.print("N - set pane/page\nAnnnn - Addr\nBnn - byte\nT - Test page/pane\nY - reTest\nF - Fill"
   "	pane/page\nD - disp\nP - Pause\n");
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


void setup(){
SPI.begin();
Serial.begin(9600);
}


void menu(){
eatSerial();
timeout=millis();
char ch = ' ';
char repCh = 0;
uint16_t res;
do{
if(millis()>timeout){
  Serial.print("\ncmd> ");
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
    case 'N' : 
       setPage();
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
			 fillRamBytes(bp,b,true);
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
		case '?' : choices();
       timeout=millis();
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
err=X.init(SS,0x27,0);
if (err==0){
	for(err=0;err<7;err++){
		win[err]=err;
	  }
	err=X.setWindow(win);
	Serial.print(" setWindow=");
	Serial.println(err,DEC);
  menu();
  }
else {
	Serial.print(" Memory Panes Init Failure =");
	Serial.println(err,DEC);
  }
}
