#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

// ---- Écran LCD ----
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- Pavé numerique ----
const int rangees[4] = {13, 14, 25, 26};
const int colonnes[4] = {27, 32, 33, 23};

const char touches[4][4] = {
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};

const int MIN_CHIFFRES = 4;
const int MAX_CHIFFRES = 6;

// ---- Servomoteur ----
Servo serrure;

const int PIN_SERVO = 18;

const int POSITION_VERROUILLEE = 0;
const int POSITION_DEVERROUILLEE = 90;

bool coffreDeverrouille = false;

unsigned long debutDeverrouillage = 0;
const unsigned long DELAI_VERROUILLAGE = 15000;

bool porteOuverteDepuisDeverrouillage = false;

// ---- Buzzer ----
const int PIN_BUZZER = 15;
const unsigned long DELAI_ALARME = 15000;

enum TypeSon {
  AUCUN_SON,
  SON_AUTORISE,
  SON_REFUSE,
  SON_VERROUILLAGE,
  SON_ALARME
};

TypeSon sonActuel = AUCUN_SON;

unsigned long debutSon = 0;
unsigned long debutOuverture = 0;

bool buzzerActif = false;
bool ouvertureEnCours = false;
int nombreBipsTermines = 0;

// ---- DEL ----
const int PIN_DEL_ROUGE = 5;
const int PIN_DEL_VERTE = 4;

const unsigned long DUREE_CLIGNOTEMENT = 150;

bool clignotementRougeActif = false;
unsigned long debutClignotementRouge = 0;
int etapeClignotementRouge = 0;

// ---- Authentification ----
enum EtatAuthentification {
  SAISIE_UTILISATEUR,
  SAISIE_MOT_DE_PASSE,
  VALIDATION,
  RESULTAT
};

EtatAuthentification etatActuel = SAISIE_UTILISATEUR;

String numeroUtilisateur = "";
String motDePasse = "";

// ---- Bluetooth BLE ----
const char* UUID_SERVICE =
    "6e400001-b5a3-f393-e0a9-e50e24dcca9e";

const char* UUID_REQUETE =
    "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

const char* UUID_REPONSE =
    "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

BLECharacteristic* caracteristiqueRequete = nullptr;

volatile bool raspberryConnecte = false;
volatile bool relancerPublicite = false;
volatile int reponseAuthentification = -1;

unsigned long numeroDemande = 0;
unsigned long debutValidation = 0;

const unsigned long DELAI_REPONSE = 10000;
const unsigned long DELAI_AFFICHAGE = 300;

bool validationAffichee = false;

const int PIN_BOUTON_PORTE = 19;

void initialiserBluetooth();
void envoyerIdentifiants();
void verifierReponse();
void afficherEtat();
void afficherResultat(bool autorise);
void afficherErreur(String message);
bool porteFermee();
void verifierVerrouillageAutomatique();
void afficherAlarme();
void arreterSon();
void demarrerSon(TypeSon type);
void mettreAJourSon();
void demarrerClignotementRouge();
void mettreAJourDelRouge();
void verrouiller();
void deverrouiller();
char lireTouche();
void effacerDernierChiffre();
void confirmerSaisie();
void reinitialiserSaisie();
void traiterTouche(char touche);

// ---- DEL ----
void demarrerClignotementRouge() {
  if (sonActuel == SON_ALARME) {
    return;
  }

  clignotementRougeActif = true;
  etapeClignotementRouge = 0;
  debutClignotementRouge = millis();

  digitalWrite(PIN_DEL_ROUGE, HIGH);
}

void mettreAJourDelRouge() {
  if (sonActuel == SON_ALARME) {
    return;
  }

  if (!clignotementRougeActif) {
    return;
  }

  if (millis() - debutClignotementRouge <
      DUREE_CLIGNOTEMENT) {
    return;
  }

  debutClignotementRouge = millis();
  etapeClignotementRouge++;

  if (etapeClignotementRouge >= 4) {
    digitalWrite(PIN_DEL_ROUGE, LOW);
    clignotementRougeActif = false;
    return;
  }

  if (etapeClignotementRouge % 2 == 0) {
    digitalWrite(PIN_DEL_ROUGE, HIGH);
  } else {
    digitalWrite(PIN_DEL_ROUGE, LOW);
  }
}

// ---- Buzzer ----
void arreterSon() {
  if (sonActuel == SON_ALARME) {
    digitalWrite(PIN_DEL_ROUGE, LOW);
  }

  ledcWriteTone(PIN_BUZZER, 0);

  sonActuel = AUCUN_SON;
  buzzerActif = false;
  nombreBipsTermines = 0;
}

