#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

void afficherAccueil() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("COFFRE-FORT");
    lcd.setCursor(0, 1);
    lcd.print("Entrez votre ID");
}

void afficherErreur() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acces refuse");
    lcd.setCursor(0, 1);
    lcd.print("Reessayez");
}

void afficherSucces() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acces autorise");
    lcd.setCursor(0, 1);
    lcd.print("Ouverture...");
}

void setup() {
    Wire.begin(21, 22);
    lcd.init();
    lcd.backlight();

    afficherAccueil();
}

void loop() {

}