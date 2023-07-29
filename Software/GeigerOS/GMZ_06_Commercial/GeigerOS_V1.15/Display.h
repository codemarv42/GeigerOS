#ifndef DISPLAY_H
#define DISPLAY_H

extern bool TimeFormat;
extern int Language;
extern bool GraphUpdate;
extern bool MeterUpdate;
extern bool ForceUpdateDisplay;

extern unsigned int PageNumber;
extern unsigned int MainPageNumber;
extern unsigned int QuickMenuPage;
extern unsigned int TimedCountPage;
extern unsigned int SettingsPage;

extern void TFTInit();
extern void TFTCalibrate();
extern uint16_t calData[5];

extern void DrawBootPage();
extern void MedicalMode();
extern void DrawShutdownPage();
extern void DosimeterMode();

extern void DrawMainButtons();
extern void DrawButton(int ButtonType, int Position);
extern void Graph(int x, float y, int gx, int gy, int w, int h, int xlo, int xhi, int xinc, int ylo, int yhi, int yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int pcolor, unsigned int tcolor, unsigned int bcolor, boolean redraw, int mode);
extern void UpdateDisplay();
extern void DrawInfoBar();

extern void MeterMode();
extern void DrawAnalogMeter(int value, int h);
extern void AnalogMeter(int h);
extern void PlotNeedle(int value, byte ms_delay, int h);

extern void ReadTouchScreen();
extern void TouchConditionsMainPage();
extern void RotateDisplay();

extern void QuickMenu(int page);
extern void AverageSpeedPage();

extern void TimedCountSettingsPage(int page);
extern bool TimedCountSDLogging;
extern unsigned int TimedCountDuration;

extern unsigned int TimedCountStartHour;
extern unsigned int TimedCountStartMinute;
extern unsigned int TimedCountStartSecond;

extern unsigned int TimedCountEndHour;
extern unsigned int TimedCountEndMinute;
extern unsigned int TimedCountEndSecond;

extern void Settings(int page);
extern void TouchCalApprovalPage();

#endif
