// Librairies utiles
#include <avr/wdt.h>
#include <avr/sleep.h>
volatile bool routineWDT = false;
volatile bool routineINT0 = false;
volatile bool routineINT1 = false;

//programme d'interruption declenche par le chien de garde
ISR(WDT_vect) {
  EIMSK = 0x00;                              //desactivation des inter externes
  WDTCSR = (1 << WDCE) | (1 << WDE);         //desactivation du chien de garde
  WDTCSR = 0b0000000;                        //
  routineWDT = true;
}

ISR(INT0_vect) {
  EIMSK = 0x00;                              //desactivation des inter externes
  WDTCSR = (1 << WDCE) | (1 << WDE);         //desactivation du chien de garde
  WDTCSR = 0b0000000;                        //
  routineINT0 = true;
}

ISR(INT1_vect) {
  EIMSK = 0x00;                              //desactivation des inter externes
  WDTCSR = (1 << WDCE) | (1 << WDE);         //desactivation du chien de garde
  WDTCSR = 0b0000000;                        //
  routineINT1 = true;
}
int main (void) {
  init();
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  // Initialisation de la liaison série
  Serial.begin(9600);
  Serial.println(F("======================================================================================="));
  Serial.println(F("PRG2 - Test du 'mode interruption' du chien de garde d'un ATmega328P (Arduino Uno, ici)"));
  Serial.println(F("======================================================================================="));
  Serial.println(F("Démarrage du programme…"));
  Serial.println("");

  while (1) {
    if (routineWDT) {
      Serial.println("la watchdog a declenche!!");
      routineWDT = false;
    }
    if (routineINT0) {
      EIFR = 0x00;
      Serial.println("INT0 a declenche !!");
      routineINT0 = false;
      delay(6000);
      Serial.println("fin de la routine INT0");
      wdt_reset();
    }
    if (routineINT1) {
      wdt_reset();
      Serial.println("INT1 a declenche !!");
      routineINT1 = false;
      delay(3000);
      Serial.println("fin de la routine INT1");
      wdt_reset();
    }

    //autorise les interruptions des deux boutons poussoirs
    EIFR = 0b00000011; //efface les drapeaux !!  evite de repartir en interruption si
                        //l'on a appuye sur plusieurs boutons
    EIMSK = 0x03;     //validation des interruptions sur les pattes PD2 et PD3 (INT0 et INT1)
    EICRA = 0b00001010; //front descendant sur int1 et int0



    //autorise les interruptions du watchdog (toutes les 8 secondes )
    wdt_reset();
    WDTCSR = (1 << WDCE) | (1 << WDE);          // Déverrouille les bits de configuration du Watchdog (procédure spécifiée par le fabricant)
    WDTCSR = 0b01100001;                        // Mise à 1 des bits WDP3 et WDIE, pour activer les interruption toutes les 4 secondes


    //mise à 0 des drapeaus pour int0 et int1
    EIFR = 0b00000000;
    // mise en sommeil
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    // Et on met le µC en sommeil maintenant
    Serial.println("Mise en sommeil du µC");
    Serial.println("");
    Serial.flush();         // On attend la fin de la transmission des données séries, avant de mettre le microcontrôleur en sommeil
    sleep_mode();
  }
}
