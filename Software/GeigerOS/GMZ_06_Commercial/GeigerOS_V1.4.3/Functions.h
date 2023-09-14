#ifndef FUNCTIONS_H
#define FUNCTIONS_H

extern void HardwareInit();
extern void SerialLog();

extern bool BuzzerMode;
extern bool LEDMode;

extern void SetHV(unsigned int mode);
extern unsigned int HVLevel;

extern unsigned int DoseLevel;
extern void SetFullDoseLevel();
extern void SetDoseLevel();
extern unsigned int TotalDoseAlertRate;
extern unsigned int DoseAlertRate;
extern unsigned int TotalDoseLevel;

extern void SetMaxDoseRate();
extern unsigned int MaxDoseUnit;
extern float MaxDoseRate;

extern bool TriggerLedOn;
extern void Task1code( void * pvParameters );

extern unsigned int BuzzerCh;
extern void ESPTone(unsigned int frequency, unsigned long duration);
extern void ESPNoTone();

extern void SetDisplayBrightness(unsigned int D_Bright);
extern unsigned int DBright;
extern unsigned int DTimeout;

extern void writeUnsignedIntIntoEEPROM(int address, unsigned int number);
extern unsigned int readUnsignedIntFromEEPROM(int address);
extern void writeIntIntoEEPROM(int address, int number);
extern int readIntFromEEPROM(int address);
extern void EepromLoad();
extern void EepromSave();
extern bool EEPROMValid;
extern void ResetEEPROM();

extern unsigned int GetSerialNumber();

extern bool FirstBoot;

#endif
