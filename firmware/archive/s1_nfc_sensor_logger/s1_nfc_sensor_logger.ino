#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include "Adafruit_SHT31.h"

// Use Wire1 for I2C on Seeed Studio XIAO RP2040
#define SDA_PIN 6
#define SCL_PIN 7

PN532_I2C pn532_i2c(Wire1);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
Adafruit_SHT31 sht31 = Adafruit_SHT31(); // Initialize SHT31 sensor

void setup() 
{
    Serial.begin(115200);
    while (!Serial); // Wait for serial connection

    Serial.println("System initialized");

    // Initialize Wire1 for I2C on XIAO RP2040
    Wire1.setSDA(SDA_PIN);
    Wire1.setSCL(SCL_PIN);
    Wire1.begin();

    nfc.begin();

    if (!sht31.begin(0x44)) {
        Serial.println("Couldn't find SHT31 sensor!");
        while (1) delay(1);
    }
    Serial.println("SHT31 sensor initialized");
}

void loop() 
{
    writeNFC();
    delay(5000);
}

void writeNFC() 
{
    Serial.println("Place NFC tag to write...");

    if (nfc.tagPresent()) 
    {
        // Read temperature and humidity from SHT31 sensor
        float temperature = sht31.readTemperature();
        float humidity = sht31.readHumidity();

        // Check if readings are valid
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from SHT31 sensor");
            return;
        }

        // Construct the URL with live sensor data
        String url = "https://matsuouu.github.io/mechaminds/nfc-data.html?temp=" + String(temperature, 1) + "&humidity=" + String(humidity, 1);
        
        NdefMessage message;
        message.addUriRecord(url.c_str());

        Serial.println("Writing to NFC tag...");
        Serial.println(url);

        bool success = nfc.write(message);

        if (success) {
            Serial.println("NFC tag written successfully!");
        } else {
            Serial.println("Write failed! Try again.");
        }
        
        delay(2000); // Short delay after writing
    }
}