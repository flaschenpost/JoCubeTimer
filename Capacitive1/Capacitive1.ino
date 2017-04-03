#include <ADCTouch.h>

#define LEFT A1
#define OK A2
#define RIGHT A3

int refl, refr, refOK;     //reference values to remove offset


void setup() 
{
    // No pins to setup, pins can still be used regularly, although it will affect readings

    Serial.begin(9600);
    delay(10);
    Serial.println("READY");

    refl = ADCTouch.read(LEFT, 500);    //create reference values to 

    refr = ADCTouch.read(RIGHT, 1000);    //account for the capacitance of the pad
    
    refOK = ADCTouch.read(OK, 1000);    //account for the capacitance of the pad
    
    pinMode(13, OUTPUT);
    Serial.println(refl);
    Serial.println(refr);
} 

int lastl = 0;
int lastr = 0;
int lastOK = 0;
int limit=43;
int limitOK=48;


int getChanged(int value, int limit, int &last){
  if(value > limit && last == 0){
    last = 1;
    return 1;
  }
  if(value < limit && last == 1){
    last = 0;
    return 0;
  }
  return 0;
}

void loop() 
{
    int valuel = ADCTouch.read(LEFT,40);   //no second parameter
    int valuer = ADCTouch.read(RIGHT,40);   //   --> 100 samples
    int valueOK = ADCTouch.read(OK,100);   //   --> 100 samples

    valuel -= refl;       //remove offset
    valuer -= refr;
    valueOK -= refOK;

    byte left = 0;
    byte right=0;
    byte ok=0;

    left = getChanged(valuel, limit, lastl);

    right = getChanged(valuer, limit, lastr);

    ok = getChanged(valueOK, limitOK, lastOK);

    if(right > 0 || left > 0 || ok > 0){
      Serial.print("left: ");Serial.print(left);Serial.print(" OK: ");Serial.print(ok);Serial.print(" right: ");Serial.println(right);
    }
}
