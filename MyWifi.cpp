/**
   MyWifi.cpp
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019

*/

#define myDEBUG

#include "Configuration.h"
#include "MyWifi.h"
#include "MySpiffs.h"

#define USE_LITTLEFS
#define USING_CORS_FEATURE         true
 
#define USE_ESP_WIFIMANAGER_NTP    false
#define USE_CLOUDFLARE_NTP         false
 
#define USE_STATIC_IP_CONFIG_IN_CP false  

#include <ESPAsync_WiFiManager.h>

AsyncWebServer server(80);
AsyncDNSServer dns1;
static String ip;

static MySpiffs *myfs;
static Settings *settings;

MyWifi* MyWifi::instance = 0;




ESPAsync_WiFiManager wifiManager(&server, &dns1);
static boolean  _wpsSuccess;
static boolean  _got_ip;
static int      _wifi_stat;
static String   bodyData;
static size_t   uploadLength=0;

extern String execCallback(EXEC_CALLBACK, String arg);

String processor(const String& var) {
  if(var=="IP")
    return(ip);
  if(var=="VERSION")
    return(compile_date);
  return String();
}

void handle_notfound(AsyncWebServerRequest *request) {
  String msg = request->url();
  String mime_type;

  DEBUG_PRINTLN("handle_notfound: "+msg+" not found");

  if(msg=="/resetWifi") {
    request->send_P(200, "text/html", "ok");
    execCallback(WIFI_RESET,"");
    return;
  }
  
  if(msg=="/cleanup") {
    request->send_P(200, "text/html", "ok");
    myfs->cleanup();
    return;
  }

  if(msg=="/format") {
    request->send_P(200, "text/html", "ok");
    myfs->format();
    return;
  }

  if(msg=="/init") {
    request->send_P(200, "text/html", INI_Setup_html);
    return;
  }

  if(msg=="/state") {
    request->send_P(200, "text/html", execCallback(GET_STATE,"").c_str());
    return;
  }

  if(msg=="/getSettings") {
    String tmp=settings->getAll();
    request->send_P(200, TEXTPLAIN, tmp.c_str()); // Send all setting values only to client ajax request
  }

  if(msg=="/") {
    msg="/index.html";
  }
  
  if(myfs->exists(msg)) {
    if(msg.endsWith(F(".htm")) || msg.endsWith(F(".html")) || msg.endsWith(F(".log")))
      request->send(LittleFS, msg, String(), false, processor);
    else {
      if(msg.endsWith(F(".jpg")) || msg.endsWith(F(".jpeg")))
        mime_type = F("image/jpeg");
      else if(msg.endsWith(".png"))
        mime_type = F("image/png");
      else if(msg.endsWith(".ico"))
        mime_type = F("image/x-icon");
      else if(msg.endsWith(".bmp"))
        mime_type = F("image/bmp");
      else if(msg.endsWith(".gif"))
        mime_type = F("image/gif");
      else if(msg.endsWith(".css"))
        mime_type = F("text/css");
      else if(msg.endsWith(".pdf"))
        mime_type = F("application/pdf");
      else if(msg.endsWith(".txt"))
        mime_type = F("text/plain");
      else if(msg.endsWith(".json"))
        mime_type = F("application/json");
      else if(msg.endsWith(".mp3"))
        mime_type = F("audio/mpeg");
      else
        mime_type = F("application/octet-stream");
  
      DEBUG_PRINTLN("handle_notfound: mime_type="+mime_type+", file="+msg);
      request->send(LittleFS, msg, mime_type);
    }
  } else {
    request->send(404, TEXTPLAIN, msg.c_str()); 
  }
}


