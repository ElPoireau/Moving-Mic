// MovingMic.ino
// Simon Girard - 2024 - TE:AV
// H24 - 243.B40 - Projet en électronique

/* 

- MICROPHONE ASSERVIS - MOVING MIC - 

Crédis : 

* --------------------------------------------------------------------------------
https://github.com/Francien99/Stepper-driver/blob/master/Stepper_driver.ino
Copyright (C) 2017 F. Ramakers. 
* --------------------------------------------------------------------------------

*/

//// IMPORTATION DES BIBLIOTHÈQUES ////

#include <DMXSerial.h> // Gère le DMX
#include <LiquidCrystal.h> // Gère l'écran à cristaux liquide 
#include <LCDMenuLib2.h> // Gère le menu



//// DÉCLARATION DES CONSTANTES ET VARIABLES ////

/// CONSTANTES ///

/// BROCHES VERS PBL3775 ///
// MOTEUR PANORAMIQUE //
// const byte PANO_DIS1 = ; // Non connecté
// const byte PANO_DIS2 = ; // Non connecté
const byte PANO_PHASE1 = 5;
const byte PANO_PHASE2 = 7;
const byte PANO_VR1 = 4;
const byte PANO_VR2 = 6;

const byte PANO_INTERUPT = 52; // Interrupteur de l'axe panoramique

// MOTEUR D'INCLINAISON //
// const byte INCL_DIS1 = ; // Non connecté
// const byte INCL_DIS2 = ; // Non connecté
const byte INCL_PHASE1 = 11;
const byte INCL_PHASE2 = 9;
const byte INCL_VR1 = 10;
const byte INCL_VR2 = 8;

const byte INCL_INTERUPT = 53;  // Interrupteur de l'axe d'inclinaison

/// BROCHES DES DELS D'ÉTAT ///
const byte DEL_ALIM = 39;
const byte DEL_DMX = 43;



/// MOTEURS ///
const int INCL_BUFFER = 1; // Rapport de vitesse les deux axes
const int VEC_PAS_PANO = 2900; // Nombre de pas que l'axe peut faire au total (du début à la fin)
const int VEC_PAS_INCL = 640;

/* !!! NON IMPLÉMENTÉ !!!
/// ÉCRAN À CRISTAUX LIQUIDE /// 
const int ECL_RS = 31; // Broche RS de L'ÉCL
const int ECL_EN = 29; // Broche EN de L'ÉCL

const byte ECL_D4 = 27; // Broche D4 de L'ÉCL
const byte ECL_D5 = 25; // Broche D5 de L'ÉCL
const byte ECL_D6 = 23; // Broche D6 de L'ÉCL
const byte ECL_D7 = 21; // Broche D7 de L'ÉCL
*/ 


/// VARIABLES ///

/// MOTEURS ///
// Position des moteurs
int panoEtapePas = 4;  // Compte l'étape actuelle du cheminement du moteur 
int inclEtapePas = 4;  // Compte l'étape actuelle du cheminement du moteur 

signed long panoPosition = 50;
signed long inclPosition = 50;

// Calibration
int inclBuffer = 0; // voir INCL_BUFFER

// Moteur Panoramique
signed long panoNivHaut = 2890;
signed long panoNivBas = 0;

// Moteur d'inclinaison
signed long inclNivHaut = 630;
signed long inclNivBas = 0;

/// DMX ///
char DMX_Mode = 0; // Mode DMX du "Projecteur"

int DMXAdrDepart = 1; // Adresse de départ DMX
int DMXAdrPano = 3; // Adresse de la position panoramique
int DMXAdrIncl = 5; // Adresse de la position panoramique

/// Écran à cristaux liquide ///
LiquidCrystal lcd(ECL_RS, ECL_EN, ECL_D4, ECL_D5, ECL_D6, ECL_D7);



