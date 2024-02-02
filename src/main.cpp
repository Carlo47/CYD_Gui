/**
 * Program      CYD_Gui with ESP32_2432S028R aka "Cheap Yellow Display (CYD)"
 * 
 * Author       2024-01-31 Charles Geiser
 * 
 * Purpose      Shows how to implement some graphical user interface components
 *                  uiButton with a value field on the button and a label to the right
 *                  uiLed    on/off toggle with callback
 * 
 * Board        ESP32-2432S028 with touchscreen and SD card from AITEXM ROBOT
 *              https://www.aliexpress.com/item/1005005616073472.html?gps-id=pcStoreJustForYou&scm=1007.23125.137358.0&scm_id=1007.23125.137358.0&scm-url=1007.23125.137358.0&pvid=629012e6-491d-40f0-b41b-033335bc0c49&_t=gps-id:pcStoreJustForYou,scm-url:1007.23125.137358.0,pvid:629012e6-491d-40f0-b41b-033335bc0c49,tpp_buckets:668%232846%238114%231999&pdp_npi=4%40dis%21CHF%2110.65%218.62%21%21%2112.03%219.74%21%40210324bf17060488843367930ea758%2112000033759549673%21rec%21CH%21767770434%21&spm=a2g0o.store_pc_home.smartJustForYou_2007716161329.1005005616073472
 * 
 * Remarks      👉 For some inexplicable reason, no screenshot can be saved 
 *                 to the SD card when the touchscreen is active. 
 * 
 * SPI pins                    SPI_HOST        SPIClass  MISO   MOSI   SCLK  CS  IRQ   Type
 *              LCD       SPI2_HOST=HSPI_HOST    HSPI     12     13     14   15   -    ILI9341  
 *              SD Card   SPI3_HOST=VSPI_HOST    VSPI     19     23     18    5   -    ENC28J60 
 *              Touchpad  SPI3_HOST=VSPI_HOST    VSPI     39     32     25   33   36   XPT2046
 * 
 * References   https://github.com/lovyan03/LovyanGFX
 *              https://github.com/rzeldent/platformio-espressif32-sunton/
 *              https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
 *              https://www.youtube.com/watch?app=desktop&v=6JCLHIXXVus  (fixing the audio issues)
 *              https://github.com/hexeguitar/ESP32_TFT_PIO (extending the LDR range)
 *              https://chrishewett.com/blog/true-rgb565-colour-picker/
*/
#pragma GCC optimize ("Ofast")
#include <Arduino.h>
#include <SD.h>
#include "UiComponents.h"   // includes also LovyanGFX.hpp and lgfx_ESP32_2432S028.h
#include "PulseGen.h"

using Action = void(&)(LGFX &lcd);

extern void calibrateTouch(LGFX &lcd);
extern void initDisplay(LGFX &lcd, lgfx::v1::GFXfont *theFont, Action greet);
extern void initNTP();
extern void initSDCard(SPIClass &spi);
extern void initWiFi();
extern void listFiles(File dir, int indent=0);
extern void printConnectionDetails();
extern void printSDCardInfo();
extern bool saveToSD_16bit(LGFX &lcd, const char *filename, bool swapBytes=true);


// Forward declaration of the button actions
void reverseDirection();
void switchLeds(bool);
void takeScreenshot();

// Forward declaration of the callbacks for the uiLed components
void toggleFan(bool);
void toggleHeating(bool);
void toggleWater(bool);


SPIClass sdcardSPI(HSPI);
LGFX lcd;
GFXfont myFont = fonts::DejaVu18;

//                Text       Button  Border  Shadow  Font
uiTheme blueTheme(TFT_BLACK, 0x07df, 0x03df, 0x01ca, &fonts::DejaVu12);
uiTheme defaultTheme;

// Create the UI components
uiButton btnClear(lcd, 10,10,40,20, "", "Clear");
uiButton btnOn(lcd, 125,10,50,20,blueTheme, "On");
uiButton btnOff(lcd, 185,10,50,20,blueTheme, "Off");
uiButton adc(lcd, 10, 130, 60, 24, "", "LDR value");
uiButton theTime(lcd, 10, 170, 94, 24);
uiButton theDate(lcd, 112, 170, 122, 24);
uiButton btnReverse(lcd, 50, 250, 60, 24, ">>", "forward");
uiButton btnScreenshot(lcd, 20, 290, 200, 20, "Save Screenshot");
uiLED    led1(lcd, 30, 50, 10,  TFT_RED, toggleHeating, blueTheme, "Heating");
uiLED    led2(lcd, 30, 80, 10,  TFT_YELLOW, toggleFan, blueTheme, "Fan");
uiLED    led3(lcd, 30, 110, 10, TFT_BLUE, toggleWater, "Water");


void toggleHeating(bool state)
{
    Serial.printf("Heating is now %s\n", state ? "on" : "off");
}

void toggleFan(bool state)
{
    Serial.printf("Fan is now %s\n", state ? "on" : "off");
}

void toggleWater(bool state)
{
    Serial.printf("Water is now %s\n", state ? "on" : "off");
}

void switchLeds(bool state)
{
    if (state)
    {
        led1.on(); led2.on(); led3.on();
    }
    else
    {
        led1.off(); led2.off(); led3.off();
    }
}