void demarrerSon(TypeSon type) {
  sonActuel = type;
  debutSon = millis();
  nombreBipsTermines = 0;
  buzzerActif = true;

  if (type == SON_ALARME) {
    clignotementRougeActif = false;
    digitalWrite(PIN_DEL_ROUGE, HIGH);
  }

  ledcWriteTone(PIN_BUZZER, 2000);
}

void mettreAJourSon() {
  if (sonActuel == AUCUN_SON) {
    return;
  }

  unsigned long maintenant = millis();
  unsigned long dureeBip = 120;
  unsigned long dureePause = 120;
  int nombreBips = 1;

  if (sonActuel == SON_AUTORISE) {
    dureeBip = 120;
    dureePause = 120;
    nombreBips = 2;
  }

  else if (sonActuel == SON_REFUSE) {
    dureeBip = 600;
    nombreBips = 1;
  }

  else if (sonActuel == SON_VERROUILLAGE) {
    dureeBip = 150;
    nombreBips = 1;
  }

  else if (sonActuel == SON_ALARME) {
    dureeBip = 200;
    dureePause = 500;
  }

  if (buzzerActif) {
    if (maintenant - debutSon >= dureeBip) {
      ledcWriteTone(PIN_BUZZER, 0);

      if (sonActuel == SON_ALARME) {
        digitalWrite(PIN_DEL_ROUGE, LOW);
      }

      buzzerActif = false;
      debutSon = maintenant;
      nombreBipsTermines++;

      if (sonActuel != SON_ALARME &&
          nombreBipsTermines >= nombreBips) {
        arreterSon();
      }
    }
  }

  else if (maintenant - debutSon >= dureePause) {
    ledcWriteTone(PIN_BUZZER, 2000);

    if (sonActuel == SON_ALARME) {
      digitalWrite(PIN_DEL_ROUGE, HIGH);
    }

    buzzerActif = true;
    debutSon = maintenant;
  }
}

// ---- Controle de la serrure + gestion porte ----
bool porteFermee() {
  return digitalRead(PIN_BOUTON_PORTE) == LOW;
}

void verrouiller() {
  serrure.write(POSITION_VERROUILLEE);
  coffreDeverrouille = false;

  digitalWrite(PIN_DEL_VERTE, LOW);
}

void deverrouiller() {
  serrure.write(POSITION_DEVERROUILLEE);
  coffreDeverrouille = true;

  digitalWrite(PIN_DEL_VERTE, HIGH);

  debutDeverrouillage = millis();
  porteOuverteDepuisDeverrouillage = !porteFermee();

  demarrerSon(SON_AUTORISE);
}

void verifierVerrouillageAutomatique() {
  if (!porteFermee()) {
    if (coffreDeverrouille) {
      porteOuverteDepuisDeverrouillage = true;
    }

    if (!ouvertureEnCours) {
      ouvertureEnCours = true;
      debutOuverture = millis();
    }

    if (millis() - debutOuverture >= DELAI_ALARME &&
        sonActuel != SON_ALARME) {

      demarrerSon(SON_ALARME);
      afficherAlarme();
    }

    return;
  }

  ouvertureEnCours = false;

  bool alarmeEtaitActive = sonActuel == SON_ALARME;

  if (alarmeEtaitActive) {
    arreterSon();
  }

  if (coffreDeverrouille &&
      (porteOuverteDepuisDeverrouillage ||
       millis() - debutDeverrouillage >= DELAI_VERROUILLAGE)) {

    verrouiller();

    demarrerSon(SON_VERROUILLAGE);
    demarrerClignotementRouge();

    if (etatActuel == RESULTAT) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Coffre verrou.");
      lcd.setCursor(0, 1);
      lcd.print("* pour retour");
    }
  }

  else if (alarmeEtaitActive) {
    if (etatActuel == RESULTAT) {
      lcd.clear();
      lcd.setCursor(0, 0);

      if (coffreDeverrouille) {
        lcd.print("Coffre deverrou.");
      } else {
        lcd.print("Coffre verrou.");
      }

      lcd.setCursor(0, 1);
      lcd.print("* pour retour");
    } else {
      afficherEtat();
    }
  }
}

void afficherAlarme() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (coffreDeverrouille) {
    lcd.print("Coffre deverrou.");
  } else {
    lcd.print("Coffre verrou.");
  }

  lcd.setCursor(0, 1);
  lcd.print("ALARME:PORTE OUV");
}

