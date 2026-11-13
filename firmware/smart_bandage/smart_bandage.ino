#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "nRF52_PWM.h"

// Debug configuration
#define DEBUG 1  // Set to 0 for production to disable debug output

#if DEBUG
  #define DEBUG_INIT() Serial.begin(115200); while(!Serial);
  #define DEBUG_PRINT(x) Serial.println(x)
  #define DEBUG_PRINT_DATA(x, y) Serial.print("Sent: "); Serial.print(x, 2); Serial.print(","); Serial.println(y, 2)
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINT_DATA(x, y)
#endif

// Hardware configuration
#define SDA_PIN D4  
#define SCL_PIN D5  
#define MOTOR_PIN D6       
#define PWM_FREQUENCY 50   // 50Hz
#define PWM_DUTY_CYCLE 50.0 // 50% duty cycle when ON

// Timing constants
const uint16_t SENSOR_UPDATE_INTERVAL = 1000;  // 1 second

// Global objects
Adafruit_SHT31 sht31 = Adafruit_SHT31();
BLEUart bleUart;
nRF52_PWM pwmMotor(MOTOR_PIN, PWM_FREQUENCY, 0.0); // Start with motor off

// State variables
volatile unsigned long lastSensorUpdate = 0;
volatile bool motorState = false;
volatile bool bleConnected = false;

// BLE callback forward declarations
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void bleuart_rx_callback(uint16_t conn_handle);

void setup() {
    DEBUG_INIT();
    DEBUG_PRINT("Initializing system...");

    // Initialize I2C
    Wire.begin();
    
    // Initialize sensor
    if (!sht31.begin(0x44)) {
        DEBUG_PRINT("SHT31 initialization failed!");
        while(1);
    }

    // Initialize BLE
    initializeBLE();

    DEBUG_PRINT("System initialized");
}

void loop() {
    unsigned long currentTime = millis();

    // Read and send sensor data every second
    if (currentTime - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        readAndSendSensorData();
    }

    // Sleep if nothing to do (saves power)
    if (currentTime - lastSensorUpdate < SENSOR_UPDATE_INTERVAL) {
        sd_app_evt_wait();
    }
}

void initializeBLE() {
    Bluefruit.begin();
    Bluefruit.setName("Smart Bandage");
    
    // Setup BLE callbacks
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

    // Configure and start BLE UART
    bleUart.begin();
    bleUart.setRxCallback(bleuart_rx_callback);

    // Advertising setup
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleUart);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.start(0); // 0 = Don't stop advertising
}

// BLE Connection Callbacks
void connect_callback(uint16_t conn_handle) {
    bleConnected = true;
    DEBUG_PRINT("BLE Connected");
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    bleConnected = false;
    DEBUG_PRINT("BLE Disconnected");
}

// BLE UART Receive Callback
void bleuart_rx_callback(uint16_t conn_handle) {
    if (bleUart.available()) {
        char command[8] = {0}; // Buffer for command
        uint8_t index = 0;
        
        // Read until newline or buffer full
        while (bleUart.available() && index < sizeof(command)-1) {
            char c = bleUart.read();
            if (c == '\n') break;
            command[index++] = c;
        }
        command[index] = '\0'; // Null-terminate
        
        // Process command
        if (strcmp(command, "ON") == 0) {
            pwmMotor.setPWM(MOTOR_PIN, PWM_FREQUENCY, PWM_DUTY_CYCLE);
            motorState = true;
            DEBUG_PRINT("Motor ON - App Control");
        } 
        else if (strcmp(command, "OFF") == 0) {
            pwmMotor.setPWM(MOTOR_PIN, PWM_FREQUENCY, 0.0);
            motorState = false;
            DEBUG_PRINT("Motor OFF - App Control");
        }
    }
}

void readAndSendSensorData() {
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        // Use stack-allocated buffer instead of String
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