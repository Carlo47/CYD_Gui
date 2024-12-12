/**
 * Program      CYD_Gui for the ESP32_2432S028R aka "Cheap Yellow Display (CYD)"
 * 
 * Author       2024-12-12 Charles Geiser
 * 
 * Purpose      Shows how to implement some graphical user interface components
 *                  UiPanel   the container for the gui components
 *                  UiButton  with a value field on the button and a label to the right. It is
 *                            the base class of UiLed and UiSlider 
 *                  UiLed     on/off toggle
 *                  UiHslider a horizontal slider that sweeps over a certain range of values
 *                  UiKeypad  a numeric keypad to enter numeric values
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
 * References   https://github.com/lovyan03/LovyanGFX                      (graphic library)
 *              https://github.com/rzeldent/platformio-espressif32-sunton/ (board definitions)
 *              https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display (CYD examples)
 *              https://www.youtube.com/watch?app=desktop&v=6JCLHIXXVus    (fixing the audio issues)
 *              https://github.com/hexeguitar/ESP32_TFT_PIO                (extending the LDR range)
 *              https://chrishewett.com/blog/true-rgb565-colour-picker/    (a true color picker, as the name says)
*/
#pragma GCC optimize ("Ofast")
#include <Arduino.h>
#include <SD.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "ESP32AutoConnect.h"
#include "lgfx_ESP32_2432S028.h"
#include "UiComponents.h"
#include "PulseGen.h"
#include "Wait.h"

using Action = void(&)(LGFX &lcd);
enum class ROTATION { LANDSCAPE_USB_RIGHT, PORTRAIT_USB_UP, 
                      LANDSCAPE_USB_LEFT,  PORTRAIT_USB_DOWN };
const char hostname[] = "cyd-gui";
AsyncWebServer server(80);
Preferences prefs;
LGFX lcd;
GFXfont myFont = fonts::DejaVu18;
//SPIClass sdcardSPI(VSPI); // uncomment this line to take screenshots

extern void nop(LGFX &lcd);
extern void initDisplay(LGFX &lcd, uint8_t rotation=0, lgfx::v1::GFXfont *theFont=&myFont, Action greet=nop);
extern void initESP32AutoConnect(AsyncWebServer &webServer, Preferences &prefs, const char hostname[]);
extern void initPrefs();
extern void initRTC(const char *timeZone, bool disconnect = false);
extern void initSDCard(SPIClass &spi);
extern bool getMappedTouch(LGFX &lcd, int &x, int &y);
extern void listFiles(File dir, int indent=0);
extern void printConnectionDetails();
extern void printDateTime(int format);
extern void printNearbyNetworks();
extern void printSDCardInfo();
extern void printPrefs();
extern bool saveBmpToSD_16bit(LGFX &lcd, const char *filename);
extern bool saveBmpToSD_24bit(LGFX &lcd, const char *filename);

extern const char *MEZ_MESZ;


/**
 * Panel 1 holds a slider and a value field linked to it.
 * The allowed input range is set to -3.3..6.6. 
 * This value range can be scrolled through using the slider.
 * Values can also be entered directly in the value field using a
 * displayed keypad. The keypad appears when the value field is tapped.
 * The entered value is limited to the specified value range.
*/
class UiPanel1 : public UiPanel
{
    public:
        UiPanel1(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            _sliderA->addValueField(_valueField);

            _sliderA->setRange(-3.3, 6.60);    // Set a double value range
            _sliderA->slideToValue(1.078000);  // Set initial value. Trailing zeros are truncated.

            //_sliderA->setRange(0, 255);   // Set an integer value range
            //_sliderA->slideToValue(128);  // Set initial value

            if (! _hidden) { show(); }
        }

        void handleKeys(int x, int y);

