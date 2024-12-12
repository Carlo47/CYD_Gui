#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ESP32AutoConnect.h"

/**
 * Print nearby WiFi networks with SSID und RSSI 
 */
void printNearbyNetworks()
{
  int n = WiFi.scanNetworks();
  printf(R"(
Nearby WiFi networks:
--------------------
)");
  for (int i = 0; i < n; i++)
  {
    printf("%s\t%d\r\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
}


/**
 * Use a raw string literal to print a formatted
 * string of WiFi connection details
 */
void printConnectionDetails()
{
  printf(R"(
Connection Details:
------------------
  SSID       : %s
  Hostname   : %s
  IP-Address : %s
  MAC-Address: %s
  RSSI       : %d (received signal strength indicator)
  )", WiFi.SSID().c_str(),
      //WiFi.hostname().c_str(), // ESP8266
	  WiFi.getHostname(),   // ESP32 
      WiFi.localIP().toString().c_str(),
      WiFi.macAddress().c_str(),
      WiFi.RSSI());  
}


void initESP32AutoConnect(AsyncWebServer &webServer, Preferences &prefs, const char hostname[])
{
  ESP32AutoConnect ac(webServer, prefs, hostname);
  //ac.clearCredentials();  // activate this line to remove stored credentials
  ac.autoConnect();
}