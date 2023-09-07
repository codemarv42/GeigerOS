#ifndef PINS_H
#define PINS_H


//GMZ-05 Pins/////////////////////////////////////////////////////////////////////////

////////////////////Status LED//////////////////////

#define STAT_LED_RED    17
#define STAT_LED_GREEN  2

//////////////////////TFT LCD///////////////////////

#define D_CS            27
#define D_RST           4
#define D_DC            16
#define D_BL            32

/////////////////////SPI PINS///////////////////////

#define SCK             18
#define MISO            19
#define MOSI            23

////////////////DISPLAY TOUCHPAD////////////////////

#define T_CS            25
#define T_IRQ           36  //Adjust pin also under Display/Quic Menu/Power settings in Sleep Mode

//////////////////MICRO SD CARD/////////////////////

#define SD_CS           26
#define SD_INT          35

///////////////////GEIGERTUBE///////////////////////

#define G_INT           15
#define G_EXT           14
#define TIMER           5

/////////////////////BUZZER/////////////////////////

#define BUZZER          13

////////////////POWER MANAGEMENT////////////////////

#define P_ON            33
#define VBAT            39
#define BAT_STAT        34
#define EN_RST          12

///////////////////I2C PORTS////////////////////////

#define RTC_ADRESS      0x51


#endif
