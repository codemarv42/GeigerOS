///////////////////Includes//////////////////

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>  //https://github.com/Bodmer/TFT_eSPI

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"
#include "ESP32_BLE.h"

#define LTBLUE 0xB6DF
#define LTTEAL 0xBF5F
#define LTGREEN 0xBFF7
#define LTCYAN 0xC7FF
#define LTRED 0xFD34
#define LTMAGENTA 0xFD5F
#define LTYELLOW 0xFFF8
#define LTORANGE 0xFE73
#define LTPINK 0xFDDF
#define LTPURPLE 0xCCFF
#define LTGREY 0xE71C

#define BLUE 0x001F
#define TEAL 0x0438
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define ORANGE 0xFC00
#define PINK 0xF81F
#define PURPLE 0x8010
#define GREY 0xC618
#define WHITE 0xFFFF
#define BLACK 0x0000

#define DKBLUE 0x000D
#define DKTEAL 0x020C
#define DKGREEN 0x03E0
#define DKCYAN 0x03EF
#define DKRED 0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW 0x8400
#define DKORANGE 0x8200
#define DKPINK 0x9009
#define DKPURPLE 0x4010
#define DKGREY 0x4A49

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

#define DIGITAL_BLUE 0x3B8F

#define M_SIZE 1.3333

int AccentColor = STD_ACCENT_COLOR;

TFT_eSPI tft = TFT_eSPI();

/*
  Display/Page Structure:
  ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯

  DrawBootPage()
  │
  PageNumber
  │
  ├───0-MainPageNumber
  │   ├───0-MedicalMode()
  │   ├───1-GraphMode()
  │   ├───2-EducationMode()
  │   ├───3-TextMode()
  │   ├───4-MineralMode()
  │   ├───5-MeterMode() //Mit Meter aus TFT_espi bsp
  │   ├───6-SimpleMode()
  │   └───7-Dosimeter()
  │
  ├───1-QuickMenu()
  │   ├───ToneMode
  │   ├───LedMode
  │   ├───LoggingMode()
  │   ├───PowerMode
  │   ├───RotatingDisplay
  │   └───QuickGuide()
  │
  └───2-Settings()
    ├───Units()*
    ├───SoundSettings()
    ├───AlarmSettings()*
    ├───TubeSettings()* //Geigertube or semiconductor settings
    ├───TimeSettings()*
    ├───GeneralSettings()* //Language
    ├───SDSettings()*
    ├───AboutTheGeigerCounter()
    ├───WiFiSettings()*
    ├───BluetoothSettings()*
    ├───CalibrateTouch()*
    └───Sensors()

*/

/////////////////////////////////////////////

//////////////////Variables//////////////////

uint16_t calData[5];

//Graph//

double ox, oy;
int xUpperLimit = 60;
int graphx = 0;
bool GraphUpdate = true;

//Meter//
float ltx = 0;                                    // Saved x coord of bottom of needle
uint16_t osx = M_SIZE * 120, osy = M_SIZE * 120;  // Saved x & y coords
uint32_t updateTime = 0;                          // time for next update
int old_analog = -999;                            // Value last displayed
bool MeterUpdate = true;

unsigned int PageNumber = 0;
unsigned int MainPageNumber = 0;
unsigned int QuickMenuPage = 0;
unsigned int TimedCountPage = 0;
unsigned int SettingsPage = 0;

bool TimeFormat = 0;  //0 - 24H, 1 - 12H
bool Keylock = false;
bool ClearPage = true;  //true - Clear the page, false - Clear not the page
int Language = 0;       //0 - German, 1 - English
bool ForceUpdateDisplay = false;
bool TouchDetected = false;

unsigned long PreviousMillis4;
unsigned int TouchX;
unsigned int TouchY;

//Timed count//

bool TimedCountSDLogging = false;
unsigned int TimedCountDuration = 1;

unsigned int TimedCountStartHour;
unsigned int TimedCountStartMinute;
unsigned int TimedCountStartSecond;

unsigned int TimedCountEndHour;
unsigned int TimedCountEndMinute;
unsigned int TimedCountEndSecond;

unsigned int TMPDay, TMPMonth, TMPYear, TMPHour, TMPMinute, TMPSecond;

unsigned int TimedCountTmpMillis;

/////////////////////////////////////////////

////////////////////Code/////////////////////

void TFTInit() {
  tft.init();
  tft.setRotation(0);
  //TFTCalibrate();
  tft.setTouch(calData);
}

void TFTCalibrate() {

  SetDisplayBrightness(255);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (Language == 0) {
    tft.println("Ecken wie angegeben beruehren");
  } else {
    tft.println("Touch corners as indicated");
  }

  tft.setTextFont(1);
  tft.println();

  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

  Serial.println();
  Serial.println();
  Serial.print("  uint16_t calData[5] = ");
  Serial.print("{ ");

  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(calData[i]);
    if (i < 4) Serial.print(", ");
  }

  Serial.println(" };");

  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  if (Language == 0) {
    tft.println("Kalibrierung abgeschlossen!");
  } else {
    tft.println("Calibration complete!");
  }

  delay(2000);

  tft.println("");

  if (Language == 0) {
    tft.println("Beruehren Sie die Home-Taste innerhalb von 20 Sekunden, um die Beruehrungsdaten anzuwenden!");
  } else {
    tft.println("Touch home button in 20s to apply touch data!");
  }

  tft.setTouch(calData);

  DrawButton(2, 1);

  unsigned long TmpPreviousMillis = millis();

  bool TmpTouched = false;

  uint16_t TX, TY;  //Touch coordinates

  while ((TmpTouched == false) && ((TmpPreviousMillis + 20000) > millis())) {
    tft.getTouch(&TX, &TY);

    if ((TX > 136) && (TX < (136 + 48))) {  //Home Button Middle Pos. 1
      if ((TY > 429) && (TY <= (429 + 48))) {

        Serial.println("Home Button pressed");

        TmpTouched = true;
      }
    }
  }

  if (TmpTouched == true) {
    EepromSave();
  } else {
    ESP.restart();
  }
}

void UpdateDisplay() {
  if (PageNumber == 0) {  //MainPage
    if (MainPageNumber == 0) {
      MedicalMode();
    } else if (MainPageNumber == 1) {
      MedicalModeCPM();
    } else if (MainPageNumber == 2) {
      MeterMode();
    } else if (MainPageNumber == 3) {
      //DosimeterMode();
      MainPageNumber = 0;
    } else if (MainPageNumber == 4) {  //MineralMode
      MainPageNumber = 0;
    } else if (MainPageNumber == 5) {  //SimpleMode
      MainPageNumber = 0;
    }
  } else if (PageNumber == 1) {  //QuickMenu
    QuickMenu(QuickMenuPage);
  } else if (PageNumber == 2) {  //Settings
    Settings(SettingsPage);
  } else if (PageNumber == 3) {  //EXTRA: Touch cal Approval
    TouchCalApprovalPage();
  } else if (PageNumber == 4) {  //Average Speed Settings Page
    AverageSpeedPage();
  } else if (PageNumber == 5) {  //Timed count Settings Page
    TimedCountSettingsPage(TimedCountPage);
  }
}

