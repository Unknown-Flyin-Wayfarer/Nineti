// include necessary libraries
#include "BLEDevice.h"
#include "WiFi.h"

//BLE Server name as seen the nRF Connect App
#define bleServerName "UFW-ESP32"

/* UUID's of the service, given in the question*/
static BLEUUID bmeServiceUUID("00000002-0000-0000-FDFD-FDFDFDFDFDFD");

// BLE Characteristics
//SSID Characteristic
static BLEUUID ssidCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");

//Password Characteristic
static BLEUUID passwordCharacteristicUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
//Characteristicd that we want to read
static BLERemoteCharacteristic* ssidCharacteristic;
static BLERemoteCharacteristic* passwordCharacteristic;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

//Variables to store ssid and password
char* ssid;
char* password;

//Flags to check whether new ssid and password readings are available
boolean newssid = false;
boolean newpassword = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bmeServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  ssidCharacteristic = pRemoteService->getCharacteristic(ssidCharacteristicUUID);
  passwordCharacteristic = pRemoteService->getCharacteristic(passwordCharacteristicUUID);

  if (ssidCharacteristic == nullptr || passwordCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  ssidCharacteristic->registerForNotify(ssidNotifyCallback);
  passwordCharacteristic->registerForNotify(passwordNotifyCallback);
  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};
 
//When the BLE Server sends a new ssid reading with the notify property
static void ssidNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store ssid value
  ssid = (char*)pData;
  newssid = true;
}

//When the BLE Server sends a new password reading with the notify property
static void passwordNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store password value
  password = (char*)pData;
  newpassword = true;
  Serial.print(newpassword);
}

void setup() {
  
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      ssidCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      passwordCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  //if new ssid readings are available, print in the OLED
  if (newssid && newpassword){
    newssid = false;
    newpassword = false;
  }
  delay(1000); // Delay a second between loops.

  WiFi.mode(WIFI_STA); //Optional
    WiFi.begin(ssid, password);
    Serial.println("\nConnecting");

    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}