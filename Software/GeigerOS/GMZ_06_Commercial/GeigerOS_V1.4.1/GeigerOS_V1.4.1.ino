/////////////////////////////////////////////
//         GeigerOS by Marvin Ramm         //
//               REV: V1.4.1               //
//     https://github.com/codemarv42/      //
//  https://github.com/codemarv42/GeigerOS //
//        Last modified: 13.09.2023        //
//      © Copyright 2023 CC-BY-NC-SA       //
/////////////////////////////////////////////

/*
   Arduino ESP32 Settings:
   Board: "ESP32 Dev Module"
   Upload Speed: "921600"
   CPU Frequency: "240MHz (WiFi/BT)"
   Flash Frequency: "80MHz"
   Flash Mode: "QIO"
   Flash Size: "4MB"
   Partition Scheme: "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
   Programmer: "AVRISP mkll"
*/

/*
   Improvements:
   -Speicher Test und Esp32 Test (RSSI, Psram, flash, wifi, bt)
   -Screen anzeigen wenn SD geupdated wird. auf RST EN aufpassen !! nicht vor Fuktion zum suchen von Firmware
   -In normaler GeigerOS Software, GeigerOS Test implementieren
   -Display sperre Symbol auf Timing Count page und Mute Button
   -GMZ Tube auto calibration
   -first boot Page mit, Sprache auswählen -> bedanken, dass sie den gmz nutzen (extra bit in EEprom anlegen)
   -SD Insertion manchmal nicht richtig mounten, wahrscheinlich delay nach Erkennung der Einlegung der SD
   -Akku ladevorgänge
   -Lab View implementierung
   -overflow einprogrammieren
   -CPM, CPS switch in Einstellungen
   -stand geigerzähler entwickeln mit wifi anbindung und lan + SD card mit Data upload
   -GMZ bei niedrigem Akku ausschalten
   -Information Bar unterschiedliche designs
   -Serial UI bzw um tests usw zu starten von chatgpt entwickeln
   -Variablen für Display Touch Bereich
   -Micro SD delete button
*/


///////////////////Includes//////////////////

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"
#include "ESP32_BLE.h"

//////////////////Variables//////////////////

unsigned long PreviousMillis1;
unsigned long PreviousMillis3;  //PreviousMillis4 in use
unsigned long PreviousMillis5;

/////////////////////////////////////////////

////////////////////Setup////////////////////

void setup() {
  HardwareInit();
}

/////////////////////////////////////////////

////////////////////Loop/////////////////////

void loop() {

  if (millis() - PreviousMillis1 >= 1000) {
    PreviousMillis1 = millis();

    CalculateBATVoltage();
    CalculateCPM();
    CalculateDose();
    SetFullDoseLevel();
    SetDoseLevel();
    SetMaxDoseRate();
    UpdateSDStat();
    UpdateCPMPotency();

    if ((GeigerTubeModel == 4) && !SelectIntExtTube) {
      SetHV(CustomHVLevel);
    } else {
      SetHV(HVLevel);
    }

    SerialLog();
  }

  if (DTimeout != 3660000) {  //61min == display timeout is off
    if (millis() - PreviousMillis5 >= DTimeout) {

      SetDisplayBrightness(0);

    } else {
      SetDisplayBrightness(DBright);
    }
  } else {
    SetDisplayBrightness(DBright);
  }

  if ((millis() - PreviousMillis3 >= 500) || (ForceUpdateDisplay == true)) {  //Every 500ms
    PreviousMillis3 = millis();

    ReadRTC();
    UpdateDisplay();
    BLELoop();

    if (ForceUpdateDisplay == true) {
      ForceUpdateDisplay = false;
    }
  }

  if (DisplayPressed()) {
    PreviousMillis5 = millis();
  }

  ReadTouchScreen();
}

/////////////////////////////////////////////