//// FONCTION D'INITIALISATION ////
void setup() {

  // Déclaration des broches
  pinMode(PANO_PHASE1, OUTPUT);
  pinMode(PANO_PHASE2, OUTPUT);
  pinMode(PANO_VR1, OUTPUT);
  pinMode(PANO_VR2, OUTPUT);

  pinMode(INCL_PHASE1, OUTPUT);
  pinMode(INCL_PHASE2, OUTPUT);
  pinMode(INCL_VR1, OUTPUT);
  pinMode(INCL_VR2, OUTPUT);

  pinMode(INCL_INTERUPT, INPUT_PULLUP);
  pinMode(PANO_INTERUPT, INPUT_PULLUP);

  pinMode(DEL_ALIM, OUTPUT);
  pinMode(DEL_DMX, OUTPUT);
  
  digitalWrite(DEL_ALIM, HIGH);

  // Écran à cristaux liquide
  lcd.begin(16, 2);
  lcd.setCursor(0,0), lcd.print(" Initialisation ");


  // initialisation des moteurs 
  initialisationMoteur(PANO_PHASE1, PANO_PHASE2, PANO_VR1, PANO_VR2); // Moteur panoramique
  initialisationMoteur(INCL_PHASE1, INCL_PHASE2, INCL_VR1, INCL_VR2); // Moteur d'inclinaison

  // initialisation du DMX
  DMXSerial.init(DMXReceiver);

  DMXSerial.write(DMXAdrPano, 0);
  DMXSerial.write(DMXAdrIncl, 0);

  Serial.begin(115200);
  Serial.println(" -- Microphone asservis -- ");
  Serial.println("     -- Version 1.0 --     \n");


  // Calibration des moteurs 
  Serial.println("Calibration de l'axe panoramique...");
  calibrationMoteur(PANO_PHASE1, PANO_PHASE2, panoEtapePas, PANO_INTERUPT);
  panoPosition = 0;
  Serial.println("Axe panoramique calibré!\n");


  Serial.println("Calibration de l'axe d'inclinaison...");
  calibrationMoteur(INCL_PHASE1, INCL_PHASE2, inclEtapePas, INCL_INTERUPT);
  Serial.println("Axe d'inclinaison calibré!\n");
  inclPosition = 0;

  //avancerMoteur(PANO_PHASE1, PANO_PHASE2, 50, 2);
}



//// FONCTION DE BOUCLE ////
void loop() {

  unsigned long lastPacket = DMXSerial.noDataSince();

  if (lastPacket < 500) { // Vérifie si le DMX est connecté
    digitalWrite(DEL_DMX, HIGH);

    // Recalibration des moteurs
    if (digitalRead(PANO_INTERUPT) == LOW && panoPosition != 0) {
      panoPosition == 0;
      Serial.println("Axe panoramique recalibré.");
    }

    if ((digitalRead(INCL_INTERUPT) == LOW) && (inclPosition != 0)) {
      inclPosition == 0;
      Serial.println("Axe d'inclinaison recalibré.");
    }

    // Calcul de la nouvelle position des moteurs
    signed long panoNouvPos = map(DMXSerial.read(DMXAdrPano), 0, 255, panoNivBas, panoNivHaut);
    signed long inclNouvPos = map(DMXSerial.read(DMXAdrIncl), 0, 255, inclNivBas, inclNivHaut);

    // Déplacement des moteurs 

    // Panoramique
    if (panoPosition < panoNouvPos) {
      panoEtapePas = avancerMoteur(PANO_PHASE1, PANO_PHASE2, panoEtapePas, 1, 0);
      panoPosition++;         
      Serial.print("Position du pano: "); Serial.println(panoPosition);
    } 
    
    else if (panoPosition > panoNouvPos) {
      panoEtapePas = reculerMoteur(PANO_PHASE1, PANO_PHASE2, panoEtapePas, 1, 0);
      panoPosition--;     
       Serial.print("Position du pano: "); Serial.println(panoPosition);
    }

    // Inclinaison
    if (inclBuffer == INCL_BUFFER) {
      inclBuffer = 0;
      if (inclPosition < inclNouvPos) {
        inclEtapePas = avancerMoteur(INCL_PHASE1, INCL_PHASE2, inclEtapePas, 1, 0);
        inclPosition++;
        Serial.print("Position du incl: "); Serial.println(inclPosition);
      } 
      
      else if (inclPosition > inclNouvPos) {
        inclEtapePas = reculerMoteur(INCL_PHASE1, INCL_PHASE2, inclEtapePas, 1, 0);
        inclPosition--;
        Serial.print("Position du incl: "); Serial.println(inclPosition);
      }
    }
    
    else {
      inclBuffer++;
    }
    delay(2);
  }

  else {
    digitalWrite(DEL_DMX, LOW);
  }
}

