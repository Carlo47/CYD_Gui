#include <Arduino.h>
#include <WiFi.h>

// https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
const char *NTP_SERVER_POOL = "ch.pool.ntp.org";  // used when dns is available
const char *time_a_g_nist_gov = "129.6.15.28";    // fallback ip when dns is not available
const char *MEZ_MESZ = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Mitteleuropäische Zeit / Sommerzeit
const char *EST_EDT  = "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00";    // Eastern standard time / dayligt saving time
const char *IST_IDT  = "IST-2IDT,M3.4.4/26,M10.5.0";                   // Isral standard time / daylight saving time
const char *WGT_WGST = "WGT3WGST,M3.5.0/-2,M10.5.0/-1";                // Western Greenland time / daylight saving time
const char TZ_ZURICH[]    = "MEZ-1MESZ,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char TZ_MELBOURNE[] = "AEST10AEDT,M3.5.0/1,M10.5.0";
const char TZ_BERLIN[]    = "CET-1CEST,M3.5.0,M10.5.0/3";
const char TZ_LISBON[]    = "WET0WEST,M3.5.0/1,M10.5.0";
const char TZ_NEWYORK[]   = "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00";

/**
 * Initialize the ESP32 RTC with local time
 * and close the no longer needed WiFi connection
 * when disconnect is passed as true, default is false
*/
void initRTC(const char *timeZone, bool disconnect = false)
{
  tm rtcTime;
  configTzTime(timeZone, NTP_SERVER_POOL, time_a_g_nist_gov);
  while(! getLocalTime(&rtcTime))
  {
    Serial.println("...Failed to obtain time");
    vTaskDelay(2000);
    ESP.restart();
  }
  log_i("Got time from NTP Server");
  if (disconnect) WiFi.disconnect(true); //rtc is set, wifi connection no longer needed
  log_i("==> done");
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
