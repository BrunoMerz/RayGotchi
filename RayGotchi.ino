/*
 * Used Libraries:
 * FS                 2.0.0
 * LittleFS           2.0.0
 * WiFi               1.2.7/2.0.0
 * NimBLE-Arduino     1.4.1
 * ESPAsyncWebServer  1.2.3
 * AsyncTCP           1.1.1
 * ESPAsyncDNSServer  1.0.0
 * AsyncUDP           2.0.0
 * ArduinoJson        6.19.4
 * ESPAsync_WiFiManager 1.13.0
 * ESP32-targz        1.1.4
 * TFT_eSPI           2.4.72
 */
 
#include <Arduino.h>
#include <Update.h>

#define DEST_FS_USES_LITTLEFS
#include <ESP32-targz.h>

#define myDEBUG

#include "Configuration.h"


#include "MyTft.h"

#include "NimBLEDevice.h"
#include "Settings.h"
#include "MyWifi.h"
#include "MySpiffs.h"
#include "IconRenderer.h"

// for upload process
static boolean settingsUpdate=false;
static String uploadFilename="";
static boolean uploadStarted=false;
static size_t uploadLength=0;

// for BLE process
static uint32_t lastScan;
static  char deviceName[32];
static int16_t interval;

// some helper
static boolean writeState=false;


// Pointers to single object of these classes
Settings    *settings;
MyWifi      *mywifi;
MySpiffs    *myfs;
IconRenderer *ir;

MyTft tft = MyTft();       // see Configuration.h


/*
 * Start BLEServer and created two characteristics
 * C_CHAR1: for readable name of RayGotchi
 * C_CHAR2: for technical name (MAC Address) which is unique over all ESPs
 * 
 */
void startBLEServer() {
    NimBLEServer *pServer = NimBLEDevice::createServer();
    NimBLEService *pService = pServer->createService(C_SERVICE);
    NimBLECharacteristic *pCharacteristic1 = pService->createCharacteristic(C_CHAR1);
    NimBLECharacteristic *pCharacteristic2 = pService->createCharacteristic(C_CHAR2);
    
    pService->start();
    pCharacteristic1->setValue(deviceName);
    pCharacteristic2->setValue(NimBLEDevice::getAddress().toString());
    DEBUG_PRINTLN(NimBLEDevice::getAddress().toString().c_str());
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(C_SERVICE); 
    pAdvertising->start();
    DEBUG_PRINTLN("BLEServer started");
}

/*
 * BLE Client which searches for all RayGotchis nearby
 * if found ask for readable and technical name
 */
void scanForBLEServers() {
  uint16_t  y_pos=0;
  uint16_t font_number=STD_FONT;
  uint16_t font_height = tft.fontHeight(font_number)+2;
  
  tft.clearMainCanvas();
  
  NimBLEScan *pScan = NimBLEDevice::getScan();
  NimBLEScanResults results = pScan->start(10);

  NimBLEUUID serviceUuid(C_SERVICE);

  for(int i = 0; i < results.getCount(); i++) {
      NimBLEAdvertisedDevice device = results.getDevice(i);
      
      if (device.isAdvertisingService(serviceUuid)) {
          NimBLEClient *pClient = NimBLEDevice::createClient();
          
          if (pClient->connect(&device)) {
              NimBLERemoteService *pService = pClient->getService(serviceUuid);
              
              if (pService != nullptr) {
                  NimBLERemoteCharacteristic *pCharacteristic1 = pService->getCharacteristic(C_CHAR1);
                  String value="";
                  if (pCharacteristic1 != nullptr) {
                      value = F("Name: ");
                      value += pCharacteristic1->readValue();
                      DEBUG_PRINTLN(value);
                      myfs->writeLog((char*)value.c_str());
                      myfs->writeLog("\n",false);
                      tft.drawString(value,0,y_pos+=font_height,font_number);
                  }
                 
                  NimBLERemoteCharacteristic *pCharacteristic2 = pService->getCharacteristic(C_CHAR2);
                  if (pCharacteristic2 != nullptr) {
                      value = F("MAC: ");
                      value += pCharacteristic2->readValue();
                      DEBUG_PRINTLN(value);
                      myfs->writeLog((char*)value.c_str());
                      myfs->writeLog("\n",false);
                      tft.drawString(value,0,y_pos+=font_height,font_number);
                      
                  }
                  value=F("RSSI: ");
                  value += String(pClient->getRssi());
                  DEBUG_PRINTLN(value);
                  myfs->writeLog((char*)value.c_str());
                  myfs->writeLog("\n",false);
                  tft.drawString(value,0,y_pos+=font_height,font_number);
              }
          } else {
              // failed to connect
              DEBUG_PRINTLN("failed to connect");
          }
          
          NimBLEDevice::deleteClient(pClient);
      }
  }
  DEBUG_PRINTLN("scanForBLEServers done");
  lastScan = millis();
}

