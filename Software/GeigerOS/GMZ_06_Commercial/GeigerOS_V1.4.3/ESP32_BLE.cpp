///////////////////Includes//////////////////

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "Bitmap.h"
#include "Display.h"
#include "Functions.h"
#include "Measure.h"
#include "Pins.h"
#include "Settings.h"
#include "ESP32_SD.h"
#include "ESP32_BLE.h"

// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR2_UUID "e3223119-9445-4e96-a4a1-85358c4046a2"
#define CHAR3_UUID "f33ed397-47ca-426a-9fd7-17cfbf5a90b5"
#define CHAR4_UUID "ee5c91a8-2e8e-477e-88c9-d28e7806e46c"
#define CHAR5_UUID "bf02b4c6-b886-4a13-beff-aaa9d5e119e2"
#define CHAR6_UUID "90abe9e7-91f2-4782-9ba9-b248804cb290"
#define CHAR7_UUID "f9d0f50d-f558-4472-9151-e8f053ae2f38"
#define CHAR8_UUID "f08aa913-8e8f-494a-ac3c-c586b6b719f0"
#define CHAR9_UUID "79b5e4af-5d56-4311-8e16-96708c8649e1"
#define CHAR10_UUID "71d5574a-b20c-4904-a0f2-135d2d39504f"

BLEServer* pServer = NULL;

BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLECharacteristic* pCharacteristic_3 = NULL;
BLECharacteristic* pCharacteristic_4 = NULL;
BLECharacteristic* pCharacteristic_5 = NULL;
BLECharacteristic* pCharacteristic_6 = NULL;
BLECharacteristic* pCharacteristic_7 = NULL;
BLECharacteristic* pCharacteristic_8 = NULL;
BLECharacteristic* pCharacteristic_9 = NULL;
BLECharacteristic* pCharacteristic_10 = NULL;

BLE2902* pBLE2902;

/*
  CHAR Value:

  -CHAR1 - CPM
  -CHAR2 - CurrentCount
  -CHAR3 - BATPercent
  -CHAR4 - HVLevel
  -CHAR5 - DoseLevel
  -CHAR6 - DoseRate
  -CHAR7 - SelectIntExtTube
  -CHAR8 - TotalDose
  -CHAR9 - MaxDoseRate
  -CHAR10 - GEIGEROS_VERSION
*/

/////////////////////////////////////////////

//////////////////Variables//////////////////

unsigned int BLEStat = 0;  //0 - OFF, 1 - ON, 2 - connected

bool deviceConnected = false;
bool oldDeviceConnected = false;

/////////////////////////////////////////////

////////////////////Code/////////////////////

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    BLEStat = 2;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    BLEStat = 1;
  }
};

