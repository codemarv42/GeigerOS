#ifndef MEASURE_H
#define MEASURE_H

extern bool SelectIntExtTube;
extern void InternTubeInterrupt();
extern void ExternTubeInterrupt();
extern unsigned int InternCurrentCount;
extern unsigned int InternCumulativeCount;
extern unsigned int ExternCurrentCount;
extern unsigned int ExternCumulativeCount;
extern unsigned int GeigerTubeModel;
extern unsigned int Custom_InterruptTime;
extern unsigned int DeadTimeCustom;

extern float BATVoltage;
extern unsigned int BATPercent;
extern unsigned int BATPixelMaped;
extern void CalculateBATVoltage();
extern bool IsCharging();
extern float ADCTOBAT(float x);

extern unsigned int CPM;
extern unsigned int AverageSpeed;
extern void CalculateCPM();
extern void UpdateCPMPotency();
extern unsigned int CPMPotency;
extern void ResetCPMArray();

extern unsigned int Day, Month, Year, Hour, Minute, Second;
extern void ReadRTC();
extern void SetRTC(unsigned int sec, unsigned int min, unsigned int hor, unsigned int day, unsigned int mth, unsigned int yar);
bool RTCInit();

extern void CalculateDose();
extern bool DoseUnit;
extern float DoseRate;
extern float TotalDose;
extern unsigned int GeigerTubeModel;
extern unsigned int DoseUnitSivert;
extern unsigned int DoseUnitRem;
extern unsigned int CustomZeroEffect;
extern float EffectiveDose(int WT);
extern unsigned int CustomHVLevel;

extern bool DisplayPressed();

extern float TimedCountCPM;
extern unsigned int TimedCountCurrentCount;
extern unsigned long TimedCountStartTime;

#endif
