#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---- Branchement pavé numerique ----
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

void effacerDernierChiffre() {
  String* saisie = (etatActuel == SAISIE_UTILISATEUR)
      ? &numeroUtilisateur
      : &motDePasse;

  if (saisie->length() > 0) {
    saisie->remove(saisie->length() - 1);
    afficherEtat();
  }
}

void confirmerSaisie() {
  String* saisie = (etatActuel == SAISIE_UTILISATEUR)
      ? &numeroUtilisateur
      : &motDePasse;

  if (saisie->length() < MIN_CHIFFRES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Min. 4 chiffres");

    delay(1200);
    afficherEtat();
    return;
  }

  if (etatActuel == SAISIE_UTILISATEUR) {
    etatActuel = SAISIE_MOT_DE_PASSE;
  } else {
    etatActuel = VALIDATION;
  }

  afficherEtat();
}

// ---- Reponse du Raspberry Pi ----
void afficherResultat(bool autorise) {
  etatActuel = RESULTAT;
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(autorise ? "Acces autorise" : "Acces refuse");
}

void reinitialiserSaisie() {
  numeroUtilisateur = "";
  motDePasse = "";
  etatActuel = SAISIE_UTILISATEUR;

  afficherEtat();
}

void traiterTouche(char touche) {
  if (etatActuel == VALIDATION || etatActuel == RESULTAT) {
    if (touche == '*') {
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
    String* saisie = (etatActuel == SAISIE_UTILISATEUR)
        ? &numeroUtilisateur
        : &motDePasse;

    if (saisie->length() < MAX_CHIFFRES) {
      *saisie += touche;
      afficherEtat();
    }
  }
}

void setup() {
  for (int r = 0; r < 4; r++) {
    pinMode(rangees[r], OUTPUT);
    digitalWrite(rangees[r], LOW);
  }

  for (int c = 0; c < 4; c++) {
    pinMode(colonnes[c], INPUT_PULLDOWN);
  }

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  afficherEtat();
}

void loop() {
  static bool toucheEnfoncee = false;

  char touche = lireTouche();

  if (touche == '\0') {
    toucheEnfoncee = false;
  } else if (!toucheEnfoncee) {
    toucheEnfoncee = true;
    traiterTouche(touche);
    delay(30);
  }
}