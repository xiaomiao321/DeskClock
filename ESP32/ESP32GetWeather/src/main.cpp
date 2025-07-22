#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define WIFI_SSID "xiaomiao"
#define WIFI_PASS "xiaomiao123"
#define API_URL "https://restapi.amap.com/v3/weather/weatherInfo?city=120104&key=8a4fcc66268926914fff0c968b3c804c"
#define BAUD_RATE 115200

// 辅助函数：从字符串中提取指定字段值
String getValue(String data, String key, String end) {
  int start = data.indexOf(key);
  if (start == -1) return "N/A";
  start += key.length();
  int endIndex = data.indexOf(end, start);
  if (endIndex == -1) return "N/A";
  return data.substring(start, endIndex);
}

void setup() {
  Serial2.begin(BAUD_RATE);

  // 连接 WiFi
  Serial2.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial2.print(".");
  }
  Serial2.println("\nWiFi Connected, IP: " + WiFi.localIP().toString());
}

void loop() {
  Serial2.println("Loop started");
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    //Serial2.println("Starting HTTP request...");
    http.begin(API_URL);

    int httpCode = http.GET();
    //Serial2.print("HTTP Response Code: ");
    //Serial2.println(httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      //Serial2.print("Raw Payload: ");
      //Serial2.println(payload);

      int startIndex = payload.indexOf("\"lives\":[{");
      if (startIndex != -1) {
        startIndex += 9; // 跳到 "lives":[{" 后的位置
        int endIndex = payload.indexOf("}]", startIndex);
        if (endIndex != -1) {
          String liveData = payload.substring(startIndex, endIndex + 1);
        //  Serial2.print("Live Data: ");
        //  Serial2.println(liveData);

          String temperature = getValue(liveData, "\"temperature\":\"", "\"");
          String humidity = getValue(liveData, "\"humidity\":\"", "\"");

          String output = "{\n";
          output += "  \"temperature\": \"" + temperature + "\",\n";
          output += "  \"humidity\": \"" + humidity + "\"\n";
          output += "}";
         // Serial2.println("Sending: " + output);
          Serial2.println(output); // 通过 UART0 发送到 STM32
        } else {
          Serial2.println("Error: No closing '}]' found");
        }
      } else {
        Serial2.println("Error: No 'lives' array found");
      }
    } else {
      Serial2.println("HTTP请求失败, 详情: " + http.errorToString(httpCode));
    }
    http.end();
  } else {
    Serial2.println("WiFi Disconnected");
  }

  delay(60000); // 每分钟更新一次
}