// ---- Affichage LCD ----
void afficherEtat() {
  lcd.clear();

  if (etatActuel == SAISIE_UTILISATEUR) {
    lcd.setCursor(0, 0);
    lcd.print("Entrez votre ID");

    lcd.setCursor(0, 1);
    lcd.print(numeroUtilisateur);
  }

  else if (etatActuel == SAISIE_MOT_DE_PASSE) {
    lcd.setCursor(0, 0);
    lcd.print("Entrez le code");

    lcd.setCursor(0, 1);

    for (int i = 0; i < motDePasse.length(); i++) {
      lcd.print("*");
    }
  }

  else if (etatActuel == VALIDATION) {
    lcd.setCursor(0, 0);
    lcd.print("Validation...");

    lcd.setCursor(0, 1);
    lcd.print("Attente serveur");
  }
}

void afficherResultat(bool autorise) {
  etatActuel = RESULTAT;

  lcd.clear();
  lcd.setCursor(0, 0);

  if (autorise) {
    lcd.print("Acces autorise");
  } else {
    lcd.print("Acces refuse");
  }

  lcd.setCursor(0, 1);

  if (autorise) {
    lcd.print("Coffre deverrou.");
  } else {
    lcd.print("* pour retour");
  }
}

void afficherErreur(String message) {
  etatActuel = RESULTAT;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message);

  lcd.setCursor(0, 1);
  lcd.print("* pour retour");
}

// ---- Lecture du pavé numerique ----
char lireTouche() {
  for (int r = 0; r < 4; r++) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(rangees[i], LOW);
    }

    digitalWrite(rangees[r], HIGH);
    delayMicroseconds(50);

    for (int c = 0; c < 4; c++) {
      if (digitalRead(colonnes[c]) == HIGH) {
        digitalWrite(rangees[r], LOW);
        return touches[r][c];
      }
    }

    digitalWrite(rangees[r], LOW);
  }

  return '\0';
}

// ---- Saisie des identifiants ----
void effacerDernierChiffre() {
  if (etatActuel == SAISIE_UTILISATEUR) {
    if (numeroUtilisateur.length() > 0) {
      numeroUtilisateur.remove(
          numeroUtilisateur.length() - 1
      );
    }
  }

  else if (etatActuel == SAISIE_MOT_DE_PASSE) {
    if (motDePasse.length() > 0) {
      motDePasse.remove(
          motDePasse.length() - 1
      );
    }
  }

  afficherEtat();
}

void confirmerSaisie() {
  int nombreChiffres = 0;

  if (etatActuel == SAISIE_UTILISATEUR) {
    nombreChiffres = numeroUtilisateur.length();
  }

  else if (etatActuel == SAISIE_MOT_DE_PASSE) {
    nombreChiffres = motDePasse.length();
  }

  if (nombreChiffres < MIN_CHIFFRES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Min. 4 chiffres");

    delay(1200);
    afficherEtat();
    return;
  }

  if (etatActuel == SAISIE_UTILISATEUR) {
    etatActuel = SAISIE_MOT_DE_PASSE;
    afficherEtat();
  }

  else if (etatActuel == SAISIE_MOT_DE_PASSE) {
    envoyerIdentifiants();
  }
}

void reinitialiserSaisie() {
  numeroUtilisateur = "";
  motDePasse = "";

  reponseAuthentification = -1;
  etatActuel = SAISIE_UTILISATEUR;

  afficherEtat();
}

void traiterTouche(char touche) {
  if (sonActuel == SON_ALARME) {
    return;
  }

  if (etatActuel == VALIDATION) {
    return;
  }

  // Apres le résultat, * permet de recommencer
  if (etatActuel == RESULTAT) {
    if (touche == '*') {
      if (coffreDeverrouille && porteFermee()) {
        verrouiller();

        demarrerSon(SON_VERROUILLAGE);
        demarrerClignotementRouge();
      }

      reinitialiserSaisie();
    }

    return;
  }

  if (touche == '*') {
    effacerDernierChiffre();
  }

  else if (touche == '#') {
    confirmerSaisie();
  }

  else if (touche >= '0' && touche <= '9') {
    if (etatActuel == SAISIE_UTILISATEUR) {
      if (numeroUtilisateur.length() < MAX_CHIFFRES) {
        numeroUtilisateur += touche;
        afficherEtat();
      }
    }

    else if (etatActuel == SAISIE_MOT_DE_PASSE) {
      if (motDePasse.length() < MAX_CHIFFRES) {
        motDePasse += touche;
        afficherEtat();
      }
    }
  }
}

// ---- Evénements Bluetooth ----
class ConnexionCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* serveur) override {
    raspberryConnecte = true;
  }

  void onDisconnect(BLEServer* serveur) override {
    raspberryConnecte = false;
    relancerPublicite = true;
  }
};

class ReponseCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* caracteristique) override {
    String valeur = caracteristique->getValue().c_str();

    int separateur = valeur.indexOf('|');

    if (separateur == -1) {
      return;
    }

