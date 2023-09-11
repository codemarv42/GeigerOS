///////////////////Includes//////////////////

#include <Arduino.h>
#include <PCF8563.h>  //https://github.com/Bill2462/PCF8563-Arduino-Library

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"

PCF8563 rtc;

/////////////////////////////////////////////

//////////////////Variables//////////////////

//RTC//

unsigned int Day, Month, Year, Hour, Minute, Second;

//Geiger Tube//

bool SelectIntExtTube = true;  //0 - Extern, 1 - Intern
unsigned long InternCurrentCount;
unsigned long InternCumulativeCount;
unsigned long ExternCurrentCount;
unsigned long ExternCumulativeCount;
unsigned int DeadTimeSBM20 = 1430;    //us - 150us wegen buzzer
unsigned int DeadTimeSBM19 = 1650;    //us
unsigned int DeadTimeTypB = 30;       //us
unsigned int DeadTimeLudlum449 = 30;  //us
unsigned int DeadTimeCustom = 500;    //us
unsigned long PreviousIntMicrosExtern;

//Battery//

float BATVoltage;
unsigned int BATPercent;
unsigned int BATPixelMaped;

//CalculateCPM//

unsigned int CPM;
unsigned int AverageSpeed = 0;  //0 = Normal (60s) 1 = Fast (5s), 2 = Slow (120s)
unsigned int i = 0;
unsigned int j = 0;
unsigned int k = 0;
bool ResetCalculateCPM = false;
unsigned long Count[61];
unsigned long FastCount[6];  //Arrays to store running counts
unsigned long SlowCount[181];

//UpdateCPMPotency

unsigned int CPMPotency = 2;
unsigned int PreviousCPMPotency = 2;

//Calculate dose//

bool DoseUnit = 0;  //0 - Sivert, 1 - Rem
float DoseRate;
float TotalDose;
float ConversionFactorJ305 = 0.005644;  //(CPM/uSv/h)^1 = Conversion Factor
float ConversionFactorSBM20 = 0.005732;
float ConversionFactorSBM19 = 0.00177;
float ConversionFactorTypB = 0.007746;
float ConversionFactorLudlum449 = 0.0004;
unsigned int GeigerTubeModel = 1;  //0 - SBM-20, 1 - SBM-19/STS-6, 2 - 15 mm (Typ B), 3 - Ludlum 44-9, 4 - Custom
unsigned int DoseUnitSivert = 0;   //0 = uSv/h, 1 = mSv/h, 2 = Sv/h
unsigned int DoseUnitRem = 0;      //0 = mRem/h, 1 = Rem/h
unsigned int CustomZeroEffect = 30;
unsigned int CustomHVLevel = 4;

//Timed Count//

float TimedCountCPM;
unsigned int TimedCountCurrentCount = 0;
unsigned long TimedCountStartTime;

/////////////////////////////////////////////

////////////////////Code/////////////////////

