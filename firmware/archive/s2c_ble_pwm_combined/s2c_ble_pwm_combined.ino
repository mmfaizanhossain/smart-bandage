#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "nRF52_PWM.h"

// I2C Pins
#define SDA_PIN D4  
#define SCL_PIN D5  

// Motor Control
#define MOTOR_PIN D6       
#define FREQUENCY 50       // Frequency in Hz
#define VIBRATION_ON_TIME 2000  // 2 seconds ON
#define VIBRATION_OFF_TIME 2000 // 2 seconds OFF

// Sensor Object
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// BLE UART Service
BLEUart bleUart;

// PWM Instance for Motor
nRF52_PWM* PWM_Instance;

// Timer Variables
unsigned long lastSensorUpdate = 0;
unsigned long lastVibrationToggle = 0;
bool isVibrating = true;

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Initializing BLE, sensor, and motor...");

    // I2C Initialization
    Wire.begin();
    
    // SHT31 Sensor Initialization
    if (!sht31.begin(0x44)) {
        Serial.println("SHT31 not found! Check wiring.");
        while (1);
    }

    // BLE Initialization
    Bluefruit.begin();
    Bluefruit.setName("Smart Bandage");

    // Start BLE UART Service
    bleUart.begin();

    // BLE Advertising Setup
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleUart);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.start();

    Serial.println("BLE ready, advertising device...");

    // Motor PWM Initialization
    PWM_Instance = new nRF52_PWM(MOTOR_PIN, FREQUENCY, 50.0);
    PWM_Instance->setPWM(MOTOR_PIN, FREQUENCY, 50.0); // Start vibrating
    lastVibrationToggle = millis();
}

void loop() {
    unsigned long currentTime = millis();

    // Read and send sensor data every second
    if (currentTime - lastSensorUpdate >= 1000) {
        lastSensorUpdate = currentTime;

        if (Bluefruit.connected()) {
            float temperature = sht31.readTemperature();
            float humidity = sht31.readHumidity();

            if (!isnan(temperature) && !isnan(humidity)) {
                String sensorData = String(temperature, 2) + "," + String(humidity, 2);
                bleUart.println(sensorData);
                bleUart.flush();
                Serial.println("Sent: " + sensorData);
            } else {
                Serial.println("Sensor Error");
            }
        }
    }

    // Toggle motor vibration every 2 seconds
    if (isVibrating && currentTime - lastVibrationToggle >= VIBRATION_ON_TIME) {
        PWM_Instance->setPWM(MOTOR_PIN, FREQUENCY, 0.0); // Stop vibrating
        isVibrating = false;
        lastVibrationToggle = currentTime;
        Serial.println("Motor OFF");
    } else if (!isVibrating && currentTime - lastVibrationToggle >= VIBRATION_OFF_TIME) {
        PWM_Instance->setPWM(MOTOR_PIN, FREQUENCY, 50.0); // Start vibrating
        isVibrating = true;
        lastVibrationToggle = currentTime;
        Serial.println("Motor ON");
    }
}
