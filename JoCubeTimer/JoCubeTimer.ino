#include <ADCTouch.h>
#include <LiquidCrystal.h>
#include <DS3231.h>


#define LEFTPAD  A1
#define OKPAD    A2
#define RIGHTPAD A3

#define BLOCKTIME 250


#define START1_BEEP_TIME 7000
#define START1_END_TIME 10000

#define BEEP1_FREQUENZ 440
#define BEEP1_LAENGE 200

#define BEEP2_FREQUENZ 1590
#define BEEP2_LAENGE 50

#define BEEP3_FREQUENZ 1190
#define BEEP3_LAENGE 50

#define AKTUALISIEREN 100

#define SPEAKERPIN A10
// in welchem Status sind wir?
enum State {BEREIT, START1, ANSCHAUEN, PIEP, START2, LOESEN, ANZEIGE, ZUSPAET} state = BEREIT;

int refl, refr, refOK;     //reference values to remove offset

#define   BACKLIGHT_PIN  7
LiquidCrystal lcd(9, 8, 7, 6, 5, 4, BACKLIGHT_PIN, POSITIVE );

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);


unsigned long beepAnschauen;
unsigned long endeAnschauen;

unsigned long startLoesen = 0;
unsigned long anzeigeLoesen = 0;
unsigned long loeseZeit = 0;

unsigned long sperrZeit = 0;

const char startloesen[] = {'S', 't', 'a', 'r', 't', ' ', 'L', 239, 's', 'e', 'n', 0};
const char loesen[] = {'l', 239, 's', 'e', 'n', 0};
const char geloest[] = {'G', 'e', 'l', 239, 's', 't', ':', ' ', 0};

unsigned long best = -1;

