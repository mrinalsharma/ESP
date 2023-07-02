#include <ArduinoWiFiServer.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiGratuitous.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

#include <AsyncMqttClient.h>
#include <AsyncMqttClient.hpp>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <Uri.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <Wire.h>
#include <SPI.h>
#include "Event.h"
#include <EEPROM.h>
#include <ESP8266mDNS.h>

// Raspberry Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, 177)
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

// MQTT Topics
#define MQTT_PUB_TEMP "esp/bme680/temperature"
#ifndef SSID
#define APSSID "MrinalESP8266"
#define APPSK "12345678"
#endif

//DNS port
const byte DNS_PORT = 53;
IPAddress apIP(172, 217, 28, 1);
IPAddress netMsk(255, 255, 255, 0);
const char* softAP_ssid = APSSID;
const char* softAP_password = APPSK;
/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char* myHostname = "switch";
const long interval = 10000;

// Variables to hold sensor readings
float temperature = 1.0;
AsyncMqttClient mqttClient;
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
;
Ticker wifiReconnectTimer;
Ticker mqttReconnectTimer;
unsigned long previousMillis = 0;  // Stores last time temperature was published
DNSServer dnsServer;
/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[33] = "\0";
char password[65] = "\0";
char deviceName[33] = "switch";

boolean connect;
boolean apsetup = false;
int wifiConnectionRetries = 0;

//relay variables
int relayGPIO = 5;

// Web server
ESP8266WebServer server(80);


void apSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(1000);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);
  setupMqttClient();
}

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  Serial.println(ssid);
  Serial.println(password);
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
  //wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
  wifiConnectionRetries = 0;
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach();  // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiConnectionRetries++;
  if (wifiConnectionRetries <= 5) {
    wifiReconnectTimer.once(2, connectToWifi);
    Serial.printf("Continue WiFi connection retry, count(%d).\n", wifiConnectionRetries);
  } else {
    wifiConnectionRetries = 0;
    wifiReconnectTimer.detach();
    wifiDisconnectHandler.reset();
    Serial.printf("Stopped WiFi connection retry, count(%d).\n", wifiConnectionRetries);
    if (WiFi.getMode() == WIFI_STA) {
      Serial.println("Switching to AP mode.");
      apsetup = true;
    }
  }
}

void setupMqttClient() {
  Serial.println("Setup MQTT Client...");
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onMessage(onMqttMessage);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  mqttClient.subscribe((String(deviceName)+String("/command")).c_str(), 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  mqttReconnectTimer.detach();
  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

struct userData {
  char ssid[33];
  char password[65];
  char deviceName[33];
};

/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  userData data;
  EEPROM.get(0, data);
  //EEPROM.get(0 + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(data), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = '\0';
    password[0] = '\0';
    deviceName[0] = '\0';
  } else {
    ets_strcpy(ssid, data.ssid);
    ets_strcpy(password, data.password);
    ets_strcpy(deviceName, data.deviceName);
  }
  Serial.printf("Recovered credentials:  SSID(%s), Password(%s), DeviceName(%s) \n", ssid, strlen(password) > 0 ? "********" : "<no password>", deviceName);
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  userData data;
  String localDeviceName = String(deviceName);
  localDeviceName.trim();
  if (!isspace(localDeviceName.c_str()[0])) {
    ets_strcpy(data.deviceName, localDeviceName.c_str());
  } else {
    ets_strcpy(deviceName, "switch");
  }
  ets_strcpy(data.ssid, ssid);
  ets_strcpy(data.password, password);
  EEPROM.begin(512);
  EEPROM.put(0, data);
  //EEPROM.put(0 + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(data), ok);
  EEPROM.commit();
  EEPROM.end();
}


void setup() {
  Serial.begin(9600);
  Serial.println();
  pinMode(relayGPIO, OUTPUT);
  digitalWrite(relayGPIO, HIGH);
  switch (ESP.getResetInfoPtr()->reason) {
    case REASON_EXT_SYS_RST:
    case REASON_WDT_RST:
    case REASON_SOFT_WDT_RST:
      Serial.printf("Reset ESP8266 reason (%s) \n", ESP.getResetReason().c_str());
      EEPROM.begin(512);
      // write a 0 to all 512 bytes of the EEPROM
      for (int i = 0; i < 512; i++) { EEPROM.write(i, 0); }
      EEPROM.end();
  }



  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/generate_204", handleRoot);  // Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);        // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound(handleNotFound);
  server.begin();  // Web server start
  Serial.println("HTTP server started");
  loadCredentials();
  connect = strlen(ssid) > 0;  // Request WLAN connect if there is a SSID
  if (!connect) {
    apsetup = true;
  }
}

void loop() {
  unsigned long currentMillis = millis();
  dnsServer.processNextRequest();
  // HTTP
  server.handleClient();

  if (connect) {
    Serial.println("Connect requested");
    connect = false;
    connectToWifi();
  }

  if (apsetup) {
    apSetup();
    apsetup = false;
  }

  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;

    if (mqttClient.connected()) {

      HomeEvent event;
      event.setEvent("switch", digitalRead(relayGPIO) == 1 ? "Light On" : "Light OFF", digitalRead(relayGPIO) == 1 ? "ON" : "OFF", "Kitchen", deviceName);
      String stringEvent = event.toJson();
      Serial.printf("Message: %s \n", stringEvent.c_str());
      uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, stringEvent.c_str());
      Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
    }
  }
}