void ReadTouchScreen() {
  uint16_t TX, TY;  //Touch coordinates

  if (Keylock == false) {
    if (tft.getTouch(&TX, &TY)) {
      PreviousMillis4 = millis();
      if (TouchDetected == false) {
        TouchX = TX;
        TouchY = TY;

        //tft.fillCircle(TX, TY, 2, TFT_BLACK); //Draw a block spot to show where touch was calculated to be

        //Defines the button area for the touch function
        if (PageNumber == 0) {        //MainPage same as UpdateDisplay()
          if (MainPageNumber == 0) {  //MedicalMode

            TouchConditionsMainPage();

          } else if (MainPageNumber == 1) {  //GraphMode

            TouchConditionsMainPage();

          } else if (MainPageNumber == 2) {  //EducationMode

            TouchConditionsMainPage();

          } else if (MainPageNumber == 3) {  //TextMode

            TouchConditionsMainPage();

          } else if (MainPageNumber == 4) {  //MineralMode

            TouchConditionsMainPage();

          } else if (MainPageNumber == 5) {  //SimpleMode

            TouchConditionsMainPage();
          }
        } else if (PageNumber == 1) {  //QuickMenu
          if (QuickMenuPage == 0) {

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Energy options Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Energy options Button pressed");

                QuickMenuPage = 1;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Cal. Speed Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Switch Tone Mode Button pressed");

                PageNumber = 4;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //LED Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Switch LED Mode Button pressed");

                if (LEDMode == 1) {
                  LEDMode = 0;
                } else {
                  LEDMode = 1;
                }

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Timed count Button
              if ((TY > 214) && (TY <= (214 + 44))) {

                Serial.println("Timed count Button pressed");

                PageNumber = 5;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Display brightness Button
              if ((TY > 264) && (TY <= (264 + 44))) {

                Serial.println("Display brightness Button pressed");

                QuickMenuPage = 2;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Rotate display Button
              if ((TY > 264 + 50) && (TY <= (264 + 50 + 44))) {

                Serial.println("Rotate display Button pressed");

                RotateDisplay();

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if (TYPE != 1) {
              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Select counting tube Button
                if ((TY > 264 + 100) && (TY <= (264 + 100 + 44))) {

                  Serial.println("Select counting tube Button pressed");

                  if (SelectIntExtTube == true) {
                    SelectIntExtTube = false;
                  } else {
                    SelectIntExtTube = true;
                  }

                  ClearPage = true;
                  GraphUpdate = true;
                  graphx = 0;
                  MeterUpdate = true;
                  ResetCPMArray();

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Home Button Pos.0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Home Button pressed");
                ClearPage = true;
                PageNumber = 0;
                GraphUpdate = true;
                graphx = 0;
                MeterUpdate = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (QuickMenuPage == 1) {

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Shutdown Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Shutdown Button pressed");

                DrawShutdownPage();

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Restart Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Restart Button pressed");

                EepromSave();
                ESP.restart();

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Sleep mode Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Sleep mode Button pressed");

                esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 0);
                SetDisplayBrightness(0);
                SetHV(0);

                delay(1000);
                esp_light_sleep_start();

                delay(300);

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Energy save mode Button
              if ((TY > 164 + 50) && (TY <= (164 + 50 + 44))) {

                Serial.println("Energy save mode Button pressed");

                DBright = 30;


                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }


            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");
                ClearPage = true;
                QuickMenuPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (QuickMenuPage == 2) {  //Display brightness control

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                QuickMenuPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (DBright >= 250) {
                  DBright = 255;
                } else {
                  DBright = DBright + 10;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (DBright <= 20) {
                  DBright = 20;
                } else {
                  DBright = DBright - 10;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }
          }
        } else if (PageNumber == 2) {  //Settings

          if (SettingsPage == 0) {

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Main Settings Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Main Settings Button pressed");

                SettingsPage = 2;
                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Alert Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Alert Button pressed");

                SettingsPage = 17;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if (TYPE != 1) {

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Micro SD Button
                if ((TY > 164) && (TY <= (164 + 44))) {

                  Serial.println("Micro SD Button pressed");

                  SettingsPage = 14;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Cal Touch Button
                if ((TY > 214) && (TY <= (214 + 44))) {

                  Serial.println("Cal Touch Button pressed");

                  PageNumber = 3;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //WiFi Button
                if ((TY > 214 + 50) && (TY <= (214 + 50 + 44))) {

                  Serial.println("WiFi Button pressed");

                  SettingsPage = 23;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Bluetooth Button
                if ((TY > 214 + 100) && (TY <= (214 + 100 + 44))) {

                  Serial.println("Bluetooth Button pressed");

                  SettingsPage = 11;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //System Button
                if ((TY > 214 + 150) && (TY <= (214 + 150 + 44))) {

                  Serial.println("System Button pressed");

                  SettingsPage = 20;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            } else {

              if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Cal Touch Button
                if ((TY > 164) && (TY <= (164 + 44))) {

                  Serial.println("Cal Touch Button pressed");

                  PageNumber = 3;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //WiFi Button
                if ((TY > 164 + 50) && (TY <= (164 + 50 + 44))) {

                  Serial.println("WiFi Button pressed");

                  SettingsPage = 23;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Bluetooth Button
                if ((TY > 164 + 100) && (TY <= (164 + 100 + 44))) {

                  Serial.println("Bluetooth Button pressed");

                  SettingsPage = 11;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //System Button
                if ((TY > 164 + 150) && (TY <= (164 + 150 + 44))) {

                  Serial.println("System Button pressed");

                  SettingsPage = 20;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Home Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Home Button pressed");
                ClearPage = true;
                PageNumber = 0;
                GraphUpdate = true;
                graphx = 0;
                MeterUpdate = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }


          } else if (SettingsPage == 1) {  //Display timeout

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (DTimeout >= 3660000) {
                  DTimeout = 3660000;
                } else {
                  DTimeout = DTimeout + 60000;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (DTimeout <= 60000) {
                  DTimeout = 60000;
                } else {
                  DTimeout = DTimeout - 60000;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 2) {  //Main Settings Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Language Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Language Button pressed");

                SettingsPage = 3;
                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Units Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Units Button pressed");

                SettingsPage = 8;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if (TYPE != 1) {

              if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Time/Date Button
                if ((TY > 164) && (TY <= (164 + 44))) {

                  Serial.println("Time/Date Button pressed");

                  SettingsPage = 4;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Tube Settings Button
                if ((TY > 214) && (TY <= (214 + 44))) {

                  Serial.println("Tube Settings Button pressed");

                  SettingsPage = 7;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Display timeout Button
                if ((TY > 214 + 50) && (TY <= (214 + 44 + 50))) {

                  Serial.println("Display timeout Button pressed");

                  SettingsPage = 1;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Accent color
                if ((TY > 214 + 100) && (TY <= (214 + 44 + 100))) {

                  Serial.println("Accent color Button pressed");

                  SettingsPage = 27;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            } else {

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Display timeout Button
                if ((TY > 164) && (TY <= (164 + 44))) {

                  Serial.println("Display timeout Button pressed");

                  SettingsPage = 1;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Accent color
                if ((TY > 164 + 50) && (TY <= (164 + 44 + 50))) {

                  Serial.println("Accent color Button pressed");

                  SettingsPage = 27;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 3) {  //Language Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //German Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("German Button pressed");

                Language = 0;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //English Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("English Button pressed");

                Language = 1;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 4) {  //Time/Date Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Time Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Time Button pressed");

                SettingsPage = 5;

                TMPHour = Hour;
                TMPMinute = Minute;
                TMPSecond = Second;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Date Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Date Button pressed");

                SettingsPage = 6;

                TMPDay = Day;
                TMPMonth = Month;
                TMPYear = Year;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                SetRTC(TMPSecond, TMPMinute, TMPHour, TMPDay, TMPMonth, TMPYear);

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 5) {  //Time Page

            if ((TX > 15) && (TX < (15 + 60))) {  //Plus Button Hour
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Hour pressed");

                if (TMPHour == 23) {
                  TMPHour = 23;
                } else {
                  TMPHour = TMPHour + 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 15) && (TX < (15 + 60))) {  //Minus Button Hour
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Hour pressed");

                if (TMPHour == 0) {
                  TMPHour = 0;
                } else {
                  TMPHour = TMPHour - 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 90) && (TX < (90 + 60))) {  //Plus Button Minute
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Minute pressed");

                if (TMPMinute == 59) {
                  TMPMinute = 59;
                } else {
                  TMPMinute = TMPMinute + 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 90) && (TX < (90 + 60))) {  //Minus Button Minute
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Minute pressed");

                if (TMPMinute == 0) {
                  TMPMinute = 0;
                } else {
                  TMPMinute = TMPMinute - 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 165) && (TX < (165 + 60))) {  //Plus Button Second
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Second pressed");

                if (TMPSecond >= 55) {
                  TMPSecond = 59;
                } else {
                  TMPSecond = TMPSecond + 5;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 165) && (TX < (165 + 60))) {  //Minus Button Second
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Second pressed");

                if (TMPSecond <= 5) {
                  TMPSecond = 0;
                } else {
                  TMPSecond = TMPSecond - 5;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 4;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 6) {  //Date Page

            if ((TX > 15) && (TX < (15 + 60))) {  //Plus Button Year
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Year pressed");

                if (TMPYear == 99) {
                  TMPYear = 99;
                } else {
                  TMPYear = TMPYear + 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 15) && (TX < (15 + 60))) {  //Minus Button Year
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Year pressed");

                if (TMPYear == 0) {
                  TMPYear = 0;
                } else {
                  TMPYear = TMPYear - 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 90) && (TX < (90 + 60))) {  //Plus Button Month
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Month pressed");

                if (TMPMonth == 12) {
                  TMPMonth = 12;
                } else {
                  TMPMonth = TMPMonth + 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 90) && (TX < (90 + 60))) {  //Minus Button Month
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Month pressed");

                if (TMPMonth == 1) {
                  TMPMonth = 1;
                } else {
                  TMPMonth = TMPMonth - 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 165) && (TX < (165 + 60))) {  //Plus Button Day
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button Day pressed");

                if (TMPDay == 31) {
                  TMPDay = 31;
                } else {
                  TMPDay = TMPDay + 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 165) && (TX < (165 + 60))) {  //Minus Button Day
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button Day pressed");

                if (TMPDay == 1) {
                  TMPDay = 1;
                } else {
                  TMPDay = TMPDay - 1;
                }

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 4;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 7) {  //Geiger Tube Settings

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //SBM-20
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("SBM-20 Button pressed");

                GeigerTubeModel = 0;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //SBM-19/STS-6 Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("SBM-19/STS-6 Button pressed");

                GeigerTubeModel = 1;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //15 mm (Typ B) Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("15 mm (Typ B) Button pressed");

                GeigerTubeModel = 2;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Ludlum 44-9 Button
              if ((TY > 214) && (TY <= (214 + 44))) {

                Serial.println("Ludlum 44-9 Button pressed");

                GeigerTubeModel = 3;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Custom Button
              if ((TY > 214 + 50) && (TY <= (214 + 50 + 44))) {

                Serial.println("Custom Button pressed");

                GeigerTubeModel = 4;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Custom settings Button
              if ((TY > 214 + 100) && (TY <= (214 + 100 + 44))) {

                Serial.println("Custom settings Button pressed");


                ClearPage = true;
                SettingsPage = 24;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 8) {  //Units Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Dose Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Dose Button pressed");

                SettingsPage = 9;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Time Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Time Button pressed");

                SettingsPage = 10;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 9) {  //Dose Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Sivert Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Sivert Button pressed");

                DoseUnit = 0;
                MaxDoseRate = 0;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Rem Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Rem Button pressed");

                DoseUnit = 1;
                MaxDoseRate = 0;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 8;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 10) {  //Time Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //24H Format
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("24H Format Button pressed");

                TimeFormat = 0;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //12H Format
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("12H Format Button pressed");

                TimeFormat = 1;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 8;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 11) {

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Bluetooth Info Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Bluetooth Info Button pressed");

                SettingsPage = 13;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Bluetooth STAT Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Bluetooth STAT Button pressed");

                if (BLEStat == 0) {
                  StartBLE();
                  Serial.println("BLE ON");
                } else {
                  StopBLE();
                  Serial.println("BLE OFF");
                }

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 12) {

///////////////////////////////////////////////////////////FREI////////////////////////////////////////////////////////////////////////

          } else if (SettingsPage == 13) {  //BT Info Page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 11;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 14) {  //Micro SD settings page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Micro SD Info Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Micro SD Info Button pressed");

                // SDAvailable();

                SettingsPage = 15;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //System Log Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("System Log Button pressed");

                /*
                  if (SDSystemLog == true) {
                  SDSystemLog = false;
                  } else {
                  SDSystemLog = true;
                  }

                */

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Micro SD Tools Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Micro SD Tools Button pressed");

                SettingsPage = 16;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 15) {  //Micro SD Info Page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 14;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 16) {      //Micro SD Tools Page
            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 14;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }
          } else if (SettingsPage == 17) {

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Dose alert Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Dose alert Button pressed");

                SettingsPage = 18;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Total dose alert Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Total dose alert Button pressed");

                SettingsPage = 19;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 18) {  //Dose alert Page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 17;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (DoseAlertRate >= 90) {
                  DoseAlertRate = 90;
                } else {
                  DoseAlertRate = DoseAlertRate + 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (DoseAlertRate <= 10) {
                  DoseAlertRate = 10;
                } else {
                  DoseAlertRate = DoseAlertRate - 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 19) {  //total dose alert page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 17;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (TotalDoseAlertRate >= 90) {
                  TotalDoseAlertRate = 90;
                } else {
                  TotalDoseAlertRate = TotalDoseAlertRate + 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (TotalDoseAlertRate <= 30) {
                  TotalDoseAlertRate = 30;
                } else {
                  TotalDoseAlertRate = TotalDoseAlertRate - 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 20) {  //System Page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //System data Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("System data Button pressed");

                SettingsPage = 21;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Update Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Update Button pressed");

                /*
                  BTStat = 0;
                  BTOff();
                  WiFiSetup();

                */

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Factory settings Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Factory settings Button pressed");

                SettingsPage = 22;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 21) {  //System data page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 20;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 22) {  //Factory settings page

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Agree Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Agree Button pressed");

                ResetEEPROM();
                ESP.restart();
                delay(200);

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Decline Button
              if ((TY > 214) && (TY <= (214 + 44))) {

                Serial.println("Decline Button pressed");

                ClearPage = true;
                SettingsPage = 20;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 23) {  //Wifi Settings

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Wi-Fi info Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Wi-Fi info Button pressed");

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Wi-Fi STAT Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Wi-Fi STAT Button pressed");

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Wi-Fi setup Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Wi-Fi setup Button pressed");

                //BTStat = 0;
                //WiFiStat = true;
                //BTOff();
                //WiFiSetup();
                //WiFiStat = false;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Wi-Fi log Button
              if ((TY > 214) && (TY <= (214 + 44))) {

                Serial.println("Wi-Fi log Button pressed");

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 24) {  //Custom GMT settings

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Voltage Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Voltage Button pressed");

                SettingsPage = 25;
                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Zero effect Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Zero effect Button pressed");

                SettingsPage = 26;
                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Auto calibrate Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Auto calibrate Button pressed");


                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 7;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          } else if (SettingsPage == 25) {

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 24;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (CustomHVLevel >= 4) {
                  CustomHVLevel = 4;
                } else {
                  CustomHVLevel++;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (CustomHVLevel <= 0) {
                  CustomHVLevel = 0;
                } else {
                  CustomHVLevel--;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }
          } else if (SettingsPage == 26) {

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                ClearPage = true;
                SettingsPage = 24;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (CustomZeroEffect >= 255) {
                  CustomZeroEffect = 255;
                } else {
                  CustomZeroEffect = CustomZeroEffect + 1;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (CustomZeroEffect <= 1) {
                  CustomZeroEffect = 1;
                } else {
                  CustomZeroEffect = CustomZeroEffect - 1;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }
          }  else if (SettingsPage == 27) {  //Accent color

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Red Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Red Button pressed");

                AccentColor = RED;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Green Button
              if ((TY > 114) && (TY <= (114 + 44))) {

                Serial.println("Green Button pressed");

                AccentColor = DKGREEN;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80)) && (!RestrictedModeStat)) {  //Blue Button
              if ((TY > 164) && (TY <= (164 + 44))) {

                Serial.println("Blue Button pressed");

                AccentColor = BLUE;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Yellow Settings Button
              if ((TY > 214) && (TY <= (214 + 44))) {

                Serial.println("Yellow Button pressed");

                AccentColor = DKYELLOW;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Digital blue Button
              if ((TY > 214 + 50) && (TY <= (214 + 44 + 50))) {

                Serial.println("Digital blue Button pressed");

                AccentColor = DIGITAL_BLUE;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Orange Button
              if ((TY > 214 + 100) && (TY <= (214 + 44 + 100))) {

                Serial.println("Orange Button pressed");

                AccentColor = ORANGE;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                EepromSave();

                ClearPage = true;
                SettingsPage = 2;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

          }

        } else if (PageNumber == 3) {  //Touch Cal. Approval

          if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Agree Button
            if ((TY > 164) && (TY <= (164 + 44))) {

              Serial.println("Agree Button pressed");

              TFTCalibrate();

              PageNumber = 2;
              SettingsPage = 0;

              ClearPage = true;
              TouchDetected = true;
              ForceUpdateDisplay = true;
            }
          }

          if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Decline Button
            if ((TY > 214) && (TY <= (214 + 44))) {

              Serial.println("Decline Button pressed");

              PageNumber = 2;
              SettingsPage = 0;

              ClearPage = true;
              TouchDetected = true;
              ForceUpdateDisplay = true;
            }
          }



        } else if (PageNumber == 4) {  //Average Speed Settings Page

          if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Normal Button
            if ((TY > 64) && (TY <= (64 + 44))) {

              Serial.println("Normal Button pressed");

              AverageSpeed = 0;

              ClearPage = true;
              TouchDetected = true;
              ForceUpdateDisplay = true;
            }
          }

          if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Fast Button
            if ((TY > 114) && (TY <= (114 + 44))) {

              Serial.println("Fast Button pressed");

              AverageSpeed = 1;

              ClearPage = true;
              TouchDetected = true;
              ForceUpdateDisplay = true;
            }
          }

          if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Slow Button
            if ((TY > 164) && (TY <= (164 + 44))) {

              Serial.println("Slow Button pressed");

              AverageSpeed = 2;

              ClearPage = true;
              TouchDetected = true;
              ForceUpdateDisplay = true;
            }
          }

          if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
            if ((TY > 429) && (TY <= (429 + 48))) {

              Serial.println("Back Button pressed");

              PageNumber = 1;
              ClearPage = true;
              TouchDetected = true;
              QuickMenuPage = 0;
              ForceUpdateDisplay = true;
            }
          }

        } else if (PageNumber == 5) {  //Timed count Settings Page
          if (TimedCountPage == 0) {

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                PageNumber = 1;
                ClearPage = true;
                QuickMenuPage = 0;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Start Button
              if ((TY > 64) && (TY <= (64 + 44))) {

                Serial.println("Start Button pressed");

                TimingCountSDLog();

                TimedCountPage = 2;
                TimedCountStartTime = millis();
                TimedCountCurrentCount = 0;
                TimedCountCPM = 0;

                TimedCountStartHour = Hour;
                TimedCountStartMinute = Minute;
                TimedCountStartSecond = Second;

                ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if (TYPE != 1) {

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //SD Logging Button
                if ((TY > 114) && (TY <= (114 + 44))) {

                  Serial.println("SD Logging Button pressed");

                  if (TimedCountSDLogging == true) {
                    TimedCountSDLogging = false;
                  } else {
                    TimedCountSDLogging = true;
                  }

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Time Button
                if ((TY > 164) && (TY <= (164 + 44))) {

                  Serial.println("Time Button pressed");

                  TimedCountPage = 1;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            } else {

              if ((TX > 3) && (TX < (3 + 234 + 80))) {  //Time Button
                if ((TY > 114) && (TY <= (114 + 44))) {

                  Serial.println("Time Button pressed");

                  TimedCountPage = 1;

                  ClearPage = true;
                  TouchDetected = true;
                  ForceUpdateDisplay = true;
                }
              }

            }


          } else if (TimedCountPage == 1) {  //Time settings page

            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                TimedCountPage = 0;
                ClearPage = true;
                ForceUpdateDisplay = true;
                TouchDetected = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Plus Button
              if ((TY > 70) && (TY <= (70 + 60))) {

                Serial.println("Plus Button pressed");

                if (TimedCountDuration == 995) {
                  TimedCountDuration = 995;
                } else if (TimedCountDuration == 1) {
                  TimedCountDuration = 5;
                } else {
                  TimedCountDuration = TimedCountDuration + 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }

            if ((TX > 160) && (TX < (160 + 60))) {  //Minus Button
              if ((TY > 185) && (TY <= (185 + 60))) {

                Serial.println("Minus Button pressed");

                if (TimedCountDuration == 5) {
                  TimedCountDuration = 1;
                } else if (TimedCountDuration == 1) {
                  TimedCountDuration = 1;
                } else {
                  TimedCountDuration = TimedCountDuration - 5;
                }

                tft.fillRect(160, 132, 60, 50, BLACK);

                //ClearPage = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }


          } else if (TimedCountPage == 2) {
            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                TimedCountPage = 0;

                ClearPage = true;
                ForceUpdateDisplay = true;
                TouchDetected = true;
              }
            }

            if ((TX > 112) && (TX < (112 + 96))) {  //Stop Button Pos. 1
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Stop Button pressed");

                TimedCountPage = 3;
                TimedCountEndHour = Hour;
                TimedCountEndMinute = Minute;
                TimedCountEndSecond = Second;

                TimedCountTmpMillis = millis();

                ClearPage = true;
                ForceUpdateDisplay = true;
                TouchDetected = true;

                TimingCountSDLog();
              }
            }


          } else if (TimedCountPage == 3) {
            if ((TX > 4) && (TX < (4 + 48))) {  //Back Button Pos. 0
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Back Button pressed");

                TimedCountPage = 0;

                ClearPage = true;
                ForceUpdateDisplay = true;
                TouchDetected = true;
              }
            }

            if ((TX > 136) && (TX < (136 + 48))) {  //Home Button Middle Pos. 1
              if ((TY > 429) && (TY <= (429 + 48))) {

                Serial.println("Home Button pressed");


                TimedCountPage = 0;

                ClearPage = true;
                PageNumber = 0;
                GraphUpdate = true;
                graphx = 0;
                MeterUpdate = true;
                TouchDetected = true;
                ForceUpdateDisplay = true;
              }
            }
          }
        }
      }
    } else {
      if (TouchDetected == true) {
        TouchDetected = false;
      }
    }
  } else {  //Keylock == true
    if (tft.getTouch(&TX, &TY)) {
      PreviousMillis4 = millis();
      if (TouchDetected == false) {
        TouchX = TX;
        TouchY = TY;

        if ((TX > 68) && (TX < (68 + 55))) {  //Keylock Button
          if ((TY > 420) && (TY <= (420 + 55))) {

            Serial.println("Keylock Button pressed");
            ClearPage = true;
            PageNumber = 0;
            GraphUpdate = true;
            graphx = 0;
            MeterUpdate = true;
            Keylock = false;
            TouchDetected = true;
            ForceUpdateDisplay = true;
          }
        }
      }
    } else {
      if (TouchDetected == true) {
        TouchDetected = false;
      }
    }
  }
}

void TouchConditionsMainPage() {
  if ((TouchX > 192) && (TouchX < (192 + 55))) {  //QuickMenu Button
    if ((TouchY > 420) && (TouchY <= (420 + 55))) {

      Serial.println("QuickMenu Button pressed");
      PageNumber = 1;
      QuickMenuPage = 0;
      ClearPage = true;
      TouchDetected = true;
      ForceUpdateDisplay = true;
    }
  }

  if ((TouchX > 68) && (TouchX < (68 + 55))) {  //Keylock Button
    if ((TouchY > 420) && (TouchY <= (420 + 55))) {

      Serial.println("Keylock Button pressed");
      ClearPage = true;
      PageNumber = 0;
      GraphUpdate = true;
      graphx = 0;
      MeterUpdate = true;
      Keylock = true;
      TouchDetected = true;
      ForceUpdateDisplay = true;
    }
  }

  if ((TouchX > 6) && (TouchX < (6 + 55))) {  //Settings Button
    if ((TouchY > 420) && (TouchY <= (420 + 55))) {

      Serial.println("Settings Button pressed");
      ClearPage = true;
      PageNumber = 2;
      SettingsPage = 0;
      TouchDetected = true;
      ForceUpdateDisplay = true;
    }
  }

  if ((TouchX > 130) && (TouchX < (130 + 55))) {  //Switch Button
    if ((TouchY > 420) && (TouchY <= (420 + 55))) {

      Serial.println("Switch Button pressed");
      ClearPage = true;
      PageNumber = 0;
      GraphUpdate = true;
      graphx = 0;
      MeterUpdate = true;
      MainPageNumber = MainPageNumber + 1;

      if (MainPageNumber > 5) {
        MainPageNumber = 0;
      }

      Serial.print("MainPageNumber: ");
      Serial.println(MainPageNumber);
      TouchDetected = true;
      ForceUpdateDisplay = true;
    }
  }

  if ((TouchX > 255) && (TouchX < (255 + 55))) {  //Tone Button
    if ((TouchY > 420) && (TouchY <= (420 + 55))) {

      Serial.println("Switch Tone Mode Button pressed");

      if (BuzzerMode == 1) {
        BuzzerMode = 0;
      } else {
        BuzzerMode = 1;
      }

      ClearPage = true;
      TouchDetected = true;
      ForceUpdateDisplay = true;
      GraphUpdate = true;
      graphx = 0;
      MeterUpdate = true;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void DosimeterMode() {
  //tft.invertDisplay(1); //Display invertieren

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  tft.fillRect(160, 100, 140, 300, BLACK);

  if (DoseUnit == 0) {
    if (EffectiveDose(15) < (0.25 * TotalDoseAlertRate)) {  //Head
      tft.drawBitmap(160, 100, HumanHead, 140, 300, GREEN);
    } else if ((EffectiveDose(15) >= (0.25 * TotalDoseAlertRate)) && (EffectiveDose(15) < (0.5 * TotalDoseAlertRate))) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, YELLOW);
    } else if ((EffectiveDose(15) >= (0.5 * TotalDoseAlertRate)) && (EffectiveDose(15) < TotalDoseAlertRate)) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, ORANGE);
    } else if (EffectiveDose(15) >= TotalDoseAlertRate) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, RED);
    }

    tft.setTextColor(WHITE);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    if (EffectiveDose(15) < 10) {
      tft.setCursor(190, 142);
      tft.print(EffectiveDose(15));
      tft.println("uSv");
    } else if ((EffectiveDose(15) >= 10) && (EffectiveDose(15) < 100)) {
      tft.setCursor(180, 142);
      tft.print(EffectiveDose(15));
      tft.println("uSv");
    } else if ((EffectiveDose(15) >= 100) && (EffectiveDose(15) < 1000)) {
      tft.setCursor(170, 142);
      tft.print(EffectiveDose(15));
      tft.println("uSv");
    } else if ((EffectiveDose(15) >= 1000) && (EffectiveDose(15) < 10000)) {
      tft.setCursor(180, 142);
      tft.print(EffectiveDose(15) / 1000);
      tft.println("mSv");
    }

    if (EffectiveDose(16) < (0.25 * TotalDoseAlertRate)) {  //Head
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, GREEN);
    } else if ((EffectiveDose(16) >= (0.25 * TotalDoseAlertRate)) && (EffectiveDose(16) < (0.5 * TotalDoseAlertRate))) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, YELLOW);
    } else if ((EffectiveDose(16) >= (0.5 * TotalDoseAlertRate)) && (EffectiveDose(16) < TotalDoseAlertRate)) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, ORANGE);
    } else if (EffectiveDose(16) >= TotalDoseAlertRate) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, RED);
    }

    tft.setTextColor(WHITE);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    if (EffectiveDose(16) < 10) {
      tft.setCursor(190, 210);
      tft.print(EffectiveDose(16));
      tft.println("uSv");
    } else if ((EffectiveDose(16) >= 10) && (EffectiveDose(16) < 100)) {
      tft.setCursor(180, 210);
      tft.print(EffectiveDose(16));
      tft.println("uSv");
    } else if ((EffectiveDose(16) >= 100) && (EffectiveDose(16) < 1000)) {
      tft.setCursor(170, 210);
      tft.print(EffectiveDose(16));
      tft.println("uSv");
    } else if ((EffectiveDose(16) >= 1000) && (EffectiveDose(16) < 10000)) {
      tft.setCursor(180, 210);
      tft.print(EffectiveDose(16) / 1000);
      tft.println("mSv");
    }

    if (EffectiveDose(17) < (0.25 * TotalDoseAlertRate)) {  //Head
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, GREEN);
    } else if ((EffectiveDose(17) >= (0.25 * TotalDoseAlertRate)) && (EffectiveDose(17) < (0.5 * TotalDoseAlertRate))) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, YELLOW);
    } else if ((EffectiveDose(17) >= (0.5 * TotalDoseAlertRate)) && (EffectiveDose(17) < TotalDoseAlertRate)) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, ORANGE);
    } else if (EffectiveDose(17) >= TotalDoseAlertRate) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, RED);
    }

    tft.setTextColor(WHITE);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    if (EffectiveDose(17) < 10) {
      tft.setCursor(190, 320);
      tft.print(EffectiveDose(17));
      tft.println("uSv");
    } else if ((EffectiveDose(17) >= 10) && (EffectiveDose(17) < 100)) {
      tft.setCursor(180, 320);
      tft.print(EffectiveDose(17));
      tft.println("uSv");
    } else if ((EffectiveDose(17) >= 100) && (EffectiveDose(17) < 1000)) {
      tft.setCursor(170, 320);
      tft.print(EffectiveDose(17));
      tft.println("uSv");
    } else if ((EffectiveDose(17) >= 1000) && (EffectiveDose(17) < 10000)) {
      tft.setCursor(180, 320);
      tft.print(EffectiveDose(17) / 1000);
      tft.println("mSv");
    }

  } else {

    if (EffectiveDose(15) < (0.25 * (TotalDoseAlertRate * 0.1))) {  //Head
      tft.drawBitmap(160, 100, HumanHead, 140, 300, GREEN);
    } else if ((EffectiveDose(15) >= (0.25 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(15) < (0.5 * (TotalDoseAlertRate * 0.1)))) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, YELLOW);
    } else if ((EffectiveDose(15) >= (0.5 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(15) < (TotalDoseAlertRate * 0.1))) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, ORANGE);
    } else if (EffectiveDose(15) >= (TotalDoseAlertRate * 0.1)) {
      tft.drawBitmap(160, 100, HumanHead, 140, 300, RED);
    }

    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setCursor(150, 40);
    if (EffectiveDose(15) >= 100.00) {
      tft.print(EffectiveDose(15) / 1000);
      tft.println("Rem");
    } else {
      tft.print(EffectiveDose(15));
      tft.println("mRem");
    }

    if (EffectiveDose(16) < (0.25 * (TotalDoseAlertRate * 0.1))) {  //Head
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, GREEN);
    } else if ((EffectiveDose(16) >= (0.25 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(16) < (0.5 * (TotalDoseAlertRate * 0.1)))) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, YELLOW);
    } else if ((EffectiveDose(16) >= (0.5 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(16) < (TotalDoseAlertRate * 0.1))) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, ORANGE);
    } else if (EffectiveDose(16) >= (TotalDoseAlertRate * 0.1)) {
      tft.drawBitmap(160, 100, HumanUpperBody, 140, 300, RED);
    }

    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setCursor(200, 40);
    if (EffectiveDose(16) >= 100.00) {
      tft.print(EffectiveDose(16) / 1000);
      tft.println("Rem");
    } else {
      tft.print(EffectiveDose(16));
      tft.println("mRem");
    }

    if (EffectiveDose(17) < (0.25 * (TotalDoseAlertRate * 0.1))) {  //Head
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, GREEN);
    } else if ((EffectiveDose(17) >= (0.25 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(17) < (0.5 * (TotalDoseAlertRate * 0.1)))) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, YELLOW);
    } else if ((EffectiveDose(17) >= (0.5 * (TotalDoseAlertRate * 0.1))) && (EffectiveDose(17) < (TotalDoseAlertRate * 0.1))) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, ORANGE);
    } else if (EffectiveDose(17) >= (TotalDoseAlertRate * 0.1)) {
      tft.drawBitmap(160, 100, HumanLowerBody, 140, 300, RED);
    }

    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setCursor(250, 40);
    if (EffectiveDose(17) >= 100.00) {
      tft.print(EffectiveDose(17) / 1000);
      tft.println("Rem");
    } else {
      tft.print(EffectiveDose(17));
      tft.println("mRem");
    }
  }


  /*

    //Dose with correct scale
    if (DoseUnit == 0) { //Sv
    if (DoseUnitSivert == 0) { //uSv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b); //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) { //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(25, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }
      tft.setCursor(178, 195);
      tft.println("uSv/h");
    } else if (DoseUnitSivert == 1) { //mSv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b); //9,12,18,24pt

      if (DoseLevel == 0) { //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(16, 195);

      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }

      tft.setCursor(172, 195);
      tft.println("mSv/h");

    } else if (DoseUnitSivert == 2) { //Sv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b); //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) { //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(21, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 4);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 3);
      } else {
        tft.println(DoseRate, 5);
      }
      tft.setCursor(205, 195);
      tft.println("Sv/h");
    }

    } else if (DoseUnit == 1) { //Rem

    if (DoseUnitRem == 0) { //mRem/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b); //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) { //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(10, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 2);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 1);
      } else {
        tft.println(DoseRate, 3);
      }
      tft.setCursor(135, 195);
      tft.println("mRem/h");

    } else if (DoseUnitRem == 1) { //Rem/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b); //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) { //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(16, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }
      tft.setCursor(168, 195);
      tft.println("Rem/h");

    }

    }

    //Dose information
    if (DoseLevel == 0) { //Radiation
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) { //German
      tft.setCursor(16, 240);
      tft.println("Niedrige Strahlung");
    } else if (Language == 1) { //English
      tft.setCursor(18, 240);
      tft.println("    Low radiation");
    }
    } else if (DoseLevel == 1) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) { //German
      tft.setCursor(13, 240);
      tft.println("Erhoehte Strahlung");
    } else if (Language == 1) { //English
      tft.setCursor(21, 240);
      tft.println("Elevated radiation");
    }
    } else if ((DoseLevel == 2) || (DoseLevel == 3)) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) { //German
      tft.setCursor(40, 240);
      tft.println("Hohe Strahlung");
    } else if (Language == 1) { //English
      tft.setCursor(44, 240);
      tft.println(" High radiation");
    }
    } else if (DoseLevel == 4) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) { //German
      tft.setCursor(6, 240);
      tft.println("Toedliche Strahlung");
    } else if (Language == 1) { //English
      tft.setCursor(4, 240);
      tft.println("Dangerous radiation");
    }
    }



    tft.drawLine(10, 258, 310, 258, WHITE);
    tft.drawLine(9, 259, 311, 259, WHITE);
    tft.drawLine(10, 260, 310, 260, WHITE);



    //Additional Data


    tft.fillRect(0, 262, 320, 147, BLACK);


    tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(15, 292);
    tft.print("CPM: ");
    tft.println(CPM);

    tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) { //German
    tft.setCursor(15, 327);
    tft.print("Max. Dosis: ");
    } else if (Language == 1) { //English
    tft.setCursor(15, 327);
    tft.print("Max. dose: ");
    }

    if (MaxDoseUnit == 0) {
    tft.print(MaxDoseRate);
    tft.println("uSv/h");
    } else if (MaxDoseUnit == 1) {
    tft.print(MaxDoseRate);
    tft.println("mSv/h");
    } else if (MaxDoseUnit == 2) {
    tft.print(MaxDoseRate);
    tft.println("Sv/h");
    } else if (MaxDoseUnit == 3) {
    tft.print(MaxDoseRate);
    tft.println("mRem/h");
    } else if (MaxDoseUnit == 4) {
    tft.print(MaxDoseRate);
    tft.println("Rem/h");
    }

    tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
    tft.setTextSize(1);

    if (TotalDoseLevel == 0) { //TotalDoseLevel
    tft.setTextColor(GREEN);
    } else if (TotalDoseLevel == 1) {
    tft.setTextColor(YELLOW);
    } else if (TotalDoseLevel == 2) {
    tft.setTextColor(ORANGE);
    } else if (TotalDoseLevel == 3) {
    tft.setTextColor(RED);
    }

    if (Language == 0) { //German
    tft.setCursor(15, 361);
    tft.print("Ges. Dosis: ");
    } else if (Language == 1) { //English
    tft.setCursor(15, 361);
    tft.print("Full dose: ");
    }

    if (DoseUnit == 0) {
    if (TotalDose >= 1000.00) {
      tft.print(TotalDose / 1000, 5);
      tft.println("mSv");
    } else {
      tft.print(TotalDose, 5);
      tft.println("uSv");
    }
    } else if (DoseUnit == 1) {
    if (TotalDose >= 100.00) {
      tft.print(TotalDose / 1000, 4);
      tft.println("Rem");
    } else {
      tft.print(TotalDose, 4);
      tft.println("mRem");
    }
    }

    tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) { //German
    tft.setCursor(15, 395);
    tft.print("Zaehlungen: ");
    } else if (Language == 1) { //English
    tft.setCursor(15, 395);
    tft.print("Counts: ");
    }

    if(SelectIntExtTube){ //Intern
    tft.print(InternCurrentCount);
    }else{
    tft.print(ExternCurrentCount);
    }

  */


  DrawMainButtons();
}

void RotateDisplay() {

  if (tft.getRotation() == 0) {
    tft.setRotation(2);
    calData[4] = 2;
  } else {
    tft.setRotation(0);
    calData[4] = 4;
  }

  tft.setTouch(calData);
}

void FirstBootPage() {
  tft.fillRect(0, 0, 320, 480, BLACK);

  tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(8, 65);
  tft.println("GeigerOS");

  tft.drawBitmap(220, 0, radiationSymbolLarge, 100, 100, YELLOW);  //OS LOGO

  tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setCursor(7, 115);
  tft.setTextColor(WHITE);
  tft.print("V");
  tft.setTextColor(YELLOW);
  tft.print(GEIGEROS_VERSION);
  tft.setTextColor(WHITE);
  tft.println("     Developed by Marvin Ramm");

  tft.setCursor(0, 170);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.println("Sehr geehrter Nutzer,");
    tft.setCursor(0, 170 + 40);
    tft.println("vielen Dank, dass Sie unser Messsystem verwenden. Sie koennen die Anleitung zum System durch Scannen des beigefuegten QR-Codes auf Ihrem Smartphone aufrufen. Bei Fragen stehen wir Ihnen gerne zur Verfuegung.");
  } else if (Language == 1) {  //English
    tft.println("Dear User,");
    tft.setCursor(0, 170 + 40);
    tft.println("thank you for choosing our measurement system. You can access the system manual by scanning the attached QR code with your smartphone. If you have any questions, please feel free to contact us.");
  }

  DrawButton(2, 1);

  uint16_t TX, TY;  //Touch coordinates

  bool TmpTouched = false;

  while (TmpTouched == false) {
    tft.getTouch(&TX, &TY);

    if ((TX > 136) && (TX < (136 + 48))) {  //Home Button Middle Pos. 1
      if ((TY > 429) && (TY <= (429 + 48))) {

        Serial.println("Home Button pressed");
        TmpTouched = true;
      }
    }
  }

}

void DrawBootPage() {
  tft.fillRect(0, 0, 320, 480, BLACK);

  tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(55, 242);
  tft.println("GeigerOS");

  tft.drawBitmap(109, 100, radiationSymbolLarge, 100, 100, YELLOW);  //OS LOGO

  if (BITMAP_MODE == 1) {
    tft.drawBitmap(40, 270, CustomLogoSmall1, 240, 50, WHITE);
  } else if (BITMAP_MODE == 2) {
    tft.drawBitmap(40, 270, CustomLogoSmall1, 240, 50, WHITE);
    tft.drawBitmap(40, 340, CustomLogoSmall2, 240, 50, WHITE);
  }

  tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setCursor(7, 470);
  tft.setTextColor(WHITE);
  tft.print("V");
  tft.setTextColor(YELLOW);
  tft.print(GEIGEROS_VERSION);
  tft.setTextColor(WHITE);
  tft.println("     Developed by Marvin Ramm");

  for (int i = 1; i <= 300; i++) {
    SetDisplayBrightness(i);
    tft.drawLine(10, 420, 10 + i, 420, YELLOW);
    tft.drawLine(9, 421, 11 + i, 421, YELLOW);
    tft.drawLine(9, 422, 11 + i, 422, YELLOW);
    tft.drawLine(10, 423, 10 + i, 423, YELLOW);
    delay(5);
  }

  SetDisplayBrightness(255);
  tft.fillRect(0, 0, 320, 480, BLACK);
}

void DrawShutdownPage() {
  tft.fillRect(0, 0, 320, 480, BLACK);

  if (IsCharging() == true) {

    tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(YELLOW);
    tft.setCursor(55, 162);
    tft.println("GeigerOS");

    tft.drawBitmap(109, 20, radiationSymbolLarge, 100, 100, YELLOW);  //OS LOGO


    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(30, 280);
      tft.println("Trennen Sie das");
      tft.setCursor(53, 320);
      tft.println("  Ladekabel!");
    } else if (Language == 1) {  //English
      tft.setCursor(32, 280);
      tft.println(" Disconnect the");
      tft.setCursor(26, 320);
      tft.println(" Charging cable!");
    }

    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setCursor(7, 470);
    tft.setTextColor(WHITE);
    tft.print("V");
    tft.setTextColor(YELLOW);
    tft.print(GEIGEROS_VERSION);
    tft.setTextColor(WHITE);
    tft.println("     Developed by Marvin Ramm");

    EepromSave();

    for (int i = 1; i <= 300; i++) {
      tft.drawLine(10, 420, 10 + i, 420, YELLOW);
      tft.drawLine(9, 421, 11 + i, 421, YELLOW);
      tft.drawLine(9, 422, 11 + i, 422, YELLOW);
      tft.drawLine(10, 423, 10 + i, 423, YELLOW);
      delay(4);
    }

    digitalWrite(P_ON, LOW);  //Power button is set to LOW
    ClearPage = true;
  } else {

    tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(YELLOW);
    tft.setCursor(55, 242);
    tft.println("GeigerOS");

    tft.drawBitmap(109, 100, radiationSymbolLarge, 100, 100, YELLOW);  //OS LOGO

    if ((BATPercent <= 1) && (IsCharging() == false)){
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(85 , 320);
      tft.print("Akku leer");
    } else if (Language == 1) {  //English
      tft.setCursor(52, 320);
      tft.print("Battery empty");
    }
    }

    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setCursor(7, 470);
    tft.setTextColor(WHITE);
    tft.print("V ");
    tft.setTextColor(YELLOW);
    tft.print(GEIGEROS_VERSION);
    tft.setTextColor(WHITE);
    tft.println("     Developed by Marvin Ramm");

    EepromSave();

    int a = 300;
    for (int i = 1; i <= 300; i++) {
      SetDisplayBrightness(a);
      tft.drawLine(10, 420, 10 + i, 420, YELLOW);
      tft.drawLine(9, 421, 11 + i, 421, YELLOW);
      tft.drawLine(9, 422, 11 + i, 422, YELLOW);
      tft.drawLine(10, 423, 10 + i, 423, YELLOW);
      delay(7);
      a--;
    }

    digitalWrite(P_ON, LOW);  //Power button is set to LOW
    ClearPage = true;
    SetDisplayBrightness(255);
  }
}

void DrawInfoBar() {

  tft.fillRect(0, 0, 110, 22, BLACK);

  if (IsCharging() == 1) {
    tft.drawRoundRect(290, 4, 26, 14, 3, WHITE);
    tft.drawLine(289, 8, 289, 13, WHITE);  //Battery symbol
    tft.drawLine(288, 8, 288, 13, WHITE);
    tft.fillRect(292, 6, 22, 10, BLACK);

    tft.drawBitmap(295, 3, chargeSymbol, 15, 15, YELLOW);
  } else if ((BATPercent > 15) && (BATPercent < 35)) {  //Battery status
    tft.drawRoundRect(290, 4, 26, 14, 3, WHITE);
    tft.drawLine(289, 8, 289, 13, WHITE);  //Battery symbol
    tft.drawLine(288, 8, 288, 13, WHITE);
    tft.fillRect(292, 6, 22, 10, BLACK);

    tft.fillRect(BATPixelMaped, 6, (314 - BATPixelMaped), 10, YELLOW);

  } else if (BATPercent < 15) {
    tft.drawRoundRect(290, 4, 26, 14, 3, RED);
    tft.drawLine(289, 8, 289, 13, RED);  //Battery symbol
    tft.drawLine(288, 8, 288, 13, RED);
    tft.fillRect(292, 6, 22, 10, BLACK);

    tft.fillRect(BATPixelMaped, 6, (314 - BATPixelMaped), 10, RED);
  } else {
    tft.drawRoundRect(290, 4, 26, 14, 3, WHITE);
    tft.drawLine(289, 8, 289, 13, WHITE);  //Battery symbol
    tft.drawLine(288, 8, 288, 13, WHITE);
    tft.fillRect(292, 6, 22, 10, BLACK);

    tft.fillRect(BATPixelMaped, 6, (314 - BATPixelMaped), 10, GREEN);
  }

  if (TYPE != 1) {

    if (TimeFormat == 0) {  //24H Format

      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      tft.setCursor(2, 16);
      if (Hour < 10) {
        tft.print("0");
        tft.print(Hour);
      } else {
        tft.print(Hour);
      }

      tft.print(":");
      if (Minute < 10) {
        tft.print("0");
        tft.print(Minute);
      } else {
        tft.print(Minute);
      }

      tft.print(":");
      if (Second < 10) {
        tft.print("0");
        tft.print(Second);
      } else {
        tft.print(Second);
      }

      tft.setTextColor(WHITE);
      tft.drawLine(88, 1, 88, 19, WHITE);  //Dividing line

    } else if (TimeFormat == 1) {  //12H Format

      tft.setFreeFont(&FreeSans9pt7b);
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      tft.setCursor(2, 16);

      int TmpH = 0;
      bool AMPM = 0;  //AM - 0, PM - 1

      if (Hour == 0) {
        TmpH = 12;
        AMPM = 0;
      } else if ((Hour >= 1) && (Hour <= 11)) {
        TmpH = Hour;
        AMPM = 0;
      } else if ((Hour >= 12) && (Hour <= 23)) {
        AMPM = 1;
        if (Hour == 12) {
          TmpH = Hour;
        } else if (Hour == 13) {
          TmpH = 1;
        } else if (Hour == 14) {
          TmpH = 2;
        } else if (Hour == 15) {
          TmpH = 3;
        } else if (Hour == 16) {
          TmpH = 4;
        } else if (Hour == 17) {
          TmpH = 5;
        } else if (Hour == 18) {
          TmpH = 6;
        } else if (Hour == 19) {
          TmpH = 7;
        } else if (Hour == 20) {
          TmpH = 8;
        } else if (Hour == 21) {
          TmpH = 9;
        } else if (Hour == 22) {
          TmpH = 10;
        } else if (Hour == 23) {
          TmpH = 11;
        }
      }

      if (TmpH < 10) {
        tft.print("0");
        tft.print(TmpH);
      } else {
        tft.print(TmpH);
      }

      tft.print(":");

      if (Minute < 10) {
        tft.print("0");
        tft.print(Minute);
      } else {
        tft.print(Minute);
      }

      tft.print(":");
      if (Second < 10) {
        tft.print("0");
        tft.print(Second);
      } else {
        tft.print(Second);
      }

      if (AMPM == 0) {
        tft.print("AM");
      } else {
        tft.print("PM");
      }
    }

  } else {
    tft.setFreeFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(2, 16);

    tft.print("GMZ-0");
    tft.print(GMZ_MODEL_NR);
    tft.print(" Lite");
  }

  if (TYPE != 1) {
    if (TotalDoseLevel == 0) {  //Total Radiation
      tft.drawBitmap(103, 2, MedicalHeartBitmap, 18, 18, GREEN);
    } else if (TotalDoseLevel == 1) {
      tft.drawBitmap(103, 2, MedicalHeartBitmap, 18, 18, YELLOW);
    } else if (TotalDoseLevel == 2) {
      tft.drawBitmap(103, 2, MedicalHeartBitmap, 18, 18, ORANGE);
    } else if (TotalDoseLevel == 3) {
      tft.drawBitmap(103, 2, MedicalHeartBitmap, 18, 18, RED);
    }

    if (DoseLevel == 0) {  //Radiation
      tft.drawBitmap(126, 1, radiationSymbol, 19, 19, GREEN);
    } else if (DoseLevel == 1) {
      tft.drawBitmap(126, 1, radiationSymbol, 19, 19, YELLOW);
    } else if (DoseLevel == 2) {
      tft.drawBitmap(126, 1, radiationSymbol, 19, 19, ORANGE);
    } else if (DoseLevel == 3) {
      tft.drawBitmap(126, 1, radiationSymbol, 19, 19, RED);
    } else if (DoseLevel == 4) {
      tft.drawBitmap(126, 1, radiationSymbol, 19, 19, RED);
    }

    //if (WiFiStat == 1) { //WIFI
    tft.drawBitmap(150, 1, wifiBitmap, 19, 19, WHITE);
    //} else {
    //  tft.fillRect(150, 1, 19, 19, BLACK);
    //}

    if (BLEStat == 1) { //Bluetooth
    tft.fillRect(171, 1, 19, 19, BLACK);
    tft.drawBitmap(171, 1, bluetoothBitmap, 19, 19, WHITE);  //Bluetooth enabled
    } else if (BLEStat == 2) {
      tft.drawBitmap(171, 1, bluetoothConnectedBitmap, 19, 19, WHITE); //Bluetooth connected
    } else {
      tft.fillRect(171, 1, 19, 19, BLACK);
    }


    if (SDStat == 1) {  //SD
      tft.drawBitmap(188, 1, sdCardBitmap, 19, 19, RED);
    } else if (SDStat == 2) {
      tft.drawBitmap(188, 1, sdCardBitmap, 19, 19, WHITE);
    } else {
      tft.fillRect(188, 1, 19, 19, BLACK);
    }

  } else {

    if (TotalDoseLevel == 0) {  //Total Radiation
      tft.drawBitmap(103 + 12, 2, MedicalHeartBitmap, 18, 18, GREEN);
    } else if (TotalDoseLevel == 1) {
      tft.drawBitmap(103 + 12, 2, MedicalHeartBitmap, 18, 18, YELLOW);
    } else if (TotalDoseLevel == 2) {
      tft.drawBitmap(103 + 12, 2, MedicalHeartBitmap, 18, 18, ORANGE);
    } else if (TotalDoseLevel == 3) {
      tft.drawBitmap(103 + 12, 2, MedicalHeartBitmap, 18, 18, RED);
    }

    if (DoseLevel == 0) {  //Radiation
      tft.drawBitmap(126 + 12, 1, radiationSymbol, 19, 19, GREEN);
    } else if (DoseLevel == 1) {
      tft.drawBitmap(126 + 12, 1, radiationSymbol, 19, 19, YELLOW);
    } else if (DoseLevel == 2) {
      tft.drawBitmap(126 + 12, 1, radiationSymbol, 19, 19, ORANGE);
    } else if (DoseLevel == 3) {
      tft.drawBitmap(126 + 12, 1, radiationSymbol, 19, 19, RED);
    } else if (DoseLevel == 4) {
      tft.drawBitmap(126 + 12, 1, radiationSymbol, 19, 19, RED);
    }

    //if (WiFiStat == 1) { //WIFI
    tft.drawBitmap(150 + 12, 1, wifiBitmap, 19, 19, WHITE);
    //} else {
    //  tft.fillRect(150, 1, 19, 19, BLACK);
    //}

    if (BLEStat == 1) { //Bluetooth
    tft.drawBitmap(171 + 12, 1, bluetoothBitmap, 19, 19, WHITE);  //Bluetooth enabled
    } else if (BLEStat == 2) {
      tft.drawBitmap(171, 1, bluetoothConnectedBitmap, 19, 19, WHITE); //Bluetooth connected
    } else {
      tft.fillRect(171, 1, 19, 19, BLACK);
    }

  }



  tft.fillRect(207, 0, 80, 22, BLACK);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (SelectIntExtTube) {  //Intern
    tft.setCursor(213, 16);
    tft.print("IN:");
  } else {
    tft.setCursor(211, 16);
    tft.print("EX:");
  }

  if ((GeigerTubeModel == 4) && !SelectIntExtTube) {
    if (CustomHVLevel == 0) {
      tft.print("250");
      tft.print("V");
    } else if (CustomHVLevel == 1) {
      tft.print("300");
      tft.print("V");
    } else if (CustomHVLevel == 2) {
      tft.print("350");
      tft.print("V");
    } else if (CustomHVLevel == 3) {
      tft.print("400");
      tft.print("V");
    } else if (CustomHVLevel == 4) {
      tft.print("450");
      tft.print("V");
    }
  } else {
    if (HVLevel == 0) {
      tft.print("250");
      tft.print("V");
    } else if (HVLevel == 1) {
      tft.print("300");
      tft.print("V");
    } else if (HVLevel == 2) {
      tft.print("350");
      tft.print("V");
    } else if (HVLevel == 3) {
      tft.print("400");
      tft.print("V");
    } else if (HVLevel == 4) {
      tft.print("450");
      tft.print("V");
    }
  }
}

void TimedCountSettingsPage(int page) {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  if (page == 0) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(24 + 40, 48);
      tft.println("Zeitliche Zaehlung");
    } else if (Language == 1) {  //English
      tft.setCursor(27 + 40, 48);
      tft.println("    Timing count");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREEN);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(80 + 40, 95);
      tft.println("Starten");
    } else if (Language == 1) {  //English
      tft.setCursor(79 + 40, 95);
      tft.println("  Start");
    }

    if (TYPE != 1) {

      if (TimedCountSDLogging == true) {
        tft.fillRoundRect(3, 114, 314, 44, 4, RED);
        tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(37 + 40, 145);
          tft.println("SD Logging aus");
        } else if (Language == 1) {  //English
          tft.setCursor(39 + 40, 145);
          tft.println(" SD logging off");
        }
      } else {
        tft.fillRoundRect(3, 114, 314, 44, 4, DKGREEN);
        tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(42 + 40, 145);
          tft.println("SD Logging an");
        } else if (Language == 1) {  //English
          tft.setCursor(41 + 40, 145);
          tft.println(" SD logging on");
        }
      }

      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(97 + 40, 195);
        tft.println("Zeit");
      } else if (Language == 1) {  //English
        tft.setCursor(79 + 40, 195);
        tft.println("  Time");
      }

    } else {

      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(97 + 40, 145);
        tft.println("Zeit");
      } else if (Language == 1) {  //English
        tft.setCursor(79 + 40, 145);
        tft.println("  Time");
      }

    }

    DrawButton(0, 0);

  } else if (page == 1) {
    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(97 + 40, 48);
      tft.println("Zeit");
    } else if (Language == 1) {  //English
      tft.setCursor(80 + 40, 48);
      tft.println("  Time");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    if (Language == 0) {  //German
      tft.setCursor(19, 162);
      tft.print("Zeit in Minuten: ");
    } else if (Language == 1) {  //English
      tft.setCursor(19, 162);
      tft.print("Time in minutes: ");
    }

    if (TimedCountDuration < 10) {  //Digits of one
      tft.setCursor(183, 165);
    } else if ((TimedCountDuration >= 10) && (TimedCountDuration < 100)) {  //tens digit
      tft.setCursor(176, 165);
    } else {  //Hundreds digit
      tft.setCursor(169, 165);
    }

    tft.setFreeFont(&FreeSans12pt7b);
    tft.println(TimedCountDuration);

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);

  } else if (page == 2) {  //Processing Page

    if ((millis() - TimedCountStartTime) >= TimedCountDuration * 60000) {
      TimedCountPage = 3;

      TimedCountEndHour = Hour;
      TimedCountEndMinute = Minute;
      TimedCountEndSecond = Second;

      TimedCountTmpMillis = millis();

      ClearPage = true;
      TimingCountSDLog();
    }

    TimingCountSDLog();

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(24 + 40, 48);
      tft.println("Zeitliche Zaehlung");
    } else if (Language == 1) {  //English
      tft.setCursor(27 + 40, 48);
      tft.println("    Timing count");
    }

    tft.drawRoundRect(3, 63, 314, 100, 4, WHITE);

    tft.drawRect(10, 103, 300, 20, WHITE);

    tft.setFreeFont(&FreeSans12pt7b);
    if (Language == 0) {  //German
      tft.setCursor(10, 89);
      tft.println("Fortschritt:");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 89);
      tft.println("Progress:");
    }

    int progress = map(millis() - TimedCountStartTime, 0, TimedCountDuration * 60000, 0, 296);
    if ((millis() - TimedCountStartTime) >= (TimedCountDuration * 60000)) {
      progress = 296;
    }
    tft.fillRect(12, 105, progress, 16, GREEN);

    tft.setFreeFont(&FreeSans12pt7b);
    if (Language == 0) {  //German
      tft.setCursor(10, 150);
      tft.print("Laufzeit: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 150);
      tft.print("Runtime: ");
    }
    tft.print(TimedCountDuration);
    tft.println(" min");


    tft.fillRect(5, 176, 310, 128, BLACK);

    if (TYPE != 1) {
      tft.drawRoundRect(3, 173, 314, 135, 4, WHITE);
    } else {
      tft.drawRoundRect(3, 173, 314, 135 - 32, 4, WHITE);
    }

    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 202);
      tft.print("Zaehlungen: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 202);
      tft.print("Counts: ");
    }
    tft.println(TimedCountCurrentCount);

    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(10, 233);
    tft.print("CPM: ");
    tft.println(TimedCountCPM);


    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (DoseLevel == 0) {  //Radiation
      tft.setTextColor(GREEN);
    } else if (DoseLevel == 1) {
      tft.setTextColor(YELLOW);
    } else if (DoseLevel == 2) {
      tft.setTextColor(ORANGE);
    } else if (DoseLevel == 3) {
      tft.setTextColor(RED);
    } else if (DoseLevel == 4) {
      tft.setTextColor(RED);
    }

    if (Language == 0) {  //German
      tft.setCursor(10, 262);
      tft.print("Dosis: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 262);
      tft.print("Dose: ");
    }

    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {
        tft.print(DoseRate, 3);
        tft.println("uSv/h");
      } else if (DoseUnitSivert == 1) {
        tft.print(DoseRate, 3);
        tft.println("mSv/h");
      } else if (DoseUnitSivert == 2) {
        tft.print(DoseRate, 3);
        tft.println("Sv/h");
      }
    } else if (DoseUnit == 1) {
      if (DoseUnitRem == 0) {
        tft.print(DoseRate, 3);
        tft.println("mRem/h");
      } else if (DoseUnitRem == 1) {
        tft.print(DoseRate, 3);
        tft.println("Rem/h");
      }
    }

    if (TYPE != 1) {

      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 293);
        tft.print("Micro SD Logging:");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 293);
        tft.print("Micro SD logging:");
      }
      if (TimedCountSDLoggingStatus == true) {
        tft.setTextColor(GREEN);
        if (Language == 0) {  //German
          tft.print(" AKTIV");
        } else if (Language == 1) {  //English
          tft.print(" ACTIVE");
        }
      } else {
        tft.setTextColor(RED);
        if (Language == 0) {  //German
          tft.print(" PASSIV");
        } else if (Language == 1) {  //English
          tft.print("PASSIVE");
        }
      }

    }

    DrawButton(0, 0);
    DrawButton(3, 1);

  } else if (page == 3) {  //Result Page

    if (ClearPage == true) {
      tft.fillRect(0, 0, 320, 480, BLACK);
      ClearPage = false;
    }

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(24 + 40, 48);
      tft.println("Zeitliche Zaehlung");
    } else if (Language == 1) {  //English
      tft.setCursor(27 + 40, 48);
      tft.println("    Timing count");
    }

    if (TYPE != 1) {

      tft.fillRoundRect(3, 63, 314, 193, 4, DKGREY);
      tft.drawRoundRect(3, 63, 314, 193, 4, WHITE);
    } else {
      tft.fillRoundRect(3, 63, 314, 193 - 60, 4, DKGREY);
      tft.drawRoundRect(3, 63, 314, 193 - 60, 4, WHITE);
    }
    tft.setFreeFont(&FreeSans12pt7b);
    if (Language == 0) {  //German
      tft.setCursor(10, 89);
      tft.println("Resultat:");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 89);
      tft.println("Result:");
    }


    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 120);
      tft.print("Zaehlungen: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 120);
      tft.print("Counts: ");
    }
    tft.println(TimedCountCurrentCount);


    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(10, 151);
    tft.print("CPM: ");
    tft.println(TimedCountCPM);

    if (TYPE != 1) {

      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 182);
        tft.print("Start: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 182);
        tft.print("Start: ");
      }


      if (TimeFormat == 0) {  //Time format 24H

        if (TimedCountStartHour < 10) {
          tft.print("0");
          tft.print(TimedCountStartHour);
        } else {
          tft.print(TimedCountStartHour);
        }
        tft.print(":");
        if (TimedCountStartMinute < 10) {
          tft.print("0");
          tft.print(TimedCountStartMinute);
        } else {
          tft.print(TimedCountStartMinute);
        }
        tft.print(":");
        if (TimedCountStartSecond < 10) {
          tft.print("0");
          tft.print(TimedCountStartSecond);
        } else {
          tft.print(TimedCountStartSecond);
        }

        if (Language == 0) {
          tft.print("Uhr");
        } else if (Language == 1) {
          tft.print("H");
        }

        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(10, 213);
          tft.print("Ende: ");
        } else if (Language == 1) {  //English
          tft.setCursor(10, 213);
          tft.print("End: ");
        }


        if (TimedCountEndHour < 10) {
          tft.print("0");
          tft.print(TimedCountEndHour);
        } else {
          tft.print(TimedCountEndHour);
        }
        tft.print(":");
        if (TimedCountEndMinute < 10) {
          tft.print("0");
          tft.print(TimedCountEndMinute);
        } else {
          tft.print(TimedCountEndMinute);
        }
        tft.print(":");
        if (TimedCountEndSecond < 10) {
          tft.print("0");
          tft.print(TimedCountEndSecond);
        } else {
          tft.print(TimedCountEndSecond);
        }

        if (Language == 0) {
          tft.print("Uhr");
        } else if (Language == 1) {
          tft.print("H");
        }


      } else if (TimeFormat == 1) {  //Time format 12H

        int TmpH = 0;
        bool AMPM = 0;  //AM - 0, PM - 1

        if (TimedCountStartHour == 0) {
          TmpH = 12;
          AMPM = 0;
        } else if ((TimedCountStartHour >= 1) && (TimedCountStartHour <= 11)) {
          TmpH = TimedCountStartHour;
          AMPM = 0;
        } else if ((TimedCountStartHour >= 12) && (TimedCountStartHour <= 23)) {
          AMPM = 1;
          if (TimedCountStartHour == 12) {
            TmpH = TimedCountStartHour;
          } else if (TimedCountStartHour == 13) {
            TmpH = 1;
          } else if (TimedCountStartHour == 14) {
            TmpH = 2;
          } else if (TimedCountStartHour == 15) {
            TmpH = 3;
          } else if (TimedCountStartHour == 16) {
            TmpH = 4;
          } else if (TimedCountStartHour == 17) {
            TmpH = 5;
          } else if (TimedCountStartHour == 18) {
            TmpH = 6;
          } else if (TimedCountStartHour == 19) {
            TmpH = 7;
          } else if (TimedCountStartHour == 20) {
            TmpH = 8;
          } else if (TimedCountStartHour == 21) {
            TmpH = 9;
          } else if (TimedCountStartHour == 22) {
            TmpH = 10;
          } else if (TimedCountStartHour == 23) {
            TmpH = 11;
          }
        }

        if (TmpH < 10) {
          tft.print("0");
          tft.print(TmpH);
        } else {
          tft.print(TmpH);
        }

        tft.print(":");
        if (TimedCountStartMinute < 10) {
          tft.print("0");
          tft.print(TimedCountStartMinute);
        } else {
          tft.print(TimedCountStartMinute);
        }
        tft.print(":");
        if (TimedCountStartSecond < 10) {
          tft.print("0");
          tft.print(TimedCountStartSecond);
        } else {
          tft.print(TimedCountStartSecond);
        }

        if (AMPM == 0) {
          tft.print(" AM");
        } else {
          tft.print(" PM");
        }


        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(10, 213);
          tft.print("Ende: ");
        } else if (Language == 1) {  //English
          tft.setCursor(10, 213);
          tft.print("End: ");
        }

        if (TimedCountEndHour == 0) {
          TmpH = 12;
          AMPM = 0;
        } else if ((TimedCountEndHour >= 1) && (TimedCountEndHour <= 11)) {
          TmpH = TimedCountEndHour;
          AMPM = 0;
        } else if ((TimedCountEndHour >= 12) && (TimedCountEndHour <= 23)) {
          AMPM = 1;
          if (TimedCountEndHour == 12) {
            TmpH = TimedCountEndHour;
          } else if (TimedCountEndHour == 13) {
            TmpH = 1;
          } else if (TimedCountEndHour == 14) {
            TmpH = 2;
          } else if (TimedCountEndHour == 15) {
            TmpH = 3;
          } else if (TimedCountEndHour == 16) {
            TmpH = 4;
          } else if (TimedCountEndHour == 17) {
            TmpH = 5;
          } else if (TimedCountEndHour == 18) {
            TmpH = 6;
          } else if (TimedCountEndHour == 19) {
            TmpH = 7;
          } else if (TimedCountEndHour == 20) {
            TmpH = 8;
          } else if (TimedCountEndHour == 21) {
            TmpH = 9;
          } else if (TimedCountEndHour == 22) {
            TmpH = 10;
          } else if (TimedCountEndHour == 23) {
            TmpH = 11;
          }
        }

        if (TmpH < 10) {
          tft.print("0");
          tft.print(TmpH);
        } else {
          tft.print(TmpH);
        }

        tft.print(":");
        if (TimedCountEndMinute < 10) {
          tft.print("0");
          tft.print(TimedCountEndMinute);
        } else {
          tft.print(TimedCountEndMinute);
        }
        tft.print(":");
        if (TimedCountEndSecond < 10) {
          tft.print("0");
          tft.print(TimedCountEndSecond);
        } else {
          tft.print(TimedCountEndSecond);
        }

        if (AMPM == 0) {
          tft.print(" AM");
        } else {
          tft.print(" PM");
        }
      }

      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 244);
        tft.print("Laufzeit: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 244);
        tft.print("Runtime: ");
      }
      tft.print(float(TimedCountTmpMillis - TimedCountStartTime) / 60000.00);
      tft.println(" min");

    } else {

      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 182);
        tft.print("Laufzeit: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 182);
        tft.print("Runtime: ");
      }
      tft.print(float(TimedCountTmpMillis - TimedCountStartTime) / 60000.00);
      tft.println(" min");

    }

    DrawButton(0, 0);
    DrawButton(2, 1);
  }
}

