#include <SPI.h>
#include <Wire.h>

//Industrino IO
#include <Indio.h>

//Watchdog timer
#include <Adafruit_SleepyDog.h>

//Panel
#include <IndioPanel.h>
IndioPanel panel;

//LCD
#include <UC1701.h>
static UC1701 lcd;

//RTC
#include <IndioRtc.h>
IndioRtc rtc;
now_t now;
now_t bootTime;
char rtcTimeText[20];

//SD Card
#include <SD.h>
File sdFile;

//Menus
int menuLevel = 10;
bool menuForceEnter = false;
byte cursorPosition = 0;
byte cursorLowLimit = 0;
byte cursorHighLimit = 0;


void setup()
{
    Wire.begin();
    rtc.begin();
    rtc.getTime(&bootTime);
    lcd.begin();
    lcd.clear();
    panel.begin();
    panel.setBacklightTimeout(10000); //10 Seconds
    SerialUSB.begin(9600); // Initialize SerialUSB port
}


void loop()
{
    if (rtc.newSecond()) MenuDisplayTime();
    MenuNavigate();
}


void MenuDisplayTime()
{
    rtc.getTime(&now);
    sprintf(rtcTimeText, "%02d/%02d/%02d %02d:%02d:%02d", now.day, now.month, now.year, now.hour, now.minute, now.second); //Date in DD/MM/YY format
    lcd.setCursor(6, 0);
    lcd.print(rtcTimeText);
}


void MenuNavigate()
{
    //If we are not at the home screen but the timeout has expired, go home
    if (menuLevel > 0 && panel.timeoutExpired())
    {
        MenuHome_Asleep();
    }

    //If any button has been pressed
    if (panel.read() || menuForceEnter)
    {
        if (panel.enterPressed() || menuForceEnter)
        {
            menuForceEnter = false;
            switch (menuLevel + cursorPosition)
            {
                //Home
                case 0:
                    MenuHome_Awake(); break;
                case 10:
                    MenuSettings(); break;
                //Settings
                case 24:
                    MenuSettings_BootupTime(); break;
                case 25:
                    MenuSettings_SetTime(); break;
                case 26:
                    MenuHome_Awake(); break;
            }
        }

        //If there is a cursor to display
        if (cursorPosition != 0) MenuProcessCursor();
    }
}


void MenuNew(int _menuLevel, byte _cursorLowLimit, byte _cursorHighLimit)
{
    menuLevel = _menuLevel;
    cursorLowLimit = _cursorLowLimit;
    cursorHighLimit = _cursorHighLimit;
    cursorPosition = _cursorHighLimit;
    lcd.clear();
    MenuDisplayTime();
}


void MenuHome()
{
    lcd.setCursor(12, 4);
    lcd.print(F("Home Screen"));
}


void MenuHome_Asleep()
{
    MenuNew(0, 0, 0);
    MenuHome();
}


void MenuHome_Awake()
{
    MenuNew(10, 0, 0);
    MenuHome();
}


void MenuSettings()
{
    MenuNew(20, 4, 6); //MenuLevel 20, Cursor display from line 4 to line 6
    lcd.setCursor(12, 4);
    lcd.print(F("Bootup time"));
    lcd.setCursor(12, 5);
    lcd.print(F("Set date & time"));
    lcd.setCursor(12, 6);
    lcd.print(F("Exit"));
}


void MenuSettings_BootupTime()
{
    MenuNew(10, 0, 0);
    lcd.setCursor(12, 3);
    lcd.print(F("Bootup time"));
    lcd.setCursor(12, 4);
    sprintf(rtcTimeText, "%02d/%02d/%02d %02d:%02d:%02d", bootTime.day, bootTime.month, bootTime.year, bootTime.hour, bootTime.minute, bootTime.second); //Date in DD/MM/YY format
    lcd.print(rtcTimeText);
}


