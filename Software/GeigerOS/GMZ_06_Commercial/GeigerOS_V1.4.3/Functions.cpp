///////////////////Includes//////////////////

#include <Arduino.h>
#include <EEPROM.h>

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"
#include "ESP32_BLE.h"

#include "Pitches.h"

#define EEPROM_SIZE 24  //20x 8Bit - light values, 40x 8Bit - green values, 40x 8Bit red values //MAX 500x 8Bit

void IRAM_ATTR InternTubeInterrupt();
void IRAM_ATTR ExternTubeInterrupt();

TaskHandle_t Task1;

/////////////////////////////////////////////

//////////////////Variables//////////////////

bool BuzzerMode = true;                //true = Buzzer on, false = Buzzer off
bool LEDMode = true;                   //true = LED on, false = LED off
unsigned int DoseLevel = 0;            //0 - Normal, 1 - Activity detectet, 2 - High Doselevel Alert Rate, 3 - Dangerous level
unsigned int DoseAlertRate = 30;       //10 - 90 uSv/h
unsigned int TotalDoseAlertRate = 35;  //30 - 90 uSv
unsigned int TotalDoseLevel = 0;
unsigned int MaxDoseUnit = 0;
float MaxDoseRate;
bool TriggerLedOn = false;
unsigned int BuzzerCh = 1;
unsigned int D_BLCh = 10;
bool EEPROMValid = true;
unsigned int HVLevel = 4;
unsigned int DBright = 255;
unsigned int DTimeout = 600000;  //in ms
bool FirstBoot = false;

/////////////////////////////////////////////

////////////////////Code/////////////////////

