//We always have to include the library
#include "LedControl.h"
#include <ADCTouch.h>

/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,11,10,1);
int left = 0;
int right = 0;
#define SCHWELLE 3
/* we always wait a bit between updates of the display */
unsigned long delaytime=200;

void setup() {
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,1);
  /* and clear the display */
  lc.clearDisplay(0);

  writeArduinoOn7Segment();
  scrollDigits();
}


/*
 This method will display the characters for the
 word "Arduino" one after the other on digit 0. 
 */
void writeArduinoOn7Segment() {
  lc.setChar(0,0,'a',false);
  delay(delaytime);
  lc.setRow(0,0,0x05);
  delay(delaytime);
  lc.setChar(0,0,'d',false);
  delay(delaytime);
  lc.setRow(0,0,0x1c);
  delay(delaytime);
  lc.setRow(0,0,B00010000);
  delay(delaytime);
  lc.setRow(0,0,0x15);
  delay(delaytime);
  lc.setRow(0,0,0x1D);
  delay(delaytime);
  lc.clearDisplay(0);
  delay(delaytime);
} 

void writeTouch(int left, int right){
  lc.clearDisplay(0);
  byte pos = 0;
  while (left > 0 && pos < 4){
    lc.setDigit(0, 4+pos, left%10, false);
    left /= 10;  
    pos++;
  }
  pos = 0;
  while(right > 0 && pos < 4){
    lc.setDigit(0, pos, right%10, false);
    right /= 10; 
    pos++;
  }
}
/*
  This method will scroll all the hexa-decimal
 numbers and letters on the display. You will need at least
 four 7-Segment digits. otherwise it won't really look that good.
 */
void scrollDigits() {
  for(int i=0;i<17;i++) {
    lc.setDigit(0,7,i,false);
    lc.setDigit(0,6,i+1,false);
    lc.setDigit(0,5,i+2,false);
    lc.setDigit(0,4,i+3,false);
    lc.setDigit(0,3,i+4,false);
    lc.setDigit(0,2,i+5,false);
    lc.setDigit(0,1,i+6,false);
    lc.setDigit(0,0,i+7,false);
    delay(delaytime);
  }
  lc.clearDisplay(0);
  delay(delaytime);
}

void loop() {
  int newleft = ADCTouch.read(A0, 70);
  int newright = ADCTouch.read(A1, 70);
  byte changed = 0;
  if(newleft < left-SCHWELLE || newleft > left+SCHWELLE){
    left = newleft;
    changed = 1;
  }
  if(newright < right-SCHWELLE || newright> right+SCHWELLE){
    right = newright;
    changed = 1;
  }
  if(changed){
    writeTouch(left, right); 
  }
}