        void show()
        {
            UiPanel::show();
            panelText(10, 10, "Slider with assigned value field, which", TFT_WHITE, fonts::DejaVu9);
            panelText(10, 25, "is also used for input with the keypad", TFT_WHITE, fonts::DejaVu9);
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        };

        
    private:
        UiButton *_valueField = new UiButton(this, _x+10,_y+40,95,25, "", "slider value");
        UiHslider *_sliderA   = new UiHslider(this, _x+10, _y+75, 200, 12, TFT_CYAN, "A");
        std::vector<UiButton *> _btns = {_valueField, _sliderA};
};


/**
 * Panel 2 holds different buttons. 3 round LED buttons
 * and 2 rectangular buttons to switch off and on the LEDs
*/
class UiPanel2 : public UiPanel
{
    public:
        UiPanel2(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) show();
        }

        void show()
        {
            UiPanel::show();
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        };

        void    handleKeys(int x, int y);
        UiButton *getButton(uint8_t i);

    private:
        UiButton  *_btnOn  = new UiButton(this, _x+180,_y+20,50,24, blueTheme, "On");
        UiButton  *_btnOff = new UiButton(this, _x+180,_y+60,50,24, blueTheme, "Off");
        UiLed     *_led1   = new UiLed(this, _x+20, _y+15, 10, TFT_RED, "Heating", true); // preselect led1
        UiLed     *_led2   = new UiLed(this, _x+20, _y+50, 10, TFT_YELLOW, "Fan", true);  // preselect led2
        UiLed     *_led3   = new UiLed(this, _x+20, _y+85, 10, TFT_BLUE, "Water", false); // initially off
        
        std::vector<UiButton *> _btns = { _btnOn, _led1, _led2, _led3, _btnOff };    
};


/**
 * Panel 3 contains 2 value fields to display time and date from an
 * NTP server. A third value field shows the adc-value read from the
 * built-in photoresistor. No keyhandler is required for this panel.
 * The time is updated every second using the Wait class.
*/
class UiPanel3 : public UiPanel
{
    public:
        UiPanel3(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) show();
        }

        void show()
        {
            UiPanel::show();
            panelText(30, 10, "Internet Time", TFT_WHITE, fonts::DejaVu9);
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        };

      void updateDateTime();
      void updateCdsLdr();

    private:
        UiButton *_theTime = new UiButton(this, _x+25, _y+18,  94, 24, "");
        UiButton *_theDate = new UiButton(this, _x+10, _y+48, 122, 24, "");
        UiButton *_cdsLdr  = new UiButton(this, _x+10, _y+80,  50, 20, blueTheme, "", "LDR value");
        
        std::vector<UiButton *> _btns = { _theTime, _theDate, _cdsLdr };    
};


/**
 * Panel 4 contains 4 LED buttons that behave like radiobuttons. 
 * They change the brightness of the display in 4 steps.
*/
class UiPanel4 : public UiPanel
{
    public:
        UiPanel4(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
        UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) show();
        }

        void show()
        {
            UiPanel::show();
            panelText(10, 10, "Radiobuttons", TFT_WHITE, fonts::DejaVu9);
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        }
        void handleKeys(int x, int y);

    private:
        UiLed     *_led1   = new UiLed(this, _x+15, _y+30, 7, TFT_RED,    blueTheme, "****", true); // preselect led1
        UiLed     *_led2   = new UiLed(this, _x+15, _y+50, 7, TFT_GREEN,  blueTheme, "***");
        UiLed     *_led3   = new UiLed(this, _x+15, _y+70, 7, TFT_BLUE,   blueTheme, "**");
        UiLed     *_led4   = new UiLed(this, _x+15, _y+90, 7, TFT_YELLOW, blueTheme, "*");

        std::vector<UiButton *> _btns = { _led1, _led2, _led3, _led4 };                
};

// Declare pointers to the panels and initialize them with nullptr
UiPanel1  *panel1 = nullptr; 
UiPanel2  *panel2 = nullptr; 
UiPanel3  *panel3 = nullptr;
UiPanel4  *panel4 = nullptr;

