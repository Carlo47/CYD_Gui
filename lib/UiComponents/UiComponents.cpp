#include "UIcomponents.h"

//--- uiButton ---
void uiButton::draw()
{
    _lcd.drawRoundRect(_x+2, _y+2, _w, _h, _r, _theme._shadowColor);
    _lcd.drawRoundRect(_x+1, _y+1, _w, _h, _r, _theme._shadowColor);
    _lcd.fillRoundRect(_x, _y, _w, _h, _r, _theme._borderColor);
    _lcd.fillRoundRect(_x+2, _y+2, _w-4, _h-4, _r, _theme._bodyColor);
    _lcd.setTextDatum(textdatum_t::middle_center);
    _lcd.setTextColor(_theme._textColor);
    _lcd.setFont(_theme._font);
    _lcd.drawString(_value, _x+_w/2, _y+2+_h/2);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
}

bool uiButton::touched(int x, int y)
{
    return (x > _x && x < _x+_w && y > _y && y < _y+_h);
}

void uiButton::clearValue()
{
    _value= "";
    draw();
}

void uiButton::updateValue(String value)
{
    _value = value;
    draw();
}

void uiButton::updateValue(int value)
{
    _value = String(value);
    draw();
}

void uiButton::updateValue(float value)
{
    _value = String(value);
    draw();
}

void uiButton::setLabel(String label)
{
    _label = label;
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
}

void uiButton::clearLabel()
{
    _lcd.setTextColor(_lcd.getBaseColor());
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
    _lcd.setTextColor(_theme._textColor); 
}


//--- uiLED ---
void uiLED::draw()
{
    _lcd.fillCircle(_x+2, _y+2, _r, _theme._shadowColor);
    _lcd.fillCircle(_x, _y, _r, _theme._borderColor);
    _lcd.fillCircle(_x, _y, _r-2, _color);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(_theme._textColor);
    _lcd.setFont(_theme._font);
    _lcd.drawString(_label, _x+_r*2+_d, _y);
} 

void uiLED::on()
{
    if (! _isOn)
    {
        _lcd.fillCircle(_x, _y, _r-2, _color);
        _isOn = true;
    }
    _cb(_isOn);
}  

void uiLED::off()
{
    if (_isOn)
    {
        _lcd.fillCircle(_x, _y, _r-2, _theme._bodyColor);
        _isOn = false;
    }
    _cb(_isOn);
} 

void uiLED::setLabel(String txt)
{
    _label = txt;     
}    

void uiLED::toggle()
{
    if (_isOn)
    {
        _lcd.fillCircle(_x, _y, _r-2, _theme._bodyColor);
        _isOn = false;
    }
    else
    {
        _lcd.fillCircle(_x, _y, _r-2, _color);
        _isOn = true;
    }
    _cb(_isOn);
}

bool uiLED::touched(int x, int y)
{
    return (x > _x-_r && x < _x+_r && y > _y-_r && y < _y+_r);
}


//--- uiSlider ---

void uiHslider::draw()
{
    _lcd.drawRoundRect(_x+2, _y+2, _w, _h, _r, _theme._shadowColor);
    _lcd.drawRoundRect(_x+1, _y+1, _w, _h, _r, _theme._shadowColor);
    _lcd.fillRoundRect(_x, _y, _w, _h, _r, _theme._borderColor);
    _lcd.fillRoundRect(_x+2, _y+2, _w-4, _h-4, _r, _theme._bodyColor);
    _lcd.fillCircle(_position, _y+_h/2, _h, _color);
    _lcd.drawCircle(_position, _y+_h/2, _h, _theme._borderColor);
    _cb(_value);    
}


void uiHslider::slideTo(int x)
{
    
    _lcd.fillCircle(_position, _y+_h/2, _h, _lcd.getBaseColor());
    _position = x;
    _value = (_position-_x) * 100 / _w;
    draw(); 
}

bool uiHslider::touched(int x, int y)
{
    return (x >= _x && x <= _x+_w && y >= _y && y <= _y+_h);
}