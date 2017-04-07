#include <ADCTouch.h>
#include <LiquidCrystal.h>
#include <DS3231.h>


#define linksPAD  A1
#define OKPAD    A2
#define rechtsPAD A3

#define SETTINGSCHALTER A0

#define BLOCKZEIT 250

#define START1_BEEP_ZEIT 7000
#define START1_END_ZEIT 10000

#define BEEP1_FREQUENZ 440
#define BEEP1_LAENGE 200

#define BEEP2_FREQUENZ 1590
#define BEEP2_LAENGE 50

#define BEEP3_FREQUENZ 1190
#define BEEP3_LAENGE 50

#define AKTUALISIEREN 100

#define LAUTSPRECHERPIN A10

#define BESTENLAENGE 10

#define AUTOMATISCH_ABSCHALTEN 20000l

unsigned long bestenListe[BESTENLAENGE+1] = {0};

volatile boolean statusWechsel = 0;

byte platz = 0;
byte listeOben = 0;

// in welchem Status sind wir?
enum Status {BEREIT, START1, ANSCHAUEN, PIEP, START2, LOESEN, ANZEIGE, ZUSPAET, LOESCHEN} status = BEREIT;

// in welchem SettingStatus sind wir?
enum SettingStatus {TOP10} settingStatus = TOP10;

unsigned int referenzLinks, referenzRechts, referenzOK;     //reference values to remove offset

#define   BACKLIGHT_PIN  7
LiquidCrystal lcd(9, 8, 7, 6, 5, 4, BACKLIGHT_PIN, POSITIVE );

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

byte sensorSchwelle = 30;

byte aktionLinks = 0;
byte statusLinks = 1;
unsigned long blockiertLinks = 0;

byte aktionRechts = 0;
byte statusRechts = 1;
unsigned long blockiertRechts = 0;

byte aktionOK = 0;
byte statusOK = 1;
unsigned long blockiertOK = 0;

unsigned long beepAnschauen;
unsigned long endeAnschauen;

unsigned long startLoesen = 0;
unsigned long anzeigeLoesen = 0;
unsigned long loeseZeit = 0;

unsigned long sperrZeit = 0;

char *startloesen = "Start l.sen    ";
char *loesen      = "l.sen          ";
char *geloest     = "gel.st         ";
char *loeschen    = "L.SCHEN??      ";

unsigned long letzteAktion = 0;
byte schlaeft = 0;

byte istSettings=0;

const char* statusText(Status thestatus) {
  switch (thestatus) {
    case BEREIT:     return "Bereit         ";
    case START1:     return "Start Anschau  ";
    case ANSCHAUEN:  return "Anschauen      ";
    case PIEP:       return "Schnell!!      ";
    case START2:     return startloesen;
    case LOESEN:     return loesen;
    case ANZEIGE:    return "Fertig         ";
    case ZUSPAET:    return "Schade!        ";
    case LOESCHEN:   return loeschen;
  }
}
void printstatus(Status thestatus) {
  lcd.setCursor(0, 0);
  lcd.print(statusText(thestatus));
}

void printzeit(const char*text, unsigned long zeit, byte zeile=1) {
  if (text > 0) {
    lcd.setCursor(0, zeile);
    lcd.print(text);
  }
  int laenge = 1;
  unsigned long sekunden = zeit / 1000;
  if (sekunden >= 1000) {
    laenge = 3;
    sekunden=999;
  }
  else if (sekunden >= 100) {
    laenge = 3;
  }
  else if (sekunden >= 10) {
    laenge = 2;
  }
  lcd.setCursor(12 - laenge, zeile);
  lcd.print(sekunden);
  lcd.print(",");
  int teil = (zeit % 1000l)/10;
  lcd.print(teil);lcd.print(" ");
}



byte zeitEinfuegen(unsigned long zeit){
  byte eplatz =0;
  
  for(byte i=0; i<=BESTENLAENGE; i++){
    if((bestenListe[i] == 0) || (bestenListe[i] > zeit)){
      eplatz = i+1;
      break;
    }
  }
  if(eplatz == 0){
    return 0;
  }

  for(byte i=BESTENLAENGE; i>= eplatz; i--){
    bestenListe[i] = bestenListe[i-1];
  }
  bestenListe[eplatz-1] = zeit;
  if(eplatz == 1){
    printzeit("To BEAT: ",zeit , 3);
  }
  return eplatz;
}
  
void loeschePlatz(byte eplatz){
  if(eplatz==0){
    return;
  }
  for (int i=eplatz-1; i<=BESTENLAENGE-1; i++){
    bestenListe[i] = bestenListe[i+1];
  }
  bestenListe[BESTENLAENGE] = 0;
  if(eplatz == 1){
    printzeit("To BEAT: ",bestenListe[0], 3);    
  }
}

