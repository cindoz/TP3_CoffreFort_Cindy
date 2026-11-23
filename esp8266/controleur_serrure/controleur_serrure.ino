#include <Servo.h>

Servo serrure;

const int PIN_SERVO = 14;

const int POSITION_VERROUILLEE = 0;
const int POSITION_DEVERROUILLEE = 90;

void verrouiller() {
    serrure.write(POSITION_VERROUILLEE);
}

void deverrouiller() {
    serrure.write(POSITION_DEVERROUILLEE);
}

void setup() {
    serrure.attach(PIN_SERVO);
    verrouiller();
}

void loop() {

}