#include <ADCTouch.h>
#include <LiquidCrystal.h>
#include <DS3231.h>
#include <SD.h>

#define OKPAD    A1
#define RECHTSPAD A2
#define LINKSPAD  A3

#define SETTINGSCHALTER A0

#define BLOCKZEIT 150

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

#define SDPIN 1

char* top10Filename = "JOTOP3X3.TXT";
const char* logFilename = "JOLOG";

unsigned long bestenListe[BESTENLAENGE + 1] = {0};

volatile boolean statusWechsel = 0;

byte platz = 0;
byte listeOben = 0;
byte hasSDCard = 0;


// in welchem Status sind wir?
enum Status {S_BEREIT, S_START1, S_ANSCHAUEN, S_PIEP, S_START2, S_LOESEN, S_ZUSPAET, S_BESTAETIGEN} status = S_BEREIT;

// in welchem SettingStatus sind wir?
enum SettingStatus {SET_TOP10, SET_MENU, SET_LOESCHEN, SET_TON, SET_SHOWZEIT, SET_TYP, SET_AENDERE_ZEIT} settingStatus = SET_TOP10;

enum MENUEEINTRAG {MENU_ZEIGETOP10, MENU_LOESCHEN, MENU_TON, MENU_ZEIT, MENU_TYP, MENU_ENDE} aktiverEintrag = MENU_ZEIGETOP10;

enum ZEIT_SETZE_TYP {ZEIT_JAHR, ZEIT_MONAT, ZEIT_TAG, ZEIT_STUNDE, ZEIT_MINUTE, ZEIT_OK} zeitSetzeTyp = ZEIT_JAHR;
unsigned int referenzLinks, referenzRechts, referenzOK;     //reference values to remove offset

enum CUBETYP {TYP3x3, TYP2x2, TYPSQ1, TYPENDE} cubeTyp = TYP3x3;
#define   BACKLIGHT_PIN  7
LiquidCrystal lcd(9, 8, 7, 6, 5, 4, BACKLIGHT_PIN, POSITIVE );

// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);

byte sensorSchwelle = 50;

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

Time time;

char *startloesen = "Start l.sen    ";
char *loesen      = "l.sen          ";
char *geloest     = "gel.st         ";
char *loeschen    = "L.schen        ";
char *bestaetigen = "Best.tigen     ";
char *ungueltig   = "ung.ltig       ";

byte istSettings = 0;

byte istStumm = 0;

void printTyp(CUBETYP typ, byte i=0){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Typ");
  lcd.setCursor(0, i);
  switch(typ){
    case TYP3x3: lcd.print("3x3    ");break;
    case TYP2x2: lcd.print("2x2    ");break;
    case TYPSQ1: lcd.print("Square1");break;
  }
}

void printStatus(Status thestatus, byte i=0) {
  lcd.setCursor(0, i);
  switch (thestatus) {
    case S_BEREIT:     lcd.print( "Bereit         ");break;
    case S_START1:     lcd.print( "Start Anschau  ");break;
    case S_ANSCHAUEN:  lcd.print( "Anschauen      ");break;
    case S_PIEP:       lcd.print( "Schnell!!      ");break;
    case S_START2:     lcd.print(startloesen);break;
    case S_LOESEN:     lcd.print(loesen);break;
    case S_ZUSPAET:    lcd.print( "Schade!        ");break;
    case S_BESTAETIGEN: lcd.print( "Richtig?       ");break;
  }
}

void printMenu(Status thestatus, byte i=0) {
  lcd.setCursor(0, i);
  switch (thestatus) {
    case MENU_ZEIGETOP10:      lcd.print("Top10         ");break;
    case MENU_LOESCHEN:        lcd.print(loeschen);break;
    case MENU_TON:             lcd.print("Ton           ");break;
    case MENU_ZEIT:            lcd.print("Zeit          ");break;
    case MENU_TYP:             lcd.print("Typ           ");break;
    case MENU_ENDE:            lcd.print("--------------");break;
  }
}

void writeSDStatus() {
  File top10File = SD.open(top10Filename, FILE_WRITE);
  if (top10File) {
    top10File.seek(0);
    for (int i = 0; i < BESTENLAENGE; i++) {
      top10File.println(bestenListe[i]);
      // Serial.print("SD::written "); Serial.println(bestenListe[i]);
    }
    top10File.close();
    lcd.setCursor(13, 0);
    lcd.print("WSD");

  }
}