void HardwareInit() {
  if (SERIAL) {
    Serial.begin(115200);
  }

  //Output
  pinMode(STAT_LED_RED, OUTPUT);
  pinMode(STAT_LED_GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(P_ON, OUTPUT);
  pinMode(EN_RST, OUTPUT);
  pinMode(D_BL, OUTPUT);
  pinMode(TIMER, OUTPUT);

  //Input
  pinMode(VBAT, INPUT);
  pinMode(BAT_STAT, INPUT);
  pinMode(SD_INT, INPUT);
  pinMode(T_IRQ, INPUT);

  if (TYPE == 0) {
    pinMode(G_INT, INPUT_PULLUP);
    pinMode(G_EXT, INPUT_PULLUP);
  } else if (TYPE == 1) {
    pinMode(G_INT, INPUT_PULLUP);
  } else if (TYPE == 2) {
    pinMode(G_EXT, INPUT_PULLUP);
  }

  digitalWrite(P_ON, HIGH);  //Power button is set to HIGH

  digitalWrite(STAT_LED_GREEN, HIGH);
  digitalWrite(STAT_LED_RED, HIGH);

  Serial.print("SN: ");
  Serial.println(GetSerialNumber());

  ledcSetup(D_BLCh, 10000, 8);
  ledcAttachPin(D_BL, D_BLCh);

  SetDisplayBrightness(0);

  SetHV(HVLevel);

  if (LOG >= 1) {
    if (RTCInit()) {
      Serial.println("RTC ONLINE");
    } else {
      Serial.println("RTC OFFLINE");
    }
  } else {
    RTCInit();
  }

  EEPROM.begin(EEPROM_SIZE);

  EepromLoad();

  if (!EEPROMValid) {
    ResetEEPROM();
    EepromLoad();
    TFTInit();
    SDInit();
    TFTCalibrate();
    ESP.restart();
  }

  TFTInit();

  SDInit();

  DrawBootPage();

  digitalWrite(EN_RST, HIGH);  //Enable the RST Function from the ON OFF Button

  if (FirstBoot == true) {
    FirstBootPage();
    FirstBoot = false;
    EepromSave();
  }

  if (TYPE == 0) {
    attachInterrupt(G_INT, InternTubeInterrupt, FALLING);
    attachInterrupt(G_EXT, ExternTubeInterrupt, FALLING);
  } else if (TYPE == 1) {
    attachInterrupt(G_INT, InternTubeInterrupt, FALLING);
  } else if (TYPE == 2) {
    attachInterrupt(G_EXT, ExternTubeInterrupt, FALLING);
    SelectIntExtTube = false;
  }

  xTaskCreatePinnedToCore(
    Task1code, /* Task function. */
    "Task1",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */
}

void Task1code(void* pvParameters) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for (;;) {
    if (DoseLevel == 0) {
      if (TriggerLedOn && LEDMode) {
        digitalWrite(STAT_LED_GREEN, LOW);
        vTaskDelay(20);
        digitalWrite(STAT_LED_GREEN, HIGH);
        TriggerLedOn = false;
      }
    } else if (DoseLevel == 1) {
      if (TriggerLedOn && LEDMode) {
        digitalWrite(STAT_LED_GREEN, LOW);
        digitalWrite(STAT_LED_RED, LOW);
        vTaskDelay(20);
        digitalWrite(STAT_LED_GREEN, HIGH);
        digitalWrite(STAT_LED_RED, HIGH);
        TriggerLedOn = false;
      }
    } else if (DoseLevel == 2) {
      if (TriggerLedOn && LEDMode) {
        digitalWrite(STAT_LED_RED, LOW);
        vTaskDelay(20);
        digitalWrite(STAT_LED_RED, HIGH);
        TriggerLedOn = false;
      }
    } else if (DoseLevel >= 3) {
      if (BuzzerMode && LEDMode) {
        digitalWrite(STAT_LED_RED, LOW);
        ESPTone(NOTE_A4, 500);
        digitalWrite(STAT_LED_RED, HIGH);
        vTaskDelay(300);
      } else if (!BuzzerMode && LEDMode) {
        digitalWrite(STAT_LED_RED, LOW);
        vTaskDelay(500);
        digitalWrite(STAT_LED_RED, HIGH);
        vTaskDelay(300);
      } else if (BuzzerMode && !LEDMode) {
        ESPTone(NOTE_A4, 500);
        vTaskDelay(300);
      }
    }
    vTaskDelay(1);

  }
}

void SetDisplayBrightness(unsigned int D_Bright) {  //D_Bright from 0 to 255 (8Bit)
  if (D_Bright > 255) {
    ledcWrite(D_BLCh, 255);
  } else {
    ledcWrite(D_BLCh, D_Bright);
  }
}

void SetHV(unsigned int mode) {  //0 - 250V, 1 - 300V, 2 - 350V, 3 - 400V, 4 - 450V

  if (mode == 0) {
    analogWrite(TIMER, 255);
  } else if (mode == 1) {
    analogWrite(TIMER, 203);
  } else if (mode == 2) {
    analogWrite(TIMER, 200);
  } else if (mode == 3) {
    analogWrite(TIMER, 120);
  } else if (mode == 4) {
    analogWrite(TIMER, 0);
  }
}

void SetDoseLevel() {
  if (DoseUnit == 0) {  //Sivert
    if (DoseUnitSivert == 0) {
      if (DoseRate < 0.3) {
        DoseLevel = 0;
      } else if ((DoseRate >= 0.3) && (DoseRate < 3)) {  //Activity detectet
        DoseLevel = 1;
      } else if ((DoseRate >= 3) && (DoseRate < DoseAlertRate)) {  //High Rate
        DoseLevel = 2;
      } else if ((DoseRate >= DoseAlertRate) && (DoseRate < 100)) {  //High Doselevel Alert Rate
        DoseLevel = 3;
      } else {
        DoseLevel = 4;  //Very high rate
      }
    } else if ((DoseUnitSivert == 1) || (DoseUnitSivert == 2)) {
      DoseLevel = 4;
    }
  } else if (DoseUnit == 1) {  //Rem
    if (DoseUnitRem == 0) {
      if ((DoseRate / 0.1) < 0.3) {
        DoseLevel = 0;
      } else if (((DoseRate / 0.1) >= 0.3) && ((DoseRate / 0.1) < 3)) {  //Activity detectet
        DoseLevel = 1;
      } else if (((DoseRate / 0.1) >= 3) && ((DoseRate / 0.1) < DoseAlertRate)) {  //High Rate
        DoseLevel = 2;
      } else if (((DoseRate / 0.1) >= DoseAlertRate) && ((DoseRate / 0.1) < 100)) {  //High Doselevel Alert Rate
        DoseLevel = 3;
      } else {
        DoseLevel = 4;  //Very high rate
      }
    } else if (DoseUnitRem == 1) {
      DoseLevel = 4;  //Very high rate
    }
  }
}

void SetFullDoseLevel() {
  if (DoseUnit == 0) {  //Sv and Gy
    if (TotalDose < 20.00) {
      TotalDoseLevel = 0;
    } else if ((TotalDose >= 20.00) && (TotalDose < TotalDoseAlertRate)) {  //30-90uSv range
      TotalDoseLevel = 1;
    } else if ((TotalDose >= TotalDoseAlertRate) && (TotalDose < 100.00)) {  //Alert rate
      TotalDoseLevel = 2;
    } else {
      TotalDoseLevel = 3;
    }
  } else if (DoseUnit == 1) {  //Rem
    if ((TotalDose / 0.1) < 20.00) {
      TotalDoseLevel = 0;
    } else if (((TotalDose / 0.1) >= 20.00) && ((TotalDose / 0.1) < TotalDoseAlertRate)) {  //30-90uSv range
      TotalDoseLevel = 1;
    } else if (((TotalDose / 0.1) >= TotalDoseAlertRate) && ((TotalDose / 0.1) < 100.00)) {  //Alert rate
      TotalDoseLevel = 2;
    } else {
      TotalDoseLevel = 3;
    }
  }
}

void SetMaxDoseRate() {
  if (DoseRate > MaxDoseRate) {
    MaxDoseRate = DoseRate;
    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {  //uSv/h
        MaxDoseUnit = 0;
      } else if (DoseUnitSivert == 1) {  //mSv/h
        MaxDoseUnit = 1;
      } else if (DoseUnitSivert == 2) {  //Sv/h
        MaxDoseUnit = 2;
      }
    } else if (DoseUnit == 1) {
      if (DoseUnitRem == 0) {  //mRem/h
        MaxDoseUnit = 3;
      } else if (DoseUnitRem == 1) {  //Rem/h
        MaxDoseUnit = 4;
      }
    }
  }

  if (DoseUnitSivert > MaxDoseUnit) {
    MaxDoseRate = DoseRate;
    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {  //uSv/h
        MaxDoseUnit = 0;
      } else if (DoseUnitSivert == 1) {  //mSv/h
        MaxDoseUnit = 1;
      } else if (DoseUnitSivert == 2) {  //Sv/h
        MaxDoseUnit = 2;
      }
    } else if (DoseUnit == 1) {
      if (DoseUnitRem == 0) {  //mRem/h
        MaxDoseUnit = 3;
      } else if (DoseUnitRem == 1) {  //Rem/h
        MaxDoseUnit = 4;
      }
    }
  }

  if ((DoseUnitRem + 3 > MaxDoseUnit) && (MaxDoseUnit < 5) && (MaxDoseUnit > 2)) {
    MaxDoseRate = DoseRate;
    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {  //uSv/h
        MaxDoseUnit = 0;
      } else if (DoseUnitSivert == 1) {  //mSv/h
        MaxDoseUnit = 1;
      } else if (DoseUnitSivert == 2) {  //Sv/h
        MaxDoseUnit = 2;
      }
    } else if (DoseUnit == 1) {
      if (DoseUnitRem == 0) {  //mRem/h
        MaxDoseUnit = 3;
      } else if (DoseUnitRem == 1) {  //Rem/h
        MaxDoseUnit = 4;
      }
    }
  }
}

