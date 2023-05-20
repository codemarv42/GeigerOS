#ifndef ESP32_SD_H
#define ESP32_SD_H

extern unsigned int SDStat;

extern void SDInit();
extern void UpdateSDStat();

extern uint8_t cardType;

extern bool TimedCountSDLoggingStatus;

extern void TimingCountSDLog();

#endif