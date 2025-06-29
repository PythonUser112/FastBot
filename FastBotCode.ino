#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// Display
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// Überholmanöver, Zeit angegeben in Millisekuden
int manoever_drehen = 200;
int manoever_gerade_kurve = 500;
int manoever_ueberhol_dauer = 1000;

// Motoren
// Pins
int mot_left_1 = 3;
int mot_left_2 = 2;
int mot_right_1 = 5;
int mot_right_2 = 4;
// PWM-Korrektur
int mot_left_korrektur = 15;
int mot_right_korrektur = 0;

// Ultraschallsensor und zugehöriges Servo
int dist_trigger = 6;
int dist_echo = 7;
long entfernung;
int ultraschall_servo_pin = 8;
Servo ultraschall_servo;

// Lichtersteuerung
// Pins
int ldr_pin = A1;
int beleuchtung = 9;
int blinker_l = 10;
int blinker_r = 11;
int lichter = 0;
int rueckfahrtlicht = 12;
// Lichterstatus
int BLINKER_LINKS = 1;
int BLINKER_RECHTS = 2;
int angeschaltet = 0;
long letzter_blinker_wechsel;
int blinker_rechts_angesteuert;

// Globaler Timer
long millisekunden_seit_start = 0;

// Motorsteuerung
int l1 = 0;
int l2 = 0;
int r1 = 0;
int r2 = 0;

// Andere Pins
int taster_pin = 13; // Starttaster

// Liest die Distanz zurück.
void get_distance_cm() {
  // Triggern des Messvorgangs
  digitalWrite(dist_trigger, LOW);
  warte_ms(5);
  digitalWrite(dist_trigger, HIGH);
  warte_ms(10);
  digitalWrite(dist_trigger, LOW);

  // Zeit messen, bis das Signal zurückkommt
  long zeit = pulseIn(dist_echo, HIGH);
  entfernung = 3432 * zeit / 200000; // Diese Umrechnung verwendet Ganzzahlen, das Ergebnis bleibt aber gleich.
  millisekunden_seit_start = millisekunden_seit_start + zeit / 1000; // Diese Rechnung korrigiert den Timer des FastBots
}


// Basisfahrfunktion
void motorWrite() {
  digitalWrite(rueckfahrtlicht, HIGH);
  // Lichtersteuerung: Macht Blinker bei Vorwärts- / Rückwärtsfahrten aus
  if(l1 == r1) {
    lichter = 0;
    angeschaltet = 0;
    digitalWrite(blinker_l, LOW);
    digitalWrite(blinker_r, LOW);
    if(l1 == 0 && l2 == 1 && l2 == r2) { // Macht das Rückfahrtlicht an
      digitalWrite(rueckfahrtlicht, HIGH);
    }
  }
  analogWrite(mot_left_1, 255 * l1 - mot_left_korrektur);
  digitalWrite(mot_left_2, l2);
  analogWrite(mot_right_1, 255 * r1 - mot_right_korrektur);
  digitalWrite(mot_right_2, r2);
}

// Fahrfunktionen: Volle Geschwindigkeit

void vorwaerts() {
  l1 = 1;
  l2 = 0;
  r1 = 1;
  r2 = 0;
  motorWrite();
}

void rueckwaerts() {
  l1 = 0;
  l2 = 1;
  r1 = 0;
  r2 = 1;
  motorWrite();
}

void links_fahren() {
  l1 = 1;
  l2 = 0;
  r1 = 0;
  r2 = 0;
  motorWrite();
  lichter = BLINKER_LINKS;
}

void links_drehen() {
  l1 = 1;
  l2 = 0;
  r1 = 0;
  r2 = 1;
  motorWrite();
  lichter = BLINKER_LINKS;
}

void rechts_fahren() {
  l1 = 0;
  l2 = 0;
  r1 = 1;
  r2 = 0;
  motorWrite();
  lichter = BLINKER_RECHTS;
}