void MenuSettings_SetTime()
{
    now_t newTime;
    rtc.getTime(&newTime);

    MenuNew (10, 0, 0);
    lcd.setCursor(24, 3);
    lcd.print(F("Set Hour"));
    lcd.setCursor(24, 4);
    LcdPrintPaddedZero(newTime.hour, 2);
    lcd.print(':');
    LcdPrintPaddedZero(newTime.minute, 2);

    while (!panel.timeoutExpired())
    {
        if (rtc.newSecond()) MenuDisplayTime();
        if (panel.read())
        {
            newTime.hour = PanelAdjustByte(newTime.hour, 0, 23);
            lcd.setCursor(24, 4);
            LcdPrintPaddedZero(newTime.hour, 2);
            if (panel.enterPressed()) break;
        }
    }

    lcd.setCursor(24, 3);
    lcd.print(F("Set Minute"));
    while (!panel.timeoutExpired())
    {
        if (rtc.newSecond()) MenuDisplayTime();
        if (panel.read())
        {
            newTime.minute = PanelAdjustByte(newTime.minute, 0, 59);
            lcd.setCursor(42, 4);
            LcdPrintPaddedZero(newTime.minute, 2);
            if (panel.enterPressed()) break;
        }
    }
    rtc.setMinute(newTime.minute);
    rtc.setHour(newTime.hour);
    rtc.setSecond(0);

    lcd.clear();
    MenuDisplayTime();
    lcd.setCursor(24, 3);
    lcd.print(F("Set Date"));
    lcd.setCursor(24, 4);
    LcdPrintPaddedZero(newTime.day, 2);
    lcd.print('/');
    LcdPrintPaddedZero(newTime.month, 2);
    lcd.print('/');
    LcdPrintPaddedZero(newTime.year, 2);

    while (!panel.timeoutExpired())
    {
        if (rtc.newSecond()) MenuDisplayTime();
        if (panel.read())
        {
            newTime.day = PanelAdjustByte(newTime.day, 1, 31);
            lcd.setCursor(24, 4);
            LcdPrintPaddedZero(newTime.day, 2);
            if (panel.enterPressed()) break;
        }
    }

    lcd.setCursor(24, 3);
    lcd.print(F("Set Month"));
    while (!panel.timeoutExpired())
    {
        if (rtc.newSecond()) MenuDisplayTime();
        if (panel.read())
        {
            newTime.month = PanelAdjustByte(newTime.month, 1, 12);
            lcd.setCursor(42, 4);
            LcdPrintPaddedZero(newTime.month, 2);
            if (panel.enterPressed()) break;
        }
    }

    lcd.setCursor(24, 3);
    lcd.print(F("Set Year "));
    while (!panel.timeoutExpired())
    {
        if (rtc.newSecond()) MenuDisplayTime();
        if (panel.read())
        {
            newTime.year = PanelAdjustByte(newTime.year, 21, 99);
            lcd.setCursor(60, 4);
            LcdPrintPaddedZero(newTime.year, 2);
            if (panel.enterPressed()) break;
        }
    }

    if (!rtc.setDate(newTime.day, newTime.month, newTime.year))
    {
        lcd.clear();
        lcd.setCursor(24, 3);
        lcd.print(F("Invalid date!"));
        delay(2000);
        lcd.clear();
    }
    MenuDisplayTime();
    menuForceEnter = true;
}


int PanelAdjustByte( byte value, byte lower, byte upper)
{
    if (panel.upPressed())
    {
        if (value == upper) value = lower;
        else value++;
    }
    if (panel.downPressed())
    {
        if (value == lower) value = upper;
        else value--;
    }
    return value;
}


int MenuProcessCursor( )
{
    if (panel.upPressed())
    {
        if (cursorPosition == cursorLowLimit) cursorPosition = cursorHighLimit;
        else cursorPosition--;
    }
    if (panel.downPressed())
    {
        if (cursorPosition == cursorHighLimit) cursorPosition = cursorLowLimit;
        else cursorPosition++;
    }
    for (byte i = 2; i <= 7; i++)
    {
        lcd.setCursor(0, i);
        if (i == cursorPosition) lcd.print('>');
        else lcd.print(' ');
    }
}


void LcdPrintPaddedZero (int value, byte places)
{
    if (value < 10000 && places > 4) lcd.print('0');
    if (value < 1000 && places > 3) lcd.print('0');
    if (value < 100 && places > 2) lcd.print('0');
    if (value < 10 && places > 1) lcd.print('0');
    lcd.print (value);
}