// Declare the static class variable again in main
std::vector<UiPanel *> UiPanel::panels;

// Create they keypad hidden
UiKeypad keypad(lcd, 20,80, TFT_GOLD, true);    

Wait waitUserInput(100);  // look for user input every 100 ms
Wait waitDateTime(1000);  // Get time and date every second
Wait waitCdsLdr(2500);    // Read CDS LDR all 2.5 seconds


/**
 * Keyhandler for Panel 1. 
 * If the tapped coordinates x,y correspond to a button, the associated 
 * function is executed. 
 *  - If the value field of the slider is tapped, the value field is 
 *    registered with the keypad and this is then displayed.
 *  - If the slider is tapped or moved, the corresponding value is 
 *    displayed in the value field.
 * The order of the buttons, i.e. value field and slider,  is determined by
 * their definition in the std::vector<UiButtons *> of the associated panel. 
*/
void UiPanel1::handleKeys(int x, int y)
{
    for (int i = 0; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y)) 
        {
            switch(i)
            {
                case 0: // The value field of the slider has been tapped
                    _pKeypad->addValueField(_btns.at(i)); // Register the value field with the keypad
                    _pKeypad->show(); // show keypad 
                break;

                case 1: // The slider has been tapped
                    _sliderA->slideToPosition(x);
                break;
            }
        }
    }
}


/**
 * Keyhandler for Panel 2
 * If the tapped coordinates x,y correspond to a button, the associated 
 * function is executed.
 *  - The 3 LED buttons only change their status, which is indicated by a color change.
 *  - The 2 buttons on and off switch all LEDs on or off.
 * The order of the buttons is determined by their definition in 
 * the std::vector<UiButtons *> of the associated panel.
*/
void UiPanel2::handleKeys(int x, int y)
{
    for (int i = 0; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y)) 
        {
            switch(i)
            {
                case 0: // _btnOn
                  _led1->on(); _led2->on(); _led3->on();
                break;

                case 1: // _led1
                  _led1->toggle();
                break;

                case 2: // _led2
                   _led2->toggle();
                break;

                case 3: // _led3
                   _led3->toggle();
                break;

                case 4: // _btnOff
                   _led1->off(); _led2->off(); _led3->off();
                break;
            }
            delay(300);
        }      
    }
}


/**
 * Action for Panel 3
 * Updates time and date in the corresponding value fields
*/
void UiPanel3::updateDateTime()
{
    tm   rtcTime;
    char buf[12];
    getLocalTime(&rtcTime);
    strftime(buf, sizeof(buf), "%T", &rtcTime); // hh:mm:ss
    _theTime->updateValue(buf);
    strftime(buf, sizeof(buf), "%F", &rtcTime); // YYYY-MM-DD
    _theDate->updateValue(buf);
}


/**
 * Action for Panel 3
 * Updates the reading from the photo resistor
*/
void UiPanel3::updateCdsLdr()
{
    uint16_t adc_value = analogRead(CDS_LDR);
    _cdsLdr->updateValue(adc_value);
}


/**
 * Keyhandler for Panel 4
 * If the tapped coordinates x,y correspond to a button, the associated 
 * function is executed.
 * The 4 LED buttons behave like radiobuttons, only one can be active.
 * They vary the brightness of the display in 4 steps.
* The order of the buttons is determined by their definition in 
 * the std::vector<UiButtons *> of the associated panel.
*/
void UiPanel4::handleKeys(int x, int y)
{
    for (int i = 0; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y)) 
        {
            for (int b = 0; b < _btns.size(); b++)  // switch all LED-buttons off
            {
                reinterpret_cast<UiLed *>(_btns.at(b))->off();
            }
            reinterpret_cast<UiLed *>(_btns.at(i))->on();
            switch(i)
            {
                case 0:
                    _lcd.setBrightness(255);
                break;
                case 1:
                    _lcd.setBrightness(128);
                break;
                case 2:
                    _lcd.setBrightness(64);
                break;
                case 3:
                    _lcd.setBrightness(32);
                break;
            }
        }      
    }
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


