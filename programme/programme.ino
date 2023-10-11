#include <SPI.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "HX711.h"
#include "fonction.h"

#define RST 9  //broche PB1 patte 15 sur l'atmel reset de l'afficheur SPI
#define CE  10 //broche PB2 patte 16 sur l'atmel chip enable de l'afficheur SPI
#define DO  12 //broche PB4 patte 18 sur l'atmel miso de l'afficheur SPI
#define DL  8  //broche PB0 patte 14 sur l'atmel retro eclairage de l'afficheur
#define CLK 13 //broche PB5 patte 19 sur l'atmel
#define DIN 11 //broche PB3 patte 17 sur l'atmel

#define DT  6  //ligne data du convertisseur pour cellule de charge HX711 (broche PD6 patte 12 sur l'atmel)
#define SCK 7  //ligne d'horloge pour le convertisseur HX711 (broche PD7 patte 13 sur l'atmel)

#define debug false

//afficheur NOKIA 5510

#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

//Branchement des deux signaux Dout et Clock sur PD6 et PD7
const int LOADCELL_DOUT_PIN = DT;    
const int LOADCELL_SCK_PIN = SCK;       


//on garde les broches 4 et 5 de l'atmel (PD2 et PD3) comme entree d'interuption pour les futurs boutons poussoirs
//d'ou le deplacement du reset de l'afficheur LCD

//instanciation du capteur et de l'afficheur
//afficheur nokia 5110 utilisé en SPI hardware donc broche 11 et 13
//nécessairement utilisées (patte 17 pour mosi et 19 pour clock sur l'atmel 328p)
Adafruit_PCD8544 lcd = Adafruit_PCD8544(CLK, DIN, DO, CE, RST);     //afficheur Nokia sur liaison SPI hardware
                                                      

//variable pour les routines d'interruption

volatile bool routineWDT = false;
volatile bool routineINT0 = false;
volatile bool routineINT1 = false;
volatile float poids=0.0;
volatile unsigned long chrono;
volatile unsigned int compteur=0;

//traitement des trois interruptions possible (watchdog,INT0 sur PD2 et INT1 sur PD3)
ISR(WDT_vect) {
  routineWDT = true;
  compteur++;
}

ISR(INT0_vect) {                             //mise en mémoire du poids sur PD2
  EIMSK = 0x00;                              //desactivation des inter externes
  WDTCSR = (1 << WDCE) | (1 << WDE);         //desactivation du chien de garde
  WDTCSR = 0b0000000;                        //
  routineINT0 = true;
}

ISR(INT1_vect) {                             //rétro-eclairage sur PD3
  EIMSK = 0x00;                              //desactivation des inter externes
  WDTCSR = (1 << WDCE) | (1 << WDE);         //desactivation du chien de garde
  WDTCSR = 0b0000000;                        //
  chrono=millis();
  routineINT1 = true;
}  


int main (void){
  init();                             //initialisation pour utiliser l'environnement arduino
  HX711 balance;                      //instanciation d'un objet HX711
  boolean start=true;                 //variable pour le premier lancement du programme
  float memPoids=0.0; 

#if debug
  Serial.begin(9600);
#endif

//broche retro eclairage et interuption
  pinMode(DL, OUTPUT);         //retro_eclairage actif a l'etat haut
  pinMode(2, INPUT_PULLUP);   //interupteur sur broche D2 INT0 : mise en memoire
  pinMode(3, INPUT_PULLUP);   //interupteur sur broche D3 INT1 : retro-eclairage
 
//initialisation de la balance  
  balance.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN,128);   //gain de 128 sur le channel A du convertisseur (le 32 reservé pour le canal B)

//(tare + facteur de convertion)
  while (!balance.is_ready());                              //attention !! fonction bloquante tant que Dout n'est pas au niveau bas
  balance.set_scale(2297.f);                                //valeur trouvee après la procedure de calibration
  balance.tare(10);                                         //permet de soustraire la valeur au démarrage pour afficher 0 (tarage de la balance) avec 10 mesures

// Afficheur lcd
  lcd.begin();
  lcd.setRotation(0);         //(pose problème pour l'affichage de plusieur info sans clearDisplay !! laisser 0 par défaut)
  lcd.clearDisplay();
  lcd.setContrast(60);        //!!!! a régler pour chaque afficheur....
  lcd.display();

#if debug
  Serial.println("balance prête !!");