void printzeit(const char*text, unsigned long zeit, byte zeile = 1) {
  if (text > 0) {
    lcd.setCursor(0, zeile);
    lcd.print(text);
  }
  int laenge = 1;
  lcd.setCursor(8, zeile);
  lcd.print("      ");
  unsigned long sekunden = zeit / 1000;
  if (sekunden >= 1000) {
    laenge = 3;
    sekunden = 999;
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
  int teil = (zeit % 1000l) ;
  lcd.print(teil); lcd.print(" ");
}


void showzeit(const char* msg, const byte pos) {

  lcd.setCursor(0, 3);

  // Send date
  lcd.print(rtc.getDateStr(FORMAT_XS));
  lcd.print(" ");
  // Send zeit
  lcd.print(rtc.getTimeStr(FORMAT_LONG));
  if (msg > 0) {
    lcd.setCursor(0, 2);
    lcd.print(msg);
  }
  if (pos > 0) {
    lcd.setCursor(pos, 3);
    lcd.blink();
  }
  else {
    lcd.noBlink();
  }

}

byte zeitEinfuegen(unsigned long zeit) {
  byte eplatz = 0;

  for (byte i = 0; i <= BESTENLAENGE; i++) {
    if ((bestenListe[i] == 0) || (bestenListe[i] > zeit)) {
      eplatz = i + 1;
      break;
    }
  }
  if (eplatz == 0) {
    return 0;
  }

  for (byte i = BESTENLAENGE; i >= eplatz; i--) {
    bestenListe[i] = bestenListe[i - 1];
  }
  bestenListe[eplatz - 1] = zeit;
  if (eplatz == 1) {
    // Serial.print("bestZeit: ");Serial.println(zeit);
    printzeit("To BEAT: ", zeit , 3);
  }
  writeSDStatus();
  return eplatz;
}

void loeschePlatz(byte eplatz) {
  if (eplatz == 0) {
    return;
  }
  for (int i = eplatz - 1; i <= BESTENLAENGE - 1; i++) {
    bestenListe[i] = bestenListe[i + 1];
  }
  bestenListe[BESTENLAENGE] = 0;
  if (eplatz == 1) {
    printzeit("To BEAT: ", bestenListe[0], 3);
  }
  writeSDStatus();
}

void loadCubetype(){
  switch(cubeTyp){
    case TYP3x3: strncpy(top10Filename+5, "3X3", 3);break;
    case TYP2x2: strncpy(top10Filename+5, "2X2", 3);break;
    case TYPSQ1: strncpy(top10Filename+5, "SQ1", 3);break;
  }
  lcd.setCursor(0,0);
  lcd.print(top10Filename);
  delay(1500);
  readSDStatus();
}

void zeigeTop10() {
  char* comment = "Platz  1";
  for (byte i = 0; i < 4; i++) {
    byte row = (i + listeOben) % BESTENLAENGE;
    comment[6] = ' ';
    comment[7] = '1' + row;
    if (row == 9) {
      comment[6] = '1';
      comment[7] = '0';
    }
    printzeit(comment, bestenListe[row], i);
  }
}

void zeigeMenue() {
  lcd.clear();
  for (int i = 0; i < 4; i++) {
    printMenu((aktiverEintrag + i) % (MENU_ENDE + 1), i);
  }
}

void zeigeStummEinstellung() {
  lcd.clear();
  if (istStumm) {
    lcd.print("Stumm");
  }
  else {
    lcd.print("Mit Ton");
  }
}

inline byte auswertePlusMinus(byte links, byte rechts, unsigned int &wert) {
  if (links == 0) {
    wert--;
    return;
  }
  if (rechts == 0) {
    wert++;
    return;
  }
}
inline byte auswertePlusMinus(byte links, byte rechts, byte &wert) {
  if (links == 0) {
    wert--;
    return;
  }
  if (rechts == 0) {
    wert++;
    return;
  }
}

void auswerteSettingsPins(byte links, byte rechts, byte ok) {

  switch (settingStatus) {
    case SET_AENDERE_ZEIT:
      if (time.year == 0) {
        time.year = 2017;
      }
      if (zeitSetzeTyp == ZEIT_OK && ok == 0) {
        rtc.setTime(time.hour, time.min, 0);
        rtc.setDate(time.date, time.mon, time.year);
        zeitSetzeTyp = ZEIT_JAHR;
        settingStatus = SET_SHOWZEIT;
        break;
      }
      if (zeitSetzeTyp == ZEIT_JAHR) {
        if (ok == 0) {
          zeitSetzeTyp = ZEIT_MONAT;
          break;
        }
        auswertePlusMinus(links, rechts, time.year);
        rtc.setDate(time.date, time.mon, time.year);
        lcd.setCursor(8,2);
        lcd.print(time.year);
        showzeit("Jahr:", 5);

        break;
      }
      if (zeitSetzeTyp == ZEIT_MONAT) {
        if (ok == 0) {
          zeitSetzeTyp = ZEIT_TAG;
          break;
        }
        auswertePlusMinus(links, rechts, time.mon);
        rtc.setDate(time.date, time.mon, time.year);
        showzeit("Monat:", 5);

        break;
      }
      if (zeitSetzeTyp == ZEIT_TAG) {
        if (ok == 0) {
          zeitSetzeTyp = ZEIT_STUNDE;
          break;
        }
        auswertePlusMinus(links, rechts, time.date);
        rtc.setDate(time.date, time.mon, time.year);
        showzeit("Tag:", 2);

        break;
      }
      if (zeitSetzeTyp == ZEIT_STUNDE) {
        if (ok == 0) {
          zeitSetzeTyp = ZEIT_MINUTE;
          break;
        }
        auswertePlusMinus(links, rechts, time.hour);
        rtc.setTime(time.hour, time.min, 0);
        showzeit("Stunde:", 8);
        break;
      }
      if (zeitSetzeTyp == ZEIT_MINUTE) {
        if (ok == 0) {
          zeitSetzeTyp = ZEIT_OK;
          break;
        }        
        auswertePlusMinus(links, rechts, time.min);
        rtc.setTime(time.hour, time.min, 0);
        showzeit("Minute:", 12);

        break;
      }
      break;

    case SET_SHOWZEIT:
      lcd.clear();
      showzeit("Zeit:", 0);
      if (ok == 0) {
        settingStatus = SET_MENU;
        break;
      }
      if (links == 0 || rechts == 0) {
        Time time = rtc.getTime();
        if (time.year == 0) {
          time.year = 2017;
        }
        settingStatus = SET_AENDERE_ZEIT;
        break;
      }
      break;
    case SET_TYP:
      printTyp(cubeTyp,1);
      if (rechts == 0){
        cubeTyp = (cubeTyp + 1 ) % (TYPENDE);
        break;
      }
      if (links == 0){
        cubeTyp = (cubeTyp - 1 + TYPENDE ) % (TYPENDE);
        break;
      }
      if (ok == 0) {
        loadCubetype();
        settingStatus = SET_MENU;
      }
      break;
    case SET_TON:
      if (rechts == 0 || links == 0) {
        istStumm = 1 - istStumm;
      }
      if (ok == 0) {
        settingStatus = SET_MENU;
      }
      zeigeStummEinstellung();
      break;
    case SET_TOP10:
      if (ok == 0) {
        settingStatus = SET_MENU;
        aktiverEintrag = MENU_ZEIGETOP10;

        zeigeMenue();
        break;
      }
      if (links == 0) {
        listeOben = (BESTENLAENGE + listeOben - 1) % BESTENLAENGE;
        break;
      }
      if (rechts == 0) {
        listeOben = (BESTENLAENGE + listeOben + 1) % BESTENLAENGE;
        break;
      }
      zeigeTop10();
      break;
    case SET_LOESCHEN:
      if (ok == 0) {
        loeschePlatz(listeOben + 1);

        zeigeTop10();
        settingStatus = SET_MENU;
        break;
      }
      if (links == 0) {
        listeOben = (BESTENLAENGE + listeOben - 1) % BESTENLAENGE;
        break;
      }
      if (rechts == 0) {
        listeOben = (BESTENLAENGE + listeOben + 1) % BESTENLAENGE;
        break;
      }
      zeigeTop10();
      break;
    case SET_MENU:
      if (links == 0) {
        aktiverEintrag = (aktiverEintrag + MENU_ENDE ) % (MENU_ENDE + 1);
        break;
      }
      if (rechts == 0) {
        aktiverEintrag = (aktiverEintrag + 1) % (MENU_ENDE + 1);
        break;
      }
      if (ok == 0) {
        switch (aktiverEintrag) {
          case MENU_ZEIGETOP10:
            settingStatus = SET_TOP10;
            break;
          case MENU_LOESCHEN:
            settingStatus = SET_LOESCHEN;
            break;
          case MENU_TON:
            settingStatus = SET_TON;
            zeigeStummEinstellung();
            break;
          case MENU_ZEIT:
            settingStatus = SET_SHOWZEIT;
            break;
          case MENU_TYP:
            settingStatus = SET_TYP;
            break;
          case MENU_ENDE:
            break;
        }
      }
      zeigeMenue();
      break;
    default:
      settingStatus = SET_MENU;
      zeigeMenue();
      break;

  }
}

void auswerteSpielePins(unsigned long jetzt, byte links, byte rechts, byte ok) {

  // Serial.print(jetzt/1000l);Serial.print(" auswerte: ");Serial.print(links);Serial.print(rechts);Serial.print(ok);Serial.print(" ");Serial.println(status);
  switch (status) {
    case S_BEREIT:
      if (links == 0 && rechts == 0) {
        statusWechsel = 1;
        status = S_START1;
      }
      break;

    case S_START1:
      if (links == 1 && rechts == 1) {
        status = S_ANSCHAUEN;
        statusWechsel = 1;
        endeAnschauen = jetzt + START1_END_ZEIT;
        beepAnschauen = jetzt + START1_BEEP_ZEIT;
      }
      break;
    case S_ANSCHAUEN:
    case S_PIEP:
      if (links == 0 && rechts == 0) {
        status = S_START2;
        statusWechsel = 1;
      }
      break;
    case S_START2:
      if (links == 1 && rechts == 1) {
        status = S_LOESEN;
        statusWechsel = 1;
        startLoesen = jetzt;
        anzeigeLoesen = startLoesen + AKTUALISIEREN;
      }
      break;
    case S_LOESEN:
      if (links == 0 && rechts == 0) {
        status = S_BESTAETIGEN;
        lcd.setCursor(0, 0);
        loeseZeit = jetzt - startLoesen;
        // Serial.println(loeseZeit);
        statusLinks = 1;
        statusRechts = 1;
        aktionLinks = 0;
        aktionRechts = 0;
        statusWechsel = 1;
        status = S_BESTAETIGEN;
        printzeit(geloest, loeseZeit);
        beep4();
        jetzt = millis();
        blockiertOK = jetzt + BLOCKZEIT;
        blockiertRechts = jetzt + 5 * BLOCKZEIT;
        blockiertLinks = jetzt + 5 * BLOCKZEIT;
        // Serial.print("jetzt=");Serial.print(jetzt);Serial.print(" ");Serial.println(blockiertOK);
      }
      break;
    case S_BESTAETIGEN:
      if (ok == 1 && (links == 0 && rechts == 0)) {
        // Serial.print("ungueltig / ");Serial.println(ungueltig);
        lcd.setCursor(0, 1);
        lcd.print(ungueltig);
        statusWechsel = 1;
        status = S_BEREIT;
      }
      if (ok == 0) {
        // Serial.print("bestaetigt: l=");Serial.println(loeseZeit);
        platz = zeitEinfuegen(loeseZeit);
        lcd.setCursor(0, 2);
        if (platz > 0) {
          lcd.print("Platz: ");
          lcd.print(platz);
        }
        else {
          lcd.print("          ");
        }
        statusWechsel = 1;
        status = S_BEREIT;
      }

      break;
    default:
      status = S_BEREIT;
      printStatus(status);
      break;
  }
}

void readSDStatus() {
  // Serial.print("called readSDStatus ");Serial.println(top10Filename);
  if (SD.exists(top10Filename)) {
    // Serial.print("exists");Serial.println(top10Filename);
    File top10File = SD.open(top10Filename, FILE_READ);

    /*
       if (top10File) {
       Serial.println("dummy byte content:");
       while (top10File.available()) {
       Serial.print((char)top10File.read());
       }
       top10File.seek(0);
       }
       Serial.println("-----");
    */
    if (top10File) {
      top10File.seek(0);
      int zeileNr = 0;
      while (top10File.available()) {
        char zeile[20] = {0};
        top10File.readBytesUntil(0x0a, zeile, 19);
        // Serial.print("Zeile ");Serial.println(zeile);
        String szeile(zeile);
        // Serial.print("String Zeile "); Serial.println(szeile);
        bestenListe[zeileNr] = szeile.toInt();
        // Serial.print("inhalt: "); Serial.println(bestenListe[zeileNr]);
        zeileNr++;
      }
      top10File.close();
    }

  }
  // else{
  // Serial.println("not found!");
  // }
}

void setup() {
  pinMode(SDPIN, OUTPUT);
  digitalWrite(SDPIN, 1);
  lcd.backlight();
  lcd.begin(16, 4);
  delay(20);

  // Initialize the rtc object
  rtc.begin();
  startloesen[7] = 239;
  loesen[1] = 239;
  printStatus(S_BEREIT);
  geloest[3] = 239;

  loeschen[1] = 239;
  if (0) {
    Serial.begin(9600);
    for (int i = 0; i < 1000; i++) {
      if (Serial) {
        break;
      }
      delay(1);
    }
  }

  if (SD.begin(SDPIN)) {
    hasSDCard = 1;
    readSDStatus();
    lcd.setCursor(14, 0);
    lcd.print("SD");
  }

  bestaetigen[4] = 225;
  ungueltig[3]  = 245;

  /*
     pinMode(1, OUTPUT);
     digitalWrite(1,HIGH);
     delay(200);
     digitalWrite(1,LOW);
     delay(200);
     pinMode(1, INPUT);
     if (!SD.begin(1)) {
     lcd.setCursor(0,2);
     lcd.print("NO SD");
     }
     else{
     lcd.print("YEAH, SD");
     }
     delay(2000);
    // */
  // Serial.begin(38400);
  // while(!Serial){};
  // Serial.println("OK, Start!");


  // dummy first reference creation seems to give other results on battery power
  int dummy = ADCTouch.read(LINKSPAD, 80);  //create reference values to
  dummy = ADCTouch.read(RECHTSPAD, 80);    //account for the capacitance of the pad
  dummy = ADCTouch.read(OKPAD, 80);


  referenzLinks = ADCTouch.read(LINKSPAD, 200);    //create reference values to
  referenzRechts = ADCTouch.read(RECHTSPAD, 200);    //account for the capacitance of the pad
  referenzOK = ADCTouch.read(OKPAD, 200);    //account for the capacitance of the pad

  //lcd.setCursor(0,3);lcd.print(referenzLinks);
  //lcd.setCursor(8,3);lcd.print(referenzRechts);

  referenzLinks += sensorSchwelle;
  referenzRechts += sensorSchwelle;
  referenzOK += sensorSchwelle / 3;
  delay(100);

  pinMode(SETTINGSCHALTER, INPUT);
  digitalWrite(SETTINGSCHALTER, 1);

  showzeit(0, 0);
}

void beep1() {
  //Serial.println("beep2");
  if (istStumm) {
    return;
  }
  tone(LAUTSPRECHERPIN, BEEP1_FREQUENZ, BEEP1_LAENGE);
}
void beep2() {
  if (istStumm) {
    return;
  }
  //Serial.println("beep2");
  tone(LAUTSPRECHERPIN, BEEP2_FREQUENZ, BEEP2_LAENGE);
}


int lastzeit = 0;

void beep3() {
  //Serial.println("beep2");
  if (istStumm) {
    return;
  }
  long freq = BEEP3_FREQUENZ;
  for (int i = 0; i < 10; i++) {
    tone(LAUTSPRECHERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 10);
    freq /= 1.24;
  }
}
void beep4() {
  if (istStumm) {
    return;
  }
  long freq = BEEP1_FREQUENZ;
  for (int i = 0; i < 9; i++) {
    tone(LAUTSPRECHERPIN, freq, BEEP3_LAENGE);
    delay(BEEP3_LAENGE + 1);
    freq *= 1.27;
  }
}

void debugout(char *msg) {
  lcd.setCursor(0, 2);
  lcd.print(msg);
  delay(1500);
}
void debugout(char *msg, int x) {
  lcd.setCursor(0, 2);
  lcd.print(msg); lcd.print(" "); lcd.print(x);
  delay(1500);
}
void debugout(char *msg, int x, int y, int z) {
  lcd.setCursor(0, 2);
  lcd.print(msg); lcd.print(" "); lcd.print(x); lcd.print(" "); lcd.print(y); lcd.print(" "); lcd.print(z);
  delay(1500);
}

void loop() {
  unsigned long jetzt = millis();
  /** DEBUG
    lcd.setCursor(0,0);lcd.print("L ");lcd.print(ADCTouch.read(LINKSPAD, 100));
    lcd.setCursor(0,1);lcd.print("R ");lcd.print(ADCTouch.read(RECHTSPAD, 100));
    lcd.setCursor(0,2);lcd.print("O ");lcd.print(ADCTouch.read(OKPAD, 100));
    delay(50);
    return;
  */

  aktionLinks = 0;
  aktionRechts = 0;
  aktionOK = 0;

  if (jetzt > blockiertOK) {
    int wert = ADCTouch.read(OKPAD, 100);
    byte nichtsOK = (wert < referenzOK);
    if (nichtsOK != statusOK) {
      aktionOK = 1;
      blockiertOK = jetzt + BLOCKZEIT;
      statusOK = nichtsOK;
    }
  }

  if (jetzt > blockiertLinks) {
    int wert = ADCTouch.read(LINKSPAD, 40);
    byte nichtsLinks = (wert < referenzLinks);
    if (nichtsLinks != statusLinks) {
      aktionLinks = 1;
      blockiertLinks = jetzt + BLOCKZEIT;
      statusLinks = nichtsLinks;
    }
    /*
       lcd.setCursor(0,1);lcd.print(nichtsLinks);
       lcd.setCursor(0,2);lcd.print(raisedl);
       lcd.setCursor(0,3);lcd.print(wert);
      // */
  }

  if (jetzt > blockiertRechts) {
    int wert = ADCTouch.read(RECHTSPAD, 40);
    byte nichtsRechts = (wert < referenzRechts);
    if (nichtsRechts != statusRechts) {
      aktionRechts = 1;
      blockiertRechts = jetzt + BLOCKZEIT;
      statusRechts = nichtsRechts;
    }
    /*
       lcd.setCursor(8,1);lcd.print(nichtsRechts);
       lcd.setCursor(8,2);lcd.print(raisedr);
       lcd.setCursor(8,3);lcd.print(wert);
      // */
  }

  if (digitalRead(SETTINGSCHALTER) == 0) {
    if (istSettings == 0) {
      istSettings = 1;
      lcd.clear();
      lcd.setCursor(15, 0); lcd.print("S");
      settingStatus = SET_TOP10;
      status = S_BEREIT;
      auswerteSettingsPins(statusLinks, statusRechts, statusOK);
      return;
    }
    istSettings = 1;
  }
  if (digitalRead(SETTINGSCHALTER) == 1) {
    if (istSettings == 1) {
      lcd.clear();
      lcd.setCursor(15, 0); lcd.print("P");
      settingStatus = SET_TOP10;
      status = S_BEREIT;
      istSettings = 0;
      statusWechsel = 1;
    }
  }

  if (aktionOK || aktionLinks || aktionRechts) {
    if (istSettings) {
      auswerteSettingsPins(statusLinks, statusRechts, statusOK);
    }
    else {
      auswerteSpielePins(jetzt, statusLinks, statusRechts, statusOK);
    }
  }

  if (statusWechsel) {
    printStatus(status);
    if (status == S_BESTAETIGEN ) {
      printzeit(geloest, loeseZeit);
    }
    if (status == S_LOESEN ) {
      anzeigeLoesen += AKTUALISIEREN;
    }
    statusWechsel = 0;
    return;
  }
  if (status == S_ANSCHAUEN && jetzt > beepAnschauen) {
    beep1();
    status = S_PIEP;
    printStatus(S_PIEP);
    beepAnschauen += 900;
  }
  if (status == S_PIEP && jetzt > beepAnschauen) {
    beepAnschauen += 250;
    beep2();
  }
  if (status == S_PIEP && jetzt > endeAnschauen) {
    //Serial.println("Abbrechen...");
    printStatus(S_ZUSPAET);
    status = S_ZUSPAET;
    endeAnschauen = endeAnschauen + 2000;
    beep3();
    return;
  }

  if ((status == S_LOESEN) && anzeigeLoesen > 0 && anzeigeLoesen <= jetzt) {
    printzeit("Run      ", jetzt - startLoesen);
    anzeigeLoesen += AKTUALISIEREN;
  }
  if (status == S_ZUSPAET && jetzt > endeAnschauen) {
    printStatus(S_BEREIT);
    status = S_BEREIT;
    endeAnschauen = 0;
  }
  if (jetzt - lastzeit > 1000) {
    lastzeit = jetzt;
    // showzeit();
  }
  //  */
}