bool RTCInit() {
  if (TYPE != 1) {
    rtc.init();

    Time nowTime = rtc.getTime();
    if ((nowTime.second) == 85) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

void ReadRTC() {
  if (TYPE != 1) {

    Time nowTime = rtc.getTime();

    Second = nowTime.second;
    Minute = nowTime.minute;
    Hour = nowTime.hour;
    Day = nowTime.day;
    Month = nowTime.month;
    Year = nowTime.year;
  }
}

void SetRTC(unsigned int sec, unsigned int min, unsigned int hor, unsigned int day, unsigned int mth, unsigned int yar) {
  if (TYPE != 1) {
    rtc.stopClock();

    rtc.setYear(yar);
    rtc.setMonth(mth);
    rtc.setDay(day);
    rtc.setHour(hor);
    rtc.setMinut(min);
    rtc.setSecond(sec);

    rtc.startClock();
  }
}

void InternTubeInterrupt() {
  InternCurrentCount++;
  InternCumulativeCount++;  //total counts
  if (BuzzerMode && SelectIntExtTube && (DoseLevel < 3)) {
    digitalWrite(BUZZER, HIGH);
    delayMicroseconds(150);
    digitalWrite(BUZZER, LOW);
  }
  if (SelectIntExtTube) {
    TriggerLedOn = true;
    if ((TimedCountPage == 2) && (PageNumber == 5)) {
      TimedCountCurrentCount++;
    }
  }
}

void ExternTubeInterrupt() {
  if (GeigerTubeModel == 0) {  //SBM-20
    if ((micros() - DeadTimeSBM20) > PreviousIntMicrosExtern) {
      ExternCurrentCount++;
      ExternCumulativeCount++;  //total counts
      if (BuzzerMode && !SelectIntExtTube && (DoseLevel < 3)) {
        digitalWrite(BUZZER, HIGH);
        delayMicroseconds(150);
        digitalWrite(BUZZER, LOW);
      }
      if (!SelectIntExtTube) {
        TriggerLedOn = true;
        if ((TimedCountPage == 2) && (PageNumber == 5)) {
          TimedCountCurrentCount++;
        }
      }
    }
  } else if (GeigerTubeModel == 1) {  //SBM-19/STS-6
    if ((micros() - DeadTimeSBM19) > PreviousIntMicrosExtern) {
      ExternCurrentCount++;
      ExternCumulativeCount++;  //total counts
      if (BuzzerMode && !SelectIntExtTube && (DoseLevel < 3)) {
        digitalWrite(BUZZER, HIGH);
        delayMicroseconds(150);
        digitalWrite(BUZZER, LOW);
      }
      if (!SelectIntExtTube) {
        TriggerLedOn = true;
        if ((TimedCountPage == 2) && (PageNumber == 5)) {
          TimedCountCurrentCount++;
        }
      }
    }
  } else if (GeigerTubeModel == 2) {  //15 mm (Typ B)
    if ((micros() - DeadTimeTypB) > PreviousIntMicrosExtern) {
      ExternCurrentCount++;
      ExternCumulativeCount++;  //total counts
      if (BuzzerMode && !SelectIntExtTube && (DoseLevel < 3)) {
        digitalWrite(BUZZER, HIGH);
        delayMicroseconds(150);
        digitalWrite(BUZZER, LOW);
      }
      if (!SelectIntExtTube) {
        TriggerLedOn = true;
        if ((TimedCountPage == 2) && (PageNumber == 5)) {
          TimedCountCurrentCount++;
        }
      }
    }
  } else if (GeigerTubeModel == 3) {  //Ludlum 44-9
    if ((micros() - DeadTimeLudlum449) > PreviousIntMicrosExtern) {
      ExternCurrentCount++;
      ExternCumulativeCount++;  //total counts
      if (BuzzerMode && !SelectIntExtTube && (DoseLevel < 3)) {
        digitalWrite(BUZZER, HIGH);
        delayMicroseconds(150);
        digitalWrite(BUZZER, LOW);
      }
      if (!SelectIntExtTube) {
        TriggerLedOn = true;
        if ((TimedCountPage == 2) && (PageNumber == 5)) {
          TimedCountCurrentCount++;
        }
      }
    }
  } else if (GeigerTubeModel == 4) {  //Custom
    if ((micros() - DeadTimeCustom) > PreviousIntMicrosExtern) {
      ExternCurrentCount++;
      ExternCumulativeCount++;  //total counts
      if (BuzzerMode && !SelectIntExtTube && (DoseLevel < 3)) {
        digitalWrite(BUZZER, HIGH);
        delayMicroseconds(150);
        digitalWrite(BUZZER, LOW);
      }
      if (!SelectIntExtTube) {
        TriggerLedOn = true;
        if ((TimedCountPage == 2) && (PageNumber == 5)) {
          TimedCountCurrentCount++;
        }
      }
    }
  }

  PreviousIntMicrosExtern = micros();
}

float ADCTOBAT(float x) {  //Values from measurements
  float punkte[][2] = {{4095, 4.29}, {4015, 4.24}, {3700, 4.10}, {3600, 4.05},
    {3526, 4.00}, {3460, 3.95}, {3380, 3.90}, {3300, 3.85},
    {3240, 3.80}, {3175, 3.75}, {3120, 3.71}, {3060, 3.65},
    {3010, 3.60}, {2955, 3.55}, {2890, 3.50}, {2845, 3.45},
    {2795, 3.40}, {2745, 3.35}, {2690, 3.30}, {2640, 3.25},
    {2575 , 3.20}
  };

  int anzahlPunkte = sizeof(punkte) / sizeof(punkte[0]);

  // Suche nach dem nächsten Punkt
  int index = -1;
  for (int i = anzahlPunkte - 1; i >= 0; i--) {
    if (x >= punkte[i][0]) {
      index = i;
      break;
    }
  }

  // Wenn kein passender Punkt gefunden wurde oder x kleiner als der erste Punkt ist,
  // gib den ersten y-Wert zurück
  if (index == -1 || x < punkte[anzahlPunkte - 1][0]) {
    return punkte[anzahlPunkte - 1][1];
  }

  // Wenn x größer als der letzte Punkt ist, gib den letzten y-Wert zurück
  if (x >= punkte[0][0]) {
    return punkte[0][1];
  }

  // Berechnung des y-Werts durch lineare Interpolation
  float x1 = punkte[index][0];
  float x2 = punkte[index - 1][0];
  float y1 = punkte[index][1];
  float y2 = punkte[index - 1][1];

  float m = (y2 - y1) / (x2 - x1);
  float b = y1 - m * x1;

  return m * (x - x1) + y1;
}

void CalculateBATVoltage() {

  unsigned int TmpVbatArray[8];

  for (int i = 0; i <= 7; i++) {
    TmpVbatArray[i] = analogRead(VBAT);
  }

  unsigned int TmpVbatAvg = (TmpVbatArray[0] + TmpVbatArray[1] + TmpVbatArray[2] + TmpVbatArray[3] + TmpVbatArray[4] + TmpVbatArray[5] + TmpVbatArray[6] + TmpVbatArray[7]) / 8;

  BATVoltage = ADCTOBAT(TmpVbatAvg) - 0.10;

  if (BATVoltage > 4.25) {
    BATVoltage = 4.25;
  } else if (BATVoltage < 3.30) {
    BATVoltage = 3.30;
  }

  if (BATVoltage > 4.15) {
    BATPercent = 100;
  } else if (BATVoltage < 3.40) {
    BATPercent = 0;
  } else {
    BATPercent = (BATVoltage - 3.40) * (100 - 0) / (4.15 - 3.40) + 0;
  }

  BATPixelMaped = map(BATPercent, 100, 0, 292, 313);
}

bool IsCharging() {
  bool BATStat = digitalRead(BAT_STAT);
  if (BATStat == LOW) {
    return true;
  } else {
    return false;
  }
}

void ResetCPMArray() {
  for (int i = 0; i < 61; i++) {
    Count[i] = 0;
  }

  for (int i = 0; i < 6; i++) {
    FastCount[i] = 0;
  }

  for (int i = 0; i < 181; i++) {
    SlowCount[i] = 0;
  }

  i = 0;
  j = 0;
  k = 0;

  InternCurrentCount = 0;
  ExternCurrentCount = 0;
}

void CalculateCPM() {

  /*
    unsigned int ti = 61000 / LoopTime;
    unsigned int tj = 6000 / LoopTime;
    unsigned int tk = 181000 / LoopTime;
  */

  if (SelectIntExtTube) {  //Intern
    Count[i] = InternCurrentCount;
    i++;
    FastCount[j] = InternCurrentCount;
    j++;
    SlowCount[k] = InternCurrentCount;
    k++;
  } else {  //Extern
    Count[i] = ExternCurrentCount;
    i++;
    FastCount[j] = ExternCurrentCount;
    j++;
    SlowCount[k] = ExternCurrentCount;
    k++;
  }

  if (i == 61) {
    i = 0;
  }

  if (j == 6) {
    j = 0;
  }

  if (k == 181) {
    k = 0;
  }

  if (SelectIntExtTube) {  //Intern

    if (AverageSpeed == 0) {
      CPM = InternCurrentCount - Count[i];  //Count[i] stores the value from 60 seconds ago.
    }
    if (AverageSpeed == 1) {
      CPM = (InternCurrentCount - FastCount[j]) * 12;
    }
    if (AverageSpeed == 2) {
      CPM = (InternCurrentCount - SlowCount[k]) / 3;
    }

  } else {  //Extern

    if (AverageSpeed == 0) {
      CPM = ExternCurrentCount - Count[i];  //Count[i] stores the value from 60 seconds ago.
    }
    if (AverageSpeed == 1) {
      CPM = (ExternCurrentCount - FastCount[j]) * 12;
    }
    if (AverageSpeed == 2) {
      CPM = (ExternCurrentCount - SlowCount[k]) / 3;
    }
  }

  CPM = ((CPM) / (1 - 0.00000333 * float(CPM)));

  if ((TimedCountPage == 2) && (PageNumber == 5)) {
    TimedCountCPM = float(TimedCountCurrentCount) / float((millis() - TimedCountStartTime) / 60000.000000);
  }
}

void CalculateDose() {
  if (SelectIntExtTube) {  //Intern
    HVLevel = 4;
    if (DoseUnit == 0) {  //uSv/h
      DoseUnitSivert = 0;
      DoseRate = CPM * ConversionFactorJ305;  //Äquivalentdose
      TotalDose = InternCumulativeCount / (60 / ConversionFactorJ305);
      if (DoseRate >= 1000.00) {  //1000uSv/h
        DoseUnitSivert = 1;
        DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
        if (DoseRate >= 1000.00) {
          DoseUnitSivert = 2;
          DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
        }
      }
    } else {
      DoseUnitRem = 0;
      DoseRate = (CPM * ConversionFactorJ305) * 0.1;  //Äquivalentdose
      TotalDose = (InternCumulativeCount / (60 / ConversionFactorJ305)) * 0.1;
      if (DoseRate >= 1000.00) {  //1000mRem/h
        DoseUnitRem = 1;
        DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
      }
    }
  } else {                         //Extern
    if (DoseUnit == 0) {           //Sivert
      if (GeigerTubeModel == 0) {  //0 = SBM-20 Tube
        HVLevel = 3;
        DoseUnitSivert = 0;
        DoseRate = CPM * ConversionFactorSBM20;  //Äquivalentdose
        TotalDose = ExternCumulativeCount / (60 / ConversionFactorSBM20);
        if (DoseRate >= 1000.00) {  //1000uSv/h
          DoseUnitSivert = 1;
          DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
          if (DoseRate >= 1000.00) {
            DoseUnitSivert = 2;
            DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
          }
        }
      } else if (GeigerTubeModel == 1) {  //SBM-19/STS-6
        HVLevel = 3;
        DoseUnitSivert = 0;
        DoseRate = CPM * ConversionFactorSBM19;  //Äquivalentdose
        TotalDose = ExternCumulativeCount / (60 / ConversionFactorSBM19);
        if (DoseRate >= 1000.00) {  //1000uSv/h
          DoseUnitSivert = 1;
          DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
          if (DoseRate >= 1000.00) {
            DoseUnitSivert = 2;
            DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
          }
        }
      } else if (GeigerTubeModel == 2) {  //15 mm (Typ B)
        HVLevel = 4;
        DoseUnitSivert = 0;
        DoseRate = CPM * ConversionFactorTypB;  //Äquivalentdose
        TotalDose = ExternCumulativeCount / (60 / ConversionFactorTypB);
        if (DoseRate >= 1000.00) {  //1000uSv/h
          DoseUnitSivert = 1;
          DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
          if (DoseRate >= 1000.00) {
            DoseUnitSivert = 2;
            DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
          }
        }
      } else if (GeigerTubeModel == 3) {  //Ludlum 44-9
        HVLevel = 4;
        DoseUnitSivert = 0;
        DoseRate = CPM * ConversionFactorLudlum449;  //Äquivalentdose
        TotalDose = ExternCumulativeCount / (60 / ConversionFactorLudlum449);
        if (DoseRate >= 1000.00) {  //1000uSv/h
          DoseUnitSivert = 1;
          DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
          if (DoseRate >= 1000.00) {
            DoseUnitSivert = 2;
            DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
          }
        }
      } else if (GeigerTubeModel == 4) {  //Custom
        DoseUnitSivert = 0;
        float ConversionFactorCustom = 1.20 / float(CustomZeroEffect);
        DoseRate = CPM * ConversionFactorCustom;  //Äquivalentdose
        TotalDose = ExternCumulativeCount / (60 / ConversionFactorCustom);
        if (DoseRate >= 1000.00) {  //1000uSv/h
          DoseUnitSivert = 1;
          DoseRate = DoseRate / 1000.00;  //uSv/h to mSv/h
          if (DoseRate >= 1000.00) {
            DoseUnitSivert = 2;
            DoseRate = DoseRate / 1000.00;  //mSv/h to Sv/h
          }
        }
      }
    } else {                       //Rem
      if (GeigerTubeModel == 0) {  //0 = SBM-20 Tube
        HVLevel = 3;
        DoseUnitRem = 0;
        DoseRate = (CPM * ConversionFactorSBM20) * 0.1;  //Äquivalentdose
        TotalDose = (ExternCumulativeCount / (60 / ConversionFactorSBM20)) * 0.1;
        if (DoseRate >= 1000.00) {  //1000mRem/h
          DoseUnitRem = 1;
          DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
        }
      } else if (GeigerTubeModel == 1) {  //SBM-19/STS-6
        HVLevel = 3;
        DoseUnitRem = 0;
        DoseRate = (CPM * ConversionFactorSBM19) * 0.1;  //Äquivalentdose
        TotalDose = (ExternCumulativeCount / (60 / ConversionFactorSBM19)) * 0.1;
        if (DoseRate >= 1000.00) {  //1000mRem/h
          DoseUnitRem = 1;
          DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
        }
      } else if (GeigerTubeModel == 2) {  //15 mm (Typ B)
        HVLevel = 4;
        DoseUnitRem = 0;
        DoseRate = (CPM * ConversionFactorTypB) * 0.1;  //Äquivalentdose
        TotalDose = (ExternCumulativeCount / (60 / ConversionFactorTypB)) * 0.1;
        if (DoseRate >= 1000.00) {  //1000mRem/h
          DoseUnitRem = 1;
          DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
        }
      } else if (GeigerTubeModel == 3) {  //Ludlum 44-9
        HVLevel = 4;
        DoseUnitRem = 0;
        DoseRate = (CPM * ConversionFactorLudlum449) * 0.1;  //Äquivalentdose
        TotalDose = (ExternCumulativeCount / (60 / ConversionFactorLudlum449)) * 0.1;
        if (DoseRate >= 1000.00) {  //1000mRem/h
          DoseUnitRem = 1;
          DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
        }
      } else if (GeigerTubeModel == 4) {  //Custom
        DoseUnitRem = 0;
        float ConversionFactorCustom = 1.20 / float(CustomZeroEffect);
        DoseRate = (CPM * ConversionFactorCustom) * 0.1;  //Äquivalentdose
        TotalDose = (ExternCumulativeCount / (60 / ConversionFactorCustom)) * 0.1;
        if (DoseRate >= 1000.00) {  //1000mRem/h
          DoseUnitRem = 1;
          DoseRate = DoseRate / 1000.00;  //mRem/h to Rem/h
        }
      }
    }
  }
}

float EffectiveDose(int WT) {  //WT - Gewebe-Wichtungsfaktoren (können im Internet gefunden werden)
  float EffectiveDoseTmp;

  if (WT == 0) {  //Knochenmark (rot) - Bone marrow (red)
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 1) {  //Dickdarm - Colon
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 2) {  //Lunge - Lungs
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 3) {  //Magen - Stomach
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 4) {  //Brust - Chest
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 5) {  //Keimdrüsen - Gonads
    EffectiveDoseTmp = TotalDose * 0.08;
  } else if (WT == 6) {  //Blase - Bladder
    EffectiveDoseTmp = TotalDose * 0.04;
  } else if (WT == 7) {  //Speiseröhre - Esophagus
    EffectiveDoseTmp = TotalDose * 0.04;
  } else if (WT == 8) {  //Leber - Liver
    EffectiveDoseTmp = TotalDose * 0.04;
  } else if (WT == 9) {  //Schilddrüse - Thyroid gland
    EffectiveDoseTmp = TotalDose * 0.04;
  } else if (WT == 10) {  //Haut - Skin
    EffectiveDoseTmp = TotalDose * 0.01;
  } else if (WT == 11) {  //Knochenoberfläche - Bone surface
    EffectiveDoseTmp = TotalDose * 0.01;
  } else if (WT == 12) {  //Gehirn - Brain
    EffectiveDoseTmp = TotalDose * 0.01;
  } else if (WT == 13) {  //Speicheldrüsen - Salivary glands
    EffectiveDoseTmp = TotalDose * 0.01;
  } else if (WT == 14) {  //übrige Organe und Gewebe - Other organs and tissues
    EffectiveDoseTmp = TotalDose * 0.12;
  } else if (WT == 15) {  //Head
    EffectiveDoseTmp = TotalDose * 0.24;
  } else if (WT == 16) {  //Upper Body
    EffectiveDoseTmp = TotalDose * 0.33;
  } else if (WT == 17) {  //Lower Body
    EffectiveDoseTmp = TotalDose * 0.43;
  }

  return EffectiveDoseTmp;
}

void UpdateCPMPotency() {
  if (CPM <= 100) {
    CPMPotency = 2;  //10^2
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 100) && (CPM <= 1000)) {
    CPMPotency = 3;  //10^3
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 1000) && (CPM <= 10000)) {
    CPMPotency = 4;  //10^4
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 10000) && (CPM <= 100000)) {
    CPMPotency = 5;  //10^5
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 100000) && (CPM <= 1000000)) {
    CPMPotency = 6;  //10^6
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 1000000) && (CPM <= 10000000)) {
    CPMPotency = 7;  //10^7
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 10000000) && (CPM <= 100000000)) {
    CPMPotency = 8;  //10^8
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  } else if ((CPM > 100000000) && (CPM <= 1000000000)) {
    CPMPotency = 9;  //10^9
    if (PreviousCPMPotency != CPMPotency) {
      GraphUpdate = true;
      MeterUpdate = true;
    }
  }
  PreviousCPMPotency = CPMPotency;
}

bool DisplayPressed() {
  if (digitalRead(T_IRQ) == 1) {
    return false;
  } else {
    return true;
  }
}

/////////////////////////////////////////////
