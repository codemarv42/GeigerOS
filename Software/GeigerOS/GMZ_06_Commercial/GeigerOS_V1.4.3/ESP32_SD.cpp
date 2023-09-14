///////////////////Includes//////////////////

#include <Arduino.h>
#include "FS.h"
#include <SD.h>
#include <SPI.h>
#include <Update.h>

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"
#include "ESP32_BLE.h"

extern void listDir(fs::FS &fls, String dirname, uint8_t levels);
extern void createDir(fs::FS &fls, String path);
extern void removeDir(fs::FS &fls, String path);
extern void readFile(fs::FS &fls, String path);
extern void writeFile(fs::FS &fls, String path, String message);
extern void appendFile(fs::FS &fls, String path, String message);
extern void renameFile(fs::FS &fls, String path1, String path2);
extern void deleteFile(fs::FS &fls, String path);
extern void testFileIO(fs::FS &fls, String path);
extern void FactoryResetBySDFile(fs::FS &fls);

extern bool FileAvailable(fs::FS &fs, String path);
extern void WriteTimingCountFile(fs::FS &fs, unsigned int part, String path);
extern String FloatToStr(float value);

/////////////////////////////////////////////

//////////////////Variables//////////////////

unsigned int SDStat = 0;  //0 - No SD inserted, 1 - Card mount fail, 2 - Mounted
uint8_t cardType;
bool TimedCountSDLoggingStatus = false;
String FileName = "/GMZ_Data/TimingCountLogs/TimingCount000001.txt";
bool RestrictedModeStat = true; //true - active, false - off

/////////////////////////////////////////////

////////////////////Code/////////////////////

void SDInit() {
  if (RESTRICTED_MODE == 0) {
    RestrictedModeStat = false;
    Serial.println("Restricted mode disabled");
  }


  if (digitalRead(SD_INT) == HIGH) {
    Serial.println("No SD inserted!");
    SDStat = 0;
    return;
  }

  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    SDStat = 1;
    return;
  } else {
    SDStat = 2;
  }

  cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("UNKNOWN card type");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  SearchForFirmware();

  createDir(SD, "/GMZ_Data");
  createDir(SD, "/GMZ_Data/TimingCountLogs");
  createDir(SD, "/GMZ_Data/GMZ_Logs");

  FactoryResetBySDFile(SD);

  if (RESTRICTED_MODE == 1) {
    if ((FileAvailable(SD, "/ADMIN.txt") == true)) {
      RestrictedModeStat = false;
      Serial.println("Restricted mode disabled");
    }
  } else {
    RestrictedModeStat = false;
    Serial.println("Restricted mode disabled");
  }
}

void progressCallBack(size_t currSize, size_t totalSize) {
  Serial.printf("CALLBACK:  Update process at %d of %d bytes...\n", currSize, totalSize);
}

void SearchForFirmware() {
  Serial.print(F("\nSearch for firmware.."));
  File firmware =  SD.open("/firmware.bin");
  if (firmware) {
    Serial.println(F("found!"));
    Serial.println(F("Try to update!"));

    Update.onProgress(progressCallBack);

    Update.begin(firmware.size(), U_FLASH);
    Update.writeStream(firmware);
    if (Update.end()) {
      Serial.println(F("Update finished!"));
    } else {
      Serial.println(F("Update error!"));
      Serial.println(Update.getError());
    }

    firmware.close();

    if (SD.rename("/firmware.bin", "/firmware.bak")) {
      Serial.println(F("Firmware rename succesfully!"));
    } else {
      Serial.println(F("Firmware rename error!"));
    }
    delay(2000);

    ESP.restart();
  } else {
    Serial.println(F("not found!"));
  }
}

void FactoryResetBySDFile(fs::FS &fs) {
  File file = fs.open("/RESET.txt");
  if (!file) {
    Serial.println("No factory reset file");
  } else {
    file.close();
    Serial.println("Factory reset file found");
    Serial.printf("Deleting file: %s\n", "/RESET.txt");
    if (fs.remove("/RESET.txt")) {
      Serial.println("File deleted");
    } else {
      Serial.println("Delete failed");
    }
    Serial.println("Resetting all...");

    ResetEEPROM();
    //TFTCalibrate();
    ESP.restart();
  }
}