void ESPTone(unsigned int frequency, unsigned long duration) {
  ledcAttachPin(BUZZER, BuzzerCh);
  ledcWriteTone(BuzzerCh, frequency);
  if (duration) {
    delay(duration);
    ESPNoTone();
  }
}

void ESPNoTone() {
  ledcDetachPin(BUZZER);
  ledcWrite(BuzzerCh, 0);
}

void EepromSave() {  //Saves the values in the EEPROM

  for (int i = 0; i < 10; i += 2) {
    writeUnsignedIntIntoEEPROM(i, calData[i / 2]);
  }

  if (EEPROM.read(10) != Language) {
    EEPROM.write(10, Language);
  }

  if (EEPROM.read(11) != TimeFormat) {
    if (TimeFormat) {
      EEPROM.write(11, 1);
    } else {
      EEPROM.write(11, 0);
    }
  }

  if (EEPROM.read(12) != (int)(DTimeout / 60000)) {
    EEPROM.write(12, (int)(DTimeout / 60000));
  }

  if (EEPROM.read(13) != GeigerTubeModel) {
    EEPROM.write(13, GeigerTubeModel);
  }

  if (EEPROM.read(14) != DoseUnit) {
    if (DoseUnit) {
      EEPROM.write(14, 1);
    } else {
      EEPROM.write(14, 0);
    }
  }

  if (EEPROM.read(15) != CustomHVLevel) {
    EEPROM.write(15, CustomHVLevel);
  }

  if (EEPROM.read(16) != TotalDoseAlertRate) {
    EEPROM.write(16, TotalDoseAlertRate);
  }

  if (EEPROM.read(17) != DoseAlertRate) {
    EEPROM.write(17, DoseAlertRate);
  }

  writeUnsignedIntIntoEEPROM(18, CustomZeroEffect);

  if (EEPROM.read(20) != FirstBoot) {
    if (FirstBoot) {
      EEPROM.write(20, 1);
    } else {
      EEPROM.write(20, 0);
    }
  }

  writeUnsignedIntIntoEEPROM(21, (int)AccentColor);

  EEPROM.commit();  //Saves the changes

  Serial.println("All saved.");
}