#endif

  //affichage de la page de présentation
  lcd.clearDisplay();
  lcd.cp437(true);                    // Utilisation des 256 caractères de la police Cp437
  lcd.drawRect(0, 0, 83, 47, BLACK);  // cadre de l'ecran
  lcd.setTextSize(1);                 // taille normale 1:1 pixel soit 5x7 pixels
  lcd.setTextColor(BLACK);            // couleur unique
  lcd.setCursor(2,2);                 // affichage de la trame de la page
  lcd.print("mem :");
  lcd.setCursor(65,2);
  lcd.print("kg");
  lcd.setCursor(60,29);
  lcd.print(F(" Kg"));
  lcd.display();


  
  while(1){
//toute les 8 secondes ou la premiere fois 
    if(routineWDT || start){              //réveil par le chien de garde watchdog         

      start=false;                        //pour afficher les valeurs uniquement au premier lancement sans attendre 8 seconde
      poids=(balance.get_units(10)/10);
      if (poids<=0.0){                    // permet de gérer le poids si négatif
        poids=0.0;
      }
//mesure du poids
#if debug
      Serial.println(F("le watchdog à déclenché !"));
      Serial.print(poids,1);           //affiche la valeur en Kg avec 1 chiffre derrière la virgule (on tient compte ici du 
      Serial.println(" kg");                                //facteur de convertion (contrairement a get_value()
#endif

//gestion du retro eclairage      
      if (compteur>=1){
         digitalWrite(DL,LOW);             //gestion du rétro-éclairage
         compteur=0;
      }     
//effacement des anciennes données
      effacePoids(lcd);

//affichage des données
      //poids LCD
      lcd.setTextSize(2);              // taille normale 1:1 pixel soit 5x7 pixels
      lcd.setTextColor(BLACK, WHITE);  // couleur unique
      lcd.setCursor(4, 29);
      if (poids>=100.00){
        lcd.print(poids,1);
      }
      else {
        lcd.print(poids);
      }
      lcd.display();
      balance.power_down();                                 //place le capteur en basse consommation
      routineWDT=false;           
    }
    //si appuye sur le bouton de droite 
    if (routineINT0) {
      wdt_reset();
      digitalWrite(DL,HIGH);                       //allumage du rétro-éclairage
      balance.power_down(); 
      routineINT0 = false;
      wdt_reset();
#if debug
      Serial.println("INT0 a declenche !!");
      Serial.println("fin de la routine INT0");
#endif
    }
    //si appuye sur le bouton de gauche
    if (routineINT1) {
      memPoids=poids;
      effaceMem(lcd);
      lcd.setTextSize(1);              // taille normale 1:1 pixel soit 5x7 pixels
      lcd.setCursor(35, 2);
      if(memPoids>=100.00){
        lcd.print(memPoids,1);
      }
      else{
        lcd.print(memPoids);
      }
      lcd.display();
      while((!digitalRead(3))&&((millis()-chrono)<3000));
      if(!digitalRead(3)){               //appuie sur le bouton de gauche pendant 3 secondes
        effacePoids(lcd);
        balance.tare(10);
        poids=(balance.get_units(10)/10);
        lcd.setTextSize(2);              // taille normale 1:1 pixel soit 5x7 pixels
        lcd.setTextColor(BLACK, WHITE);  // couleur unique
        lcd.setCursor(4, 29);
        if (poids<=0.0){
          poids=0.0;
        }
        lcd.print(poids);
        lcd.display();
        while(!digitalRead(3));         //tant que l'on a pas relaché le bouton
        wdt_reset();
      }
#if debug
      Serial.println("INT1 a declenche !!");
      Serial.println("fin de la routine INT1");
#endif
      balance.power_down();
      routineINT1 = false;            //interuption prise en compte
    }

    //autorise les interruptions des deux boutons poussoirs
    EIFR = 0b00000011; //efface les drapeaux avant de valider les interruptions à nouveau!!
    EIMSK = 0x03;     //validation des interruptions sur les pattes PD2 et PD3 (INT0 et INT1)
    EICRA = 0b00001010; //front descendant sur INT0 et INT1 (compatible avec le mode sleep utilisé)

    //autorise les interruptions du watchdog (toutes les 8 secondes )
    wdt_reset();
    WDTCSR = (1 << WDCE) | (1 << WDE);          // Déverrouille les bits de configuration du Watchdog (procédure spécifiée par le fabricant)
    WDTCSR = 0b01100001;                        // Mise à 1 des bits WDP3 et WDIE, pour activer les interruption toutes les 8 secondes
                  
    // mise en sommeil
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    // Et on met le µC en sommeil maintenant
#if debug
    Serial.println("Mise en sommeil du µC");
    Serial.println("");
    Serial.flush();         // On attend la fin de la transmission des données séries, avant de mettre le microcontrôleur en sommeil
#endif
    sleep_mode();
#if debug
    Serial.println("réveil!!");
#endif
    balance.power_up();                                   //active le capteur (sortie du mode basse consommation)
  }
  
}