//
// Callback function which gets called from other classes like MyWifi 
// 
String execCallback(EXEC_CALLBACK ws, String arg) {
  String result="";
  DEBUG_PRINTLN("execCallback: "+String(ws)+" "+arg);
  switch(ws) {
    case SETTINGS_UPDATE:
      settingsUpdate=true;
      interval = settings->getInt(F("interval"));
      break;
    case  WIFI_OK: // got a Wifi connection
      // Display ok.ico
      DEBUG_PRINTF("Got WiFi connection, IP=%s\n", mywifi->getIP().c_str());
      ir->renderAndDisplay(F("/check.ico"),5000,1);
      tft.drawStateLine(mywifi->getIP());
      
      break;
    case WIFI_WPS:  // starting WPS connection
      // 
      ir->renderAndDisplay(F("/wps.ico"),5000,1);
      break;
    case WIFI_HOTSPOT:  // starting Hotspot
      // 
      ir->renderAndDisplay(F("/accesspoint.ico"),5000,1);
      break;
    case WIFI_CONNECTING: // trying to connect to stored Wifi connection
      //
      ir->renderAndDisplay(F("/wifi.ico"),5000,1);
      break;
    case WIFI_TIMEOUT:  // if no wifi connection available 
      // 
      ir->renderAndDisplay(F("/wifiOff.ico"),5000,1);
      break;
    case WIFI_RESET:
      mywifi->doReset();
      break;
    case FW_OR_TAR:
      ir->renderAndDisplay(F("/firmware.ico"),0,0);
      uploadFilename=arg;
      break;
    case UPLOAD:
      ir->renderAndDisplay(F("/upload.ico"),0,0);
      uploadStarted = arg.toInt();
      break;
    case UPLOAD_LENGTH:
      uploadLength = arg.toInt();
      break;
    case GET_STATE:
      writeState = true;
      result="Check Logfile";
      break;
    case STATE_TEXT:
      tft.drawStateLine(arg);
      break;
  }
  return result;
}


/*
 * Updates Firmware from File in Filesystem
 */
bool updateFW() {
  bool ret = false;
  uint8_t *data=(uint8_t *)malloc(BUFSIZE + 12);
  String toDelete;
  size_t l;
  int sum = 0;

  ir->renderAndDisplay(F("/progress0.ico"),0,1);
  
  if (myfs->openFile(uploadFilename, "r")) {
    DEBUG_PRINTLN("updateFW Open ok:" + uploadFilename + " " + String(uploadLength));
    toDelete = uploadFilename;
    if (!Update.begin(uploadLength)) {
      DEBUG_PRINTLN("Update.begin error");
      Update.printError(Serial);
      return false;
    }
    do {
      l = myfs->readBytes(data, BUFSIZE);
      sum += l;
      if (Update.write(data, l) != l) {
        DEBUG_PRINTLN("Update.write error");
        Update.printError(Serial);
        return false;
      } else {
        tft.drawStateLine("Upload bytes: "+String(sum));
      }
    } while (l == BUFSIZE);

    myfs->closeFile();
    DEBUG_PRINTLN("updateFW remove:" + toDelete);
    myfs->remove(toDelete);


    if (sum == uploadLength) {
      DEBUG_PRINTLN(F("updateFW ok"));
    } else {
      DEBUG_PRINTLN(F("updateFW sum != uploadLength"));
    }

    if (Update.end(true)) { //true to set the size to the current progress
      DEBUG_PRINTF("updateFW Update Success: %u\nRebooting...\n", sum);
      DEBUG_FLUSH();
      ret = true;
    } else {
      Update.printError(Serial);
    }
  } else {
    DEBUG_PRINTLN("updateFW open file failed");
  }
  if(data)
    free(data);
  return ret;
}

#define SZ  80

/*
 * Called for each extraced file
 */
