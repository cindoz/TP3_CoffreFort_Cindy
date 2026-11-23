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

const int MAX_CHIFFRES = 6;

String numeroUtilisateur = "";

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

void afficherIdentifiant() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Entrez votre ID");

  lcd.setCursor(0, 1);
  lcd.print(numeroUtilisateur);
}

void traiterTouche(char touche) {
  if (touche == '*') {
    if (numeroUtilisateur.length() > 0) {
      numeroUtilisateur.remove(numeroUtilisateur.length() - 1);
      afficherIdentifiant();
    }
  }

  else if (touche >= '0' && touche <= '9') {
    if (numeroUtilisateur.length() < MAX_CHIFFRES) {
      numeroUtilisateur += touche;
      afficherIdentifiant();
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

  afficherIdentifiant();
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