#ifndef PINS_H
#define PINS_H

////////////////////Status LED//////////////////////

#define STAT_LED_RED    4 //FEHLER IM SCHALTPLAN
#define STAT_LED_GREEN  2
#define STAT_LED_BLUE   0

////////////////////SPI TFT LCD/////////////////////

#define D_CS           17
#define D_RST          5
#define D_DC           16

/////////////////////SPI PINS///////////////////////

#define SCK            18
#define MISO           19
#define MOSI           23

////////////////DISPLAY TOUCHPAD////////////////////

#define T_CS           14
#define T_IRQ          27

//////////////////MICRO SD CARD/////////////////////

#define SD_CS          26
#define SD_INT_PULLUP  32

///////////////////GEIGERTUBE///////////////////////

#define G_INT          15

/////////////////////BUZZER/////////////////////////

#define BUZZER         13

////////////////POWER MANAGEMENT////////////////////

#define MAIN_ON        33
#define BATT           39
#define BATT_STAT      25

///////////////////I2C PORTS////////////////////////

#define MPU_ADRESS     0x68
#define RTC_ADRESS     0x00

#endif
