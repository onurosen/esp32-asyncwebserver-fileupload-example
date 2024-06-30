#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "webpages.h"

#define FIRMWARE_VERSION "v0.0.1"

const String default_ssid = "somessid";
const String default_wifipassword = "mypassword";
const String default_httpuser = "admin";
const String default_httppassword = "admin";
const int default_webserverporthttp = 80;

// configuration structure
struct Config {
  String ssid;               // wifi ssid
  String wifipassword;       // wifi password
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
  int webserverporthttp;     // http port number for web admin
};

// variables
Config config;                        // configuration
bool shouldReboot = false;            // schedule a reboot
AsyncWebServer *server;               // initialise webserver

// function defaults
String listFiles(bool ishtml = false);

void setup() {
  Serial.begin(115200);

  Serial.print("Yazılım Versiyon: "); Serial.println(FIRMWARE_VERSION);

  Serial.println("Boot ediliyor ...");

  Serial.println("SPIFFS Bağlanıyor ...");
  if (!SPIFFS.begin(true)) {
    // if you have not used SPIFFS before on a ESP32, it will show this error.
    // after a reboot SPIFFS will be configured and will happily work.
    Serial.println("Hata: SPIFFS bağlananadı, Yeniden Boot ediliyor...");
    rebootESP("ERROR: Cannot mount SPIFFS bağlanamadı, Yeniden Boot ediliyor...");
  }

  Serial.print("SPIFFS Boş alan: "); Serial.println(humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes())));
  Serial.print("SPIFFS Kullanılan alan: "); Serial.println(humanReadableSize(SPIFFS.usedBytes()));
  Serial.print("SPIFFS Toplam alan: "); Serial.println(humanReadableSize(SPIFFS.totalBytes()));

  Serial.println(listFiles());

  Serial.println("Konfigürasyon yükleniyor ...");

  config.ssid = default_ssid;
  config.wifipassword = default_wifipassword;
  config.httpuser = default_httpuser;
  config.httppassword = default_httppassword;
  config.webserverporthttp = default_webserverporthttp;

  Serial.print("\nWifi ağına bağlanılıyor: ");
  WiFi.begin(config.ssid.c_str(), config.wifipassword.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n\nAğ Konfigürasyonu:");
  Serial.println("----------------------");
  Serial.print("            SSID: "); Serial.println(WiFi.SSID());
  Serial.print("     Wifi Durumu: "); Serial.println(WiFi.status());
  Serial.print("Wifi Sinyal Gücü: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
  Serial.print("             MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("              IP: "); Serial.println(WiFi.localIP());
  Serial.print("          Subnet: "); Serial.println(WiFi.subnetMask());
  Serial.print("       Ağ Geçidi: "); Serial.println(WiFi.gatewayIP());
  Serial.print("           DNS 1: "); Serial.println(WiFi.dnsIP(0));
  Serial.print("           DNS 2: "); Serial.println(WiFi.dnsIP(1));
  Serial.print("           DNS 3: "); Serial.println(WiFi.dnsIP(2));
  Serial.println();

  // configure web server
  Serial.println("Web Server konfigüre ediliyor ...");
  server = new AsyncWebServer(config.webserverporthttp);
  configureWebServer();

  // startup web server
  Serial.println("Web Server başlatılıyor ...");
  server->begin();
}

void loop() {
  // reboot if we've told it to reboot
  if (shouldReboot) {
    rebootESP("Web Yöneticisi Tarafından Yeniden Başlatma");
  }
}

void rebootESP(String message) {
  Serial.print("ESP32 yeniden başlatılıyor: "); Serial.println(message);
  ESP.restart();
}

// list all of the files, if ishtml=true, return html rather than simple text
String listFiles(bool ishtml) {
  String returnText = "";
  Serial.println("SPIFFS'te depolanan dosyaları listeleme");
  File root = SPIFFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button onclick=\"downloadDeleteButton(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "Dosya: " + String(foundfile.name()) + " Boyut: " + humanReadableSize(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String humanReadableSize(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}