void reverseDirection()
{
    static bool dir = true;
    if (dir)
    {
        btnReverse.updateValue(">>");
        btnReverse.clearLabel();
        btnReverse.setLabel("forward");
        dir = ! dir;
    }
    else
    {
        btnReverse.updateValue("<<");
        btnReverse.clearLabel();
        btnReverse.setLabel("backward");      
        dir = ! dir;       
    }
}

/**
 * 👉 An empty white image is created when SD card 
 * and touch are both active. Why are the pixels
 * not output to the file? 
*/
void takeScreenshot()
{   
    saveToSD_16bit(lcd, "/SCREENSHOTS/sreen01.bmp", true);
    Serial.printf("%s\n", "Screenshot saved");
}


/**
 * Returns true, as soon as msWait milliseconds have passed.
 * Supply a reference to an unsigned long variable to hold 
 * the previous milliseconds.
 */
bool waitIsOver(uint32_t &msPrevious, uint32_t msWait)
{
  return (millis() - msPrevious >= msWait) ? (msPrevious = millis(), true) : false;
}


/**
 * Generates 3 pulse generators, each of which  
 * causes one of the RGB LEDs to flash.
 * Runs as an independent process on the second core.
*/
void blinkTask(void* arg)
{
  PulseGen pulseGenRed(RGB_LED_R, 3000000, 100000, 0);
  PulseGen pulseGenGreen(RGB_LED_G, 3000000, 100000, 3000000/3);
  PulseGen pulseGenBlue(RGB_LED_B, 3000000, 100000, 2*3000000/3);
    // Enable the pulse generators
  pulseGenRed.on();
  pulseGenGreen.on();
  pulseGenBlue.on();
  while (true)
  {
  // Let the LEDs blink
  pulseGenRed.loop();
  pulseGenGreen.loop();
  pulseGenBlue.loop();
  vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}


void initUi()
{
    lcd.setBaseColor(TFT_DARKGREEN);
    lcd.clear();
    lcd.setBrightness(50);  // Reduce lcd backlight

    // Draw UI components
    btnClear.draw();
    btnOn.draw();
    btnOff.draw();
    led1.draw();
    led2.draw();
    led3.draw();
    adc.draw();
    theTime.draw();
    theDate.draw();
    btnReverse.draw();
    btnScreenshot.draw();   
}


void setup(void)
{
    Serial.begin(115200);

    // Photoconductive cell GT36516 on pin CDS = 34 varies
    // between 5 .. 300 kOhm
    analogSetAttenuation(ADC_0db);  // Set lowest attenuation for CDS
    pinMode(CDS, INPUT);
    
    // Starts the blinking task, which causes the RGB LED to flash 
    // red, green and blue alternately every second
    xTaskCreate(blinkTask, "blinkTask", 1024, NULL, 10, NULL);

    initWiFi();
    printConnectionDetails();
    initNTP();
    initDisplay(lcd, &myFont, calibrateTouch);  // Init display and calibrate
    initSDCard(sdcardSPI);          // Init SD card
    printSDCardInfo();              // Print SD card details 
    listFiles(SD.open("/"));        // List the files on SD card 

    // Read images from SD card and show them on screen
    lcd.drawBmpFile("/sd/dodeka.bmp", 0,0) ? log_e("file opened") : log_e("file not found");
    delay(1000);
    lcd.drawJpgFile("/sd/colorSwatch.jpg", 0,0) ? log_e("file opened") : log_e("file not found");
    delay(1000);

    // Initialize the User Interface
    initUi();
}

tm   rtcTime;
const int bufSize = 12;
char buf[bufSize];

int x, y;
uint32_t msPrevious0 = 0;
uint32_t msPrevious1 = 0;
uint32_t msPrevious2 = 0;


void loop()
{
    // Read the touchscreen every 300 ms and handle user interactions
    if ( waitIsOver(msPrevious0, 300u))
    {
        if (lcd.touch())
        {
            x = y = 0;
            lcd.getTouch(&x, &y);
            if (btnClear.touched(x, y))  { switchLeds(false); adc.clearValue(); theTime.clearValue(); theDate.clearValue(); }
            if (btnOn.touched(x, y))  { switchLeds(true); }
            if (btnOff.touched(x, y)) { switchLeds(false); }
            if (led1.touched(x, y))   { led1.toggle(); }
            if (led2.touched(x, y))   { led2.toggle(); }
            if (led3.touched(x, y))   { led3.toggle(); }
            if (btnReverse.touched(x, y)) { reverseDirection(); }
            if (btnScreenshot.touched(x, y)) {takeScreenshot(); }
        }
    }

    // Read the CDS (cadmium sulfide) aka LDR (light dependent resistor) every 2 sec
    if (waitIsOver(msPrevious1, 2000u))
    {
        uint16_t adc_value = analogRead(CDS);
        adc.updateValue(adc_value);
    }

    // Refresh the time and date every second
    if (waitIsOver(msPrevious2, 1000u))
    {
        getLocalTime(&rtcTime);
        strftime(buf, bufSize, "%T", &rtcTime); // hh:mm:ss
        theTime.updateValue(buf);
        strftime(buf, bufSize, "%F", &rtcTime); // YYYY-MM-DD
        theDate.updateValue(buf);
    }
}