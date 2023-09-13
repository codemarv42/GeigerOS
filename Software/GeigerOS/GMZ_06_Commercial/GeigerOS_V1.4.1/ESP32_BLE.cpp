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

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR2_UUID          "e3223119-9445-4e96-a4a1-85358c4046a2"
#define CHAR3_UUID          "f33ed397-47ca-426a-9fd7-17cfbf5a90b5"
#define CHAR4_UUID          "ee5c91a8-2e8e-477e-88c9-d28e7806e46c"

BLEServer* pServer = NULL;

BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLECharacteristic* pCharacteristic_3 = NULL;
BLECharacteristic* pCharacteristic_4 = NULL;

BLE2902 *pBLE2902;

/////////////////////////////////////////////

//////////////////Variables//////////////////

unsigned int BLEStat = 0; //0 - OFF, 1 - ON, 2 - connected

bool deviceConnected = false;
bool oldDeviceConnected = false;

/////////////////////////////////////////////

////////////////////Code/////////////////////

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEStat = 2;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      BLEStat = 1;
    }
};

void BLELoop(){

  if (BLEStat >= 1){

  // notify changed value
    if (deviceConnected) {

        pCharacteristic->setValue(CPM);
        pCharacteristic->notify();

        if (SelectIntExtTube) {  //Intern
          pCharacteristic_2->setValue(InternCurrentCount);
        } else {  //Extern
          pCharacteristic_2->setValue(ExternCurrentCount);
        }
        pCharacteristic_2->notify();

        pCharacteristic_3->setValue(BATPercent);
        pCharacteristic_3->notify();

        pCharacteristic_4->setValue(HVLevel);
        pCharacteristic_4->notify();

    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
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

void StartBLE(){

  String stringOne = String(GetSerialNumber()); 

  String BLEDeviceName = String("GMZ-06_" + stringOne);

  BLEDevice::init(BLEDeviceName.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                   

  pCharacteristic_2 = pService->createCharacteristic(
                      CHAR2_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  
  pCharacteristic_3 = pService->createCharacteristic(
                      CHAR3_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic_4 = pService->createCharacteristic(
                      CHAR4_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );  

  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  
  // Add all Descriptors here
  pCharacteristic->addDescriptor(pBLE2902);

  pCharacteristic_2->addDescriptor(pBLE2902);

  pCharacteristic_3->addDescriptor(pBLE2902);

  pCharacteristic_4->addDescriptor(pBLE2902);

  // Start the service
  pService->start();

  BLEStat = 1;

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void StopBLE(){
  BLEStat = 0;
  BLEDevice::stopAdvertising();
  BLEDevice::deinit();
  BLEStat = 0;
}

/////////////////////////////////////////////
