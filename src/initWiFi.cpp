#include <Arduino.h>
#include <WiFi.h>

// WiFi credentials 
const char ssid[]     = "Your SSID";
const char password[] = "Your Key";
const char NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char TIME_ZONE[]       = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char HOST_NAME[] = "ESP32-CYD"; 

/**
 * Use a raw string literal to print a formatted
 * string of WiFi connection details
 */
void printConnectionDetails()
{
  Serial.printf(R"(
Connection Details:
------------------
  SSID       : %s
  Hostname   : %s
  IP-Address : %s
  MAC-Address: %s
  RSSI       : %d (received signal strength indicator)
  )", WiFi.SSID().c_str(),
      //WiFi.hostname().c_str(),  // ESP8266
      WiFi.getHostname(),    // ESP32 
      WiFi.localIP().toString().c_str(),
      WiFi.macAddress().c_str(),
      WiFi.RSSI());
  Serial.printf("\n");
}


/**
 * Establish the WiFi connection with router 
 * and set a hostname for the ESP32
*/
void initWiFi()
{
  Serial.println("Connecting to WiFi");
  WiFi.setHostname(HOST_NAME);
  WiFi.begin(ssid, password);

  // Try forever
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("...Connecting to WiFi");
    delay(1000);
  }
  Serial.println("Connected");
  log_i("===> done");
  // 👉The next line prevents the interrupt at GPIO_NUM_36 
  //   from being triggered continuously. This undesirable 
  //   effect is due to a hardware error in the ESP32 chip.
  //   This workaround can be omitted if interrupt pins 36 
  //   and 39 are not used. 
  WiFi.setSleep(WIFI_PS_NONE);
  log_i("==> done");
}


void initNTP()
{
  configTzTime(TIME_ZONE, NTP_SERVER_POOL);
  log_i("===> done");
}