void TimingCountSDLog() {
  if (SDStat == 2) {
    if ((TimedCountPage == 0) && (TimedCountSDLogging)) {

      TimedCountSDLoggingStatus = true;

      int tmp = 1;

      String TXT = ".txt";

      while (FileAvailable(SD, FileName) == true) {
        if (tmp < 10) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount00000" + String(tmp) + TXT);
        } else if (tmp < 100) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount0000" + String(tmp) + TXT);
        } else if (tmp < 1000) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount000" + String(tmp) + TXT);
        } else if (tmp < 10000) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount00" + String(tmp) + TXT);
        } else if (tmp < 100000) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount0" + String(tmp) + TXT);
        } else if (tmp < 1000000) {
          FileName = String("/GMZ_Data/TimingCountLogs/TimingCount" + String(tmp) + TXT);
        } else {
          TimedCountSDLoggingStatus = false;
          Serial.println("SD full!");
        }
        tmp++;
      }

      WriteTimingCountFile(SD, 0, FileName);

    } else if ((TimedCountPage == 2) && (TimedCountSDLogging) && (TimedCountSDLoggingStatus)) {
      WriteTimingCountFile(SD, 1, FileName);
    } else if ((TimedCountPage == 3) && (TimedCountSDLogging) && (TimedCountSDLoggingStatus)) {
      WriteTimingCountFile(SD, 2, FileName);
      TimedCountSDLoggingStatus = false;
    }
  } else {
    TimedCountSDLoggingStatus = false;
  }
}