void zeigeTop10(){
  char* comment = "Platz  1";
  for(byte i=0; i<4; i++){
    byte row = (i+listeOben) % BESTENLAENGE;
    comment[6]=' ';
    comment[7]='1'+row;
    if(row == 9){
      comment[6] = '1';
      comment[7] = '0';
    }
    printzeit(comment, bestenListe[row], i);
  }

}

void auswerteSettingsPins(byte links, byte rechts, byte ok) {

  switch (settingStatus) {
    case TOP10:
      if (links == 0) {
        listeOben = (BESTENLAENGE+listeOben-1)%BESTENLAENGE;
      }
      if (rechts == 0) {
        listeOben = (BESTENLAENGE+listeOben+1)%BESTENLAENGE;
      }
      zeigeTop10();
      break;
  }
}

void auswerteSpielePins(unsigned long jetzt, byte links, byte rechts) {

  switch (status) {
    case BEREIT:
      if (links == 0 && rechts == 0) {
        statusWechsel = 1;
        status = START1;
      }
      break;

    case START1:
      if (links == 1 && rechts == 1) {
        status = ANSCHAUEN;
        statusWechsel = 1;
        endeAnschauen = jetzt + START1_END_ZEIT;
        beepAnschauen = jetzt + START1_BEEP_ZEIT;
      }
      break;
    case ANSCHAUEN:
    case PIEP:
      if (links == 0 && rechts == 0) {
        status = START2;
        statusWechsel = 1;
      }
      break;
      break;
    case START2:
      if (links == 1 && rechts == 1) {
        status = LOESEN;
        statusWechsel = 1;
        startLoesen = jetzt;
        anzeigeLoesen = startLoesen + AKTUALISIEREN;
      }
      break;
      break;
    case LOESEN:
      if (links == 0 && rechts == 0) {
        status = ANZEIGE;
        loeseZeit = jetzt - startLoesen;
        platz = zeitEinfuegen(loeseZeit);
        if(platz > 0){
          lcd.setCursor(0,2);
          lcd.print("Platz: ");
          lcd.print(platz);
        }
        statusWechsel = 1;
        sperrZeit = jetzt + 500;
        
        //printzeit("FERTIG!",loeseZeit);
        //Serial.println("LOESEN->ANZEIGE");
      }
      break;
    case ANZEIGE:
      if (links == 0 && rechts == 0 && jetzt > sperrZeit) {
        status = START1;
        //Serial.println("ANZEIGE-> START1");
      }
      break;
  }
}


void setup() {
  lcd.backlight();
  lcd.begin(16,4);
  // Initialize the rtc object
  rtc.begin();
  startloesen[7]=239;
  loesen[1]=239;       printstatus(BEREIT);
  geloest[3]=239;
  loeschen[1]=239;     referenzLinks = ADCTouch.read(linksPAD, 100);    //create reference values to

  referenzRechts = ADCTouch.read(rechtsPAD, 100);    //account for the capacitance of the pad

  referenzOK = ADCTouch.read(OKPAD, 100);

  //lcd.setCursor(0,3);lcd.print(referenzLinks);
  //lcd.setCursor(8,3);lcd.print(referenzRechts);

  delay(200);
  referenzLinks = ADCTouch.read(linksPAD, 200);    //create reference values to
  referenzRechts = ADCTouch.read(rechtsPAD, 200);    //account for the capacitance of the pad
  referenzOK = ADCTouch.read(OKPAD, 200);    //account for the capacitance of the pad

  //lcd.setCursor(0,3);lcd.print(referenzLinks);
  //lcd.setCursor(8,3);lcd.print(referenzRechts);

  referenzLinks += sensorSchwelle;
  referenzRechts += sensorSchwelle;
  referenzOK += sensorSchwelle;
  delay(1000);

  pinMode(SETTINGSCHALTER, INPUT);
  digitalWrite(SETTINGSCHALTER, 1);

  //showzeit();
}

void beep1() {
  //Serial.println("beep2");
  tone(LAUTSPRECHERPIN, BEEP1_FREQUENZ, BEEP1_LAENGE);
}
void beep2() {
  //Serial.println("beep2");
  tone(LAUTSPRECHERPIN, BEEP2_FREQUENZ, BEEP2_LAENGE);
}

void showzeit(){
  
  lcd.setCursor(0,3);
  
  // Send date
  lcd.print(rtc.getDateStr(FORMAT_XS));
  lcd.print("T");
  // Send zeit
  lcd.print(rtc.getTimeStr(FORMAT_LONG));
    
}

int lastzeit = 0;

void beep3() {
  //Serial.println("beep2");
  long freq = BEEP3_FREQUENZ;
  for (int i = 0; i < 10; i++) {
    tone(LAUTSPRECHERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 10);
    freq /= 1.24;
  }
}
void beep4() {
  long freq = BEEP1_FREQUENZ;
  for (int i = 0; i < 9; i++) {
    tone(LAUTSPRECHERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 1);
    freq *= 1.27;
  }
}