static void myTarStatusProgressCallback( const char* name, size_t size, size_t total_unpacked ) {
  String filename(name);
  char buf[SZ];

  int l = sprintf(buf, "<li>Datei: %s, Länge : %d Bytes</li>\n", name, size);
  myfs->writeLog(buf);
  DEBUG_PRINTLN("Ext.: "+filename);
  
  myfs->writeFile((const uint8_t*)buf, l);
  tft.drawStateLine("Extracting: "+filename);

  if (filename.endsWith(".bin")) {
    uploadFilename = "/" + filename;
    uploadLength = size;
  }
}


/*
 * Uploaded tar file will be extraced and stored in filesystem
 */
bool expandTar() {
  bool ret = false;
  String tarFilename = uploadFilename;

  uploadLength = 0;
  char buf[SZ];
  int l;

  ir->renderAndDisplay(F("/progress0.ico"),0,1);
  
  myfs->openFile(String(F("/result.html")), "w");

  TarUnpacker *TARUnpacker = new TarUnpacker();
  TARUnpacker->setupFSCallbacks( targzTotalBytesFn, targzFreeBytesFn );
  TARUnpacker->setTarStatusProgressCallback( myTarStatusProgressCallback );
  if ( !TARUnpacker->tarExpander(tarGzFS, tarFilename.c_str(), tarGzFS, "/") ) {
    DEBUG_PRINTF("tarExpander failed with return code #%d\n", TARUnpacker->tarGzGetError() );
    String buf = F("<p style=\"color:red;font-size:24px;\">Zu wenig Speicherplatz, TAR Datei kann nicht entpackt werden!</p>");
    myfs->writeFile((char *)buf.c_str());
  }
  myfs->remove(tarFilename);
  File inf = LittleFS.open(INFO_FILE, "r");
  if (inf) {
    do {
      l = inf.readBytes(buf, SZ);
      myfs->writeFile((const uint8_t*)buf, l);
    } while (l == SZ);
    inf.flush();
    inf.close();
    myfs->remove(INFO_FILE);
  }

  myfs->closeFile();
  tarFilename = "";
  return ret;
}


/*
 * Setup all 
 */
void setup(void)  
{
    Serial.begin(115200);
    while(!Serial);
    DEBUG_PRINTLN("\n\nStarting RayGotchi Server&Client");

    // Mount Filesystem
    LittleFS.begin();

    // get instances of MySpiffs, MyWifi, Settings and IconRenderer (singleton pattern)
    myfs      = MySpiffs::getInstance();
    mywifi    = MyWifi::getInstance();
    settings  = Settings::getInstance();
    ir        = IconRenderer::getInstance();

    // With WIFI_RESET button all WIFI Settings can be deleted
    pinMode(WIFI_RESET_BUTTON, INPUT_PULLUP);
    
    // TFT init
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.startWrite();
    tft.setRotation(0);
    tft.setTextSize(1);
    

    // Setup icon anzeigen
    ir->renderAndDisplay(F("/setup.ico"),0,0);
    
    // Initialize settings
    settings->readSettings(SETTINGS_FILENAME);
    settings->getChar("name",deviceName,sizeof(deviceName));
    interval = settings->getInt("interval");
    if(interval<0)
      interval=30;
    if(!*deviceName)
        strcpy(deviceName,"RayGotchiX");

    // Wifi 
    mywifi->init();
    mywifi->connect(settings->get("ssid"), settings->get("pwd"));

    DEBUG_PRINTLN(settings->getAll());
    DEBUG_PRINT("DeviceName=");
    DEBUG_PRINTLN(deviceName);

    // Initialize NimBLE
    NimBLEDevice::init(deviceName);


    startBLEServer();
    lastScan = millis();

}



void loop() {
  // if upload in progress di nothing else
  if(uploadStarted)
    return;

  // if we got a file then we process it
  if(uploadFilename.length()) {
    if(uploadFilename.endsWith(F(".tar"))) {
      delay(500);
      // got tar file,lets extract it
      expandTar();
    }
    if(uploadFilename.endsWith(F(".bin"))) {
      delay(500);
      // got a bin file (Firmware), it gets installed and ESP restarted
      if(updateFW())
        ESP.restart();
    }
    uploadFilename = "";
  }

  if(!digitalRead(WIFI_RESET_BUTTON)) {
    myfs->writeLog(F("Got Interrupt, resetting WiFi Settings\n"));
    mywifi->doReset();
  }

  if(writeState) {
    myfs->getState();
    writeState=false;
  }
  
  if((lastScan+(interval*1000)) < millis()){
    scanForBLEServers();
    lastScan = millis();
  }

  

    
}
