
/******************************************************************
  Erstellt von Multiplexer - 19.07.2018 
  Project        :
  Libraries      :
  Description    : Upload nur mit Arduino IDE !!! Booter: Micronucleus 2 v. Malte - erwartet an Port 0 Low - muss mit ISP geflasht werden
  Version        : 2.0 
******************************************************************/
//
// Passwort Programm für den ATTINY85 
//
// Die LED auf dem Board wird angeschaltet und das Passwort gesendet.
// Taste Port 0 generiert neues PW
// Einstellungen Board: Digispark default (16,5 Mhz)
//
// 
//
//

 


#include <DigiKeyboard.h>
#include <EEPROM.h>

char daten[] = "abcdefghijklmopqrstuvwxABCDEFGHIJKLMNOPQRSTUVWX";
char zahlen[] = "!$%0123456789";
char sonderzeichen[] = "!$%";
int x;
int y;
String passwort;
int summe;


// Setup läuft einmal nach dem Reset
void setup()
{
  randomSeed(analogRead(1));
  pinMode(0, INPUT_PULLUP); //Eingang  PULLUP aktivieren
  pinMode(1, OUTPUT); //Eingebaute LED
  lesen();
}


void lesen()
{
  for (y = 1; y <= 18; y++)
  {
    char f = EEPROM.read(y);
    passwort += f;
    summe += EEPROM.read(y);
    
  }
    randomSeed(analogRead(1)+summe);
  
  DigiKeyboard.sendKeyStroke(0);
  delay(1100);
  DigiKeyboard.println(passwort); // sendet das Passwort UK Tastatur !!
  
}

// die Hauptschleife, wartet auf Tastendruck um PW zu senden. wenn >2s dann PW generieren
void loop()
{

  if (digitalRead(0) == LOW)
   {
   // digitalWrite(1, HIGH);   // schaltet die LED an (HIGH Level)
    DigiKeyboard.println(passwort);
    delay(2000);
    if (digitalRead(0) == LOW) pwgenerieren();  
  //  digitalWrite(1, LOW);  
    }
}

void pwgenerieren()
{
  passwort="";

  for (int y = 1; y <= 10; y++)
  {
    x = random(46);
    passwort += daten[x];
    EEPROM.write(y , daten[x]);
  }

  for (int y = 11; y <= 14; y++)
  {
    x = random(12);
    passwort += zahlen[x];
    EEPROM.write(y , zahlen[x]);
  }

  for (int y = 15; y <= 18; y++)
  {
    x = random(2);
    passwort += sonderzeichen[x];
    EEPROM.write(y , sonderzeichen[x]);
  }
  

  DigiKeyboard.println(passwort);

}
//
