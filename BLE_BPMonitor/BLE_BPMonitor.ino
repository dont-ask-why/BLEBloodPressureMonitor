#include <Wire.h>
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_system.h>
#include <BLE2902.h>

const int I2C_ERROR = 10;

boolean busActivity = false;
boolean resetInterrupt = true;
boolean newData = false;
volatile boolean readByClient = false;
int error = 0;
RTC_DATA_ATTR int systolic = 0;
RTC_DATA_ATTR int diastolic = 0;
RTC_DATA_ATTR int hr = 0;
int wakeUpTime = 0;

#define SERVICE_UUID        "00002A35-0000-1000-8000-00805f9b34fb" //specific to pulse oximetry services
#define VALUE_CHARACTERISTIC_UUID "98117140-14e4-49c2-870c-f702edb5fc3d" //chosen arbitrarily
#define READ_CHARACTERISTIC_UUID "e5cecf22-6947-42be-8d21-1748293a718b" //chosen arbitrarily
BLECharacteristic *valueCharacteristic;
BLECharacteristic *readCharacteristic;

class AdvertismentCallbacks: public BLEServerCallbacks {
    void onDisconnect(BLEServer* pServer) {
      Serial.println("Device has disconnected");
      BLEAdvertising *aAdvertising = pServer->getAdvertising();
      aAdvertising->addServiceUUID(SERVICE_UUID);
      aAdvertising->start();
    } 
};

class ReadCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
      readByClient = true; // We don't care about data but just that it is available.
    }
};

void setup() {
  delay(100); //add a small delay
  Serial.begin(115200);
  Wire.begin(21, 22); // initialise things
  delay(10); //add a small delay
  Serial.println("Wire initialized");
  wakeUpTime = millis();
  pinMode(13, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13,1);

  //BLE server is being initialized
  //create one BLEService and one Characteristic
  BLEDevice::init("Hacked™ BP Monitor");
  BLEServer *aServer = BLEDevice::createServer();
  aServer->setCallbacks(new AdvertismentCallbacks());
  
  //uuid for the BLE service is set
  BLEService *aService = aServer->createService(SERVICE_UUID);
  //uuid for the BLE characteristic is set
  //the characteristics properties are defined
  valueCharacteristic = aService->createCharacteristic(
                     VALUE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_NOTIFY  
                   );
  valueCharacteristic->addDescriptor(new BLE2902());

  readCharacteristic = aService->createCharacteristic(
                     READ_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_WRITE   |
                     BLECharacteristic::PROPERTY_NOTIFY  
                   );
  readCharacteristic->addDescriptor(new BLE2902());
  readCharacteristic->setCallbacks(new ReadCallbacks());
  
  //BLE server is being started
  aService->start();
  BLEAdvertising *aAdvertising = aServer->getAdvertising();
  aAdvertising->addServiceUUID(SERVICE_UUID);
  aAdvertising->start();
}

void loop() {
  // go to sleep with the bp monitor
  if(digitalRead(13) == LOW){
    Serial.println("Going to sleep.");
    esp_deep_sleep_start();
  }
  
  // Trigger on SDA activity, user might have stored a new measurement
  if(resetInterrupt) {
    attachInterrupt(21, busActivityDetected, CHANGE);
    Serial.println("Waiting for bus activity.");
    resetInterrupt = false;
  }
  
  // Only execute the following if something happened
  if (busActivity) {
    // Ok, forget the interrupts for now, there's work to do!
    detachInterrupt(0);
    Serial.println("Activity detected!");

    int readCount = 0;
    int ourCount = 0;
    int addr = 0x00;
  
    // Delay a bit so the monitor's MCU can finish talking
    delay(500);
  
    readCount = getMeasurementCount();
  
    // The user might have turned off the unit causing an interrupt
    if (error == I2C_ERROR) {
      Serial.println("Can't communicate with the chip.");
    } else { // Reading is valid, changed this to always send latest data if we already done so in this boot cycle.
      if (readCount != 0) {
        Serial.println("Measurements available!");
    
        // Get last measurement
        addr = (readCount - 1) * 4; // address for last measurement
        Serial.print("Reading from 0x");
        Serial.println(addr, HEX);
        refreshLastMeasurements(addr);
    
        if(newData){
          uploadMeasurements();
          newData = false;
          Serial.println("New measurment send to connected user.");
          readByClient = false;
        } else if(!readByClient){
          String toSend = String(hr) + ";" + String(systolic) + ";" + String(diastolic);
          valueCharacteristic->setValue(toSend.c_str());
          valueCharacteristic->notify();
          Serial.println("No new measurement but no user reveived it yet so any user is notified about a new measurement.");
        } else {
          Serial.println("Measurement already sent. No notifications given.");
        }
      } else {
        Serial.println("Memory was reset.");
        systolic = 0;
        diastolic = 0;
        hr = 0;
      }
    }
    busActivity = false;
    resetInterrupt = true;
  }
}

void busActivityDetected() {
  busActivity = true;
}

int getMeasurementCount() {
  int addr = 0x00;
  byte rdata = 0x00;

  // Get current number of measurements at 0xFF
  Wire.beginTransmission(0x50);
  Wire.write(0xFF);
  Wire.endTransmission();
  Wire.requestFrom(0x50, 1);

  if (Wire.available()) {
    rdata = Wire.read();
    error = 0;
  } else {
    error = I2C_ERROR;
    // Wait a little for things to settle
    delay(300);
    return 0;
  }

  if (rdata == 0) {
    Serial.println("Nothing stored.");
    return 0;
  }

  return rdata;
}

void refreshLastMeasurements(int addr) {
  byte rdata = 0x00;

  // read measurement
  Wire.beginTransmission(0x50);
  Wire.write(addr);
  Wire.endTransmission();
  Wire.requestFrom(0x50, 4);

  if (Wire.available() != 4) {
    Serial.println("Less than 4 bytes found?");
    return;
  }

  int temp_hr = Wire.read();
  int temp_diastolic = Wire.read();
  int temp_rdata = Wire.read();
  int temp_systolic = Wire.read();
  temp_systolic *= 2;
  // if this byte is 0x80, add 1 to make it odd
  // --> added 0x81, found some readings where this also indicated an uneven measurment
  if (temp_rdata == 0x80 || temp_rdata == 0x81) {
    temp_systolic += 1;
  }

  if(
     // Check if all values are not extremes
     ((temp_hr < 255       && temp_hr > 0        ) &&
     (temp_systolic < 511  && temp_systolic > 0  ) &&
     (temp_diastolic < 255 && temp_diastolic > 0 ))
     &&
     // Check if any value is different from last send value
     (temp_hr != hr || temp_systolic != systolic || temp_diastolic != diastolic)
     ) {
      // Update global values
      hr = temp_hr;
      systolic = temp_systolic;
      diastolic = temp_diastolic;
      newData = true;
  } else {
    Serial.println("Measurement invalid or already received from bp monitor.");
  }
}

void uploadMeasurements() {
  String toSend = String(hr) + ";" + String(systolic) + ";" + String(diastolic);
  Serial.println(toSend);
    
  valueCharacteristic->setValue(toSend.c_str());
  valueCharacteristic->notify();
}
