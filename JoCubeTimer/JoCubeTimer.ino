#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


#define START1_BEEP_TIME 7000
#define START1_END_TIME 10000

#define BEEP1_FREQUENZ 440
#define BEEP1_LAENGE 200

#define BEEP2_FREQUENZ 1590
#define BEEP2_LAENGE 50

#define BEEP3_FREQUENZ 1190
#define BEEP3_LAENGE 50

#define AKTUALISIEREN 100

// in welchem Status sind wir?
enum State {BEREIT, START1, ANSCHAUEN, PIEP, START2, LOESEN, ANZEIGE, ZUSPAET} state = BEREIT;

LiquidCrystal_I2C lcd(0x3F,16,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// welche Kontakte zu den Tastern
int taster1pin = 2;
int taster2pin = 3;
int speakerPin = 5;

unsigned long beepAnschauen;
unsigned long endeAnschauen;

unsigned long startLoesen = 0;
unsigned long anzeigeLoesen = 0;
unsigned long loeseZeit = 0;

unsigned long sperrZeit = 0;

const char startloesen[]={'S','t','a','r','t',' ','L',239,'s','e','n',0};
const char loesen[]={'l',239,'s','e','n',0};
const char geloest[]={'G','e','l',239,'s','t',':',' ',0};

unsigned long best = -1;

const char* stateText(State theState){
  switch(theState){
    case BEREIT: return "Bereit";
    case START1: return "Start Anschau";
    case ANSCHAUEN: return "Anschauen";
    case PIEP: return "Schnell!!";
    case START2: return startloesen;
    case LOESEN: return loesen;
    case ANZEIGE: return "Fertig";
    case ZUSPAET: return "Schade!";
  }
}
void printState(State theState){
  lcd.setCursor(0,0);
  lcd.print(stateText(theState));
  lcd.print("            ");
}

void printTime(const char*text, unsigned long theTime){
  if(text > 0){
   lcd.setCursor(0,1);
   lcd.print(text);
   lcd.print("         ");
  }
  int len = 1;
  int secs = theTime/1000;
  if(secs>= 100000){
    len=6;
  }
  else if(secs>= 10000){
    len=5;
  }
  else if(secs>= 1000){
    len=4;
  }
  else if(secs>= 100){
    len=3;
  }
  else if(secs>= 10){
    len=2;
  }
  lcd.setCursor(12-len,1);
  lcd.print(secs);
  lcd.print(",");
  lcd.print(theTime % 1000);
}


volatile boolean stateChanged = 0;

void pinChanged(){
  unsigned long now = millis();

  switch(state){
    case BEREIT:
      if(digitalRead(taster1pin)==LOW && digitalRead(taster2pin)== LOW){
        stateChanged = 1;
        state = START1;
      }
    break;
    
    case START1:
      if(digitalRead(taster1pin)==HIGH && digitalRead(taster2pin)== HIGH){
        state=ANSCHAUEN;
        stateChanged = 1;
        endeAnschauen = now + START1_END_TIME;
        beepAnschauen = now + START1_BEEP_TIME;
      }
      break;
    case ANSCHAUEN:
    case PIEP:
    if(digitalRead(taster1pin)==LOW && digitalRead(taster2pin)== LOW){
        state=START2;
        stateChanged = 1;
      }
      break;
    break;
    case START2:
      if(digitalRead(taster1pin)==HIGH && digitalRead(taster2pin)== HIGH){
        state=LOESEN;
        stateChanged = 1;
        startLoesen = now;
        anzeigeLoesen = startLoesen+AKTUALISIEREN;
      }
      break;
    break;
    case LOESEN:
      if(digitalRead(taster1pin)==LOW && digitalRead(taster2pin)== LOW){
        state=ANZEIGE;
        loeseZeit = now - startLoesen;
        if(best < 0 || best > loeseZeit){
          best = loeseZeit;
        }
        stateChanged = 1;
        sperrZeit = now + 500;
        //printTime("FERTIG!",loeseZeit);
        //Serial.println("LOESEN->ANZEIGE");
      }
    break;
    case ANZEIGE:
      if(digitalRead(taster1pin)==LOW && digitalRead(taster2pin)== LOW && now > sperrZeit){
        state=START1;
        //Serial.println("ANZEIGE-> START1");
      }
    break;
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(taster1pin, INPUT);
  pinMode(taster2pin, INPUT);
  digitalWrite(taster1pin, HIGH);
  digitalWrite(taster2pin, HIGH);

  attachInterrupt(digitalPinToInterrupt(taster1pin), pinChanged, CHANGE);  
  attachInterrupt(digitalPinToInterrupt(taster2pin), pinChanged, CHANGE);  
  printState(BEREIT);

}

void beep1(){
  //Serial.println("beep2");
  tone(speakerPin, BEEP1_FREQUENZ, BEEP1_LAENGE);
}
void beep2(){
  //Serial.println("beep2");
  tone(speakerPin, BEEP2_FREQUENZ, BEEP2_LAENGE);
}

void beep3(){
  //Serial.println("beep2");
  long freq=BEEP3_FREQUENZ;
  for(int i=0; i<10; i++){
    tone(speakerPin, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE+10);
    freq /= 1.24;
  }
}
void beep4(){
  long freq=BEEP1_FREQUENZ;
  for(int i=0; i<9; i++){
    tone(speakerPin, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE+1);
    freq *= 1.27;
  }
}


void loop() {
  unsigned long now = millis();
  if(stateChanged){
    printState(state);
    if(state == ANZEIGE){
      printTime(geloest, loeseZeit);
      beep4();
    }
    if(state == LOESEN ){
      printTime("RUN", now - startLoesen);
      anzeigeLoesen += AKTUALISIEREN;
    }
    stateChanged = 0;
  }
  if(state == ANSCHAUEN && now > beepAnschauen){
    beep1();
    state=PIEP;
    printState(PIEP);
    beepAnschauen += 900;
  }
  if(state == PIEP && now > beepAnschauen){
    beepAnschauen += 250;
    beep2();
  }
  if(state == PIEP && now > endeAnschauen){
    //Serial.println("Abbrechen...");
    printState(ZUSPAET);
    state = ZUSPAET;
    endeAnschauen = endeAnschauen + 2000;
    beep3();
    return;
  }

  if(state == LOESEN && anzeigeLoesen > 0 && anzeigeLoesen <= now){
    printTime(0, now - startLoesen);
    anzeigeLoesen += AKTUALISIEREN;
  }
  if(state == ZUSPAET && now > endeAnschauen){
    printState(BEREIT);
    state=BEREIT;
    endeAnschauen=0;
  }

}