void WriteTimingCountFile(fs::FS &fs, unsigned int part, String path) {
  if (part == 0) {
    File file = fs.open(path, FILE_WRITE);

    if (Language == 0) {  //German

      file.println("Allgemeine Daten:");

      file.print("Dateiname: ");
      file.println(FileName);

      file.print("Start Datum: ");
      file.print(Day);
      file.print("/");
      file.print(Month);
      file.print("/");
      file.print(Year);
      file.println(" (DD/MM/YY)");

      file.print("Start Zeit: ");
      file.print(Hour);
      file.print("/");
      file.print(Minute);
      file.print("/");
      file.print(Second);
      file.println(" (HH/MM/SS)");

      file.print("Dosis Maßeinheit: ");

      if (DoseUnit == 0) {
        file.println("uSv/h");
      } else if (DoseUnit == 1) {
        file.println("mRem/h");
      }

      if (HVLevel == 0) {
        file.println("Zählrohr Spannung: 250V");
      } else if (HVLevel == 1) {
        file.println("Zählrohr Spannung: 300V");
      } else if (HVLevel == 2) {
        file.println("Zählrohr Spannung: 350V");
      } else if (HVLevel == 3) {
        file.println("Zählrohr Spannung: 400V");
      } else if (HVLevel == 4) {
        file.println("Zählrohr Spannung: 450V");
      }

      if (SelectIntExtTube) {
        file.println("Zählrohr Typ: Intern");
        file.print("Zählrohr Modell: ");
        file.println("J305 80mm");
      } else {
        file.println("Zählrohr Typ: Extern");
        if (GeigerTubeModel == 0) {
          file.print("Zählrohr Modell: ");
          file.println("SBM-20");
        } else if (GeigerTubeModel == 1) {
          file.print("Zählrohr Modell: ");
          file.println("SBM-19/STS-6");
        } else if (GeigerTubeModel == 2) {
          file.print("Zählrohr Modell: ");
          file.println("15 mm (Typ B)");
        } else if (GeigerTubeModel == 3) {
          file.print("Zählrohr Modell: ");
          file.println("Ludlum 44-9");
        } else if (GeigerTubeModel == 4) {
          file.print("Zählrohr Modell: ");
          file.println("Custom");
        }
      }

      file.println("Zeit Einheit: s");

      file.println("");

      file.print("Zeit");
      file.print("\t");
      file.print("CPM");
      file.print("\t");
      file.print("Zählungen");
      file.print("\t");
      file.println("Dosis");

    } else if (Language == 1) {  //English

      file.println("General data:");

      file.print("File name: ");
      file.println(FileName);

      file.print("Start date: ");
      file.print(Day);
      file.print("/");
      file.print(Month);
      file.print("/");
      file.print(Year);
      file.println(" (DD/MM/YY)");

      file.print("Start time: ");
      file.print(Hour);
      file.print("/");
      file.print(Minute);
      file.print("/");
      file.print(Second);
      file.println(" (HH/MM/SS)");

      file.print("Dose unit: ");

      if (DoseUnit == 0) {
        file.println("uSv/h");
      } else if (DoseUnit == 1) {
        file.println("mRem/h");
      }

      if (HVLevel == 0) {
        file.println("Counting tube voltage: 250V");
      } else if (HVLevel == 1) {
        file.println("Counting tube voltage: 300V");
      } else if (HVLevel == 2) {
        file.println("Counting tube voltage: 350V");
      } else if (HVLevel == 3) {
        file.println("Counting tube voltage: 400V");
      } else if (HVLevel == 4) {
        file.println("Counting tube voltage: 450V");
      }

      if (SelectIntExtTube) {
        file.println("Counting tube type: Internal");
        file.print("Counting tube model: ");
        file.println("J305 80mm");
      } else {
        file.println("Counting tube type: External");
        if (GeigerTubeModel == 0) {
          file.print("Counting tube model: ");
          file.println("SBM-20");
        } else if (GeigerTubeModel == 1) {
          file.print("Counting tube model: ");
          file.println("SBM-19/STS-6");
        } else if (GeigerTubeModel == 2) {
          file.print("Counting tube model: ");
          file.println("15 mm (Typ B)");
        } else if (GeigerTubeModel == 3) {
          file.print("Counting tube model: ");
          file.println("Ludlum 44-9");
        } else if (GeigerTubeModel == 4) {
          file.print("Counting tube model: ");
          file.println("Custom");
        }
      }

      file.println("Time unit: s");
      file.println("");

      file.print("Time");
      file.print("\t");
      file.print("CPM");
      file.print("\t");
      file.print("Counts");
      file.print("\t");
      file.println("Dose");
    }

    file.close();

    Serial.println("File created");

  } else if (part == 1) {

    File file = fs.open(path, FILE_APPEND);

    file.print((millis() - TimedCountStartTime) / 1000);
    file.print("\t");
    file.print(CPM);
    file.print("\t");
    file.print(TimedCountCurrentCount);
    file.print("\t");
    if (DoseUnit == 0) {
      if (DoseUnitSivert == 0) {
        file.println(FloatToStr(DoseRate));
      } else if (DoseUnitSivert == 1) {
        file.println(FloatToStr(DoseRate * 1000.00));
      } else if (DoseUnitSivert == 2) {
        file.println(FloatToStr(DoseRate * 1000.00 * 1000.00));
      }
    } else if (DoseUnit == 1) {
      if (DoseUnitRem == 0) {
        file.println(FloatToStr(DoseRate));
      } else if (DoseUnitRem == 1) {
        file.println(FloatToStr(DoseRate * 1000.00));
      }
    }
    file.close();

  } else {

    File file = fs.open(path, FILE_APPEND);

    if (Language == 0) {  //German

      file.println("");

      file.print("Endzeit: ");
      file.print(TimedCountEndHour);
      file.print("/");
      file.print(TimedCountEndMinute);
      file.print("/");
      file.print(TimedCountEndSecond);
      file.println(" (HH/MM/SS)");

      file.print("Durchschnitts CPM: ");
      file.println(TimedCountCPM);

      file.print("Laufzeit: ");
      file.print(float(millis() - TimedCountStartTime) / 60000.00);
      file.println(" min");

    } else if (Language == 1) {  //English

      file.println("");

      file.print("End time: ");
      file.print(TimedCountEndHour);
      file.print("/");
      file.print(TimedCountEndMinute);
      file.print("/");
      file.print(TimedCountEndSecond);
      file.println(" (HH/MM/SS)");

      file.print("Avg. CPM: ");
      file.println(TimedCountCPM);

      file.print("Runtime: ");
      file.print(float(millis() - TimedCountStartTime) / 60000.00);
      file.println(" min");
    }

    file.close();
  }
}

String FloatToStr(float value) {  //For two decimal places
  int ival = (int)value;
  int frac = (value - ival) * 100;
  if (frac < 10) {
    return String(String(ival) + ",0" + String(frac));
  } else {
    return String(String(ival) + ',' + String(frac));
  }
}

void UpdateSDStat() {
  if (TYPE != 1) {

    if ((digitalRead(SD_INT) == LOW) && (SDStat == 0)) {
      delay(300);
      SDInit();
    }

    if ((digitalRead(SD_INT) == HIGH) && ((SDStat == 1) || (SDStat == 2))) {
      SDStat = 0;
    }
  }
}

void listDir(fs::FS &fs, String dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, String path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
    SDStat = 1;
  }
}

void removeDir(fs::FS &fs, String path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

bool FileAvailable(fs::FS &fs, String path) {
  File file = fs.open(path);
  if (!file) {
    Serial.println("File is not available");
    return false;
  } else {
    Serial.println("File available");
    return true;
  }
}

void readFile(fs::FS &fs, String path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, String path, String message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, String path, String message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, String path1, String path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, String path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, String path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}

/////////////////////////////////////////////
