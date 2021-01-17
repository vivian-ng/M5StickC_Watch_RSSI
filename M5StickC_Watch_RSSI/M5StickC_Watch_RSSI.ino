/***
 * Arduino sketch for M5StickC that displays the current time, date,
 * and WiFi signal strength of the connected SSID.
 * 
 * Adapted from this sketch: https://github.com/LordLeXaT/m5StickC-Clock
 * 
 * Shortpress button A (large button with 'M5') disables LCD (powersaving)
 * Longpress (2sec) button B (small button at top) forces resync with NTP-Server
 * 
 * Change from good to moderate signal is set at -60 dBm and from moderate to poor
 * at -70 dBm.
 * Reference used: https://eyesaas.com/wi-fi-signal-strength/
 * 
 * Put configuration settings in a separate config.h file that contains
 * #define TIMEZONE     9
 * #define WIFI_SSID   "your_wifi_ssid"
 * #define WIFI_PASSWORD   "your_wifi_password"
 * 
 * where 
 * TIMEZONE is the number of hours ahead of GMT (use negative number if behind GMT)
 * WIFI_SSID is SSID to connect to
 * WIFI_PASSWORD is password for SSID
 ***/

#include <Arduino.h>
#include <M5StickC.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include "time.h"
#include "config.h"
#include <ArduinoOTA.h>

// default hostname if not defined in config.h
#ifndef HOSTNAME
  #define HOSTNAME "m5stickc"
#endif

// use the WiFi settings in config.h file
char* ssid       = WIFI_SSID;
char* password   = WIFI_PASSWORD; 

// define the NTP server to use
char* ntpServer =  "ntp.nict.jp";

// define what timezone you are in
int timeZone = TIMEZONE * 3600;

// delay workarround
int tcount = 0;

// LCD Status
bool LCD = true;

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

//delays stopping usualy everything using this workarround
bool timeToDo(int tbase) {
  tcount++;
  if (tcount == tbase) {
    tcount = 0;
    return true;    
  } else {
    return false;
  }  
}

// Syncing time from NTP Server
void timeSync() {
    M5.Lcd.setTextSize(1);
    Serial.println("Syncing Time");
    Serial.printf("Connecting to %s ", ssid);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(20, 15);
    M5.Lcd.println("connecting WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println(" CONNECTED");
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(20, 15);
    M5.Lcd.println("Connected");
    // Set ntp time to local
    configTime(timeZone, 0, ntpServer);

    // Get local time
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
      // Set RTC time
      RTC_TimeTypeDef TimeStruct;
      TimeStruct.Hours   = timeInfo.tm_hour;
      TimeStruct.Minutes = timeInfo.tm_min;
      TimeStruct.Seconds = timeInfo.tm_sec;
      M5.Rtc.SetTime(&TimeStruct);

      RTC_DateTypeDef DateStruct;
      DateStruct.WeekDay = timeInfo.tm_wday;
      DateStruct.Month = timeInfo.tm_mon + 1;
      DateStruct.Date = timeInfo.tm_mday;
      DateStruct.Year = timeInfo.tm_year + 1900;
      M5.Rtc.SetData(&DateStruct);
      Serial.println("Time now matching NTP");
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(20, 15);
      M5.Lcd.println("S Y N C");
      delay(500);
      M5.Lcd.fillScreen(BLACK);
    }
}

void buttons_code() {
  // Button A control the LCD (ON/OFF)
  if (M5.BtnA.wasPressed()) {
    if (LCD) {
      M5.Lcd.writecommand(ST7735_DISPOFF);
      M5.Axp.ScreenBreath(0);
      LCD = !LCD;
    } else {
      M5.Lcd.writecommand(ST7735_DISPON);
      M5.Axp.ScreenBreath(255);
      LCD = !LCD;
    }
  }
  // Button B doing a time resync if pressed for 2 sec
  if (M5.BtnB.pressedFor(2000)) {
    timeSync();
  }
}

// Printing WiFi RSSI and time to LCD
void doTime() {
  //if (timeToDo(1000)) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    M5.Lcd.setCursor(10, 10);
    M5.Lcd.setTextSize(1);
    M5.Lcd.printf("%s: ", WiFi.SSID());
    long strength = WiFi.RSSI();
    if(strength < -70) M5.Lcd.setTextColor(RED, BLACK);
    else if(strength < -60) M5.Lcd.setTextColor(YELLOW, BLACK);
    else M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.printf("%02d\n", strength);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Rtc.GetTime(&RTC_TimeStruct);
    M5.Rtc.GetData(&RTC_DateStruct);
    M5.Lcd.setCursor(10, 25);
    M5.Lcd.printf("%02d:%02d:%02d\n", RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
    M5.Lcd.setCursor(15, 60);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.printf("Date: %04d-%02d-%02d\n", RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date);
  //}
}

void setup() {
  M5.begin();

  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE,BLACK);
  timeSync(); //uncomment if you want to have a timesync everytime you turn device on (if no WIFI is avail mostly bad)

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(HOSTNAME);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void loop() {
  M5.update();
  buttons_code();
  doTime();
  ArduinoOTA.handle();
}