void MedicalModeCPM() {
  //tft.invertDisplay(1); //Display invertieren

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  /*
        ///Draw graph///
        huge arguement list
        x = x data point
        y = y datapont
        gx = x graph location (lower left)
        gy = y graph location (lower left)
        w = width of graph
        h = height of graph
        xlo = lower bound of x axis Untergrenze der x-Achse
        xhi = upper bound of x asis Obergrenze der x-Achse
        xinc = division of x axis (distance not count)
        ylo = lower bound of y axis untere Schranke der y-Achse
        yhi = upper bound of y asis Obergrenze der y-Achse
        yinc = division of y axis (distance not count)
        title = title of graph
        xlabel = x asis label
        ylabel = y asis label
        gcolor = graph line colors
        acolor = axi ine colors
        pcolor = color of your plotted data
        tcolor = text color
        bcolor = background color
        &redraw = flag to redraw graph on fist call only
        mode = 0 - basic Mode (Diagram without text), 1 - advanced Mode(Diagram with all)
  */

  Graph(graphx, CPM, 10, 140, 300, 100, 0, xUpperLimit, 10, 0, pow(10, CPMPotency), pow(10, CPMPotency), "", "", "", DKBLUE, WHITE, RED, WHITE, BLACK, GraphUpdate, 0);
  graphx++;

  tft.fillRect(0, 145, 320, 65, BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (DoseLevel == 0) {  //Radiation
    tft.setTextColor(GREEN);
  } else if (DoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (DoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (DoseLevel == 3) {
    tft.setTextColor(RED);
  } else if (DoseLevel == 4) {
    tft.setTextColor(RED);
  }

  if (CPMPotency == 2) {
    if (CPM < 10) {
      tft.setCursor(12, 195);
      tft.print("000000");
      tft.print(CPM);
      tft.print(" CPM");
    } else {
      tft.setCursor(12, 195);
      tft.print("00000");
      tft.print(CPM);
      tft.print(" CPM");
    }
  } else if (CPMPotency == 3) {
    tft.setCursor(12, 195);
    tft.print("0000");
    tft.print(CPM);
    tft.print(" CPM");
  } else if (CPMPotency == 4) {
    tft.setCursor(12, 195);
    tft.print("000");
    tft.print(CPM);
    tft.print(" CPM");
  } else if (CPMPotency == 5) {
    tft.setCursor(12, 195);
    tft.print("00");
    tft.print(CPM);
    tft.print(" CPM");
  } else if (CPMPotency == 6) {
    tft.setCursor(12, 195);
    tft.print("0");
    tft.print(CPM);
    tft.print(" CPM");
  } else if (CPMPotency == 7) {
    tft.setCursor(12, 195);
    tft.print(CPM);
    tft.print(" CPM");
  } else if (CPMPotency >= 8) {
    tft.setCursor(12, 195);
    if (((int)CPM / 60) < 10) {
      tft.print("000000");
    } else if ((((int)CPM / 60) >= 10) && (((int)CPM / 60) < 100)) {
      tft.print("00000");
    } else if ((((int)CPM / 60) >= 100) && (((int)CPM / 60) < 1000)) {
      tft.print("0000");
    } else if ((((int)CPM / 60) >= 1000) && (((int)CPM / 60) < 10000)) {
      tft.print("000");
    } else if ((((int)CPM / 60) >= 10000) && (((int)CPM / 60) < 100000)) {
      tft.print("00");
    } else if ((((int)CPM / 60) >= 100000) && (((int)CPM / 60) < 1000000)) {
      tft.print("0");
    }

    tft.print((int)CPM / 60);
    tft.print(" CPS");
  }

  //Dose information
  if (DoseLevel == 0) {  //Radiation
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(16, 240);
      tft.println("Niedrige Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(18, 240);
      tft.println("    Low radiation");
    }
  } else if (DoseLevel == 1) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(13, 240);
      tft.println("Erhoehte Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(21, 240);
      tft.println("Elevated radiation");
    }
  } else if ((DoseLevel == 2) || (DoseLevel == 3)) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(40, 240);
      tft.println("Hohe Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(44, 240);
      tft.println(" High radiation");
    }
  } else if (DoseLevel == 4) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(6, 240);
      tft.println("Toedliche Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(4, 240);
      tft.println("Dangerous radiation");
    }
  }



  tft.drawLine(10, 258, 310, 258, WHITE);
  tft.drawLine(9, 259, 311, 259, WHITE);
  tft.drawLine(10, 260, 310, 260, WHITE);



  //Additional Data


  tft.fillRect(0, 262, 320, 147, BLACK);


  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (DoseLevel == 0) {  //Radiation
    tft.setTextColor(GREEN);
  } else if (DoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (DoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (DoseLevel == 3) {
    tft.setTextColor(RED);
  } else if (DoseLevel == 4) {
    tft.setTextColor(RED);
  }

  if (Language == 0) {  //German
    tft.setCursor(15, 292);
    tft.print("Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 292);
    tft.print("Dose: ");
  }

  if (DoseUnit == 0) {
    if (DoseUnitSivert == 0) {
      tft.print(DoseRate, 3);
      tft.println("uSv/h");
    } else if (DoseUnitSivert == 1) {
      tft.print(DoseRate, 3);
      tft.println("mSv/h");
    } else if (DoseUnitSivert == 2) {
      tft.print(DoseRate, 3);
      tft.println("Sv/h");
    }
  } else if (DoseUnit == 1) {
    if (DoseUnitRem == 0) {
      tft.print(DoseRate, 3);
      tft.println("mRem/h");
    } else if (DoseUnitRem == 1) {
      tft.print(DoseRate, 3);
      tft.println("Rem/h");
    }
  }

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(15, 327);
    tft.print("Max. Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 327);
    tft.print("Max. dose: ");
  }

  if (MaxDoseUnit == 0) {
    tft.print(MaxDoseRate);
    tft.println("uSv/h");
  } else if (MaxDoseUnit == 1) {
    tft.print(MaxDoseRate);
    tft.println("mSv/h");
  } else if (MaxDoseUnit == 2) {
    tft.print(MaxDoseRate);
    tft.println("Sv/h");
  } else if (MaxDoseUnit == 3) {
    tft.print(MaxDoseRate);
    tft.println("mRem/h");
  } else if (MaxDoseUnit == 4) {
    tft.print(MaxDoseRate);
    tft.println("Rem/h");
  }

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (TotalDoseLevel == 0) {  //TotalDoseLevel
    tft.setTextColor(GREEN);
  } else if (TotalDoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (TotalDoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (TotalDoseLevel == 3) {
    tft.setTextColor(RED);
  }

  if (Language == 0) {  //German
    tft.setCursor(15, 361);
    tft.print("Ges. Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 361);
    tft.print("Full dose: ");
  }

  if (DoseUnit == 0) {
    if (TotalDose >= 1000.00) {
      tft.print(TotalDose / 1000, 5);
      tft.println("mSv");
    } else {
      tft.print(TotalDose, 5);
      tft.println("uSv");
    }
  } else if (DoseUnit == 1) {
    if (TotalDose >= 100.00) {
      tft.print(TotalDose / 1000, 4);
      tft.println("Rem");
    } else {
      tft.print(TotalDose, 4);
      tft.println("mRem");
    }
  }

  /*

          tft.fillRect(0, 225, 240, 20, BLACK);
          tft.setFreeFont(&FreeSans9pt7b); //9,12,18,24pt
          tft.setTextSize(1);
          tft.setTextColor(WHITE);
          tft.setCursor(15, 240);
          tft.print("Dosis ");

          if (WT == 0){ //Knochenmark (rot) - Bone marrow (red)
            tft.print("Knochenmark: ");
          }else if (WT == 1){ //Dickdarm - Colon
            tft.print("Dickdarm: ");
          }else if (WT == 2){ //Lunge - Lungs
            tft.print("Lunge: ");
          }else if (WT == 3){ //Magen - Stomach
            tft.print("Magen: ");
          }else if (WT == 4){ //Brust - Chest
            tft.print("Brust: ");
          }else if (WT == 5){ //Keimdrüsen - Gonads
            tft.print("Keimdrüsen: ");
          }else if (WT == 6){ //Blase - Bladder
            tft.print("Blase: ");
          }else if (WT == 7){ //Speiseröhre - Esophagus
            tft.print("Speiseröhre: ");
          }else if (WT == 8){ //Leber - Liver
            tft.print("Leber: ");
          }else if (WT == 9){ //Schilddrüse - Thyroid gland
            tft.print("Schilddrüse: ");
          }else if (WT == 10){ //Haut - Skin
            tft.print("Haut: ");
          }else if (WT == 11){ //Knochenoberfläche - Bone surface
            tft.print("Knochenoberfläche: ");
          }else if (WT == 12){ //Gehirn - Brain
            tft.print("Gehirn: ");
          }else if (WT == 13){ //Speicheldrüsen - Salivary glands
            tft.print("Speicheldrüsen: ");
          }else if (WT == 14){ //übrige Organe und Gewebe - Other organs and tissues
            tft.print("Andere: ");
          }

          if (DoseUnit == 0){ //Sv
            if (DoseUnitSivert == 0){ //uSv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("uSv/h");
            }else if (DoseUnitSivert == 1) { //mSv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("mSv/h");
            }else if (DoseUnitSivert == 2){ //Sv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("Sv/h");
            }
          }else if (DoseUnit == 1){ //Rem
            if (DoseUnitRem == 0) { //mRem/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("mRem/h");
            }else if (DoseUnitRem == 1){
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("Rem/h");
            }
          }

  */

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(15, 395);
    tft.print("Zaehlungen: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 395);
    tft.print("Counts: ");
  }

  if (SelectIntExtTube) {  //Intern
    tft.print(InternCurrentCount);
  } else {
    tft.print(ExternCurrentCount);
  }


  DrawMainButtons();
}

void MedicalMode() {
  //tft.invertDisplay(1); //Display invertieren

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  /*
        ///Draw graph///
        huge arguement list
        x = x data point
        y = y datapont
        gx = x graph location (lower left)
        gy = y graph location (lower left)
        w = width of graph
        h = height of graph
        xlo = lower bound of x axis Untergrenze der x-Achse
        xhi = upper bound of x asis Obergrenze der x-Achse
        xinc = division of x axis (distance not count)
        ylo = lower bound of y axis untere Schranke der y-Achse
        yhi = upper bound of y asis Obergrenze der y-Achse
        yinc = division of y axis (distance not count)
        title = title of graph
        xlabel = x asis label
        ylabel = y asis label
        gcolor = graph line colors
        acolor = axi ine colors
        pcolor = color of your plotted data
        tcolor = text color
        bcolor = background color
        &redraw = flag to redraw graph on fist call only
        mode = 0 - basic Mode (Diagram without text), 1 - advanced Mode(Diagram with all)
  */

  Graph(graphx, CPM, 10, 140, 300, 100, 0, xUpperLimit, 10, 0, pow(10, CPMPotency), pow(10, CPMPotency), "", "", "", DKBLUE, WHITE, RED, WHITE, BLACK, GraphUpdate, 0);
  graphx++;

  //Dose with correct scale
  if (DoseUnit == 0) {          //Sv
    if (DoseUnitSivert == 0) {  //uSv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) {  //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(25, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }
      tft.setCursor(178, 195);
      tft.println("uSv/h");
    } else if (DoseUnitSivert == 1) {  //mSv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt

      if (DoseLevel == 0) {  //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(16, 195);

      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }

      tft.setCursor(172, 195);
      tft.println("mSv/h");

    } else if (DoseUnitSivert == 2) {  //Sv/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) {  //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(21, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 4);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 3);
      } else {
        tft.println(DoseRate, 5);
      }
      tft.setCursor(205, 195);
      tft.println("Sv/h");
    }

  } else if (DoseUnit == 1) {  //Rem

    if (DoseUnitRem == 0) {  //mRem/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) {  //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(10, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 2);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 1);
      } else {
        tft.println(DoseRate, 3);
      }
      tft.setCursor(135, 195);
      tft.println("mRem/h");

    } else if (DoseUnitRem == 1) {  //Rem/h

      tft.fillRect(0, 145, 320, 65, BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans24pt7b);  //9,12,18,24pt
      tft.setTextSize(1);

      if (DoseLevel == 0) {  //Radiation
        tft.setTextColor(GREEN);
      } else if (DoseLevel == 1) {
        tft.setTextColor(YELLOW);
      } else if (DoseLevel == 2) {
        tft.setTextColor(ORANGE);
      } else if (DoseLevel == 3) {
        tft.setTextColor(RED);
      } else if (DoseLevel == 4) {
        tft.setTextColor(RED);
      }

      tft.setCursor(16, 195);
      if ((DoseRate >= 10.00) && (DoseRate < 100.00)) {
        tft.println(DoseRate, 3);
      } else if (DoseRate >= 100.00) {
        tft.println(DoseRate, 2);
      } else {
        tft.println(DoseRate, 4);
      }
      tft.setCursor(168, 195);
      tft.println("Rem/h");
    }
  }

  //Dose information
  if (DoseLevel == 0) {  //Radiation
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(16, 240);
      tft.println("Niedrige Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(18, 240);
      tft.println("    Low radiation");
    }
  } else if (DoseLevel == 1) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(13, 240);
      tft.println("Erhoehte Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(21, 240);
      tft.println("Elevated radiation");
    }
  } else if ((DoseLevel == 2) || (DoseLevel == 3)) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(40, 240);
      tft.println("Hohe Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(44, 240);
      tft.println(" High radiation");
    }
  } else if (DoseLevel == 4) {
    tft.fillRect(0, 210, 320, 41, BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans18pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(6, 240);
      tft.println("Toedliche Strahlung");
    } else if (Language == 1) {  //English
      tft.setCursor(4, 240);
      tft.println("Dangerous radiation");
    }
  }



  tft.drawLine(10, 258, 310, 258, WHITE);
  tft.drawLine(9, 259, 311, 259, WHITE);
  tft.drawLine(10, 260, 310, 260, WHITE);



  //Additional Data


  tft.fillRect(0, 262, 320, 147, BLACK);


  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(15, 292);
  tft.print("CPM: ");
  tft.println(CPM);

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(15, 327);
    tft.print("Max. Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 327);
    tft.print("Max. dose: ");
  }

  if (MaxDoseUnit == 0) {
    tft.print(MaxDoseRate);
    tft.println("uSv/h");
  } else if (MaxDoseUnit == 1) {
    tft.print(MaxDoseRate);
    tft.println("mSv/h");
  } else if (MaxDoseUnit == 2) {
    tft.print(MaxDoseRate);
    tft.println("Sv/h");
  } else if (MaxDoseUnit == 3) {
    tft.print(MaxDoseRate);
    tft.println("mRem/h");
  } else if (MaxDoseUnit == 4) {
    tft.print(MaxDoseRate);
    tft.println("Rem/h");
  }

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (TotalDoseLevel == 0) {  //TotalDoseLevel
    tft.setTextColor(GREEN);
  } else if (TotalDoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (TotalDoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (TotalDoseLevel == 3) {
    tft.setTextColor(RED);
  }

  if (Language == 0) {  //German
    tft.setCursor(15, 361);
    tft.print("Ges. Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 361);
    tft.print("Full dose: ");
  }

  if (DoseUnit == 0) {
    if (TotalDose >= 1000.00) {
      tft.print(TotalDose / 1000, 5);
      tft.println("mSv");
    } else {
      tft.print(TotalDose, 5);
      tft.println("uSv");
    }
  } else if (DoseUnit == 1) {
    if (TotalDose >= 100.00) {
      tft.print(TotalDose / 1000, 4);
      tft.println("Rem");
    } else {
      tft.print(TotalDose, 4);
      tft.println("mRem");
    }
  }

  /*

          tft.fillRect(0, 225, 240, 20, BLACK);
          tft.setFreeFont(&FreeSans9pt7b); //9,12,18,24pt
          tft.setTextSize(1);
          tft.setTextColor(WHITE);
          tft.setCursor(15, 240);
          tft.print("Dosis ");

          if (WT == 0){ //Knochenmark (rot) - Bone marrow (red)
            tft.print("Knochenmark: ");
          }else if (WT == 1){ //Dickdarm - Colon
            tft.print("Dickdarm: ");
          }else if (WT == 2){ //Lunge - Lungs
            tft.print("Lunge: ");
          }else if (WT == 3){ //Magen - Stomach
            tft.print("Magen: ");
          }else if (WT == 4){ //Brust - Chest
            tft.print("Brust: ");
          }else if (WT == 5){ //Keimdrüsen - Gonads
            tft.print("Keimdrüsen: ");
          }else if (WT == 6){ //Blase - Bladder
            tft.print("Blase: ");
          }else if (WT == 7){ //Speiseröhre - Esophagus
            tft.print("Speiseröhre: ");
          }else if (WT == 8){ //Leber - Liver
            tft.print("Leber: ");
          }else if (WT == 9){ //Schilddrüse - Thyroid gland
            tft.print("Schilddrüse: ");
          }else if (WT == 10){ //Haut - Skin
            tft.print("Haut: ");
          }else if (WT == 11){ //Knochenoberfläche - Bone surface
            tft.print("Knochenoberfläche: ");
          }else if (WT == 12){ //Gehirn - Brain
            tft.print("Gehirn: ");
          }else if (WT == 13){ //Speicheldrüsen - Salivary glands
            tft.print("Speicheldrüsen: ");
          }else if (WT == 14){ //übrige Organe und Gewebe - Other organs and tissues
            tft.print("Andere: ");
          }

          if (DoseUnit == 0){ //Sv
            if (DoseUnitSivert == 0){ //uSv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("uSv/h");
            }else if (DoseUnitSivert == 1) { //mSv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("mSv/h");
            }else if (DoseUnitSivert == 2){ //Sv/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("Sv/h");
            }
          }else if (DoseUnit == 1){ //Rem
            if (DoseUnitRem == 0) { //mRem/h
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("mRem/h");
            }else if (DoseUnitRem == 1){
              tft.print(EffectiveDoseRate(WT),3);
              tft.println("Rem/h");
            }
          }

  */

  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(15, 395);
    tft.print("Zaehlungen: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 395);
    tft.print("Counts: ");
  }

  if (SelectIntExtTube) {  //Intern
    tft.print(InternCurrentCount);
  } else {
    tft.print(ExternCurrentCount);
  }


  DrawMainButtons();
}

void MeterMode() {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  DrawAnalogMeter(CPM, 50);

  DrawMainButtons();

  tft.drawLine(10, 258, 310, 258, WHITE);
  tft.drawLine(9, 259, 311, 259, WHITE);
  tft.drawLine(10, 260, 310, 260, WHITE);



  tft.fillRect(0, 262, 320, 147, BLACK);


  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  tft.setCursor(15, 292);
  tft.print("CPM: ");
  tft.println(CPM);



  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (DoseLevel == 0) {  //Radiation
    tft.setTextColor(GREEN);
  } else if (DoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (DoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (DoseLevel == 3) {
    tft.setTextColor(RED);
  } else if (DoseLevel == 4) {
    tft.setTextColor(RED);
  }

  if (Language == 0) {  //German
    tft.setCursor(15, 327);
    tft.print("Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 327);
    tft.print("Dose: ");
  }

  if (DoseUnit == 0) {
    if (DoseUnitSivert == 0) {
      tft.print(DoseRate, 3);
      tft.println("uSv/h");
    } else if (DoseUnitSivert == 1) {
      tft.print(DoseRate, 3);
      tft.println("mSv/h");
    } else if (DoseUnitSivert == 2) {
      tft.print(DoseRate, 3);
      tft.println("Sv/h");
    }
  } else if (DoseUnit == 1) {
    if (DoseUnitRem == 0) {
      tft.print(DoseRate, 3);
      tft.println("mRem/h");
    } else if (DoseUnitRem == 1) {
      tft.print(DoseRate, 3);
      tft.println("Rem/h");
    }
  }



  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);

  if (TotalDoseLevel == 0) {  //TotalDoseLevel
    tft.setTextColor(GREEN);
  } else if (TotalDoseLevel == 1) {
    tft.setTextColor(YELLOW);
  } else if (TotalDoseLevel == 2) {
    tft.setTextColor(ORANGE);
  } else if (TotalDoseLevel == 3) {
    tft.setTextColor(RED);
  }

  if (Language == 0) {  //German
    tft.setCursor(15, 361);
    tft.print("Ges. Dosis: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 361);
    tft.print("Full dose: ");
  }

  if (DoseUnit == 0) {
    if (TotalDose >= 1000.00) {
      tft.print(TotalDose / 1000, 5);
      tft.println("mSv");
    } else {
      tft.print(TotalDose, 5);
      tft.println("uSv");
    }
  } else if (DoseUnit == 1) {
    if (TotalDose >= 100.00) {
      tft.print(TotalDose / 1000, 4);
      tft.println("Rem");
    } else {
      tft.print(TotalDose, 4);
      tft.println("mRem");
    }
  }


  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(15, 395);
    tft.print("Zaehlungen: ");
  } else if (Language == 1) {  //English
    tft.setCursor(15, 395);
    tft.print("Counts: ");
  }
  if (SelectIntExtTube) {  //Intern
    tft.print(InternCurrentCount);
  } else {
    tft.print(ExternCurrentCount);
  }
}

void DrawAnalogMeter(int value, int h) {
  if (MeterUpdate == true) {
    AnalogMeter(h);
    MeterUpdate = false;
  }
  PlotNeedle(value, 0, h);
}

void AnalogMeter(int h) {

  tft.fillRect(0, 24, 320, 215, BLACK);

  tft.setTextColor(WHITE);  // Text colour

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 20;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
    uint16_t y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
    uint16_t x1 = sx * M_SIZE * 100 + M_SIZE * 120;
    uint16_t y1 = sy * M_SIZE * 100 + M_SIZE * 140;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (M_SIZE * 100 + tl) + M_SIZE * 120;
    int y2 = sy2 * (M_SIZE * 100 + tl) + M_SIZE * 140;
    int x3 = sx2 * M_SIZE * 100 + M_SIZE * 120;
    int y3 = sy2 * M_SIZE * 100 + M_SIZE * 140;

    //Yellow zone limits
    //if (i >= -50 && i < 0) {
    //  tft.fillTriangle(x0, y0, x1, y1, x2, y2, YELLOW);
    //  tft.fillTriangle(x1, y1, x2, y2, x3, y3, YELLOW);
    //}

    //Orange zone limits
    if (i >= 0 && i < 25) {
      tft.fillTriangle(x0, y0 + h, x1, y1 + h, x2, y2 + h, ORANGE);
      tft.fillTriangle(x1, y1 + h, x2, y2 + h, x3, y3 + h, ORANGE);
    }

    //Red zone limits
    if (i >= 25 && i < 50) {
      tft.fillTriangle(x0, y0 + h, x1, y1 + h, x2, y2 + h, RED);
      tft.fillTriangle(x1, y1 + h, x2, y2 + h, x3, y3 + h, RED);
    }

    // Short scale tick length
    if (i % 25 != 0) tl = 8;

    // Recalculate coords incase tick lenght changed
    x0 = sx * (M_SIZE * 100 + tl) + M_SIZE * 120;
    y0 = sy * (M_SIZE * 100 + tl) + M_SIZE * 140;
    x1 = sx * M_SIZE * 100 + M_SIZE * 120;
    y1 = sy * M_SIZE * 100 + M_SIZE * 140;

    // Draw tick
    tft.drawLine(x0, y0 + h, x1, y1 + h, WHITE);

    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0) {
      // Calculate label positions
      x0 = sx * (M_SIZE * 100 + tl + 10) + M_SIZE * 120;
      y0 = sy * (M_SIZE * 100 + tl + 10) + M_SIZE * 140;
      switch (i / 25) {
        case -2: tft.drawCentreString("0", x0, (y0 - 12) + h, 2); break;
        case -1:
          if (CPMPotency > 3) {
            tft.drawCentreString("1/4", x0, (y0 - 9) + h, 2);
            break;
          } else {
            if (CPMPotency == 2) {
              tft.drawCentreString("25", x0, (y0 - 9) + h, 2);
              break;
            } else if (CPMPotency == 3) {
              tft.drawCentreString("250", x0, (y0 - 9) + h, 2);
              break;
            }
          }
        case 0:
          if (CPMPotency > 3) {
            tft.drawCentreString("2/4", x0, (y0 - 6) + h, 2);
            break;
          } else {
            if (CPMPotency == 2) {
              tft.drawCentreString("50", x0, (y0 - 6) + h, 2);
              break;
            } else if (CPMPotency == 3) {
              tft.drawCentreString("500", x0, (y0 - 6) + h, 2);
              break;
            }
          }
        case 1:
          if (CPMPotency > 3) {
            tft.drawCentreString("3/4", x0, (y0 - 9) + h, 2);
            break;
          } else {
            if (CPMPotency == 2) {
              tft.drawCentreString("75", x0, (y0 - 9) + h, 2);
              break;
            } else if (CPMPotency == 3) {
              tft.drawCentreString("750", x0, (y0 - 9) + h, 2);
              break;
            }
          }
        case 2:
          if (CPMPotency > 3) {
            String TXT1 = "10^" + String(CPMPotency);
            tft.drawCentreString(TXT1, x0, (y0 - 12) + h, 2);
            break;
          } else {
            if (CPMPotency == 2) {
              tft.drawCentreString("100", x0, (y0 - 12) + h, 2);
              break;
            } else if (CPMPotency == 3) {
              tft.drawCentreString("1000", x0, (y0 - 12) + h, 2);
              break;
            } else {
              String TXT1 = "10^" + String(CPMPotency);
              tft.drawCentreString(TXT1, x0, (y0 - 12) + h, 2);
              break;
            }
          }
      }
    }

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * M_SIZE * 100 + M_SIZE * 120;
    y0 = sy * M_SIZE * 100 + M_SIZE * 140;
    // Draw scale arc, don't draw the last part
    if (i < 50) tft.drawLine(x0, y0, x1, y1, TFT_BLACK);
  }
}

void PlotNeedle(int value, byte ms_delay, int h) {
  tft.setTextColor(WHITE, BLACK);

  old_analog = value;  // Update immediately if delay is 0

  float sdeg = map(old_analog, -10, pow(10, CPMPotency) * 1.1, -150, -30);  // Map value to angle
  // Calcualte tip of needle coords
  float sx = cos(sdeg * 0.0174532925);
  float sy = sin(sdeg * 0.0174532925);

  // Erase old needle image
  tft.drawLine(159, M_SIZE * (140 - 20) + h - 15, osx - 1, osy + h, BLACK);
  tft.drawLine(160, M_SIZE * (140 - 20) + h - 15, osx, osy + h, BLACK);
  tft.drawLine(161, M_SIZE * (140 - 20) + h - 15, osx + 1, osy + h, BLACK);

  // Re-plot text under needle
  tft.setTextColor(WHITE);
  tft.drawCentreString("CPM", M_SIZE * 120, M_SIZE * 70 + h, 4);  // // Comment out to avoid font 4

  // Store new needle end coords for next erase
  osx = M_SIZE * (sx * 98 + 120);
  osy = M_SIZE * (sy * 98 + 140);

  // Draw the needle in the new postion, magenta makes needle a bit bolder
  // draws 3 lines to thicken needle
  tft.drawLine(159, M_SIZE * (140 - 20) + h - 15, osx - 1, osy + h, RED);
  tft.drawLine(160, M_SIZE * (140 - 20) + h - 15, osx, osy + h, RED);
  tft.drawLine(161, M_SIZE * (140 - 20) + h - 15, osx + 1, osy + h, RED);
}

void DrawMainButtons() {

  tft.drawLine(10, 411, 310, 411, WHITE);
  tft.drawLine(9, 412, 311, 412, WHITE);
  tft.drawLine(10, 413, 310, 413, WHITE);

  tft.drawBitmap(6, 420, settingSymbolSmall, 55, 55, WHITE);

  if (Keylock == true) {
    tft.drawBitmap(68, 420, closedLockSymbolSmall, 55, 55, WHITE);
  } else {
    tft.drawBitmap(68, 420, openLockSymbolSmall, 55, 55, WHITE);
  }

  tft.drawBitmap(130, 420, rotationArrowsSymbol, 55, 55, WHITE);

  tft.drawBitmap(192, 420, menuSymbolSmall, 55, 55, WHITE);

  if (BuzzerMode == true) {
    tft.drawBitmap(255, 420, speakerOn, 55, 55, WHITE);
  } else {
    tft.drawBitmap(255, 420, speakerOff, 55, 55, WHITE);
  }
}

void Graph(int x, float y, int gx, int gy, int w, int h, int xlo, int xhi, int xinc, int ylo, int yhi, int yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int pcolor, unsigned int tcolor, unsigned int bcolor, boolean redraw, int mode) {
  int ydiv, xdiv;
  // initialize old x and old y in order to draw the first point of the graph
  // but save the transformed value
  // note my transform funcition is the same as the map function, except the map uses long and we need doubles
  //static double ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
  //static double oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
  int i;
  int temp;
  int rot, newrot;

  if (graphx > (xUpperLimit - 2)) {
    graphx = 0;
    GraphUpdate = true;
  }

  if (redraw == true) {
    tft.fillRect(0, gy - (h + 9), 320, h + 15, BLACK);
    GraphUpdate = false;
    ox = (x - xlo) * (w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    for (i = ylo; i <= yhi; i += yinc) {
      // compute the transform
      temp = (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == 0) {
        tft.drawLine(gx, temp, gx + w, temp, acolor);
      } else {
        tft.drawLine(gx, temp, gx + w, temp, gcolor);
      }

      if (mode == 0) {

      } else {
        tft.setTextSize(1);
        tft.setTextColor(tcolor, bcolor);
        tft.setCursor(gx - 40, temp);
        // precision is default Arduino--this could really use some format control
        tft.println(i);
      }
    }
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc) {

      // compute the transform

      temp = (i - xlo) * (w) / (xhi - xlo) + gx;
      if (i == 0) {
        tft.drawLine(temp, gy, temp, gy - h, acolor);
      } else {
        tft.drawLine(temp, gy, temp, gy - h, gcolor);
      }

      if (mode == 0) {

      } else {
        tft.setTextSize(1);
        tft.setTextColor(tcolor, bcolor);
        tft.setCursor(temp, gy + 10);
        // precision is default Arduino--this could really use some format control
        tft.println(i);
      }
    }

    //now draw the labels

    if (mode == 0) {

    } else {
      tft.setTextSize(1);
      tft.setTextColor(acolor, bcolor);
      tft.setCursor(gx, gy + 20);
      tft.println(xlabel);

      tft.setTextSize(1);
      tft.setTextColor(acolor, bcolor);
      tft.setCursor(gx - 30, gy - h - 10);
      tft.println(ylabel);
    }
  }

  //graph drawn now plot the data
  // the entire plotting code are these few lines...
  // recall that ox and oy are initialized as static above
  x = (x - xlo) * (w) / (xhi - xlo) + gx;
  y = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

  //tft.drawLine(ox, oy, x, y, pcolor);
  //tft.drawLine(ox, oy + 1, x, y + 1, pcolor);
  //tft.drawLine(ox, oy - 1, x, y - 1, pcolor);

  tft.fillRect(x, y, x - ox, gy - y, pcolor);

  ox = x;
  oy = y;
}

void DrawButton(int ButtonType, int Position) {  //ButtonType = 0 - LeftArrow, 1 - RightArrow, 2 - Home, 3 - Stop
  //Position = 0 - Left, 2 - Right, 1 - Middle
  if (ButtonType == 0) {  //leftArrow
    if (Position == 0) {
      tft.fillRoundRect(4, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(4, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(5, 431, leftArrowSmall, 45, 45, WHITE);
    } else if (Position == 1) {
      tft.fillRoundRect(136, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(136, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(137, 431, leftArrowSmall, 45, 45, WHITE);
    } else if (Position == 2) {
      tft.fillRoundRect(269, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(269, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(270, 431, leftArrowSmall, 45, 45, WHITE);
    }
  } else if (ButtonType == 1) {  //rightArrow
    if (Position == 0) {
      tft.fillRoundRect(4, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(4, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(5, 431, rightArrowSmall, 45, 45, WHITE);
    } else if (Position == 1) {
      tft.fillRoundRect(136, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(136, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(137, 431, rightArrowSmall, 45, 45, WHITE);
    } else if (Position == 2) {
      tft.fillRoundRect(269, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(269, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(270, 431, rightArrowSmall, 45, 45, WHITE);
    }
  } else if (ButtonType == 2) {  //Home
    if (Position == 0) {
      tft.fillRoundRect(4, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(4, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(4, 431, homeSymbolSmall, 45, 45, WHITE);
    } else if (Position == 1) {
      tft.fillRoundRect(136, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(136, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(137, 431, homeSymbolSmall, 45, 45, WHITE);
    } else if (Position == 2) {
      tft.fillRoundRect(269, 429, 48, 48, 3, AccentColor);
      tft.drawRoundRect(269, 429, 48, 48, 3, WHITE);
      tft.drawBitmap(270, 431, homeSymbolSmall, 45, 45, WHITE);
    }
  } else if (ButtonType == 3) {  //Stop
    if (Position == 0) {
      tft.fillRoundRect(4, 429, 96, 48, 3, RED);
      tft.drawRoundRect(4, 429, 96, 48, 3, WHITE);
      tft.setTextColor(WHITE);
      tft.setFreeFont(&FreeSans18pt7b);
      tft.setCursor(5, 466);
      tft.print("STOP");
    } else if (Position == 1) {
      tft.fillRoundRect(112, 429, 96, 48, 3, RED);
      tft.drawRoundRect(112, 429, 96, 48, 3, WHITE);
      tft.setTextColor(WHITE);
      tft.setFreeFont(&FreeSans18pt7b);
      tft.setCursor(112, 466);
      tft.print("STOP");
    } else if (Position == 2) {
      tft.fillRoundRect(221, 429, 96, 48, 3, RED);
      tft.drawRoundRect(221, 429, 96, 48, 3, WHITE);
      tft.setTextColor(WHITE);
      tft.setFreeFont(&FreeSans18pt7b);
      tft.setCursor(222, 466);
      tft.print("STOP");
    }
  }
}

void QuickMenu(int page) {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  if (page == 0) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(99, 48);
      tft.println("Quic Menu");
    } else if (Language == 1) {  //English
      tft.setCursor(101, 48);
      tft.println("Quic menu");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(75, 95);
      tft.println("Energieoptionen");
    } else if (Language == 1) {  //English
      tft.setCursor(76, 95);
      tft.println(" Energy options");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(7 + 40, 145);
      tft.println("Berechnungsgeschw.");
    } else if (Language == 1) {  //English
      tft.setCursor(8 + 40, 145);
      tft.println("   Calculation speed");
    }

    if (LEDMode == true) {
      tft.fillRoundRect(3, 164, 314, 44, 4, RED);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(112, 195);
        tft.println("LED aus");
      } else if (Language == 1) {  //English
        tft.setCursor(116, 195);
        tft.println("LED off");
      }
    } else {
      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREEN);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(122, 195);
        tft.println("LED an");
      } else if (Language == 1) {  //English
        tft.setCursor(118, 195);
        tft.println("LED on");
      }
    }

    tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(25 + 37, 245);
      tft.println("Zeitliche Zaehlung");
    } else if (Language == 1) {  //English
      tft.setCursor(27 + 37, 245);
      tft.println("    Timing count");
    }

    tft.fillRoundRect(3, 264, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 264, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(26 + 44, 295);
      tft.println("Display Helligkeit");
    } else if (Language == 1) {  //English
      tft.setCursor(29 + 33, 295);
      tft.println("Display brightness");
    }

    tft.fillRoundRect(3, 264 + 50, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 264 + 50, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(80, 295 + 50);
      tft.println("Display drehen");
    } else if (Language == 1) {  //English
      tft.setCursor(82, 295 + 50);
      tft.println("Rotate display");
    }

    if (TYPE != 1) {
      if (SelectIntExtTube == true) {
        tft.fillRoundRect(3, 264 + 100, 314, 44, 4, RED);
        tft.drawRoundRect(3, 264 + 100, 314, 44, 4, WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(58, 295 + 100);
          tft.println("Externes Zaehlrohr");
        } else if (Language == 1) {  //English
          tft.setCursor(40, 295 + 100);
          tft.println("External counting tube");
        }
      } else {
        tft.fillRoundRect(3, 264 + 100, 314, 44, 4, DKGREEN);
        tft.drawRoundRect(3, 264 + 100, 314, 44, 4, WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) {  //German
          tft.setCursor(65, 295 + 100);
          tft.println("Internes Zaehlrohr");
        } else if (Language == 1) {  //English
          tft.setCursor(44, 295 + 100);
          tft.println("Internal counting tube");
        }
      }
    }

    DrawButton(2, 0);

  } else if (page == 1) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(35 + 40, 48);
      tft.println("Energieoptionen");
    } else if (Language == 1) {  //English
      tft.setCursor(36 + 40, 48);
      tft.println(" Energy options");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, RED);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(55 + 40, 95);
      tft.println("Ausschalten");
    } else if (Language == 1) {  //English
      tft.setCursor(50 + 40, 95);
      tft.println("   Power off");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREEN);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(60 + 40, 145);
      tft.println("Neustarten");
    } else if (Language == 1) {  //English
      tft.setCursor(60 + 40, 145);
      tft.println("   Restart");
    }

    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREEN);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(87, 195);
      tft.println("Schlaf Modus");
    } else if (Language == 1) {  //English
      tft.setCursor(95, 195);
      tft.println("Sleep mode");
    }

    tft.fillRoundRect(3, 164 + 50, 314, 44, 4, DKGREEN);
    tft.drawRoundRect(3, 164 + 50, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(79, 195 + 50);
      tft.println("Energie sparen");
    } else if (Language == 1) {  //English
      tft.setCursor(78, 195 + 50);
      tft.println(" Energy saving");
    }

    DrawButton(0, 0);

  } else if (page == 2) {
    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(25 + 44, 48);
      tft.println("Display Helligkeit");
    } else if (Language == 1) {  //English
      tft.setCursor(27 + 33, 48);
      tft.println("Display brightness");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    if (Language == 0) {  //German
      tft.setCursor(18, 162);
      tft.print("Helligkeit in %");
    } else if (Language == 1) {  //English
      tft.setCursor(14, 162);
      tft.print("Brightness in %");
    }

    tft.setFreeFont(&FreeSans12pt7b);

    if (DBright >= 100) {
      tft.setCursor(170, 165);
      tft.println(DBright);
    } else {
      tft.setCursor(176, 165);
      tft.println(DBright);
    }


    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);
  }
}

void AverageSpeedPage() {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(7 + 40, 48);
    tft.println("Berechnungsgeschw.");
  } else if (Language == 1) {  //English
    tft.setCursor(8 + 40, 48);
    tft.println("   Calculation speed");
  }

  if (AverageSpeed == 0) {
    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
  } else {
    tft.fillRoundRect(3, 64, 314, 44, 4, BLACK);
  }
  tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(80 + 40, 95);
    tft.println("Normal");
  } else if (Language == 1) {  //English
    tft.setCursor(80 + 40, 95);
    tft.println("Normal");
  }

  if (AverageSpeed == 1) {
    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
  } else {
    tft.fillRoundRect(3, 114, 314, 44, 4, BLACK);
  }
  tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(78 + 40, 145);
    tft.println("Schnell");
  } else if (Language == 1) {  //English
    tft.setCursor(75 + 40, 145);
    tft.println("  Quick");
  }

  if (AverageSpeed == 2) {
    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
  } else {
    tft.fillRoundRect(3, 164, 314, 44, 4, BLACK);
  }
  tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(69 + 40, 195);
    tft.println("Langsam");
  } else if (Language == 1) {  //English
    tft.setCursor(71 + 40, 195);
    tft.println("   Slow");
  }

  DrawButton(0, 0);
}

void Settings(int page) {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  if (page == 0) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(49 + 40, 48);
      tft.println("Einstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(47 + 40, 48);
      tft.println("     Settings");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(18 + 40, 95);
      tft.println("Grundeinstellungen");  //Device(TubeSettings,Name,),Units,Time,language
    } else if (Language == 1) {           //English
      tft.setCursor(19 + 40, 95);
      tft.println("   Default settings");  //Device(TubeSettings,Name,),Units,Time,language
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(86 + 40, 145);
      tft.println("Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(86 + 40, 145);
      tft.println(" Alert");
    }

    if (TYPE != 1) {

      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(68 + 40, 195);
        tft.println("Micro SD");
      } else if (Language == 1) {  //English
        tft.setCursor(68 + 40, 195);
        tft.println("Micro SD");
      }

      tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(20 + 40, 245);
        tft.println("Touch Kalibrierung");
      } else if (Language == 1) {  //English
        tft.setCursor(23 + 40, 245);
        tft.println(" Touch calibration");
      }

      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214 + 50, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(88 + 40, 245 + 50);
        tft.println("Wi-Fi");
      } else if (Language == 1) {  //English
        tft.setCursor(88 + 40, 245 + 50);
        tft.println("Wi-Fi");
      }

      tft.fillRoundRect(3, 214 + 100, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214 + 100, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(69 + 40, 245 + 100);
        tft.println("Bluetooth");
      } else if (Language == 1) {  //English
        tft.setCursor(69 + 40, 245 + 100);
        tft.println("Bluetooth");
      }

      tft.fillRoundRect(3, 214 + 150, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214 + 150, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(77 + 40, 245 + 150);
        tft.println("System");
      } else if (Language == 1) {  //English
        tft.setCursor(77 + 40, 245 + 150);
        tft.println("System");
      }

    } else {
      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(20 + 40, 195);
        tft.println("Touch Kalibrierung");
      } else if (Language == 1) {  //English
        tft.setCursor(23 + 40, 195);
        tft.println(" Touch calibration");
      }

      tft.fillRoundRect(3, 164 + 50, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164 + 50, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(88 + 40, 195 + 50);
        tft.println("Wi-Fi");
      } else if (Language == 1) {  //English
        tft.setCursor(88 + 40, 195 + 50);
        tft.println("Wi-Fi");
      }

      tft.fillRoundRect(3, 164 + 100, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164 + 100, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(69 + 40, 195 + 100);
        tft.println("Bluetooth");
      } else if (Language == 1) {  //English
        tft.setCursor(69 + 40, 195 + 100);
        tft.println("Bluetooth");
      }

      tft.fillRoundRect(3, 164 + 150, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164 + 150, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(77 + 40, 195 + 150);
        tft.println("System");
      } else if (Language == 1) {  //English
        tft.setCursor(77 + 40, 195 + 150);
        tft.println("System");
      }
    }

    DrawButton(2, 0);
    //DrawButton(1, 2);

  } else if (page == 1) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(55, 48);
      tft.println("Bildschirm-Timeout");
    } else if (Language == 1) {  //English
      tft.setCursor(80, 48);
      tft.println("Screen timeout");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    if (Language == 0) {  //German
      tft.setCursor(10, 162);
      tft.print("Zeit in min: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 162);
      tft.print("Time in min: ");
    }

    tft.setFreeFont(&FreeSans12pt7b);

    if ((DTimeout / 60000) < 10) {
      tft.setCursor(184, 165);
      tft.println(DTimeout / 60000);
    } else if ((DTimeout / 60000) < 61) {
      tft.setCursor(176, 165);
      tft.println(DTimeout / 60000);
    } else {
      if (Language == 0) {  //German
        tft.setCursor(165, 165);
        tft.print("AUS");
      } else if (Language == 1) {  //English
        tft.setCursor(166, 165);
        tft.print("OFF");
      }
    }

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);

  } else if (page == 2) {  //Main Settings Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(18 + 40, 48);
      tft.println("Grundeinstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(19 + 40, 48);
      tft.println("   Default settings");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 40, 95);
      tft.println("Sprache");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 40, 95);
      tft.println("Language");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(72 + 40, 145);
      tft.println("Einheiten");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 40, 145);
      tft.println("   Units");
    }

    if (TYPE != 1) {

      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(43 + 40, 195);
        tft.println("Uhrzeit/Datum");
      } else if (Language == 1) {  //English
        tft.setCursor(41 + 40, 195);
        tft.println("    Time/Date");
      }

      tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(70 + 40, 245);
        tft.println("Zeahlrohr");
      } else if (Language == 1) {  //English
        tft.setCursor(59 + 40, 245);
        tft.println("Geiger tube");
      }

      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214 + 50, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(55, 245 + 50);
        tft.println("Bildschirm-Timeout");
      } else if (Language == 1) {  //English
        tft.setCursor(80, 245 + 50);
        tft.println("Screen timeout");
      }

      tft.fillRoundRect(3, 214 + 100, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 214 + 100, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(70 + 25, 245 + 100);
        tft.println("Akzentfarbe");
      } else if (Language == 1) {  //English
        tft.setCursor(94, 245 + 100);
        tft.println("Accent color");
      }

    } else {

      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(55, 195);
        tft.println("Bildschirm-Timeout");
      } else if (Language == 1) {  //English
        tft.setCursor(80, 195);
        tft.println("Screen timeout");
      }

      tft.fillRoundRect(3, 164 + 50, 314, 44, 4, DKGREY);
      tft.drawRoundRect(3, 164 + 50, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(70 + 25, 195 + 50);
        tft.println("Akzentfarbe");
      } else if (Language == 1) {  //English
        tft.setCursor(94, 195 + 50);
        tft.println("Accent color");
      }

    }

    DrawButton(0, 0);

  } else if (page == 3) {  //Language Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 40, 48);
      tft.println("Sprache");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 40, 48);
      tft.println("Language");
    }

    if (Language == 0) {
      tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 64, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 40, 95);
      tft.println("Deutsch");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 40, 95);
      tft.println(" German");
    }

    if (Language == 1) {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(76 + 40, 145);
      tft.println("Englisch");
    } else if (Language == 1) {  //English
      tft.setCursor(75 + 40, 145);
      tft.println(" English");
    }

    DrawButton(0, 0);

  } else if (page == 4) {  //Time/Date Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(43 + 40, 48);
      tft.println("Uhrzeit/Datum");
    } else if (Language == 1) {  //English
      tft.setCursor(41 + 40, 48);
      tft.println("    Time/Date");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(81 + 40, 95);
      tft.println("Uhrzeit");
    } else if (Language == 1) {  //English
      tft.setCursor(80 + 40, 95);
      tft.println("  Time");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(83 + 40, 145);
      tft.println("Datum");
    } else if (Language == 1) {  //English
      tft.setCursor(87 + 40, 145);
      tft.println(" Date");
    }

    DrawButton(0, 0);

  } else if (page == 5) {  //Time Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(81 + 40, 48);
      tft.println("Uhrzeit");
    } else if (Language == 1) {  //English
      tft.setCursor(80 + 40, 48);
      tft.println("  Time");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setCursor(15, 298);
    tft.print("Format: HH/MM/SS");

    tft.fillRect(0, 132, 240, 50, BLACK);

    tft.setCursor(32, 165);
    tft.setFreeFont(&FreeSans12pt7b);
    if (TMPHour < 10) {
      tft.print("0");
      tft.print(TMPHour);
    } else {
      tft.print(TMPHour);
    }

    tft.setCursor(80, 165);
    tft.print(":");

    tft.setCursor(107, 165);
    if (TMPMinute < 10) {
      tft.print("0");
      tft.print(TMPMinute);
    } else {
      tft.print(TMPMinute);
    }

    tft.setCursor(155, 165);
    tft.print(":");

    tft.setCursor(182, 165);
    if (TMPSecond < 10) {
      tft.print("0");
      tft.print(TMPSecond);
    } else {
      tft.print(TMPSecond);
    }



    tft.drawRoundRect(15, 70, 60, 60, 4, WHITE);  //Plus Button Hour
    tft.fillRoundRect(16, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(15, 185, 60, 60, 4, WHITE);  //Minus Button Hour
    tft.fillRoundRect(16, 186, 58, 58, 4, DKGREY);

    tft.setCursor(24, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(33, 234);
    tft.println("-");
    tft.setTextSize(1);



    tft.drawRoundRect(90, 70, 60, 60, 4, WHITE);  //Plus Button Minute
    tft.fillRoundRect(91, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(90, 185, 60, 60, 4, WHITE);  //Minus Button Minute
    tft.fillRoundRect(91, 186, 58, 58, 4, DKGREY);

    tft.setCursor(99, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(108, 234);
    tft.println("-");
    tft.setTextSize(1);



    tft.drawRoundRect(165, 70, 60, 60, 4, WHITE);  //Plus Button Second
    tft.fillRoundRect(166, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(165, 185, 60, 60, 4, WHITE);  //Minus Button Second
    tft.fillRoundRect(166, 186, 58, 58, 4, DKGREY);

    tft.setCursor(174, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(183, 234);
    tft.println("-");
    tft.setTextSize(1);



    DrawButton(0, 0);

  } else if (page == 6) {  //Date Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(83 + 40, 48);
      tft.println("Datum");
    } else if (Language == 1) {  //English
      tft.setCursor(87 + 40, 48);
      tft.println(" Date");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    tft.setCursor(15, 298);
    tft.print("Format: YY/MM/DD");


    tft.fillRect(0, 132, 240, 50, BLACK);

    tft.setCursor(32, 165);
    tft.setFreeFont(&FreeSans12pt7b);
    if (TMPYear < 10) {
      tft.print("0");
      tft.print(TMPYear);
    } else {
      tft.print(TMPYear);
    }

    tft.setCursor(80, 165);
    tft.print("/");

    tft.setCursor(107, 165);
    if (TMPMonth < 10) {
      tft.print("0");
      tft.print(TMPMonth);
    } else {
      tft.print(TMPMonth);
    }

    tft.setCursor(155, 165);
    tft.print("/");

    tft.setCursor(182, 165);
    if (TMPDay < 10) {
      tft.print("0");
      tft.print(TMPDay);
    } else {
      tft.print(TMPDay);
    }



    tft.drawRoundRect(15, 70, 60, 60, 4, WHITE);  //Plus Button Hour
    tft.fillRoundRect(16, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(15, 185, 60, 60, 4, WHITE);  //Minus Button Hour
    tft.fillRoundRect(16, 186, 58, 58, 4, DKGREY);

    tft.setCursor(24, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(33, 234);
    tft.println("-");
    tft.setTextSize(1);



    tft.drawRoundRect(90, 70, 60, 60, 4, WHITE);  //Plus Button Minute
    tft.fillRoundRect(91, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(90, 185, 60, 60, 4, WHITE);  //Minus Button Minute
    tft.fillRoundRect(91, 186, 58, 58, 4, DKGREY);

    tft.setCursor(99, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(108, 234);
    tft.println("-");
    tft.setTextSize(1);



    tft.drawRoundRect(165, 70, 60, 60, 4, WHITE);  //Plus Button Second
    tft.fillRoundRect(166, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(165, 185, 60, 60, 4, WHITE);  //Minus Button Second
    tft.fillRoundRect(166, 186, 58, 58, 4, DKGREY);

    tft.setCursor(174, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(183, 234);
    tft.println("-");
    tft.setTextSize(1);



    DrawButton(0, 0);

  } else if (page == 7) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(70 + 40, 48);
      tft.println("Zeahlrohr");
    } else if (Language == 1) {  //English
      tft.setCursor(59 + 40, 48);
      tft.println("Geiger tube");
    }

    if (GeigerTubeModel == 0) {
      tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 64, 314, 44, 4, TFT_BLACK);
    }

    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(50 + 66, 95);
    tft.println("SBM-20");

    if (GeigerTubeModel == 1) {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, TFT_BLACK);
    }

    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(39 + 40, 145);
    tft.println("SBM-19/STS-6");

    if (GeigerTubeModel == 2) {
      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 164, 314, 44, 4, TFT_BLACK);
    }

    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(53 + 31, 195);
    tft.println("15 mm (Typ B)");


    if (GeigerTubeModel == 3) {
      tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 214, 314, 44, 4, TFT_BLACK);
    }

    tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(53 + 37, 245);
    tft.println("Ludlum 44-9");


    if (GeigerTubeModel == 4) {
      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, TFT_BLACK);
    }

    tft.drawRoundRect(3, 214 + 50, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(53 + 53, 245 + 50);
      tft.println("Individuell");
    } else if (Language == 1) {  //English
      tft.setCursor(53 + 62, 245 + 50);
      tft.println("Custom");
    }

    tft.fillRoundRect(3, 214 + 100, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 214 + 100, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(35, 245 + 100);
      tft.println("Individuell Einstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(74, 245 + 100);
      tft.println("Custom settings");
    }

    DrawButton(0, 0);

  } else if (page == 8) {  //Units Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(72 + 40, 48);
      tft.println("Einheiten");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 40, 48);
      tft.println("   Units");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(89 + 40, 95);
      tft.println("Dosis");
    } else if (Language == 1) {  //English
      tft.setCursor(85 + 40, 95);
      tft.println(" Dose");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(97 + 40, 145);
      tft.println("Zeit");
    } else if (Language == 1) {  //English
      tft.setCursor(93 + 40, 145);
      tft.println("Time");
    }

    DrawButton(0, 0);

  } else if (page == 9) {  //Dose Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(89 + 40, 48);
      tft.println("Dosis");
    } else if (Language == 1) {  //English
      tft.setCursor(85 + 40, 48);
      tft.println(" Dose");
    }

    if (DoseUnit == 0) {
      tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 64, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(88 + 40, 95);
    tft.println("Sivert");

    if (DoseUnit == 1) {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    tft.setCursor(93 + 40, 145);
    tft.println("Rem");

    DrawButton(0, 0);

  } else if (page == 10) {  //Time Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(97 + 40, 48);
      tft.println("Zeit");
    } else if (Language == 1) {  //English
      tft.setCursor(93 + 40, 48);
      tft.println("Time");
    }

    if (TimeFormat == 0) {
      tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 64, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(57 + 40, 95);
      tft.println("24h Format");
    } else if (Language == 1) {  //English
      tft.setCursor(62 + 40, 95);
      tft.println("24h format");
    }

    if (TimeFormat == 1) {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, TFT_BLACK);
    }
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(57 + 40, 145);
      tft.println("12h Format");
    } else if (Language == 1) {  //English
      tft.setCursor(62 + 40, 145);
      tft.println("12h format");
    }

    DrawButton(0, 0);

  } else if (page == 11) {  //Bluetooth page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(69 + 40, 48);
      tft.println("Bluetooth");
    } else if (Language == 1) {  //English
      tft.setCursor(69 + 40, 48);
      tft.println("Bluetooth");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(48 + 40, 95);
      tft.println("Bluetooth Info");
    } else if (Language == 1) {  //English
      tft.setCursor(48 + 40, 95);
      tft.println("Bluetooth info");
    }

      if (BLEStat >= 1) {
      tft.fillRoundRect(3, 114, 314, 44, 4, RED);
      tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) { //German
        tft.setCursor(49 + 40, 145);
        tft.println("Bluetooth aus");
      } else if (Language == 1) { //English
        tft.setCursor(49 + 40, 145);
        tft.println("Bluetooth off");
      }
      } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREEN);
      tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) { //German
        tft.setCursor(54 + 40, 145);
        tft.println("Bluetooth an");
      } else if (Language == 1) { //English
        tft.setCursor(54 + 40, 145);
        tft.println("Bluetooth on");
      }
      }

    DrawButton(0, 0);  //Back Button

  } else if (page == 12) {

////////////////////////////////////////////////////////////////////////////////////FREI/////////////////////////////////////////////////////////////////

  } else if (page == 13) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(48 + 40, 48);
      tft.println("Bluetooth Info");
    } else if (Language == 1) {  //English
      tft.setCursor(48 + 40, 48);
      tft.println("Bluetooth Info");
    }

    tft.fillRoundRect(3, 63, 314, 75, 4, DKGREY);
    tft.drawRoundRect(3, 63, 314, 75, 4, WHITE);

    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 87);
      tft.print("BLE Version: 4.2");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 87);
      tft.print("BLE version: 4.2");
    }

    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 107);
      tft.print("BLE Name: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 107);
      tft.print("BLE name: ");
    }

    String stringOne = String(GetSerialNumber()); 

    String BLEDeviceName = String("GMZ-06_" + stringOne);
    
    tft.println(BLEDeviceName);


      tft.setFreeFont(&FreeSans9pt7b); //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) { //German
      tft.setCursor(10, 127);
      tft.print("BLE Status: ");
      if (BLEStat == 1) {
        tft.setTextColor(DKGREEN);
        tft.println("Aktiv");
      } else if (BLEStat == 2) {
        tft.setTextColor(GREEN);
        tft.println("Verbunden");
      } else {
        tft.setTextColor(RED);
        tft.println("AUS");
      }
      } else if (Language == 1) { //English
      tft.setCursor(10, 127);
      tft.print("BLE status: ");
      if (BLEStat == 1) {
        tft.setTextColor(DKGREEN);
        tft.println("Active");
      } else if (BLEStat == 2) {
        tft.setTextColor(GREEN);
        tft.println("Connected");
      } else {
        tft.setTextColor(RED);
        tft.println("OFF");
      }
      }

    DrawButton(0, 0);

  } else if (page == 14) {  //Micro SD settings page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(68 + 40, 48);
      tft.println("Micro SD");
    } else if (Language == 1) {  //English
      tft.setCursor(68 + 40, 48);
      tft.println("Micro SD");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(46 + 40, 95);
      tft.println("Micro SD Info");
    } else if (Language == 1) {  //English
      tft.setCursor(46 + 40, 95);
      tft.println("Micro SD info");
    }

    /*

      if (SDSystemLog == true) {
      tft.fillRoundRect(3, 114, 314, 44, 4, RED);
      tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) { //German
        tft.setCursor(34 + 40, 145);
        tft.println("System Log aus");
      } else if (Language == 1) { //English
        tft.setCursor(44 + 40, 145);
        tft.println("System log off");
      }
      } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREEN);
      tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
      tft.setTextDatum(MC_DATUM);
      tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) { //German
        tft.setCursor(41 + 40, 145);
        tft.println("System Log an");
      } else if (Language == 1) { //English
        tft.setCursor(45 + 40, 145);
        tft.println("System log on");
      }
      }

    */

    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(38 + 40, 195);
      tft.println("Micro SD Tools");
    } else if (Language == 1) {  //English
      tft.setCursor(41 + 40, 195);
      tft.println("Micro SD tools");
    }

    DrawButton(0, 0);  //Back Button

  } else if (page == 15) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(46 + 40, 48);
      tft.println("Micro SD Info");
    } else if (Language == 1) {  //English
      tft.setCursor(46 + 40, 48);
      tft.println("Micro SD info");
    }

    if (SDStat == true) {

      tft.fillRoundRect(3, 63, 314, 120, 4, DKGREY);
      tft.drawRoundRect(3, 63, 314, 120, 4, WHITE);

      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 87);
        tft.print("Gesamtspeicher: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 87);
        tft.print("Total space: ");
      }
      //tft.print(SDTotalBytes());
      tft.println("MB");


      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 107);
        tft.print("SD Kartengr.: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 107);
        tft.print("SD card size: ");
      }

      //tft.print(SDCardSize());
      tft.println("MB");


      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 127);
        tft.print("Genutzter Platz: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 127);
        tft.print("Used space: ");
      }
      //tft.print(SDUsedSpace());
      tft.println("MB");

      /*

        tft.setFreeFont(&FreeSans9pt7b); //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);
        if (Language == 0) { //German
        tft.setCursor(10, 147);
        tft.print("SD Status: ");
        if (SDAvailable() == true) {
          tft.setTextColor(GREEN);
          tft.println("AKTIV");
        } else {
          tft.setTextColor(RED);
          tft.println("PASSIV");
        }
        } else if (Language == 1) { //English
        tft.setCursor(10, 147);
        tft.print("SD status: ");
        if (SDAvailable() == true) {
          tft.setTextColor(GREEN);
          tft.println("ACTIVE");
        } else {
          tft.setTextColor(RED);
          tft.println("PASSIVE");
        }
        }

      */

      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 167);
        tft.print("SD Kartentyp: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 167);
        tft.print("SD card type: ");
      }

      /*
        if (SDCardType() == 1) {
        tft.println("MMC");
        } else if (SDCardType() == 2) {
        tft.println("SDSC");
        } else if (SDCardType() == 3) {
        tft.println("SDHC");
        } else {
        if (Language == 0) { //German
          tft.println("UNBEKANNT");
        } else if (Language == 1) { //English
          tft.println("UNKNOWN");
        }
        }

      */

    } else {

      tft.fillRoundRect(3, 63, 314, 120, 4, DKGREY);
      tft.drawRoundRect(3, 63, 314, 120, 4, WHITE);

      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 87);
        tft.print("Gesamtspeicher: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 87);
        tft.print("Total space: ");
      }
      tft.println("?");


      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 107);
        tft.print("SD Kartengr.: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 107);
        tft.print("SD card size: ");
      }

      tft.println("?");


      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 127);
        tft.print("Genutzter Platz: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 127);
        tft.print("Used space: ");
      }
      tft.println("?");


      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 147);
        tft.print("SD Status: ");
        tft.setTextColor(RED);
        tft.println("PASSIV");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 147);
        tft.print("SD status: ");
        tft.setTextColor(RED);
        tft.println("PASSIVE");
      }

      tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
      tft.setTextSize(1);
      tft.setTextColor(WHITE);
      if (Language == 0) {  //German
        tft.setCursor(10, 167);
        tft.print("SD Kartentyp: ");
      } else if (Language == 1) {  //English
        tft.setCursor(10, 167);
        tft.print("SD card type: ");
      }

      tft.println("?");
    }

    DrawButton(0, 0);

  } else if (page == 16) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(38 + 40, 48);
      tft.println("Micro SD Tools");
    } else if (Language == 1) {  //English
      tft.setCursor(41 + 40, 48);
      tft.println("Micro SD tools");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(65 + 40, 95);
      tft.println("SD Check");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 40, 95);
      tft.println("SD check");
    }

    /*
        if (SDSystemLog == true) {
          tft.fillRoundRect(3, 114, 234, 44, 4, RED);
          tft.drawRoundRect(3, 114, 234, 44, 4, WHITE);
          tft.setTextDatum(MC_DATUM);
          tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
          tft.setTextSize(1);
          tft.setTextColor(WHITE);
          if (Language == 0) { //German
            tft.setCursor(34, 145);
            tft.println("System Log aus");
          } else if (Language == 1) { //English
            tft.setCursor(44, 145);
            tft.println("System log off");
          }
        } else {
          tft.fillRoundRect(3, 114, 234, 44, 4, DKGREEN);
          tft.drawRoundRect(3, 114, 234, 44, 4, WHITE);
          tft.setTextDatum(MC_DATUM);
          tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
          tft.setTextSize(1);
          tft.setTextColor(WHITE);
          if (Language == 0) { //German
            tft.setCursor(41, 145);
            tft.println("System Log an");
          } else if (Language == 1) { //English
            tft.setCursor(45, 145);
            tft.println("System log on");
          }
        }

        tft.fillRoundRect(3, 164, 234, 44, 4, DKGREY);
        tft.drawRoundRect(3, 164, 234, 44, 4, WHITE);
        tft.setTextDatum(MC_DATUM);
        tft.setFreeFont(&FreeSans12pt7b); //9,12,18,24pt
        tft.setTextSize(1);
        tft.setTextColor(WHITE);

        if (Language == 0) { //German
          tft.setCursor(38, 195);
          tft.println("Micro SD Tools");
        } else if (Language == 1) { //English
          tft.setCursor(41, 195);
          tft.println("Micro SD tools");
        }

    */

    DrawButton(0, 0);  //Back Button

  } else if (page == 17) {  //Alert Page

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(86 + 40, 48);
      tft.println("Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(86 + 40, 48);
      tft.println(" Alert");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(53 + 40, 95);
      tft.println("Dosis Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(62 + 40, 95);
      tft.println("Dose alert");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(30 + 40, 145);
      tft.println("Ges. Dosis Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(37 + 40, 145);
      tft.println("Total dose alert");
    }

    DrawButton(0, 0);  //Back Button

  } else if (page == 18) {  //Dose alert

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(53 + 40, 48);
      tft.println("Dosis Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(62 + 40, 48);
      tft.println("Dose alert");
    }

    if (DoseUnit == 1) {
      tft.setFreeFont(&FreeSans9pt7b);
      if (Language == 0) {  //German
        tft.setCursor(10, 162);
        tft.print("Dosis in: ");
      } else if (Language == 1) {  //English
        tft.setCursor(11, 162);
        tft.print("Dose in: ");
      }
    } else {
      tft.setFreeFont(&FreeSans9pt7b);
      if (Language == 0) {  //German
        tft.setCursor(19, 162);
        tft.print("Dosis in: ");
      } else if (Language == 1) {  //English
        tft.setCursor(19, 162);
        tft.print("Dose in: ");
      }
    }

    if (DoseUnit == 0) {
      tft.println("uSv/h");
    } else if (DoseUnit == 1) {
      tft.println("mRem/h");
    } else if (DoseUnit == 2) {
      tft.println("uGy/h");
    }

    tft.setFreeFont(&FreeSans12pt7b);

    if (DoseUnit == 0) {
      tft.setCursor(176, 165);
      tft.println(DoseAlertRate);
    } else if (DoseUnit == 1) {
      tft.setCursor(167, 165);
      tft.println(float(DoseAlertRate) * 0.1);
    } else if (DoseUnit == 2) {
      tft.setCursor(176, 165);
      tft.println(DoseAlertRate);
    }

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);

  } else if (page == 19) {  //Total dose alert

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(30 + 40, 48);
      tft.println("Ges. Dosis Alarm");
    } else if (Language == 1) {  //English
      tft.setCursor(37 + 40, 48);
      tft.println("Total dose alert");
    }

    if (DoseUnit == 1) {
      tft.setFreeFont(&FreeSans9pt7b);
      if (Language == 0) {  //German
        tft.setCursor(13, 162);
        tft.print("Dosis in: ");
      } else if (Language == 1) {  //English
        tft.setCursor(14, 162);
        tft.print("Dose in: ");
      }
    } else {
      tft.setFreeFont(&FreeSans9pt7b);
      if (Language == 0) {  //German
        tft.setCursor(22, 162);
        tft.print("Dosis in: ");
      } else if (Language == 1) {  //English
        tft.setCursor(22, 162);
        tft.print("Dose in: ");
      }
    }

    if (DoseUnit == 0) {
      tft.println("uSv");
    } else if (DoseUnit == 1) {
      tft.println("mRem");
    } else if (DoseUnit == 2) {
      tft.println("uGy");
    }

    tft.setFreeFont(&FreeSans12pt7b);

    if (DoseUnit == 0) {
      tft.setCursor(176, 165);
      tft.println(TotalDoseAlertRate);
    } else if (DoseUnit == 1) {
      tft.setCursor(167, 165);
      tft.println(float(TotalDoseAlertRate) * 0.1);
    } else if (DoseUnit == 2) {
      tft.setCursor(176, 165);
      tft.println(TotalDoseAlertRate);
    }

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);

  } else if (page == 20) {  //System tab

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 40, 48);
      tft.println("System");
    } else if (Language == 1) {  //English
      tft.setCursor(77 + 40, 48);
      tft.println("System");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(44 + 40, 95);
      tft.println("System Daten");
    } else if (Language == 1) {  //English
      tft.setCursor(52 + 40, 95);
      tft.println("System data");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 40, 145);
      tft.println("Update");
    } else if (Language == 1) {  //English
      tft.setCursor(77 + 40, 145);
      tft.println("Update");
    }

    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(17 + 40, 195);
      tft.println("Werkseinstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(37 + 40, 195);
      tft.println("Factory settings");
    }

    DrawButton(0, 0);  //Back Button

  } else if (page == 21) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(44 + 40, 48);
      tft.println("System Daten");
    } else if (Language == 1) {  //English
      tft.setCursor(52 + 40, 48);
      tft.println("System data");
    }

    tft.fillRoundRect(3, 63, 314, 178 + 60, 4, DKGREY);
    tft.drawRoundRect(3, 63, 314, 178 + 60, 4, WHITE);

    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 87);
      tft.print("Version: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 87);
      tft.print("Version: ");
    }
    tft.print("GeigerOS V");
    tft.println(GEIGEROS_VERSION);


    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 107);
      tft.print("Modell: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 107);
      tft.print("Model: ");
    }

    tft.print("GMZ-0");
    tft.print(GMZ_MODEL_NR);
    if (TYPE == 0) {
      tft.print(" PRO");
    } else if (TYPE == 1) {
      tft.print(" Lite");
    } else if (TYPE == 2) {
      tft.print(" Lite+");
    }


    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(10, 127);
      tft.print("Hardware: ");
    } else if (Language == 1) {  //English
      tft.setCursor(10, 127);
      tft.print("Hardware: ");
    }
    if (TYPE == 0) {
      tft.setCursor(10, 147);
      tft.print("GMT internal: J305 80mm");
      tft.setCursor(10, 167);
      tft.print("CPU: ESP32 4MB");
      tft.setCursor(10, 187);
      tft.print("Battery: 2600mAh ");
      if (BATPercent <= 15) {
        tft.setTextColor(RED);
      } else {
        tft.setTextColor(GREEN);
      }
      tft.print(BATVoltage);
      tft.print("V");
      tft.setTextColor(WHITE);
      tft.setCursor(10, 207);
      tft.print("Display: 320x480px");
      tft.setCursor(10, 227);
      if (Language == 0) {  //German
        tft.print("Andere: RTC, SD, USB-C");
      } else if (Language == 1) {  //English
        tft.print("Other: RTC, SD, USB-C");
      }
    } else if (TYPE == 1) {
      tft.setCursor(10, 147);
      tft.print("GMT internal: J305 80mm");
      tft.setCursor(10, 167);
      tft.print("CPU: ESP32 4MB");
      tft.setCursor(10, 187);
      tft.print("Battery: 2600mAh ");
      if (BATPercent <= 15) {
        tft.setTextColor(RED);
      } else {
        tft.setTextColor(GREEN);
      }
      tft.print(BATVoltage);
      tft.print("V");
      tft.setTextColor(WHITE);
      tft.setCursor(10, 207);
      tft.print("Display: 320x480px");
    } else if (TYPE == 2) {
      tft.setCursor(10, 147);
      tft.print("CPU: ESP32 4MB");
      tft.setCursor(10, 167);
      tft.print("Battery: 2600mAh ");
      if (BATPercent <= 15) {
        tft.setTextColor(RED);
      } else {
        tft.setTextColor(GREEN);
      }
      tft.print(BATVoltage);
      tft.print("V");
      tft.setTextColor(WHITE);
      tft.setCursor(10, 187);
      tft.print("Display: 320x480px");
      tft.setCursor(10, 207);
      if (Language == 0) {  //German
        tft.print("Andere: RTC, SD, USB-C");
      } else if (Language == 1) {  //English
        tft.print("Other: RTC, SD, USB-C");
      }
    }

    tft.setCursor(10, 227 + 20);
    tft.print("SN: ");
    tft.println(GetSerialNumber());

    tft.setCursor(10, 227 + 40);
    tft.print("Restricted mode: ");
    if (RestrictedModeStat == true) {
      tft.setTextColor(RED);
      if (Language == 0) {  //German
        tft.print("AN");
      } else if (Language == 1) {  //English
        tft.print("ON");
      }
    } else {
      tft.setTextColor(GREEN);
      if (Language == 0) {  //German
        tft.print("AUS");
      } else if (Language == 1) {  //English
        tft.print("OFF");
      }
    }

    tft.setCursor(10, 227 + 60);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.print("Systemlaufzeit: ");
    } else if (Language == 1) {  //English
      tft.print("System runtime: ");
    }
    tft.setTextColor(GREEN);
    tft.print(millis() / 1000);
    tft.print("s");

    DrawButton(0, 0);

  } else if (page == 22) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(17 + 40, 48);
      tft.println("Werkseinstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(37 + 40, 48);
      tft.println("Factory settings");
    }

    tft.fillRoundRect(0, 64, 320, 94, 4, DKGREY);

    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(48 + 40, 83);
      tft.println("WARNUNG!");
    } else if (Language == 1) {  //English
      tft.setCursor(52 + 40, 83);
      tft.println("WARNING!");
    }

    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(RED);
    if (Language == 0) {  //German
      tft.setCursor(0, 108);
      tft.print("Wollen Sie wirklich alle Einstellungen des Gereates zuruecksetzen?");
    } else if (Language == 1) {  //English
      tft.setCursor(0, 108);
      tft.print("Do you really want to reset all settings of this device?");
    }


    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREEN);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(61 + 40, 195);
      tft.println("Zustimmen");
    } else if (Language == 1) {  //English
      tft.setCursor(62 + 40, 195);
      tft.println("    Agree");
    }

    tft.fillRoundRect(3, 214, 314, 44, 4, RED);
    tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(67 + 40, 245);
      tft.println("Ablehnen");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 40, 245);
      tft.println(" Decline");
    }

  } else if (page == 23) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(88 + 40, 48);
      tft.println("Wi-Fi");
    } else if (Language == 1) {  //English
      tft.setCursor(88 + 40, 48);
      tft.println("Wi-Fi");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(67 + 40, 95);
      tft.println("Wi-Fi Info");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 40, 95);
      tft.println("Wi-Fi info");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(73 + 40, 145);
      tft.println("Wi-Fi an");
    } else if (Language == 1) {  //English
      tft.setCursor(73 + 40, 145);
      tft.println("Wi-Fi on");
    }

    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(56 + 40, 195);
      tft.println("Wi-Fi Setup");
    } else if (Language == 1) {  //English
      tft.setCursor(54 + 40, 195);
      tft.println("Wi-Fi setup");
    }

    tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(55 + 40, 245);
      tft.println("Wi-Fi Log an");
    } else if (Language == 1) {  //English
      tft.setCursor(55 + 40, 245);
      tft.println("Wi-Fi log on");
    }

    DrawButton(0, 0);  //Back Button

  } else if (page == 24) {  //Custom GMT settings

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(35, 48);
      tft.println("Individuell Einstellungen");
    } else if (Language == 1) {  //English
      tft.setCursor(74, 48);
      tft.println("Custom settings");
    }

    tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(67 + 38, 95);
      tft.println("Spannung");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 50, 95);
      tft.println("Voltage");
    }

    tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(73 + 36, 145);
      tft.println("Nulleffekt");
    } else if (Language == 1) {  //English
      tft.setCursor(71 + 30, 145);
      tft.println("Zero effect");
    }

    tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);

    if (Language == 0) {  //German
      tft.setCursor(75, 195);
      tft.println("Auto Kalibrieren");
    } else if (Language == 1) {  //English
      tft.setCursor(84, 195);
      tft.println("Auto calibrate");
    }

    DrawButton(0, 0);  //Back Button

  } else if (page == 25) {  //Voltage settings GMT

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(67 + 38, 48);
      tft.println("Spannung");
    } else if (Language == 1) {  //English
      tft.setCursor(67 + 50, 48);
      tft.println("Voltage");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    if (Language == 0) {  //German
      tft.setCursor(13, 162);
      tft.print("Spannung in: V");
    } else if (Language == 1) {  //English
      tft.setCursor(14, 162);
      tft.print("Voltage in: V");
    }

    tft.setFreeFont(&FreeSans12pt7b);


    tft.setCursor(167, 165);
    if (CustomHVLevel == 0) {
      tft.println("250");
    } else if (CustomHVLevel == 1) {
      tft.println("300");
    } else if (CustomHVLevel == 2) {
      tft.println("350");
    } else if (CustomHVLevel == 3) {
      tft.println("400");
    } else if (CustomHVLevel == 4) {
      tft.println("450");
    }

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);  //Back Button

  } else if (page == 26) {  //CustomZeroEffect set

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(73 + 36, 48);
      tft.println("Nulleffekt");
    } else if (Language == 1) {  //English
      tft.setCursor(71 + 30, 48);
      tft.println("Zero effect");
    }

    tft.setFreeFont(&FreeSans9pt7b);
    if (Language == 0) {  //German
      tft.setCursor(10, 162);
      tft.print("Nulleffekt in: CPM");
    } else if (Language == 1) {  //English
      tft.setCursor(9, 162);
      tft.print("Zero effect in: CPM");
    }

    tft.setFreeFont(&FreeSans12pt7b);


    tft.setCursor(167, 165);
    tft.println(CustomZeroEffect);

    tft.drawRoundRect(160, 70, 60, 60, 4, WHITE);  //Plus Button
    tft.fillRoundRect(161, 71, 58, 58, 4, DKGREY);

    tft.drawRoundRect(160, 185, 60, 60, 4, WHITE);  //Minus Button
    tft.fillRoundRect(161, 186, 58, 58, 4, DKGREY);

    tft.setCursor(169, 114);
    tft.setFreeFont(&FreeSans12pt7b);
    tft.setTextSize(3);
    tft.println("+");
    tft.setCursor(178, 234);
    tft.println("-");
    tft.setTextSize(1);

    DrawButton(0, 0);  //Back Button
  } else if (page == 27) {

    tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(70 + 25, 48);
      tft.println("Akzentfarbe");
    } else if (Language == 1) {  //English
      tft.setCursor(94, 48);
      tft.println("Accent color");
    }

    if (AccentColor == RED) {
      tft.fillRoundRect(3, 64, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 64, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 64, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(77 + 60, 95);
      tft.println("Rot");
    } else if (Language == 1) {  //English
      tft.setCursor(77 + 60, 95);
      tft.println("Red");
    }

    if (AccentColor == DKGREEN) {
      tft.fillRoundRect(3, 114, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 114, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 114, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(72 + 52, 145);
      tft.println("Gruen");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 54, 145);
      tft.println("Green");
    }

    if (AccentColor == BLUE) {
      tft.fillRoundRect(3, 164, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 164, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(43 + 90, 195);
      tft.println("Blau");
    } else if (Language == 1) {  //English
      tft.setCursor(43 + 91, 195);
      tft.println("Blue");
    }

    if (AccentColor == DKYELLOW) {
      tft.fillRoundRect(3, 214, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 214, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(70 + 62, 245);
      tft.println("Gelb");
    } else if (Language == 1) {  //English
      tft.setCursor(72 + 50, 245);
      tft.println("Yellow");
    }

    if (AccentColor == DIGITAL_BLUE) {
      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 214 + 50, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 214 + 50, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(55 + 44, 245 + 50);
      tft.println("Digital Blau");
    } else if (Language == 1) {  //English
      tft.setCursor(55 + 44, 245 + 50);
      tft.println("Digital Blue");
    }

    if (AccentColor == ORANGE) {
      tft.fillRoundRect(3, 214 + 100, 314, 44, 4, DKGREY);
    } else {
      tft.fillRoundRect(3, 214 + 100, 314, 44, 4, BLACK);
    }
    tft.drawRoundRect(3, 214 + 100, 314, 44, 4, WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
    tft.setTextSize(1);
    tft.setTextColor(WHITE);
    if (Language == 0) {  //German
      tft.setCursor(70 + 48, 245 + 100);
      tft.println("Orange");
    } else if (Language == 1) {  //English
      tft.setCursor(70 + 48, 245 + 100);
      tft.println("Orange");
    }

    DrawButton(0, 0);

  }
}


void TouchCalApprovalPage() {

  if (ClearPage == true) {
    tft.fillRect(0, 0, 320, 480, BLACK);
    ClearPage = false;
  }

  DrawInfoBar();

  tft.fillRoundRect(3, 23, 314, 35, 3, AccentColor);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(20 + 40, 48);
    tft.println("Touch Kalibrierung");
  } else if (Language == 1) {  //English
    tft.setCursor(23 + 40, 48);
    tft.println(" Touch calibration");
  }

  tft.fillRoundRect(0, 64, 320, 94, 4, DKGREY);

  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(RED);
  if (Language == 0) {  //German
    tft.setCursor(48 + 40, 84);
    tft.println("WARNUNG!");
  } else if (Language == 1) {  //English
    tft.setCursor(52 + 40, 84);
    tft.println("WARNING!");
  }

  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans9pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(0, 107);
    tft.print("Falsche Kalibrierung beeinträchtigt die Funktion! Verwenden Sie einen Stift fuer praezise Display-Kalibrierung.");
  } else if (Language == 1) {  //English
    tft.setCursor(0, 107);
    tft.print("Improper calibration can affect device functionality! We recommend using a stylus for precise display calibration.");
  }


  tft.fillRoundRect(3, 164, 314, 44, 4, DKGREEN);
  tft.drawRoundRect(3, 164, 314, 44, 4, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(61 + 40, 195);
    tft.println("Zustimmen");
  } else if (Language == 1) {  //English
    tft.setCursor(62 + 40, 195);
    tft.println("    Agree");
  }

  tft.fillRoundRect(3, 214, 314, 44, 4, RED);
  tft.drawRoundRect(3, 214, 314, 44, 4, WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSans12pt7b);  //9,12,18,24pt
  tft.setTextSize(1);
  tft.setTextColor(WHITE);
  if (Language == 0) {  //German
    tft.setCursor(67 + 40, 245);
    tft.println("Ablehnen");
  } else if (Language == 1) {  //English
    tft.setCursor(72 + 40, 245);
    tft.println(" Decline");
  }
}

/////////////////////////////////////////////