/*
int long changerPositionMoteur(char moteurP1, char moteurP2, int etapePas, signed long moteurPos, signed long moteurNouvPos) {

  if (moteurPos < moteurNouvPos) {
    etapePas = avancerMoteur(moteurP1, moteurP2, etapePas, 1, 0);
    panoPosition++;
    return etapePas;
  }

  else if (moteurPos > moteurNouvPos) {
    etapePas = reculerMoteur(moteurP1, moteurP2, etapePas, 1, 0);
    panoPosition--;
    return etapePas;
  }
}
*/

//// FONCTION ////

/// MOTEURS ///
int avancerMoteur(char moteurP1, char moteurP2, int etapePas, signed long nbPas, int delaiPas) {  // FULL STEP MODE

  for (int i = 0; i <nbPas; i++) {  // counting makes sure the stepper will continue from last step it took (important if step method is called >1 time)
    if (etapePas < 4) {
      etapePas++;
    }
    else {
      etapePas = 1;
    }

    if (etapePas == 1) {  // Pas 1
      digitalWrite(moteurP1, HIGH);
    } 

    else if (etapePas == 2) {  // Pas 2
      digitalWrite(moteurP2, LOW);
    } 

    else if (etapePas == 3) {  // Pas 3 
      digitalWrite(moteurP1, LOW);
    } 
    else {  // Pas 4
      digitalWrite(moteurP2, HIGH);
    }

    //delay(delaiPas);  // Délai entre les pas (Vitesse du moteur)
  }
  return etapePas;
}

int reculerMoteur(char moteurP1, char moteurP2, int etapePas, signed long nbPas, int delaiPas) {  // FULL STEP MODE, OPPOSITE DIRECTION

  for (int i = 0; i < nbPas; i++) { 
    if (etapePas > 1) {
      etapePas--;
    } 
    else {
      etapePas = 4;
    }

    if (etapePas == 1) { // Pas 1
      digitalWrite(moteurP1, HIGH);
    } 

    else if (etapePas == 2) { // Pas 2
      digitalWrite(moteurP2, LOW);
    } 

    else if (etapePas == 3) { // Pas 3
      digitalWrite(moteurP1, LOW);
    }

    else if (etapePas == 4) { // Pas 4
      digitalWrite(moteurP2, HIGH);
    }
    //delay(delaiPas); // Délai entre les pas (Vitesse du moteur)
  }
  return etapePas;
}



/// Initialisation des moteurs ///

void initialisationMoteur(char moteurP1, char moteurP2, char moteurVr1, char moteurVr2) { // Initialise la position du moteur
  // phase high: because this is the state needed for the first step the stepper needs to take
  digitalWrite(moteurP1, LOW); 
  digitalWrite(moteurP2, HIGH);

  // vRef is always set to HIGH (100%) in "Full Step Mode" and "Half Step Mode".
  // Note: in "modified half step mode, it is set to 140%, I don't know if this is possible with Arduino.
  digitalWrite(moteurVr1, HIGH);  // vRef pin is always HIGH.
  digitalWrite(moteurVr2, HIGH);
}

int calibrationMoteur(char moteurP1, char moteurP2, int etapePas, byte moteurInterupt) { // Cablibre les moteurs

  while (digitalRead(moteurInterupt) == HIGH) {
    etapePas = reculerMoteur(moteurP1, moteurP2, etapePas, 1, 0);
    delay(4);
  }

  return etapePas;
}
