#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

#define WIFI_SSID "xiaomiao"
#define WIFI_PASS "xiaomiao123"
#define API_URL "https://restapi.amap.com/v3/weather/weatherInfo?city=120104&key=8a4fcc66268926914fff0c968b3c804c"
#define BAUD_RATE 115200
#define WEATHER_UPDATE 1800000
#define RTC_UPDATE (6*3600000)

const char* ntpServer = "europe.pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // GMT+7
const int daylightOffset_sec = 3600;

// 辅助函数：从字符串中提取指定字段值
String getValue(String data, String key, String end) {
  int start = data.indexOf(key);
  if (start == -1) return "N/A";
  start += key.length();
  int endIndex = data.indexOf(end, start);
  if (endIndex == -1) return "N/A";
  return data.substring(start, endIndex);
}

// 获取 NTP 时间并返回 yyyy-mm-dd hh:mm:ss 格式
String getLocalTimeStr() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "N/A";
  }
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStr);
}

void setup() {
  Serial2.begin(BAUD_RATE);
  Serial.begin(BAUD_RATE);

  // 连接 WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long wifiTimeout = millis() + 10000; // 10秒超时
  while (WiFi.status() != WL_CONNECTED && millis() < wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected, IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi Connection Failed");
    return; // 若WiFi失败，跳过NTP
  }

  // 初始化 NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("NTP time initialized");
  String timeStr = getLocalTimeStr();
  Serial.println("Initial time: " + timeStr);
  
  // 等待 NTP 同步，最多10秒
  unsigned long ntpTimeout = millis() + 10000;
  String ntptime;
  do {
    ntptime = getLocalTimeStr();
    if (ntptime != "N/A") break;
    delay(500);
    Serial.print(".");
  } while (millis() < ntpTimeout);
  
  if (ntptime != "N/A") {
    Serial.println("NTP time initialized: " + ntptime);
    // 上电发送5次 NTP 时间，间隔2秒
    for (int i = 0; i < 5; i++) {
      if (WiFi.status() == WL_CONNECTED) {
        ntptime = getLocalTimeStr();
        if (ntptime != "N/A") {
          String output = "{\n";
          output += "  \"ntptime\": \"" + ntptime + "\"\n";
          output += "}";
          Serial.println("Sending NTP (" + String(i + 1) + "/5): " + output + " to STM32F103");
          Serial2.println(output);
        } else {
          Serial.println("NTP time not ready for attempt " + String(i + 1));
        }
      } else {
        Serial.println("WiFi Disconnected for attempt " + String(i + 1));
      }
      delay(2000); // 2秒间隔
    }
  } else {
    Serial.println("NTP initialization failed");
  }
}

void loop() {
  static unsigned long lastWeatherTime = 0;
  static unsigned long lastNTPTime = 0;
  unsigned long currentTime = millis();

  // 每 60 秒发送天气数据
  if (currentTime - lastWeatherTime >= WEATHER_UPDATE) {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      //Serial.println("Starting HTTP request...");
      http.begin(API_URL);

      int httpCode = http.GET();
      //Serial.print("HTTP Response Code: ");
      //Serial.println(httpCode);

      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        //Serial.print("Raw Payload: ");
        //Serial.println(payload);

        int startIndex = payload.indexOf("\"lives\":[{");
        if (startIndex != -1) {
          startIndex += 9;
          int endIndex = payload.indexOf("}]", startIndex);
          if (endIndex != -1) {
            String liveData = payload.substring(startIndex, endIndex + 1);
            //Serial.print("Live Data: ");
            //Serial.println(liveData);

            String temperature = getValue(liveData, "\"temperature\":\"", "\"");
            String humidity = getValue(liveData, "\"humidity\":\"", "\"");
            String reporttime = getValue(liveData, "\"reporttime\":\"", "\"");

            String output = "{\n";
            output += "  \"temperature\": \"" + temperature + "\",\n";
            output += "  \"humidity\": \"" + humidity + "\",\n";
            output += "  \"reporttime\": \"" + reporttime + "\"\n";
            output += "}";
            Serial.println("Sending weather: " + output + " to STM32F103");
            Serial2.println(output);
          } else {
            Serial.println("Error: No closing '}]' found");
          }
        } else {
          Serial.println("Error: No 'lives' array found");
        }
        http.end();
      } else {
        Serial.println("HTTP请求失败, 详情: " + http.errorToString(httpCode));
      }
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastWeatherTime = currentTime;
  }

  // 每 3600 秒（60 分钟）发送 NTP 时间
  if (currentTime - lastNTPTime >= RTC_UPDATE) {
    if (WiFi.status() == WL_CONNECTED) {
      String ntptime = getLocalTimeStr();
      String output = "{\n";
      output += "  \"ntptime\": \"" + ntptime + "\"\n";
      output += "}";
      Serial.println("Sending NTP: " + output + " to STM32F103");
      Serial2.println(output);
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastNTPTime = currentTime;
  }

  delay(1000); // 每秒检查一次
}