    String numeroRecu = valeur.substring(0, separateur);
    String resultat = valeur.substring(separateur + 1);

    if (etatActuel != VALIDATION) {
      return;
    }

    if (numeroRecu != String(numeroDemande)) {
      return;
    }

    if (resultat == "1") {
      reponseAuthentification = 1;
    }

    else if (resultat == "0") {
      reponseAuthentification = 0;
    }
  }
};

// ---- Configuration Bluetooth BLE ----
void initialiserBluetooth() {
  BLEDevice::init("CoffreESP32");

  BLEServer* serveur = BLEDevice::createServer();
  serveur->setCallbacks(new ConnexionCallbacks());

  BLEService* service = serveur->createService(UUID_SERVICE);

  caracteristiqueRequete = service->createCharacteristic(
      UUID_REQUETE,
      BLECharacteristic::PROPERTY_NOTIFY
  );

  caracteristiqueRequete->addDescriptor(new BLE2902());

  BLECharacteristic* caracteristiqueReponse =
      service->createCharacteristic(
          UUID_REPONSE,
          BLECharacteristic::PROPERTY_WRITE
      );

  caracteristiqueReponse->setCallbacks(
      new ReponseCallbacks()
  );

  service->start();

  BLEAdvertising* publicite = BLEDevice::getAdvertising();

  publicite->addServiceUUID(UUID_SERVICE);
  publicite->setScanResponse(true);

  BLEDevice::startAdvertising();
}

// ---- Communication avec le Raspberry Pi ----
void envoyerIdentifiants() {
  reponseAuthentification = -1;

  if (!raspberryConnecte) {
    afficherErreur("Pi non connecte");
    return;
  }

  numeroDemande++;

  if (numeroDemande > 999999) {
    numeroDemande = 1;
  }

  etatActuel = VALIDATION;
  debutValidation = millis();

  validationAffichee = false;

  // Format : numeroDemande|numero|motDePasse
  String demande = String(numeroDemande);
  demande += "|";
  demande += numeroUtilisateur;
  demande += "|";
  demande += motDePasse;

  caracteristiqueRequete->setValue(demande.c_str());
  caracteristiqueRequete->notify();
}

void verifierReponse() {
  if (etatActuel != VALIDATION) {
    return;
  }

  if (!raspberryConnecte) {
    reponseAuthentification = -1;
    afficherErreur("Pi deconnecte");
    return;
  }

  int reponse = reponseAuthentification;

  if (reponse == 1) {
    reponseAuthentification = -1;

    deverrouiller();
    afficherResultat(true);
  }

  else if (reponse == 0) {
    reponseAuthentification = -1;

    demarrerSon(SON_REFUSE);
    demarrerClignotementRouge();

    afficherResultat(false);
  }

  else if (millis() - debutValidation >= DELAI_REPONSE) {
    reponseAuthentification = -1;
    afficherErreur("Erreur serveur");
  }

  else if (millis() - debutValidation >= DELAI_AFFICHAGE &&
           !validationAffichee) {
    afficherEtat();
    validationAffichee = true;
  }
}

// ---- Démarrage ----
void setup() {

  // Configure le clavier
  for (int r = 0; r < 4; r++) {
    pinMode(rangees[r], OUTPUT);
    digitalWrite(rangees[r], LOW);
  }

  for (int c = 0; c < 4; c++) {
    pinMode(colonnes[c], INPUT_PULLDOWN);
  }

  // Initialise l'écran LCD
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  // Initialise le servo
  serrure.setPeriodHertz(50);
  serrure.attach(PIN_SERVO, 1000, 2000);

  // Initialise le bouton de porte
  pinMode(PIN_BOUTON_PORTE, INPUT_PULLUP);

  // Initialise les DEL
  pinMode(PIN_DEL_ROUGE, OUTPUT);
  pinMode(PIN_DEL_VERTE, OUTPUT);

  digitalWrite(PIN_DEL_ROUGE, LOW);
  digitalWrite(PIN_DEL_VERTE, LOW);

  ledcAttach(PIN_BUZZER, 2000, 8);
  arreterSon();

  verrouiller();

  afficherEtat();

  initialiserBluetooth();
}

void loop() {
  static bool toucheEnfoncee = false;

  char touche = lireTouche();

  if (touche == '\0') {
    toucheEnfoncee = false;
  }

  else if (!toucheEnfoncee) {
    toucheEnfoncee = true;
    traiterTouche(touche);
    delay(30);
  }

  verifierReponse();

  verifierVerrouillageAutomatique();

  mettreAJourSon();
  mettreAJourDelRouge();

  if (relancerPublicite) {
    relancerPublicite = false;
    BLEDevice::startAdvertising();
  }
}