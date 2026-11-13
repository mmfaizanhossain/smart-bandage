#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "nRF52_PWM.h"

// Debug configuration
#define DEBUG 1  // Set to 0 for production to disable debug output

#if DEBUG
  #define DEBUG_INIT() Serial.begin(115200); while(!Serial);
  #define DEBUG_PRINT(x) Serial.println(x)
  #define DEBUG_PRINT_DATA(x, y) Serial.println("Sent: " + String(x, 2) + "," + String(y, 2))
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT_DATA(x, y)
#endif

// Hardware configuration
#define SDA_PIN D4  
#define SCL_PIN D5  
#define MOTOR_PIN D6       

// Timing constants
const uint16_t SENSOR_UPDATE_INTERVAL = 1000;  // 1 second
const uint16_t VIBRATION_ON_TIME = 2000;       // 2 seconds
const uint16_t VIBRATION_OFF_TIME = 2000;      // 2 seconds
const uint8_t PWM_FREQUENCY = 50;              // 50Hz
const float PWM_DUTY_CYCLE = 50.0;             // 50% duty cycle

// Global objects
Adafruit_SHT31 sht31 = Adafruit_SHT31();
BLEUart bleUart;
nRF52_PWM pwmMotor(MOTOR_PIN, PWM_FREQUENCY, PWM_DUTY_CYCLE);

// State variables
volatile unsigned long lastSensorUpdate = 0;
volatile unsigned long lastVibrationToggle = 0;
volatile bool isVibrating = true;
volatile bool bleConnected = false;

// BLE callback forward declarations
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

void setup() {
    DEBUG_INIT();
    DEBUG_PRINT("Initializing system...");

    Wire.begin();

    if (!sht31.begin(0x44)) {
        DEBUG_PRINT("SHT31 initialization failed!");
        while(1);
    }

    initializeBLE();

    pwmMotor.setPWM();
    lastVibrationToggle = millis();
    DEBUG_PRINT("System initialized");
}

void loop() {
    unsigned long currentTime = millis();

    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        readAndSendSensorData();
    }

    handleMotorControl(currentTime);

    unsigned long nextSensorUpdate = lastSensorUpdate + SENSOR_UPDATE_INTERVAL;
    unsigned long nextMotorToggle = lastVibrationToggle + 
                                  (isVibrating ? VIBRATION_ON_TIME : VIBRATION_OFF_TIME);
    
    if (currentTime < min(nextSensorUpdate, nextMotorToggle)) {
        sd_app_evt_wait();
    }
}

void initializeBLE() {
    Bluefruit.begin();
    Bluefruit.setName("Smart Bandage");
    
    // Corrected BLE callback registration
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    bleUart.begin();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleUart);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.start(0);
}

// Corrected BLE callback functions
void connect_callback(uint16_t conn_handle) {
    bleConnected = true;
    DEBUG_PRINT("BLE Connected");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    bleConnected = false;
    DEBUG_PRINT("BLE Disconnected");
}

void readAndSendSensorData() {
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        char sensorData[20];
        snprintf(sensorData, sizeof(sensorData), "%.2f,%.2f", temperature, humidity);
        
        if (bleConnected) {
            bleUart.println(sensorData);
            bleUart.flush();
        }
        
        DEBUG_PRINT_DATA(temperature, humidity);
    } else {
        DEBUG_PRINT("Sensor read error");
    }
}

void handleMotorControl(unsigned long currentTime) {
    if ((isVibrating && (currentTime - lastVibrationToggle >= VIBRATION_ON_TIME)) ||
        (!isVibrating && (currentTime - lastVibrationToggle >= VIBRATION_OFF_TIME))) {
        
        isVibrating = !isVibrating;
        pwmMotor.setPWM(MOTOR_PIN, PWM_FREQUENCY, isVibrating ? PWM_DUTY_CYCLE : 0.0);
        lastVibrationToggle = currentTime;
        DEBUG_PRINT(isVibrating ? "Motor ON" : "Motor OFF");
    }
}