void handle_FileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    myfs->cleanup();
    execCallback(UPLOAD, "1");
    DEBUG_PRINTF("handle_FileUpload: start %s\n",filename.c_str());
    uploadLength=0;
    if (!filename.startsWith("/")) filename = "/" + filename;
    //DEBUG_PRINT(F("handle_FileUpload Name: "));
    //DEBUG_PRINTLN(filename);
    if(filename.length() > 1) {
      myfs->openFile(filename, "w");
    }
  }
  uint32_t freeSpace = myfs->getFreeSpace();
  //DEBUG_PRINTF("freeSpace=%d, need=%d\n",freeSpace, len);
  if(freeSpace > len) {
    myfs->writeFile(data, len);
  }
  else {
    DEBUG_PRINTLN("Upload failed, not enough space");
    myfs->closeFile();
    myfs->remove(filename);
    String msg =F("Datei zu gross, nur noch ");
    msg += myfs->getFreeSpace();
    msg += F(" Bytes frei");
    request->send(404, TEXTPLAIN, msg);
  }
  uploadLength+=len;
  if(final){
    DEBUG_PRINTF("handle_FileUpload: end %s, %d\n",filename.c_str(), uploadLength);
    myfs->closeFile();
    request->redirect("/index.html");
    if(filename.endsWith(".bin") || filename.endsWith(".tar")) {
      execCallback(UPLOAD_LENGTH, String(uploadLength));
      execCallback(FW_OR_TAR, "/"+filename);
    }
      
    execCallback(UPLOAD, "0");
  }
}


#if defined(ESP32)

#include "esp_wps.h"

static esp_wps_config_t config;

String wpspin2string(uint8_t a[]) {
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}


void wpsStop(void) {
    if(esp_wifi_wps_disable()){
      DEBUG_PRINTLN("WPS Disable Failed");
    }
}


boolean wpsStart(void) {
  _wpsSuccess=false;
  if(esp_wifi_wps_enable(&config)) {
    DEBUG_PRINTLN("WPS Enable Failed");
  } else if(esp_wifi_wps_start(0)) {
    DEBUG_PRINTLN("WPS Start Failed");
  } else
    DEBUG_PRINTLN("esp_wifi_wps_start OK");

  DEBUG_PRINTF("Waiting %d seconds for WPS connection\n", WPSTIMEOUT);
  for (int i = WPSTIMEOUT; i >=0 ; i--) {
    if(_wpsSuccess)
      break;
    execCallback(STATE_TEXT,"Sek.: "+String(i));
    delay(1000);
  }

  if(!_wpsSuccess)
    wpsStop();

  return _wpsSuccess;
}