void rechts_drehen()
{
  l1 = 0;
  l2 = 1;
  r1 = 1;
  r2 = 0;
  motorWrite();
  lichter = BLINKER_RECHTS;
}

void warnlicht() {
  anhalten();
  lichter = BLINKER_LINKS + BLINKER_RECHTS;
}

void anhalten() {
  l1 = 0;
  l2 = 0;
  r1 = 0;
  r2 = 0;
  motorWrite();
}

// Langsames Fahren:

// Fahren mit 50% Geschwindigkeit
void langsam_fahren() {
  anhalten();
  analogWrite(mot_left_1, 128 - mot_left_korrektur);
  analogWrite(mot_right_1, 128 - mot_right_korrektur);
}

// Fahren mit 37.5% Geschwindigkeit
void noch_langsamer_fahren() {
  anhalten();
  analogWrite(mot_left_1, 96 - mot_left_korrektur);
  analogWrite(mot_right_1, 96 - mot_right_korrektur);
}

// Drehen mit 75% Geschwindigkeit
void langsam_drehen_links() {
  anhalten();
  digitalWrite(mot_right_2, HIGH);
  analogWrite(mot_left_1, 192);
  analogWrite(mot_right_1, 192);
}

void langsam_drehen_rechts() {
  anhalten();
  digitalWrite(mot_left_2, HIGH);
  analogWrite(mot_left_1, 192);
  analogWrite(mot_right_1, 192);
}

// Funktion wie delay, führt aber nebenher Lichtersteuerung aus
void warte_ms(int zeit) {
  for(int i = 0; i < zeit; i = i + 1) {
    delay(1);
    millisekunden_seit_start = millisekunden_seit_start + 1;
    lichtersteuerung();
  }
}

// Umgeht ein Hindernis durch langsames drehen nach links bzw. nach rechts.
void kollisionsbewahrung() {
  lcd.clear();
  lcd.backlight();
  lcd.print("Hindernis");
  anhalten();
  ultraschall_servo.write(45);
  delay(10);
  get_distance_cm();
  ultraschall_servo.write(135);
  delay(10);
  long entfernung_links = entfernung;
  get_distance_cm();
  if(entfernung_links > entfernung) { // Dreht in die erfolgsversprechendere Richtung.
    langsam_drehen_links();
  }
  else {
    langsam_drehen_rechts();
  }
  ultraschall_servo.write(90);
  delay(10);
  while(entfernung < 20) {
    get_distance_cm();
    lichtersteuerung();
  }
  anhalten();
  vorwaerts();
  lcd.clear();
  lcd.noBacklight();
}

// Überholt durch fahren nach links und dann geradeaus mit 100% Geschwindigkeit.
void ueberholen() {
  lcd.clear();
  lcd.backlight();
  lcd.print("HEEEEEEE!!!");
  noch_langsamer_fahren();
  while(entfernung < 15) {
    lichtersteuerung();
    get_distance_cm();
  }
  links_drehen();
  warte_ms(manoever_drehen);
  vorwaerts();
  warte_ms(manoever_gerade_kurve);
  rechts_drehen();
  warte_ms(manoever_drehen);
  ultraschall_servo.write(180);
  delay(10);
  while(entfernung < 10) {
    get_distance_cm();
    warte_ms(10);
  }
  ultraschall_servo.write(90);
  rechts_drehen();
  warte_ms(manoever_drehen);
  vorwaerts();
  warte_ms(manoever_gerade_kurve);
  links_drehen();
  warte_ms(manoever_drehen);
  vorwaerts();
  lcd.clear();
  lcd.noBacklight();
}

