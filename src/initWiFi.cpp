#include <Arduino.h>
#include <WiFi.h>

// WiFi credentials 
const char ssid[]     = "Your SSID";
const char password[] = "Your Password";
const char NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char TIME_ZONE[]       = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char HOST_NAME[]       = "ESP32-CYD"; 

// Forward declarations
void initNTP();
void initWiFi();
void printConnectionDetails();
void printNearbyNetworks();     
void printDateTime(int format); // Format: Output
                                // 0:      hh:mm:ss
                                // 1:      YYYY-MM-DD
                                // 2:      January 15 2019 16:33:20 (Tuesday)
                                // 3:      2019-01-15 16:33:20
                                // 4:      Tue Jan 15 16:33:20 2019
                                // 5:      2019-01-15 16:51:18 02/2 MEZ +0100

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

    // 👉 The next line prevents the interrupt at GPIO_NUM_36 
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


/*
 * Print nearby WiFi networks with SSID und RSSI 
 */
void printNearbyNetworks()
{
  int n = WiFi.scanNetworks();
  Serial.printf(R"(
Nearby WiFi networks
--------------------
)");
  for (int i = 0; i < n; i++)
  {
    Serial.printf("%s\t%d\r\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
  }
  Serial.println();
}


/**
 * Use a raw string literal to print a formatted
 * string of WiFi connection details
 */
void printConnectionDetails()
{
  Serial.printf(R"(
Connection Details
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
 * Print date and time in various formats
 */
void printDateTime(int format)
{
  tm   rtcTime;
  char buf[40];
  int  bufSize = sizeof(buf);

  getLocalTime(&rtcTime);
  switch (format)
  {
    case 0:  
      strftime(buf, bufSize, "%T",                &rtcTime); // hh:mm:ss
    break;
    case 1:  
      strftime(buf, bufSize, "%F",                &rtcTime); // YYYY-MM-DD
    break;
    case 2:  
      strftime(buf, bufSize, "%B %d %Y %T (%A)",  &rtcTime); // January 15 2019 16:33:20 (Tuesday)
    break;
    case 3:  
      strftime(buf, bufSize, "%F %T",             &rtcTime); // 2019-01-15 16:33:20
    break;
    case 4:  
      strftime(buf, bufSize, "%c",                &rtcTime); // Tue Jan 15 16:33:20 2019
    break;
    case 5:  
      strftime(buf, bufSize, "%F %T %W/%w %Z %z", &rtcTime); // 2019-01-15 16:51:18 02/2 MEZ +0100
    break;
    default: 
      strftime(buf, bufSize, "%D %r",             &rtcTime ); // 08/23/01 02:55:02 pm
    break;
  }
  Serial.printf("%s\n", buf);
}
