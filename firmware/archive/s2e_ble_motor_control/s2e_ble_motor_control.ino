#include <bluefruit.h>
#include "nRF52_PWM.h"

// Debug configuration
#define DEBUG 1  // Set to 0 for production to disable debug output

#if DEBUG
  #define DEBUG_INIT() Serial.begin(115200); while(!Serial);
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
#endif

// Hardware configuration
#define MOTOR_PIN D6       
#define PWM_FREQUENCY 50   // 50Hz
#define PWM_DUTY_CYCLE 50.0 // 50% duty cycle when ON

// Global objects
BLEUart bleUart;
nRF52_PWM pwmMotor(MOTOR_PIN, PWM_FREQUENCY, 0.0); // Start with motor off

// State variables
volatile bool motorState = false;
volatile bool bleConnected = false;

// BLE callback forward declarations
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void bleuart_rx_callback(uint16_t conn_handle);

void setup() {
    DEBUG_INIT();
    DEBUG_PRINT("Initializing system...");

    // Initialize BLE
    initializeBLE();

    DEBUG_PRINT("System initialized");
}

void loop() {
    // Low power sleep
    sd_app_evt_wait();
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
        while (bleUart.available() && index < sizeof(command) - 1) {
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