void EepromLoad() {  //Read the data from the EEPROM and set the variables accordingly

  for (int i = 0; i < 10; i += 2) {
    if (readUnsignedIntFromEEPROM(i) == 65535) {
      EEPROMValid = false;
    } else {
      calData[i / 2] = readUnsignedIntFromEEPROM(i);
    }
  }

  if (EEPROM.read(10) > 1) {
    EEPROMValid = false;
  } else {
    Language = EEPROM.read(10);
  }

  if ((bool)EEPROM.read(11) > 1) {
    EEPROMValid = false;
  } else {
    TimeFormat = (bool)EEPROM.read(11);
  }

  if (((int)EEPROM.read(12) * 60000) > 3660000) {
    EEPROMValid = false;
  } else {
    DTimeout = (int)EEPROM.read(12) * 60000;
  }

  if (EEPROM.read(13) > 4) {
    EEPROMValid = false;
  } else {
    GeigerTubeModel = EEPROM.read(13);
  }

  if ((bool)EEPROM.read(14) > 1) {
    EEPROMValid = false;
  } else {
    DoseUnit = (bool)EEPROM.read(14);
  }

  if (EEPROM.read(15) > 4) {
    EEPROMValid = false;
  } else {
    CustomHVLevel = EEPROM.read(15);
  }

  if (EEPROM.read(16) > 90) {
    EEPROMValid = false;
  } else {
    TotalDoseAlertRate = EEPROM.read(16);
  }

  if (EEPROM.read(17) > 90) {
    EEPROMValid = false;
  } else {
    DoseAlertRate = EEPROM.read(17);
  }

  if (readUnsignedIntFromEEPROM(18) == 65535) {
    EEPROMValid = false;
  } else {
    CustomZeroEffect = readUnsignedIntFromEEPROM(18);
  }

  if ((bool)EEPROM.read(20) > 1) {
    EEPROMValid = false;
  } else {
    FirstBoot = (bool)EEPROM.read(20);
  }

  if (readUnsignedIntFromEEPROM(21) == 65535) {
    EEPROMValid = false;
  } else {
    AccentColor = readUnsignedIntFromEEPROM(21);
  }

  if (EEPROMValid == false) {
    Serial.println("EEPROM is not valid!");
  } else {
    Serial.println("All load.");
    EEPROMValid = true;
  }
}