/**
 * 👉 An empty white image is created when SD card 
 * and touch are both active. Why are the pixels
 * not output to the file? 
*/
void takeScreenshot()
{   
    static int count= 0;
    char buf[64];
    snprintf(buf, sizeof(buf), "/SCREENSHOTS/screen%03d.bmp", count++);
    saveBmpToSD_16bit(lcd, buf);
    log_i("Screenshot saved: %s\n", buf);
}


/**
 * Called when OK button of keypad is clicked 
 */
void handleOkButton(UiButton* btn)
{
    String e = btn->getValue();
    double v = e.toDouble();
    log_i("Keypad OK button clicked, entered value = %5.3f", v);
}

void setup() 
{
  Serial.begin(115200);

    // Photoconductive cell GT36516 on pin CDS_LDR = 34 varies between 5 .. 300 kOhm
    analogSetAttenuation(ADC_0db);  // Set lowest attenuation for CDS
    pinMode(CDS_LDR, INPUT);

    // Starts the blinking task, which causes the RGB LED to flash 
    // red, green and blue alternately every second
    xTaskCreate(blinkTask, "blinkTask", 1024, NULL, 10, NULL);
    initESP32AutoConnect(server, prefs, hostname);
    printConnectionDetails();
    printNearbyNetworks();

    initRTC(MEZ_MESZ);
    printDateTime(5);

    lcd.setBaseColor(DARKERGREY);
    initDisplay(lcd, static_cast<uint8_t>(ROTATION::PORTRAIT_USB_UP));
  
    //initSDCard(sdcardSPI);      // Init SD card to take screenshots
    printSDCardInfo();          // Print SD card details 
    listFiles(SD.open("/"));    // List the files on SD card 

    // Create the panels and showm them ( argument hidden is set to false)
    panel1 = new UiPanel1(lcd, 0,                 0, lcd.width(),  lcd.height()/3, TFT_OLIVE,  false);
    panel2 = new UiPanel2(lcd, 0,     lcd.height()/3, lcd.width(), lcd.height()/3, TFT_GREEN,  false);
    panel3 = new UiPanel3(lcd, 0,   2*lcd.height()/3, 145,         lcd.height()/3, TFT_SKYBLUE,false);
    panel4 = new UiPanel4(lcd, 145, 2*lcd.height()/3,  95,         lcd.height()/3, TFT_ORANGE, false);

    // Initialize the static class variable with all panels
    UiPanel::panels = {panel1, panel2, panel3, panel4};

    // Add a keypad to panel 1
    panel1->addKeypad(&keypad);
    keypad.addOkCallback(handleOkButton);

    lcd.setBrightness(255);
    waitDateTime.begin();

    log_i("==> done");
}


void loop() 
{
    int x, y;
    if (waitUserInput.isOver() && getMappedTouch(lcd, x, y))
    {
        //log_i("Key pressed at %3d, %3d\n", x, y);
        if (!panel1->isHidden()) panel1->handleKeys(x, y);
        if (!panel2->isHidden()) panel2->handleKeys(x, y);
        if (!panel4->isHidden()) panel4->handleKeys(x, y);
        if (!keypad.isHidden())  keypad.handleKeys(x, y);
    }
  
    if (!panel1->isHidden() && waitCdsLdr.isOver())   panel3->updateCdsLdr();
    if (!panel3->isHidden() && waitDateTime.isOver()) panel3->updateDateTime();

    // To take automatically screenshots uncomment the following lines
    // and also line 51 and 453. But when the SD card is activatet, the
    // touchpad is no longer funtioning.
/*     delay(2000); takeScreenshot();
    delay(2000); keypad.show();
    delay(2000); takeScreenshot(); 
    delay(2000); keypad.hide(); UiPanel::redrawPanels(); */

}
