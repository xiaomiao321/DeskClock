#include <Arduino.h>
// #include <WiFiMulti.h>
// #define WIFINAME "A5602"
// #define WIFIPASSWORD "02946BD8"

void setup() {
  Serial2.begin(115200);
  Serial.begin(115200);
}

void loop() {

  String message = Serial2.readString();
  if (!message.isEmpty()) {
    Serial.println("Reveived: " + message);
    Serial2.println("ESP32 Received Data"); // 可以选择添加换行符
  }
}