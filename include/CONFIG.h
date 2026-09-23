#pragma once

// Library
#include <WiFi.h>               // library UTAMA
#include <AsyncUDP.h>           // untuk proses UDP aplikasi android, beacon
#include <ESP32_NOW.h>          // untuk komunikasi houler <<>> loader
#include <esp_mac.h>            // conver mac
#include <Preferences.h>        // data konfigurasi
#include <Ticker.h>             // otomatisasi beacon
#include <time.h>               // epoch dan waktu dan id data
#include <cJSON.h>              // memudahkan data ke JSON
#include <vector>               // dinamic data untuk save data dan id data cache

#include <Update.h>             // OTA esp
#include <HTTPClient.h>         // esp to link
#include <WiFiClientSecure.h>   // keamanan link

// variabel library
Preferences preferences;
Ticker broadcastUDP;

// Variabel data config
String unitID;   // CHANGE: Kode dan No unit
String Loader;
int sinyal = -200;

String SSID;     // SSID: nama wifi
String PSWD;  // PSWD: pswd wifi
bool sOnline = false;

#define PLM_RX 18
#define PLM_TX 19
#define VHMS_RX 16
#define VHMS_TX 17

enum type { ping, plm, VHMS_UPD, UDP_VHMS };

struct __attribute__((__packed__)) dataGroup {
    type typeMsg;
    uint32_t epoch; 
    char sender[10];
    char target[10];
    char teksDATA[1024]; // Wadah fisik laporan teks (Zero Waste RAM)
};

dataGroup strucData;

std::vector<dataGroup> cache;

void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
  switch (event){
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.printf("Disconnected from WiFi access point\n");
      sOnline = false;
      WiFi.begin(SSID, PSWD);
    break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:      
      // 1. ambil epoch dan update
      sOnline = true;
      configTime(0, 0, "pool.ntp.org");
      time_t epochUnit = 0;
      uint8_t countGetEpoch = 0;
      while(time(&epochUnit) < 1600000000){
        countGetEpoch++;
        if(countGetEpoch >= 200){
          break;
        }
        delay(100);
      }
      Serial.printf("new Epoch: [%d]\n", epochUnit);
    break;
  }
}

void configInit(){
    preferences.begin("unit", false);
    unitID = preferences.getString("kodeUnit", "HOULER");
    SSID = preferences.getString("kodeUnit", "songolas");
    PSWD = preferences.getString("kodeUnit", "terseraH");
    preferences.end();
}

void wifiInit(){
    WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PSWD);
	WiFi.setAutoReconnect(true);
    WiFi.onEvent(WiFiEvent);
}