void writeUnsignedIntIntoEEPROM(int address, unsigned int number) {  //write unsigned int to the EEPROM an uses 2 Bytes
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

unsigned int readUnsignedIntFromEEPROM(int address) {  //read unsigned int from the EEPROM
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

void writeIntIntoEEPROM(int address, int number) {  //write int to the EEPROM an uses 2 Bytes
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address) {  //read int from the EEPROM
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

unsigned int GetSerialNumber() {
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return (int)chipId;
}

void ResetEEPROM() {
  /*
    writeUnsignedIntIntoEEPROM(0, 316);
    writeUnsignedIntIntoEEPROM(2, 3301);
    writeUnsignedIntIntoEEPROM(4, 330);
    writeUnsignedIntIntoEEPROM(6, 3514);
    writeUnsignedIntIntoEEPROM(8, 4);
  */

  EEPROM.write(10, STD_LANGUAGE);  //Language

  EEPROM.write(11, STD_TIME_FORMAT);  //Time Format

  EEPROM.write(12, 10);

  EEPROM.write(13, 0);

  EEPROM.write(14, 0);

  EEPROM.write(15, 4);

  EEPROM.write(16, 35);

  EEPROM.write(17, 30);

  writeUnsignedIntIntoEEPROM(18, 30);

  EEPROM.write(20, 1);

  writeUnsignedIntIntoEEPROM(21, (int)STD_ACCENT_COLOR);

  EEPROM.commit();
}

void SerialLog() {
  if (LOG == 1) {
  } else if (LOG == 2) {
    Serial.println("");
    Serial.print("Intern: ");
    Serial.println(InternCurrentCount);
    Serial.print("Extern: ");
    Serial.println(ExternCurrentCount);
    Serial.print("CPM: ");
    Serial.println(CPM);
    Serial.print("Dose: ");
    Serial.print(DoseRate, 6);  //6-Digits
    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {
        Serial.println("uSv/h");
      } else if (DoseUnitSivert == 1) {
        Serial.println("mSv/h");
      } else {
        Serial.println("Sv/h");
      }
    } else {
      if (DoseUnitRem == 0) {
        Serial.println("mR/h");
      } else {
        Serial.println("R/h");
      }
    }

    Serial.print("Total Dose: ");
    Serial.print(TotalDose, 6);
    if (DoseUnit == 0) {
      Serial.println("uSv");
    } else {
      Serial.println("mR");
    }

    Serial.print("Dose level: ");

    if (DoseLevel == 0) {
      Serial.println("Normal");
    } else if (DoseLevel == 1) {
      Serial.println("low activity");
    } else if (DoseLevel == 2) {
      Serial.println("High");
    } else if (DoseLevel == 3) {
      Serial.println("Alert");
    } else if (DoseLevel == 4) {
      Serial.println("Very high");
    }

    Serial.print("Total dose level: ");

    if (TotalDoseLevel == 0) {
      Serial.println("Low");
    } else if (TotalDoseLevel == 1) {
      Serial.println("Okay");
    } else if (TotalDoseLevel == 2) {
      Serial.println("Over daily dose!");
    } else if (TotalDoseLevel == 3) {
      Serial.println("Deadly");
    }

    Serial.print("Max Dose: ");
    Serial.print(MaxDoseRate, 6);

    if (MaxDoseUnit == 0) {
      Serial.println("uSv/h");
    } else if (MaxDoseUnit == 1) {
      Serial.println("mSv/h");
    } else if (MaxDoseUnit == 2) {
      Serial.println("Sv/h");
    } else if (MaxDoseUnit == 3) {
      Serial.println("mR/h");
    } else if (MaxDoseUnit == 4) {
      Serial.println("R/h");
    }

    Serial.println("");
    Serial.print("BATVCC: ");
    Serial.print(BATVoltage);
    Serial.println("V");

    Serial.print("Charging?: ");
    if (IsCharging()) {
      Serial.println("YES");
    } else {
      Serial.println("NO");
    }

    Serial.print("BATPercent: ");
    Serial.print(BATPercent);
    Serial.println("%");

    Serial.print("BATPixel: ");
    Serial.print(BATPixelMaped);
    Serial.println("px");
    Serial.println("");

    Serial.print("Time (HH:MM:SS DD/MM/YY): ");
    Serial.print(Hour);
    Serial.print(":");
    Serial.print(Minute);
    Serial.print(":");
    Serial.print(Second);
    Serial.print(" ");
    Serial.print(Day);
    Serial.print("/");
    Serial.print(Month);
    Serial.print("/");
    Serial.println(Year);
    Serial.println("");
    Serial.print("Display pressed?: ");
    if (DisplayPressed()) {
      Serial.println("YES");
    } else {
      Serial.println("NO");
    }
    Serial.println("");
  }
}

/////////////////////////////////////////////
