       /////////////////////////////////////////////
      //         GeigerOS by Marvin Ramm         //
     //                REV: V1.15               //
    //     https://github.com/codemarv42/      //
   //  https://github.com/codemarv42/GeigerOS //
  //        Last modified: 25.05.2023        //
 //      © Copyright 2023 CC-BY-NC-SA       //
/////////////////////////////////////////////

/*
   Arduino ESP32 Settings:
   Board: "ESP32 Dev Module"
   Upload Speed: "921600"
   CPU Frequency: "240MHz (WiFi/BT)"
   Flash Frequency: "80MHz"
   Flash Mode: "QIO"
   Flash Size: "16MB (128Mb)"
   Partition Scheme: "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
   Programmer: "AVRISP mkll"
*/

/*
   Improvements:
   -Micro SD format button
   -Keylock Button on Timing count
   -Custom Zählrohr mit internem kalibrieren
   -Faster calculation of the CPM
   -Serial Monitor UI
   -Betriebszeit in System Tab
   -BT Connect Symbol aktualisieren
   -(Reset Bluetooth after disconnect)
   -(Absturtz, beim Neustarten des Bluetooth)
   -Update VIA SD Code im Beispiel tab
*/

/*
   Stopped last:
   -SD Integration nochmal anschauen, weil potentielle Fehler vorhanden sind bsp. Zeit Einteilung und ...
   -GMT individuell mit spannungs anpassung
   zwei eeprom speicherplätze für 16Bit 
   System Einstellungen mit Pin freischalten um zu verhindern, dass Kinder etwas verstellen
   first start menu
   Modus mit Mensch Dosen an arme beine...
   Ladevorgänge speichern
   WIFI integration
   First Start Page
*/

///////////////////Includes//////////////////

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"

//////////////////Variables//////////////////

unsigned long PreviousMillis1;
unsigned long PreviousMillis2;
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

  if (millis() - PreviousMillis2 >= 60000) {  //Every minute
    PreviousMillis2 = millis();

    CalculateBATVoltage();
    InternCumulativeCount = InternCumulativeCount - 1;
    InternCurrentCount = InternCurrentCount - 1;
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
