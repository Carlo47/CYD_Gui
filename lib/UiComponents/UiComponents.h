#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "lgfx_ESP32_2432S028.h"

#pragma once
#define DARKERGREY 0x4208


using Callback = void(&)(bool state);

class uiTheme
{
    public:
        uiTheme(int textColor=TFT_WHITE, int btnColor=DARKERGREY, int borderColor=TFT_SKYBLUE, int shadowColor=TFT_BLACK, 
             const GFXfont *font = &fonts::DejaVu18) :
            _textColor(textColor), _btnColor(btnColor), _borderColor(borderColor), _shadowColor(shadowColor), _font(font)
        {}
        
        int _textColor;
        int _btnColor;
        int _borderColor;
        int _shadowColor;
        const GFXfont *_font;       
};
extern uiTheme defaultTheme;

class uiButton
{
    static void nop(bool arg){;}

    public:
        uiButton(LGFX &lcd, int x, int y, int w, int h, uiTheme &theme, String value="", String label="") : 
            _lcd(lcd), _x(x), _y(y), _w(w), _h(h), _theme(theme), _value(value), _label(label)
        {}

        uiButton(LGFX &lcd, int x, int y, int w, int h, String value="", String label="") : 
            _lcd(lcd), _x(x), _y(y), _w(w), _h(h), _value(value), _label(label)
        {}


        void draw();
        bool touched(int x, int y);
        void clearValue();
        void updateValue(int value);
        void updateValue(float value);
        void updateValue(String value);
        void setLabel(String label);
        void clearLabel();

    private:
        LGFX &_lcd;
        uiTheme &_theme=defaultTheme;
        int _d = 8;         // Distance from button to label
        int _r = 4;         // Radius of rounded rectangle
        int _x, _y, _w, _h; // Position and dimension
        String _value="";
        String _label="";
};


class uiLED
{
    public:
        uiLED(LGFX &lcd, int x, int y, int r, int color, Callback cb, uiTheme &theme, String label="") : 
            _lcd(lcd), _x(x), _y(y), _r(r), _color(color), _cb(cb), _theme(theme), _label(label)
        {}

        uiLED(LGFX &lcd, int x, int y, int r, int color, Callback cb, String label="") : 
            _lcd(lcd), _x(x), _y(y), _r(r), _color(color), _cb(cb), _label(label)
        {}

        void draw();
        void on();
        void off();
        void setLabel(String txt);
        void toggle();
        bool touched(int x, int y);

    private:
        LGFX &_lcd;
        uiTheme &_theme=defaultTheme;
        int _d = 8;
        int _x, _y, _r, _color; 
        bool _isOn = true;
        Callback _cb;
        String _label = "";    
};