// Funktion zum Verhindern von Kollisionen; sollte jeden(!) Loop-Durchgang ausgeführt werden7 (außer man hat einen eigenen Mechanismus)
void kollisionssteuerung() {
  get_distance_cm();
  if(entfernung < 15 && entfernung > 5) {
    langsam_fahren();
    warte_ms(500);
    get_distance_cm();
    if(entfernung < 15 && entfernung > 5) {
      // Es ist ein Fahrzeug.
      ueberholen();
    }
    else if(entfernung < 10) {
      // Es ist ein Hindernis
      lcd.clear();
      lcd.backlight();
      lcd.print("Hindernis");
      kollisionsbewahrung();
    }
    else {
      vorwaerts();
    }
  }
  else if(entfernung <= 5) {
    lcd.clear();
    lcd.backlight();
    lcd.print("Hindernis");
    kollisionsbewahrung();
  }
}

// Funktion, welche Front- und Rücklichter sowie Blinker steuert.
void lichtersteuerung() {
  // Front- und Ruecklichter ansteuern
  int helligkeit = analogRead(ldr_pin);
  if(helligkeit < 10) {
    digitalWrite(beleuchtung, HIGH);
  }
  else {
    digitalWrite(beleuchtung, LOW);
  }
  // Blinkersteuerung
  if(lichter > 0 && millisekunden_seit_start - letzter_blinker_wechsel > 500) { // Blinkerumschaltung?
    letzter_blinker_wechsel = millisekunden_seit_start;
    blinker_rechts_angesteuert = 0;
    // Da sowohl "lichter" als auch "
    if(lichter >= BLINKER_RECHTS) {
      if(angeschaltet >= BLINKER_RECHTS) {
        digitalWrite(blinker_r, LOW);
        angeschaltet = angeschaltet - BLINKER_RECHTS;
      }
      else {
        digitalWrite(blinker_r, HIGH);
        angeschaltet = angeschaltet + BLINKER_RECHTS;
      }
      blinker_rechts_angesteuert = 1;
      lichter = lichter - BLINKER_RECHTS;
    }
    if(lichter >= BLINKER_LINKS) {
      if(angeschaltet > BLINKER_RECHTS) {
        angeschaltet = angeschaltet - BLINKER_RECHTS;
        blinker_rechts_angesteuert = 2;
      }
       if(angeschaltet >= BLINKER_LINKS) {
        digitalWrite(blinker_l, LOW);
        angeschaltet = angeschaltet - BLINKER_LINKS;
      }
      else {
        digitalWrite(blinker_l, HIGH);
        angeschaltet = angeschaltet + BLINKER_LINKS;
      }
    }
    if(blinker_rechts_angesteuert == 2) {
      angeschaltet = angeschaltet + BLINKER_RECHTS;
    }
    if(blinker_rechts_angesteuert >= 1) {
      lichter = lichter + BLINKER_RECHTS;
    }
  }
}

// Ab her beginnt das Hauptprogramm

void setup() {
  // Initialisierung der Pins
  pinMode(dist_trigger, OUTPUT);
  pinMode(dist_echo, INPUT); // Nicht benötigt, da standardmäßig Input
  pinMode(mot_left_1, OUTPUT);
  pinMode(mot_left_2, OUTPUT);
  pinMode(mot_right_1, OUTPUT);
  pinMode(mot_right_2, OUTPUT);
  pinMode(beleuchtung, OUTPUT);
  pinMode(blinker_l, OUTPUT);
  pinMode(blinker_r, OUTPUT);
  pinMode(taster_pin, INPUT_PULLUP);
  pinMode(rueckfahrtlicht, OUTPUT);
  ultraschall_servo.attach(ultraschall_servo_pin);
  ultraschall_servo.write(90);
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.print("Druecke den");
  lcd.setCursor(0, 1);
  lcd.print("Taster.");
  while(digitalRead(taster_pin) == 1) { // Warten bis Starttaster gedrückt
  }
  lcd.clear();
  lcd.noBacklight();
  vorwaerts();
}

void loop() {
  warte_ms(1);
  steuerung();
  // An diesem Punkt können (mögliche) andere Steuerungen eingefügt werden
}
