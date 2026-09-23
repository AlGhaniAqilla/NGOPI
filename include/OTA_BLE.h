#pragma once

//////////////////////////////////////////////////////// VARIABEL ///////////////////////////////////
BLEOtaUpdate bleOta;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"  // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;

bool BLEstat = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

//calback data masuk print ke Serial1
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    if(pCharacteristic->getValue().startsWith("CHANGE: ")){
      unitID = pCharacteristic->getValue().substring(8);

      preferences.begin("unit", false);
      preferences.putString("kodeUnit", unitID.c_str());
      preferences.end();
    }

    uint8_t *data = pCharacteristic->getData();
    size_t len = pCharacteristic->getLength();
    Serial1.write(data, len);
    Serial.write(data, len);
  }
};

// Connection Callback
void onConnection(bool connected) {
  if (connected) {
    BLEstat = true;
  } else {
    BLEstat = false;
  }
}

void BLE_SERIAL(){
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  // pServer->setCallbacks(new MyServerCallbacks());

  // // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  

  // // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // // Start the service
  pService->start();

  // // Start advertising
  pServer->getAdvertising()->start();
}