const char* stateText(State theState) {
  switch (theState) {
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
void printState(State theState) {
  lcd.setCursor(0, 0);
  lcd.print(stateText(theState));
  lcd.print("            ");
}

void printTime(const char*text, unsigned long theTime) {
  if (text > 0) {
    lcd.setCursor(0, 1);
    lcd.print(text);
    lcd.print("         ");
  }
  int len = 1;
  int secs = theTime / 1000;
  if (secs >= 100000) {
    len = 6;
  }
  else if (secs >= 10000) {
    len = 5;
  }
  else if (secs >= 1000) {
    len = 4;
  }
  else if (secs >= 100) {
    len = 3;
  }
  else if (secs >= 10) {
    len = 2;
  }
  lcd.setCursor(12 - len, 1);
  lcd.print(secs);
  lcd.print(",");
  lcd.print(theTime % 1000);
}


volatile boolean stateChanged = 0;

void evalPins(byte left, byte right) {
  unsigned long now = millis();

  switch (state) {
    case BEREIT:
      if (left == 0 && right == 0) {
        stateChanged = 1;
        state = START1;
      }
      break;

    case START1:
      if (left == 1 && right == 1) {
        state = ANSCHAUEN;
        stateChanged = 1;
        endeAnschauen = now + START1_END_TIME;
        beepAnschauen = now + START1_BEEP_TIME;
      }
      break;
    case ANSCHAUEN:
    case PIEP:
      if (left == 0 && right == 0) {
        state = START2;
        stateChanged = 1;
      }
      break;
      break;
    case START2:
      if (left == 1 && right == 1) {
        state = LOESEN;
        stateChanged = 1;
        startLoesen = now;
        anzeigeLoesen = startLoesen + AKTUALISIEREN;
      }
      break;
      break;
    case LOESEN:
      if (left == 0 && right == 0) {
        state = ANZEIGE;
        loeseZeit = now - startLoesen;
        if (best < 0 || best > loeseZeit) {
          best = loeseZeit;
        }
        stateChanged = 1;
        sperrZeit = now + 500;
        //printTime("FERTIG!",loeseZeit);
        //Serial.println("LOESEN->ANZEIGE");
      }
      break;
    case ANZEIGE:
      if (left == 0 && right == 0 && now > sperrZeit) {
        state = START1;
        //Serial.println("ANZEIGE-> START1");
      }
      break;
  }
}

int limit = 20;

int limitOK = 20;

int actionl = 0;
byte statel = 1;
unsigned long blockedl = 0;

int actionr = 0;
byte stater = 1;
unsigned long blockedr = 0;

void setup() {
  lcd.backlight();
  lcd.begin(16,4);
  // Initialize the rtc object
  rtc.begin();

  printState(BEREIT);

  refl = ADCTouch.read(LEFTPAD, 100);    //create reference values to

  refr = ADCTouch.read(RIGHTPAD, 100);    //account for the capacitance of the pad

  refOK = ADCTouch.read(OKPAD, 100);    //account for the capacitance of the pad

  lcd.setCursor(0,3);lcd.print(refl);
  lcd.setCursor(8,3);lcd.print(refr);

  delay(1000);
  refl = ADCTouch.read(LEFTPAD, 200);    //create reference values to
  refr = ADCTouch.read(RIGHTPAD, 200);    //account for the capacitance of the pad
  refOK = ADCTouch.read(OKPAD, 200);    //account for the capacitance of the pad

  lcd.setCursor(0,3);lcd.print(refl);
  lcd.setCursor(8,3);lcd.print(refr);

  refl += limit;
  refr += limit;
  refOK += limit;
  delay(1000);

  //showTime();
}

void beep1() {
  //Serial.println("beep2");
  tone(SPEAKERPIN, BEEP1_FREQUENZ, BEEP1_LAENGE);
}
void beep2() {
  //Serial.println("beep2");
  tone(SPEAKERPIN, BEEP2_FREQUENZ, BEEP2_LAENGE);
}

void showTime(){
  lcd.setCursor(0,3);
  
  // Send date
  lcd.print(rtc.getDateStr(FORMAT_XS));
  lcd.print(" ");
  // Send time
  lcd.print(rtc.getTimeStr(FORMAT_LONG));
    
}

int lastTime = 0;

void beep3() {
  //Serial.println("beep2");
  long freq = BEEP3_FREQUENZ;
  for (int i = 0; i < 10; i++) {
    tone(SPEAKERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 10);
    freq /= 1.24;
  }
}
void beep4() {
  long freq = BEEP1_FREQUENZ;
  for (int i = 0; i < 9; i++) {
    tone(SPEAKERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 1);
    freq *= 1.27;
  }
}


int raisedl = 0;
int raisedr = 0;

void loop() {
  unsigned long now = millis();
  actionl = 0;
  actionr = 0;
  if(now > blockedl){
    int val = ADCTouch.read(LEFTPAD,40);
    int valuel = (val < refl);
    if(valuel != statel){
      actionl = 1;
      blockedl = now + BLOCKTIME;
      if(valuel == 0 && statel > 0){
        raisedl++;
      }
      statel = valuel;
    }
    /*
    lcd.setCursor(0,1);lcd.print(valuel);
    lcd.setCursor(0,2);lcd.print(raisedl);
    lcd.setCursor(0,3);lcd.print(val);
    // */
  }
  if(now > blockedr){
    int val = ADCTouch.read(RIGHTPAD,40);
    int valuer = (val < refr);
    if(valuer != stater){
      actionr = 1;
      blockedr = now + BLOCKTIME;
      if(valuer == 0){
        raisedr++;
      }
      stater = valuer;
    }
    /*
    lcd.setCursor(8,1);lcd.print(valuer);
    lcd.setCursor(8,2);lcd.print(raisedr);
    lcd.setCursor(8,3);lcd.print(val);
    // */
  }


  if(actionl || actionr){
    evalPins(statel, stater);
  }
  if (stateChanged) {
    printState(state);
    if (state == ANZEIGE) {
      printTime(geloest, loeseZeit);
      beep4();
    }
    if (state == LOESEN ) {
      printTime("RUN", now - startLoesen);
      anzeigeLoesen += AKTUALISIEREN;
    }
    stateChanged = 0;
  }
  if (state == ANSCHAUEN && now > beepAnschauen) {
    beep1();
    state = PIEP;
    printState(PIEP);
    beepAnschauen += 900;
  }
  if (state == PIEP && now > beepAnschauen) {
    beepAnschauen += 250;
    beep2();
  }
  if (state == PIEP && now > endeAnschauen) {
    //Serial.println("Abbrechen...");
    printState(ZUSPAET);
    state = ZUSPAET;
    endeAnschauen = endeAnschauen + 2000;
    beep3();
    return;
  }

  if (state == LOESEN && anzeigeLoesen > 0 && anzeigeLoesen <= now) {
    printTime(0, now - startLoesen);
    anzeigeLoesen += AKTUALISIEREN;
  }
  if (state == ZUSPAET && now > endeAnschauen) {
    printState(BEREIT);
    state = BEREIT;
    endeAnschauen = 0;
  }
  if(now - lastTime > 1000){
    lastTime = now;
    showTime();
  }
//  */
}
