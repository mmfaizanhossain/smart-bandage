#include <bluefruit.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"


// I2Cピン
#define SDA_PIN D4  
#define SCL_PIN D5  

// センサーオブジェクト
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// BLE UART サービス
BLEUart bleUart;

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Initializing BLE and sensor...");

    // I2C初期化
    Wire.begin();
    
    // SHT31センサー初期化
    if (!sht31.begin(0x44)) {
        Serial.println("SHT31が見つかりません！配線を確認してください");
        while (1);
    }

    // BLE初期化
    Bluefruit.begin();
    Bluefruit.setName("Smart Bandage");

    // BLE UARTサービス開始
    bleUart.begin();

    // BLEアドバタイズ設定
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(bleUart);
    Bluefruit.Advertising.addName();
    Bluefruit.Advertising.start();

    Serial.println("BLE ready, advertising device...");
}

void loop() {
    if (Bluefruit.connected()) {
        // センサーデータ取得
        float temperature = sht31.readTemperature();
        float humidity = sht31.readHumidity();

        // データが無効なら処理しない
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("センサーエラー");
            return;
        }

        // "温度,湿度" 形式で送信
        String sensorData = String(temperature, 2) + "," + String(humidity, 2);
        bleUart.println(sensorData);
        bleUart.flush();

        // シリアルモニターにも表示
        Serial.println("送信: " + sensorData);

        delay(1000);
    }
}
