const int motorPin = 6;  // MOSFET Gate connected to pin 9

void setup() {
    pinMode(motorPin, OUTPUT);  // Set pin as output
}

void loop() {
    digitalWrite(motorPin, HIGH); // Turn motor ON
    delay(2000);                  // Run for 2 seconds
    digitalWrite(motorPin, LOW);  // Turn motor OFF
    delay(2000);                  // Wait for 2 seconds
}