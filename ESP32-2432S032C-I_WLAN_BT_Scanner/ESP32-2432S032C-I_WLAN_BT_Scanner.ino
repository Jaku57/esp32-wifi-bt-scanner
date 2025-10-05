// Dieser Sketch ist für das Board ESP32-2432S032C-I!
// Nach dem Scannen werden links SSID's mit Channel und Pegel in dBm angezeigt.
// Auf der rechten Seite des Displays werden Bluetooth (MAC) und Pegel angezeigt.
// Wegen zu kleinem Speicher muss folgende Einstellung geändert werden: 
// Werkzeuge → Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
// Voreingestellt war: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <WiFi.h>
#include <BluetoothSerial.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

BluetoothSerial SerialBT;
BLEScan* pBLEScan;

struct WifiEntry {
  String ssid;
  int32_t rssi;
  int32_t channel;
  wifi_auth_mode_t encryption;
};

// Pegelanzeige: 1 px Abstand zur Trennlinie, rechtsbündig
void drawRSSIValue(int x, int y, int rssi) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(x - 30, y + 12);
  tft.printf("%4d", rssi);
}

// Kopfzeile
void drawHeader() {
  tft.fillRect(0, 0, 320, 24, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // WLAN
  tft.setCursor(10, 12);
  tft.print("SSID");
  tft.setCursor(118, 12);
  tft.print("CH");
  tft.setCursor(150, 12);
  tft.print("dBm");

  // Bluetooth (abgekürzt)
  tft.setCursor(200, 12);
  tft.print("BT");
  tft.setCursor(285, 12);
  tft.print("dBm");

  // Trennlinien
  tft.drawLine(0, 20, 320, 20, TFT_WHITE);
  tft.drawLine(184, 0, 184, 240, TFT_WHITE);
}

// WLAN-Ergebnisse
void drawWifiResults(WifiEntry* entries, int n) {
  const int maxRows = 9;
  for (int i = 0; i < maxRows; i++) {
    int y = 40 + i * 24;
    tft.fillRect(0, y - 19, 184, 24, TFT_BLACK);
    delay(10);
    if (i < n) {
      // SSID
      tft.setCursor(0, y);
      String ssid = entries[i].ssid;
      if (ssid.length() > 10) ssid = ssid.substring(0, 10);  // 1 Zeichen mehr
      tft.print(ssid);

      // Kanal (unter "CH")
      tft.setCursor(122, y);
      tft.printf("%2d", entries[i].channel);

      // RSSI mit 1 px Abstand zur Trennlinie
      drawRSSIValue(170, y - 14, entries[i].rssi);
    }
  }
}

// BLE-Ergebnisse
void drawBleResults(BLEScanResults* foundDevices) {
  const int maxRows = 9;
  int count = foundDevices->getCount();

  BLEAdvertisedDevice* devices[count];
  for (int i = 0; i < count; i++) {
    devices[i] = new BLEAdvertisedDevice(foundDevices->getDevice(i));
  }

  // Sortieren nach RSSI
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (devices[j]->getRSSI() > devices[i]->getRSSI()) {
        BLEAdvertisedDevice* tmp = devices[i];
        devices[i] = devices[j];
        devices[j] = tmp;
      }
    }
  }

  // Anzeige
  for (int i = 0; i < maxRows; i++) {
    int y = 40 + i * 24;
    tft.fillRect(187, y - 19, 133, 24, TFT_BLACK);
    delay(10);
    if (i < count) {
      BLEAdvertisedDevice* device = devices[i];
      tft.setCursor(190, y);

      String name;
      if (device->getName().length() > 0) {
        name = device->getName().c_str();
      } else {
        // MAC kürzen (letzte 3 Bytes)
        String mac = device->getAddress().toString().c_str();
        if (mac.length() > 8) mac = mac.substring(mac.length() - 8);
        name = mac;
      }

      if (name.length() > 11) name = name.substring(0, 11);
      tft.print(name);

      // RSSI mit 1 px Abstand zur rechten Kante
      drawRSSIValue(306, y - 14, device->getRSSI());
    }
  }

  for (int i = 0; i < count; i++) {
    delete devices[i];
  }
}

void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(&FreeMono9pt7b);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  SerialBT.begin("ESP32_BT");

  BLEDevice::init("ESP32_BLE");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);

  drawHeader();
}

void loop() {
  int n = WiFi.scanNetworks();
  WifiEntry entries[n];
  for (int i = 0; i < n; i++) {
    entries[i].ssid = WiFi.SSID(i);
    entries[i].rssi = WiFi.RSSI(i);
    entries[i].channel = WiFi.channel(i);
    entries[i].encryption = WiFi.encryptionType(i);
  }

  // Sortieren nach RSSI
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (entries[j].rssi > entries[i].rssi) {
        WifiEntry tmp = entries[i];
        entries[i] = entries[j];
        entries[j] = tmp;
      }
    }
  }

  drawWifiResults(entries, n);

  BLEScanResults* foundDevices = pBLEScan->start(2);
  drawBleResults(foundDevices);
  if (foundDevices) pBLEScan->clearResults();

  delay(200);
}