void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info){
  switch(event){
    case ARDUINO_EVENT_WIFI_STA_START:
      DEBUG_PRINTLN("Station Mode Started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      ip = WiFi.localIP().toString();
      DEBUG_PRINTLN("Connected to :" + String(WiFi.SSID()));
      DEBUG_PRINT("GOT_IP: ");
      DEBUG_PRINTLN(ip);
      myfs->writeLog(F("WiFi GOT_IP: "));
      myfs->writeLog((char *)ip.c_str(), false);
      myfs->writeLog(F("\n"), false);
      _got_ip = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DEBUG_PRINTLN("Disconnected from station, attempting reconnection");
      WiFi.reconnect();
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      DEBUG_PRINTLN("WPS Successfull, stopping WPS and connecting to: " + String(WiFi.SSID()));
      _wpsSuccess = true;
      wpsStop();
      delay(10);
      WiFi.begin();
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      DEBUG_PRINTLN("WPS Failed, retrying");
      wpsStop();
      wpsStart();
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      DEBUG_PRINTLN("WPS Timedout, retrying");
      wpsStop();
      wpsStart();
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      DEBUG_PRINTLN("WPS_PIN = " + wpspin2string(info.wps_er_pin.pin_code));
      break;
    default:
      DEBUG_PRINTF("event %d not handled\n", event);
      break;
  }
}


void wpsInitConfig(){
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}


#else // ESP8266

void WiFiEvent(WiFiEvent_t event) {
  _wifi_stat = event;
  DEBUG_PRINTF("[WiFi-event] event: %d\n", _wifi_stat);
  switch(event) {
    case WIFI_EVENT_STAMODE_CONNECTED:        // 0
      DEBUG_PRINTLN("WiFi connected");
      break;
    case WIFI_EVENT_STAMODE_DISCONNECTED:     // 1
      DEBUG_PRINTLN("WiFi disconnected");
      myfs->writeLog(F("WiFi disconnected\n"));
      break;
    case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:  // 2
      DEBUG_PRINTLN("Author mode change");
      break;
    case WIFI_EVENT_STAMODE_GOT_IP:           // 3
      ip = WiFi.localIP().toString();
      DEBUG_PRINTLN("WiFi GOT_IP");
      DEBUG_PRINTLN("IP address: ");
      DEBUG_PRINTLN(ip);
      myfs->writeLog(F("WiFi GOT_IP: "));
      myfs->writeLog((char *)ip.c_str(), false);
      myfs->writeLog(F("\n"), false);
      _got_ip = true;
      break;
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:     // 4
      DEBUG_PRINTLN("WiFi DHCP Timeout");
      break;
    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:  // 5
      DEBUG_PRINTLN("WiFi STA connected");
      myfs->writeLog(F("WiFi STA connected\n"));
      break;
    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED: // 6
      DEBUG_PRINTLN("WiFi STA disconnected");
      break;
    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:  // 7
      DEBUG_PRINTLN("WiFi probe request received");
      break;
    case WIFI_EVENT_MODE_CHANGE:                // 8
      DEBUG_PRINTLN("WiFi mode changed");
      break;
    case WIFI_EVENT_SOFTAPMODE_DISTRIBUTE_STA_IP: // 9
      DEBUG_PRINTLN("WiFi soft AP mode");
      break;
  }
}
#endif // ESP8266


MyWifi::MyWifi(void) {

}

MyWifi::~MyWifi(void) {

}


MyWifi *MyWifi::getInstance() {
    if (!instance)
    {
        instance = new MyWifi();
    }
    return instance;
}


/**
   Callback function that indicates that the ESP has switched to AP mode
*/
void MyWifi::configModeCallback (ESPAsync_WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered configModeCallback");
  //if(_wifi_signal) _wifi_signal(WIFI_HOTSPOT);
  DEBUG_PRINTLN(WiFi.softAPIP()); //imprime o IP do AP
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
}

/**
   Callback function that indicates that the ESP has saved the settings
*/
void MyWifi::saveConfigCallback (void) {
  DEBUG_PRINTLN("Enter saveConfigCallback");
  DEBUG_PRINTLN(WiFi.softAPIP()); //imprime o IP do AP
}


//Startet die WPS Konfiguration
#if defined(ESP8266)
bool MyWifi::myStartWPS(void) {
  DEBUG_PRINTLN("WPS Konfiguration gestartet");
  _wpsSuccess = WiFi.beginWPSConfig();
  if(_wpsSuccess) {
      // Muss nicht immer erfolgreich heißen! Nach einem Timeout bist die SSID leer
      String newSSID = WiFi.SSID();
      if(newSSID.length() > 0) {
        // Nur wenn eine SSID gefunden wurde waren wir erfolgreich 
        DEBUG_PRINTF("WPS fertig. Erfolgreich angemeldet an SSID '%s', PWD '%s'\n", newSSID.c_str(), WiFi.psk().c_str());
      } else {
        _wpsSuccess = false;
      }
  } else
    DEBUG_PRINTLN("beginWPSConfig failed");
  return _wpsSuccess;
}

#else // ESP32

bool MyWifi::myStartWPS(void) {
  DEBUG_PRINTLN("WPS Konfiguration gestartet");
  myfs->writeLog(F("WPS Konfiguration gestartet\n"));
  _wpsSuccess = true;

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  DEBUG_PRINTLN("Starting WPS");
  wpsInitConfig();
  wpsStart();

  return _wpsSuccess;
}

#endif




/**
   Initialize
*/
void MyWifi::init(void) {
 
  myfs = MySpiffs::getInstance();
  settings = Settings::getInstance();

  server.onNotFound(handle_notfound);
  
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200, TEXTPLAIN, "OK");
  }, handle_FileUpload);
  
  server.on("/saveSettings", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
    if(!index)
      bodyData="";

    for(int i=0; i<len; i++)
      bodyData += (char)*(data+i);

    DEBUG_PRINTF("saveSettings: index=%d, len=%d, total=%d, bodyData=\"%s\"\n", index, len, total, bodyData.c_str());

    if(index+len == total) {
      settings->set(bodyData);
      execCallback(SETTINGS_UPDATE,"");
      bodyData="";
    }
    request->send(200, TEXTPLAIN, "OK");
    });
    // Ende saveSettings

#if defined(myDEBUG)
  // Enable debug output 
  DEBUG_PRINTLN("setDebugOutput to true");
  wifiManager.setDebugOutput(true);
#else
  wifiManager.setDebugOutput(false);
#endif

  /**
     Set Callback Funktion for accesspoint konfiguration
  */
  wifiManager.setAPCallback(configModeCallback);

  /**
      Callback Funktion after connecting to a network
  */
  wifiManager.setSaveConfigCallback(saveConfigCallback);
}