inline void pruefeWachauf(unsigned long jetzt){
  if(schlaeft){
    schlaeft=0;
    //lcd.display();
    //lcd.setCursor(1,1);
  }
  letzteAktion=jetzt;
}

void loop() {
  unsigned long jetzt = millis();

  aktionLinks = 0;
  aktionRechts = 0;
  
  if( (istSettings==1 || status == ANZEIGE) && platz > 0){
    int val = ADCTouch.read(OKPAD, 100);
    int valueOK = (val < referenzOK);
    if(valueOK != statusOK){
      aktionOK = 1;
      blockiertOK = jetzt + BLOCKZEIT;
      statusOK = valueOK;
    }
    if(! valueOK && istSettings==0){
      printstatus(LOESCHEN);
      delay(500);
      val = ADCTouch.read(OKPAD, 100);
      lcd.setCursor(8,0);lcd.print(val);
      if(val > referenzOK){
        loeschePlatz(platz);
        status=BEREIT;
        printstatus(BEREIT);
      }
      pruefeWachauf(jetzt);
    }
  }

  if(jetzt > blockiertLinks){
    int val = ADCTouch.read(linksPAD,40);
    int valuel = (val < referenzLinks);
    if(valuel != statusLinks){
      aktionLinks = 1;
      blockiertLinks = jetzt + BLOCKZEIT;
      statusLinks = valuel;
    }
    /*
    lcd.setCursor(0,1);lcd.print(valuel);
    lcd.setCursor(0,2);lcd.print(raisedl);
    lcd.setCursor(0,3);lcd.print(val);
    // */
  }
  if(jetzt > blockiertRechts){
    int val = ADCTouch.read(rechtsPAD,40);
    int valuer = (val < referenzRechts);
    if(valuer != statusRechts){
      aktionRechts = 1;
      blockiertRechts = jetzt + BLOCKZEIT;
      statusRechts = valuer;
    }
    /*
    lcd.setCursor(8,1);lcd.print(valuer);
    lcd.setCursor(8,2);lcd.print(raisedr);
    lcd.setCursor(8,3);lcd.print(val);
    // */
  }

  if(digitalRead(SETTINGSCHALTER) == 0){
    if(istSettings == 0){
      istSettings = 1;
      lcd.clear();
      lcd.setCursor(15,0);lcd.print("S");
      pruefeWachauf(jetzt);
      auswerteSettingsPins(statusLinks, statusRechts, statusOK);
      return;
    }
    istSettings = 1;
  }
  if(digitalRead(SETTINGSCHALTER) == 1){
    if(istSettings == 1){
      lcd.clear();
      lcd.setCursor(15,0);lcd.print("P");
      istSettings = 0;
      pruefeWachauf(jetzt);
      statusWechsel=1;
    }
  }

  if(aktionLinks || aktionRechts){
    pruefeWachauf(jetzt);
    if(istSettings){
      auswerteSettingsPins(statusLinks, statusRechts, statusOK);
    }
    else{
      auswerteSpielePins(jetzt, statusLinks, statusRechts);
    }
  }

  if(letzteAktion + AUTOMATISCH_ABSCHALTEN < jetzt){
    //lcd.noDisplay();
    schlaeft=1;
//    pinMode(7,OUTPUT);
//    digitalWrite(7,1);
  }

  if (statusWechsel) {
    printstatus(status);
    if (status == ANZEIGE) {
      printzeit(geloest, loeseZeit);
      beep4();
    }
    if (status == LOESEN ) {
      anzeigeLoesen += AKTUALISIEREN;
    }
    statusWechsel = 0;
    return;
  }
  if (status == ANSCHAUEN && jetzt > beepAnschauen) {
    beep1();
    status = PIEP;
    printstatus(PIEP);
    beepAnschauen += 900;
  }
  if (status == PIEP && jetzt > beepAnschauen) {
    beepAnschauen += 250;
    beep2();
  }
  if (status == PIEP && jetzt > endeAnschauen) {
    //Serial.println("Abbrechen...");
    printstatus(ZUSPAET);
    status = ZUSPAET;
    endeAnschauen = endeAnschauen + 2000;
    beep3();
    return;
  }

  if (status == LOESEN && anzeigeLoesen > 0 && anzeigeLoesen <= jetzt) {
    printzeit("Run      ", jetzt - startLoesen);
    anzeigeLoesen += AKTUALISIEREN;
  }
  if (status == ZUSPAET && jetzt > endeAnschauen) {
    printstatus(BEREIT);
    status = BEREIT;
    endeAnschauen = 0;
  }
  if(jetzt - lastzeit > 1000){
    lastzeit = jetzt;
   // showzeit();
  }
//  */
}