void BLELoop() {

  if (BLEStat >= 1) {

    // notify changed value
    if (deviceConnected) {

      pCharacteristic->setValue(String(CPM).c_str());
      pCharacteristic->notify();

      if (SelectIntExtTube) {  //Intern
        pCharacteristic_2->setValue(String(InternCurrentCount).c_str());
      } else {  //Extern
        pCharacteristic_2->setValue(String(ExternCurrentCount).c_str());
      }
      pCharacteristic_2->notify();

      pCharacteristic_3->setValue(String(String(BATPercent) + "%").c_str());
      pCharacteristic_3->notify();

      if (HVLevel == 0) {
        pCharacteristic_4->setValue("250V");
      } else if (HVLevel == 1) {
        pCharacteristic_4->setValue("300V");
      } else if (HVLevel == 2) {
        pCharacteristic_4->setValue("350V");
      } else if (HVLevel == 3) {
        pCharacteristic_4->setValue("400V");
      } else if (HVLevel == 4) {
        pCharacteristic_4->setValue("450V");
      }
      pCharacteristic_4->notify();

      pCharacteristic_5->setValue(String(DoseLevel).c_str());
      pCharacteristic_5->notify();

      if (DoseUnit == 0) {
        if (DoseUnitSivert == 0) {
          pCharacteristic_6->setValue(String(String(DoseRate, 4) + " uSv/h").c_str());
        } else if (DoseUnitSivert == 1) {
          pCharacteristic_6->setValue(String(String(DoseRate) + " mSv/h").c_str());
        } else if (DoseUnitSivert == 2) {
          pCharacteristic_6->setValue(String(String(DoseRate) + " Sv/h").c_str());
        }
      } else if (DoseUnit == 1) {
        if (DoseUnitRem == 0) {
          pCharacteristic_6->setValue(String(String(DoseRate, 4) + " mRem/h").c_str());
        } else if (DoseUnitRem == 1) {
          pCharacteristic_6->setValue(String(String(DoseRate) + " Rem/h").c_str());
        }
      }
      pCharacteristic_6->notify();

      pCharacteristic_7->setValue(String(SelectIntExtTube).c_str());
      pCharacteristic_7->notify();

      if (DoseUnit == 0) {
        if (TotalDose >= 1000.00) {
          pCharacteristic_8->setValue(String(String(TotalDose, 5) + " mSv").c_str());
        } else {
          pCharacteristic_8->setValue(String(String(TotalDose, 5) + " uSv").c_str());
        }
      } else if (DoseUnit == 1) {
        if (TotalDose >= 100.00) {
          pCharacteristic_8->setValue(String(String(TotalDose / 1000, 5) + " Rem").c_str());
        } else {
          pCharacteristic_8->setValue(String(String(DoseLevel, 5) + " mRem").c_str());
        }
      }

      pCharacteristic_8->notify();

      if (MaxDoseUnit == 0) {
        pCharacteristic_9->setValue(String(String(MaxDoseRate) + " uSv/h").c_str());
      } else if (MaxDoseUnit == 1) {
        pCharacteristic_9->setValue(String(String(MaxDoseRate) + " mSv/h").c_str());
      } else if (MaxDoseUnit == 2) {
        pCharacteristic_9->setValue(String(String(MaxDoseRate) + " Sv/h").c_str());
      } else if (MaxDoseUnit == 3) {
        pCharacteristic_9->setValue(String(String(MaxDoseRate) + " mRem/h").c_str());
      } else if (MaxDoseUnit == 4) {
        pCharacteristic_9->setValue(String(String(MaxDoseRate) + " Rem/h").c_str());
      }
      pCharacteristic_9->notify();

      pCharacteristic_10->setValue(GEIGEROS_VERSION);
      pCharacteristic_10->notify();
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      delay(500);                   // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising();  // restart advertising
      Serial.println("Start advertising");
      oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }
  }
}

void StartBLE() {

  String BLEDeviceName = String("GMZ-06_" + String(GetSerialNumber()));

  BLEDevice::init(BLEDeviceName.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(BLEUUID(SERVICE_UUID), 30, 0);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
    CHAR1_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_2 = pService->createCharacteristic(
    CHAR2_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_3 = pService->createCharacteristic(
    CHAR3_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_4 = pService->createCharacteristic(
    CHAR4_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_5 = pService->createCharacteristic(
    CHAR5_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_6 = pService->createCharacteristic(
    CHAR6_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_7 = pService->createCharacteristic(
    CHAR7_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_8 = pService->createCharacteristic(
    CHAR8_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_9 = pService->createCharacteristic(
    CHAR9_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic_10 = pService->createCharacteristic(
    CHAR10_UUID,
    BLECharacteristic::PROPERTY_NOTIFY);

  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);

  // Add all Descriptors here
  pCharacteristic->addDescriptor(pBLE2902);

  pCharacteristic_2->addDescriptor(pBLE2902);

  pCharacteristic_3->addDescriptor(pBLE2902);

  pCharacteristic_4->addDescriptor(pBLE2902);

  pCharacteristic_5->addDescriptor(pBLE2902);

  pCharacteristic_6->addDescriptor(pBLE2902);

  pCharacteristic_7->addDescriptor(pBLE2902);

  pCharacteristic_8->addDescriptor(pBLE2902);

  pCharacteristic_9->addDescriptor(pBLE2902);

  pCharacteristic_10->addDescriptor(pBLE2902);

  // Start the service
  pService->start();

  BLEStat = 1;

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void StopBLE() {
  BLEStat = 0;
  BLEDevice::stopAdvertising();
  BLEDevice::deinit();
  BLEStat = 0;
}

/////////////////////////////////////////////