/**
   resets network settings and restarts ESP
*/
void MyWifi::doReset(void) {
  DEBUG_PRINTLN("MyWifi::doReset called");
  settings->remove(F("ssid"));
  settings->remove(F("pwd"));
  settings->writeSettings();
  WiFi.disconnect(true, true);
  ESP.restart();
}

/**
   connects to a network
*/
void MyWifi::connect(String ssid, String pass) {
  int ret;
  int timeout;

#if defined(myDEBUG) 
  // Enable debug output
  DEBUG_PRINTLN("setDebugOutput to true");
  wifiManager.setDebugOutput(true);
#else
  wifiManager.setDebugOutput(false);
#endif

  DEBUG_PRINTF("getWifiParams: ssid=%s, pass=%s\n",ssid.c_str(),pass.c_str());
  /**
     indicates before connecting to a network
  */  
  WiFi.onEvent(WiFiEvent);
  _got_ip=false;

  if(ssid.length() && pass.length()) {
    execCallback(WIFI_CONNECTING, "");
    timeout = WIFITIMEOUT;
  } else {
    Serial.println(F("Starting WPS"));
    myfs->writeLog(F("Starting WPS\n"));
    execCallback(WIFI_WPS, "");
    timeout = WPSTIMEOUT;
    WiFi.mode(WIFI_STA);
    if(myStartWPS()) {
      settings->set(F("ssid"), WiFi.SSID());
      settings->set(F("pwd"), WiFi.psk());
      settings->writeSettings();
      DEBUG_PRINTF("myStartWPS successfull, ssid=%s, passwd=%s\n",WiFi.SSID().c_str(), WiFi.psk().c_str());
      ESP.restart();
    }
  }

  int16_t i=0;
  DEBUG_PRINTF("pass=%s\n",pass.c_str());
  if (pass.length() > 0) {
    int j=0;
    _wifi_stat = -1;
    WiFi.disconnect();
    i=0;
#if defined(ESP8266)
    while(i++ < 10 && _wifi_stat != WIFI_EVENT_STAMODE_DISCONNECTED) {
#else
    while(i++ < 10 && _wifi_stat != SYSTEM_EVENT_STA_DISCONNECTED) {
#endif
      //Serial.println("Warten auf disconnect");
      delay(100);
    }
   
    DEBUG_PRINTF("WiFi.begin with, ssid=%s, passwd=%s, timeout=%d i=%d\n",ssid.c_str(), pass.c_str(), timeout, i);
    _wifi_stat = -1;
    DEBUG_PRINTLN("vor WiFi.begin");
    WiFi.begin(ssid.c_str(), pass.c_str());
    i=0;
    while(i++ < timeout && !_got_ip) {
      // Warten auf connect oder timeout
      DEBUG_PRINTF("_wifi_stat=%d, i=%d\n", _wifi_stat, i);
      //myfs->writeLog(F("Waiting for WiFi Connection\n"));
      delay(1000);
    }
    
  }
  else {
    timeout = WIFITIMEOUT;
    execCallback(WIFI_HOTSPOT, "");
    DEBUG_PRINTF("setConfigPortalTimeout timeout=%i\n", timeout);
    Serial.println(F("Starting Accesspoint"));
    wifiManager.setConfigPortalTimeout(timeout);
    //wifiManager.setConfigPortalChannel(0);
    ret = wifiManager.startConfigPortal(CONFIGPORTAL_SSID);
    DEBUG_PRINTF("After setConfigPortalTimeout timeout=%i\n", timeout);
    i=timeout;
    while(i-- > 0 && !_got_ip) {
      //Serial.printf("While %i\n", i);
      execCallback(STATE_TEXT,"Sek.: "+String(i));
      delay(1000);
    }
  }

  if (!_got_ip) {
    // Ohne Wifi weitermachen
    DEBUG_PRINTLN("Failed to connect, proceed witout WiFi");
    WiFi.mode(WIFI_OFF);
    execCallback(WIFI_TIMEOUT, "");
  } else {
    DEBUG_PRINTLN("Got connection");
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    execCallback(WIFI_OK, "");
    settings->set(F("ssid"), WiFi.SSID());
    settings->set(F("pwd"), WiFi.psk());
    settings->writeSettings();
    DEBUG_PRINTF("getWifiParams: ssid=%s, passwd=%s\n",WiFi.SSID().c_str(), WiFi.psk().c_str());
    server.begin();
  }
}

String MyWifi::getIP(void) {
  return ip;
}
