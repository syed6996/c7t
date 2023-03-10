//#include <WiFi.h>
#include "SPIFFS.h"
#include <String.h>
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
//#include <HTTPClient.h>
#include <FastLED.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
#include <EEPROM.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include "cert.h"
#define EEPROM_SIZE 230


#define NUM_LEDS1 110
#define NUM_LEDS2 50
#define DATA_PIN1 19
#define DATA_PIN2 18
#define sensor_trig 23
#define sensor_echo 22
#define servoPin 21
#define Lock 25
#define test 27
#define RXD2 16
#define TXD2 17

#define SECOND    1000
#define MINUTE    60* SECOND


String FirmwareVer = {
  "1.1"
};
#define URL_fw_Version "https://raw.githubusercontent.com/syed6996/c7t/main/esp32_ota/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/syed6996/c7t/main/esp32_ota/fw.bin"

//WiFiClient wifiClient;
//##########################  configuration and variables  ##################
int status = WL_IDLE_STATUS;
unsigned long lastSend;
unsigned long  lastKeepalive;
WiFiClient espClient;
PubSubClient client(espClient);
String wsid ;
String wpass;
String password ;
String static_ip ;
String myIP ;
String LOCAL_IP ;
String apname ;
String abc;
String scan_wifi ="A";
 AsyncWebServerRequest* req;
String mac = "";
const char* ssid = "ACTFIBERNET";
const char* password1 = "act12345      ";
const char* mqtt_server = "broker.hivemq.com";

const char * KeepAliveTopic = "ESP32_AIHomes/Keepalive";
const char * PublishTopic = "ESP32_AIHomes/Publish";
const char * SubTopic  ="ESP32_AIHomes/User_Details";
const char * FwTopic  ="ESP32_AIHomes/Fw_Details";
const char * Update_topic  ="ESP32_AIHomes/Update";

String Login_id ="0";
unsigned long  Login_Time = 0;

const char* file = "/config.json";   //Enter your file name
const char* s_data_file = "/ServiceData_jsonfile.txt";
String Service ;
String service_s ;
String host_ip ;
int port ;
int uinterval ;
String u_time;
String c_id ;

int QOS ; // 0
String U_name ;
String s_pass ;
String p_topic;
String Http_requestpath;

AsyncWebServer server(80);


String bottle1 = "8901764032271";// Sprite
String bottle2 = "8901764012914";// sugar free small
String bottle3 = "8901764012907";// big coke
String mystring = "";

int distance = 0;

CRGB leds1[NUM_LEDS1];

CRGB leds2[NUM_LEDS2];
Servo myservo;
//HardwareSerial Barcode (2);

char val[20] = {0};
long int c = 0;
int pos = 0;
int count = 0;
int i, sets = 0;
int duration = 0;
bool action = false, bin_full = false;
unsigned long dust_start_time = 0;

void firmwareUpdate();
int FirmwareVersionCheck();


//################################  MAC ADDRESS FUNCTION  #########################################
String getMacAddress() {
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[5], baseMac[4], baseMac[3], baseMac[2], baseMac[1], baseMac[0]);
  return String(baseMacChr);
}
//####################################### testing wifi connection function ############################
bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    Serial.print(".");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  LOCAL_IP = "Not Connected";
  WiFi.mode(WIFI_AP);
  return false;
}

//############   Conversion for acceesspoint ip into unsigned int ###################
const int numberOfPieces = 4;
String ipaddress[numberOfPieces];
void ipAdress(String& eap, String& iip1, String& iip2, String& iip3, String& iip4)
{

  int counter = 0;
  int lastIndex = 0;
  for (int i = 0; i < eap.length(); i++) {

    if (eap.substring(i, i + 1) == ".") {
      ipaddress[counter] = eap.substring(lastIndex, i);

      lastIndex = i + 1;
      counter++;
    }
    if (i == eap.length() - 1) {
      ipaddress[counter] = eap.substring(lastIndex);
    }

  }
  iip1 = ipaddress[0];
  iip2 = ipaddress[1];
  iip3 = ipaddress[2];
  iip4 = ipaddress[3];

}

void setup() {

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
   Serial.println("Booting");
   Serial.print(" Active fw version:");
    Serial.println(FirmwareVer);

   pinMode(sensor_echo, INPUT_PULLUP);
  pinMode(sensor_trig, OUTPUT);
  pinMode(Lock, OUTPUT);
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000);

  
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(3000);
  }

  //########################  reading config file ########################################

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("SPIFF successfully mounted");
  File dataFile = SPIFFS.open(file, "r");   //Open File for reading
  Serial.println("Reading Configuration Data from File:");
  if (!dataFile) {
    Serial.println("Count file open failed on read.");
  }
  else {
    for (int i = 0; i < dataFile.size(); i++) //Read upto complete file size
    {
      abc += (char)dataFile.read();
    }
    dataFile.close();
  }

  Serial.print(abc);
  Serial.println("");
  Serial.println("");

  WiFi.mode(WIFI_AP_STA);

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@  EEPROM read FOR SSID-PASSWORD- ACCESS POINT IP @@@@@@@@@@@@@@@@@@@@@@@@@@@@
  String esid ;
  String ip1, ip2, ip3, ip4;

  for (int ssidaddress = 0; EEPROM.read(ssidaddress) != '\0' ; ++ssidaddress)
  {
    esid += char(EEPROM.read(ssidaddress));
  }
    Serial.print("Access point SSID:");
    Serial.println(esid);

  String epass = "";
  for (int passaddress = 22 ; passaddress < 43 ; ++passaddress)
  {
    epass += char(EEPROM.read(passaddress));
  }
    Serial.print("Access point PASSWORD:");
    Serial.print(epass);
    Serial.println(" ");

  String eap;
  for (int APaddress = 44 ; EEPROM.read(APaddress) != '\0' ; ++APaddress)
  {
    eap += char(EEPROM.read(APaddress));
  }
    Serial.print("Access point ADDRESS: ");
    Serial.print(eap);
    Serial.println(" ");

  //##############################   ACESS POINT begin on given credential #################################

  if ((esid == NULL ) && (epass == NULL)) {

    Serial.println("###  FIRST TIME SSID PASSWORD SET  ### ");
    StaticJsonDocument<500> root;
    deserializeJson(root,abc); //jsonBuffer.parseObject(abc);
auto error = deserializeJson(root,abc);//jsonBuffer.parseObject(abc);
if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
}
    String    apsid =  root["AP_name"];
    String    appass = root["AP_pass"];

    for (int i = 0; i < 21; ++i) {
      EEPROM.write(i, 0);  //AP ssid
    }
    EEPROM.commit();
    for (int ssidaddress = 0; ssidaddress < apsid.length(); ++ssidaddress)
    {
      EEPROM.write(ssidaddress , apsid[ssidaddress]);
      Serial.print("WRITING  default AP SSID :: ");
      Serial.println(apsid[ssidaddress]);
    }
    EEPROM.commit();

    for (int i = 22; i < 43 ; ++i) {
      EEPROM.write(i, 0);  // AP password
    }
    EEPROM.commit();
    for (int i = 0; i < appass.length(); ++i)
    {
      EEPROM.write(22 + i, appass[i]);
      Serial.print("WRITING  default AP PASSWORD  :: ");
      Serial.println(appass[i]);
    }
    EEPROM.commit();

    WiFi.softAP(apsid.c_str(), appass.c_str()); //Password not used
//    WiFi.softAP(ssid, password1);
    apname = apsid;
    delay(800);

  }

  else {
    //    Serial.println("@@@@  GETTING SSID N PASSWORD  @@@@@ ");
    Serial.print("Access point SSID:");
    Serial.println(esid);
    Serial.print("Access point PASSWORD:");
    Serial.print(epass);
    Serial.println(" ");
    WiFi.softAP(esid.c_str(), epass.c_str());
//    WiFi.softAP(ssid, password1);
    apname = esid;
  }

  if (eap == NULL )
  {
    Serial.println("");
    Serial.println("FIRST TIME AP ADDRESS SETTING 192.168.4.1");

    StaticJsonDocument<500> root;
     deserializeJson(root,abc);//jsonBuffer.parseObject(abc);
    auto error = deserializeJson(root,abc);//jsonBuffer.parseObject(abc);
if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
}
    String    apip =   root["AP_IP"];
    for (int i = 0; i < apip.length(); ++i)
    {
      EEPROM.write(44 + i, apip[i]);
      Serial.print("access point IP  WRITE:: ");
      Serial.println(apip[i]);
    }
    EEPROM.commit();

    String AP;
    for (int APaddress = 44 ; APaddress < 65 ; ++APaddress)
    {
      AP += char(EEPROM.read(APaddress));
    }
    Serial.print("ACCESSPOINT ADDRESS: ");
    Serial.println(AP);
    Serial.println("");
    Serial.println("");

    ipAdress(AP, ip1, ip2, ip3, ip4);

    IPAddress Ip(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
    IPAddress NMask(255, 255, 0, 0);

    WiFi.softAPConfig(Ip, Ip, NMask );
    myIP = WiFi.softAPIP().toString();

    Serial.print("#### SERVER STARTED ON THIS: ");
    Serial.print(myIP);
    Serial.println("####");
  }

  else {
    Serial.print("Access point ADDRESS:");
    Serial.println(eap);

    ipAdress(eap, ip1, ip2, ip3, ip4);
    IPAddress Ip(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
    IPAddress NMask(255, 255, 0, 0);

    WiFi.softAPConfig(Ip, Ip, NMask );
    myIP = WiFi.softAPIP().toString();

    Serial.print("#### SERVER STARTED ON THIS: ");
    Serial.print(myIP);
    Serial.print("####");
  }

  //#########################  HTML+JS+CSS  HANDLING #####################################

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/login.html", "text/html");
  });
  server.on("/main.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/main.html", "text/html");
  });
  server.on("/js/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/bootstrap.min.js", "text/javascript");
  });
  server.on("/js/jquery-1.12.3.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/jquery-1.12.3.min.js", "text/javascript");
  });
  server.on("/js/pixie-custom.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js/pixie-custom.js", "text/javascript");
  });
  server.on("/css/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/css/bootstrap.min.css", "text/css");
  });
  server.on("/css/pixie-main.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/css/pixie-main.css", "text/css");
  });
  //############################# IMAGES HANDLING  ######################################################

  server.on("/images/ap.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/ap.png", "image/png");
  });
  server.on("/images/eye-close.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/eye-close.png", "image/png");
  });
  server.on("/images/light.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/light.png", "image/png");
  });
  server.on("/images/network.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/network.png", "image/png");
  });
  server.on("/images/other.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/other.png", "image/png");
  });
  server.on("/images/periperal.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/periperal.png", "image/png");
  });
  server.on("/images/reboot.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/reboot.png", "image/png");
  });
  server.on("/images/service.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/service.png", "image/png");
  });
  server.on("/images/status.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/status.png", "image/png");
  });
  server.on("/images/upgrade.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/upgrade.png", "image/png");
  });
  server.on("/images/timezone.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/images/timezone.png", "image/png");
  });

  //###################################   ACTIONS FROM WEBPAGE BUTTTONS  ##############################

  server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
    StaticJsonDocument<500> root;
     deserializeJson(root,abc);//jsonBuffer.parseObject(abc);
    auto error =deserializeJson(root,abc); // jsonBuffer.parseObject(abc);

  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
}
    request->send(200, "text/plain", root["Admin_pass"]);
  });

  server.on("/main", HTTP_GET, [](AsyncWebServerRequest * request) {

    String content = "{\"myIP\":\"" + myIP + "\",\"localIP\":\"" + LOCAL_IP + "\",\"s_pass\":\"" + s_pass + "\",\"wsid\":\"" + wsid + "\",\"c_id\":\"" + c_id + "\",\"Service\":\"" + Service + "\",\"host_ip\":\"" + host_ip + "\",\"port\":\"" + port + "\",\"topic\":\"" + p_topic + "\",\"apname\":\"" + apname + "\",\"service\":\"" + service_s + "\",\"MAC\":\"" + getMacAddress() + "\"}";
    Serial.println(content);

    request->send(200, "application/json", content);

  });
  server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest * request) {
    req = request;
     scan_wifi = request->getParam("scan_wifi")->value();
     Serial.println(scan_wifi);
    if (scan_wifi)
    {
      Serial.println("scan_wifi");

      String json = "[";
      int n = WiFi.scanNetworks(false,false);
      Serial.println("scan_wifi");
      if (n == 0)
      {
        Serial.println("no networks found");
      }
      else
      {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
          // Print SSID and RSSI for each network found
          if (i)
            json += ", ";
          json += " {";
          json += "\"rssi\":" + String(WiFi.RSSI(i));
          json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
          json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
          json += ",\"channel\":" + String(WiFi.channel(i));
          json += ",\"secure\":" + String(WiFi.encryptionType(i));
          //                json += ",\"hidden\":"+String(WiFi.isHidden(i)?"true":"false");
          json += "}";
          if (i == (n - 1))
          {
            json += "]";
          }
        }
//        delay(10);
        Serial.println(json);
        request->send(200, "application/json", json);
      }
    }
  });

  //################################# AP SSID-PASSWORD-IP RECEIVING FROM WEB PAGE WRITING TO EEPROM  ###############################

  server.on("/applyBtnFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {

    String txtssid = request->getParam("txtssid")->value();
    String  txtpass = request->getParam("txtpass")->value();
    String  txtaplan = request->getParam("txtaplan")->value();

    if (txtssid.length() > 0) {

      for (int i = 0; i < 21; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();

      Serial.print("RE-writting: ");
      for (int ssidaddress = 0; ssidaddress < txtssid.length(); ++ssidaddress)
      {
        EEPROM.write(ssidaddress , txtssid[ssidaddress]);
        Serial.print("WRITING AP SSID :: ");
        Serial.println(txtssid[ssidaddress]);
      }
    }
    if ( txtpass.length() > 0) {

      for (int i = 22; i < 43 ; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();

      for (int i = 0; i < txtpass.length(); ++i)
      {
        EEPROM.write(22 + i, txtpass[i]);
        Serial.print("AP PASSWORD WRITE:: ");
        Serial.println(txtpass[i]);
      }
    }
    if ( txtaplan.length() > 0) {

      for (int i = 44 ; i < 65 ; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();

      for (int i = 0; i < txtaplan.length(); ++i)
      {
        EEPROM.write(44 + i, txtaplan[i]);
        Serial.print("access point IP  WRITE:: ");
        Serial.println(txtaplan[i]);
      }
    }
    EEPROM.commit();
    request->send(200, "text/plain", "ok");
  });

  //#####################################  Receving WIFI credential from WEB Page ############################

  server.on("/connectBtnFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {

    String  wifi_ssid = request->getParam("wifi_ssid")->value();
    String  wifi_pass = request->getParam("wifi_pass")->value();
    String  wifi_MODE  = request->getParam("wifi_MODE")->value();
    Serial.println(wifi_MODE);

    if (wifi_ssid.length() > 0) {
      for (int i = 66; i < 87; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
      Serial.print("RE-writting wifi SSID: ");
      for (int j = 0; j < wifi_ssid.length(); ++j)
      {
        EEPROM.write(66 + j , wifi_ssid[j]);
        Serial.print("WRITING wifi SSID :: ");
        Serial.println(wifi_ssid[j]);
      }
      EEPROM.commit();
    }
    if ( wifi_pass.length() > 0) {

      for (int i = 88; i < 103; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();

      for (int i = 0; i < wifi_pass.length(); ++i)
      {
        EEPROM.write(88 + i, wifi_pass[i]);
        Serial.print("PASS WRITE:: ");
        Serial.println(wifi_pass[i]);
      }
      EEPROM.commit();
    }

    //##########################   Writing WIFI settings to EEPROM ###############################################

    if (wifi_MODE == "dhcp") {

      if ( wifi_MODE.length() > 0) {

        for (int i = 116; i < 122; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < wifi_MODE.length(); ++i)
        {
          EEPROM.write(116 + i, wifi_MODE[i]);
          Serial.print("wifi mode WRITE:: ");
          Serial.println(wifi_MODE[i]);
        }
        EEPROM.commit();
      }
    }
    if (wifi_MODE == "static") {
      Serial.println(" Mode STATIC selected ");

      String txtipadd  = request->getParam("txtipadd")->value();
      String net_m  = request->getParam("net_m")->value();
      String G_add  = request->getParam("G_add")->value();
      String P_dns  = request->getParam("P_dns")->value();
      String S_dns  = request->getParam("S_dns")->value();

      Serial.println(wifi_ssid);
      Serial.println(wifi_pass);
      Serial.println(txtipadd);
      Serial.println(net_m);
      Serial.println(G_add);
      Serial.println(P_dns);
      Serial.println(S_dns);

      if ( wifi_MODE.length() > 0) {

        for (int i = 116; i < 122; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < wifi_MODE.length(); ++i)
        {
          EEPROM.write(116 + i, wifi_MODE[i]);
          Serial.print("wifi mode WRITE:: ");
          Serial.println(wifi_MODE[i]);
        }
        EEPROM.commit();
      }

      if ( txtipadd.length() > 0) {

        for (int i = 123; i < 143; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < txtipadd.length(); ++i)
        {
          EEPROM.write(123 + i, txtipadd[i]);
          Serial.print("Static IP writing:: ");
          Serial.println(txtipadd[i]);
        }
        EEPROM.commit();
      }

      if ( net_m.length() > 0) {

        for (int i = 143; i < 160 ; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < net_m.length(); ++i)
        {
          EEPROM.write(143 + i, net_m[i]);
          Serial.print(" net mask writing:: ");
          Serial.println(net_m[i]);
        }
        EEPROM.commit();
      }

      if ( G_add.length() > 0) {

        for (int i = 161; i < 180 ; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < G_add.length(); ++i)
        {
          EEPROM.write(161 + i, G_add[i]);
          Serial.print("GATEWAY IP writing:: ");
          Serial.println(G_add[i]);
        }
        EEPROM.commit();
      }
      if ( P_dns.length() > 0) {

        for (int i = 181; i < 200 ; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < P_dns.length(); ++i)
        {
          EEPROM.write(181 + i, P_dns[i]);
          Serial.print("PRIMARY DNS writing:: ");
          Serial.println(P_dns[i]);
        }
        EEPROM.commit();
      }

      if ( S_dns.length() > 0) {

        for (int i = 201; i < 216 ; ++i) {
          EEPROM.write(i, 0);
        }
        EEPROM.commit();

        for (int i = 0; i < S_dns.length(); ++i)
        {
          EEPROM.write(201 + i, S_dns[i]);
          Serial.print("SECONDARY DNS writing:: ");
          Serial.println(S_dns[i]);
        }
        EEPROM.commit();
      }
    }
    request->send(200, "text/plain", "ok");

  });

  //###############################  RESTARTING DEVICE ON REBOOT BUTTON ####################################

  server.on("/rebootbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

    if (request->getParam("reboot_btn")->value() == "reboot_device") {
      Serial.print("restarting device");
      for(int i = 0; i<5000;i++){
        Serial.print(".");
      }
      
      request->send(200, "text/plain", "ok");
//      delay(5000);
      ESP.restart();
    }
  });

  //################################   ADMIN password change function   ######################################

  server.on("/adminpasswordfunction", HTTP_GET, [](AsyncWebServerRequest * request) {
    String confirmpassword = request->getParam("confirmpassword")->value();
    Serial.print(confirmpassword);

  });

  //######################################    RESET to Default  ############################################
  server.on("/resetbtnfunction", HTTP_GET, [](AsyncWebServerRequest * request) {

    if (request->getParam("reset_btn")->value() == "reset_device") {

      for (int i = 66; i < 103; ++i) {
        EEPROM.write(i, 0);  // wsid- wpass eeprom erase
      }
      EEPROM.commit();
      for (int i = 116; i < 220; ++i) {
        EEPROM.write(i, 0);  // wifi mode and parameters are cleared
      }
      EEPROM.commit();

      StaticJsonDocument<500> root;
      deserializeJson(root,abc);//jsonBuffer.parseObject(abc);
      auto error= deserializeJson(root,abc); // jsonBuffer.parseObject(abc);

//      auto error = deserializeJson(doc, input);
if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
}
      String    apsid = (root["AP_name"]);
      String    appass = root["AP_pass"];
      String    apip =   root["AP_IP"];

      for (int i = 0; i < 21; ++i) {
        EEPROM.write(i, 0);  //AP ssid
      }
      EEPROM.commit();
      for (int ssidaddress = 0; ssidaddress < apsid.length(); ++ssidaddress)
      {
        EEPROM.write(ssidaddress , apsid[ssidaddress]);
        Serial.print("WRITING  default AP SSID :: ");
        Serial.println(apsid[ssidaddress]);
      }

      for (int i = 22; i < 43 ; ++i) {
        EEPROM.write(i, 0);  // AP password
      }
      EEPROM.commit();
      for (int i = 0; i < appass.length(); ++i)
      {
        EEPROM.write(22 + i, appass[i]);
        Serial.print("AP PASSWORD WRITE:: ");
        Serial.println(appass[i]);
      }
      for (int i = 44; i < 65 ; ++i) {
        EEPROM.write(i, 0);  //AP ip
      }
      EEPROM.commit();
      for (int i = 0; i < apip.length(); ++i)
      {
        EEPROM.write(44 + i, apip[i]);
        Serial.print("access point IP  WRITE:: ");
        Serial.println(apip[i]);
      }

      EEPROM.commit();
      Serial.println("EEPROM cleared");
      SPIFFS.remove("/ServiceData_jsonfile.txt");

    }
    request->send(200, "text/plain", "ok");


  });
  //############################### RECEIVING DATA SEND MMETHODS HTTP-MQTT-TCP ##############################
  server.on("/applyServiceFunction", HTTP_GET, [] (AsyncWebServerRequest * request) {

    String parameters = request->getParam("parameters")->value();
    Serial.println(parameters);

    File f = SPIFFS.open(s_data_file, "w");

    if (!f) {
      Serial.println("file open failed");
    }
    else
    {
      Serial.println("File Writing");
      f.print(parameters);
      f.close(); //Close file
      Serial.println("File closed");
    }

    File dataFile = SPIFFS.open(s_data_file, "r");   //Open File for reading
    Serial.println("Reading Data from File function time:");
    //Data from filee
    if (!dataFile) {
      Serial.println("Count file open failed on read.");
    }
    else
    {
      size_t size = dataFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      dataFile.readBytes(buf.get(), size);

      const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
      DynamicJsonDocument  root(1024);
   deserializeJson(root,buf.get()); //doc.parseObject(buf.get());

      Service = root["service"].as<String>();
      host_ip = root["host_ip"].as<String>();
      port = root["port"];
      uinterval = root["uinterval"];
      u_time = root["u_time"].as<String>(); //
      c_id = root["c_id"].as<String>(); // "Abcdefghijl"
      QOS = root["QOS"]; // 0
      U_name = root["U_name"].as<String>();
      p_topic = root["p_topic"].as<String>();
      Http_requestpath = root["Http_requestpath"].as<String>();

            Serial.println(Service);
            Serial.println(host_ip);
            Serial.println(port);
            Serial.println(uinterval);
            Serial.println(u_time);
            Serial.println(c_id);
            Serial.println(QOS);
            Serial.println(U_name);
            Serial.println(s_pass);
            Serial.println(p_topic);
            Serial.println(Http_requestpath);
    }
    dataFile.close();
    request->send(200, "text/plain", "ok");
  });
  //################################   WiFi settings Read   #####################################

  for (int wsidaddress = 66; EEPROM.read(wsidaddress) != '\0' ; ++wsidaddress)
  {
    wsid += char(EEPROM.read(wsidaddress));
  }

  Serial.println("");
  Serial.println("");
  Serial.print("Wifi SSID: ");
  Serial.println(wsid);

  for (int passaddress = 88 ; passaddress < 103; ++passaddress)
  {
    wpass += char(EEPROM.read(passaddress));
  }
  Serial.print("Wifi PASSword: ");
  Serial.println(wpass);

  String W_mode = "";
  for (int modeaddress = 116 ; modeaddress < 122 ; ++modeaddress)
  {
    W_mode += char(EEPROM.read(modeaddress));
  }
  Serial.print("WI-FI_MODE: ");
  Serial.println(W_mode);

  if (wsid == NULL) {
    wsid = "Not Given";
    LOCAL_IP = "network not set";
  }

  //####################################### MODE CHECKING(DHCP-STATIC) AND WIFI BEGIN #######################################

  if (W_mode == "dhcp") {
    WiFi.begin(wsid.c_str(), wpass.c_str());
//WiFi.begin(ssid, password1);
    delay(2000);
    if (testWifi()) {
      Serial.print(WiFi.status());
      Serial.println("YOU ARE CONNECTED");
      LOCAL_IP = WiFi.localIP().toString();
      Serial.println(LOCAL_IP);
    }
  }


  if (W_mode == "static") {
    for (int passaddress = 123 ; passaddress < 143; ++passaddress)
    {
      static_ip += char(EEPROM.read(passaddress));
    }
    Serial.print("W-static_ip: ");
    Serial.println(static_ip);

    ipAdress(static_ip, ip1, ip2, ip3, ip4);
    String sb1, sb2, sb3, sb4;
    String sub_net = "";
    for (int passaddress = 143 ; passaddress < 160; ++passaddress)
    {
      sub_net += char(EEPROM.read(passaddress));
    }
    Serial.print("sub_net-: ");
    Serial.println(sub_net);
    delay(1000);

    ipAdress(sub_net, sb1, sb2, sb3, sb4);

    String g1, g2, g3, g4;
    String G_add = "";
    for (int passaddress = 161 ; passaddress < 180; ++passaddress)
    {
      G_add += char(EEPROM.read(passaddress));
    }
    Serial.print("G_add-: ");
    Serial.println(G_add);
    ipAdress(G_add, g1, g2, g3, g4);

    String p1, p2, p3, p4;
    String P_dns = "";
    for (int passaddress = 181 ; passaddress < 200; ++passaddress)
    {
      P_dns += char(EEPROM.read(passaddress));
    }
    Serial.print("Primary_dns-: ");
    Serial.println(P_dns);
    ipAdress(P_dns, p1, p2, p3, p4);

    String s1, s2, s3, s4;
    String S_dns = "";
    for (int passaddress = 201 ; passaddress < 216; ++passaddress)
    {
      S_dns += char(EEPROM.read(passaddress));
    }
    Serial.print("SECONDARY_dns-: ");
    Serial.println(S_dns);
    ipAdress(S_dns, s1, s2, s3, s4);

    IPAddress S_IP(ip1.toInt(), ip2.toInt(), ip3.toInt(), ip4.toInt());
    IPAddress gateway(g1.toInt(), g2.toInt(), g3.toInt(), g4.toInt());
    IPAddress subnet(sb1.toInt(), sb2.toInt(), sb3.toInt(), sb4.toInt());
    IPAddress primaryDNS(p1.toInt(), p2.toInt(), p3.toInt(), p4.toInt()); //optional
    IPAddress secondaryDNS(s1.toInt(), s2.toInt(), s3.toInt(), s4.toInt()); //optional

    if (!WiFi.config(S_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      Serial.println("STA Failed to configure");
    }

    WiFi.begin(wsid.c_str(), wpass.c_str());
    delay(1000);

    if (testWifi()) {
      Serial.print(WiFi.status());
      Serial.println("YOU ARE CONNECTED");
      LOCAL_IP = WiFi.localIP().toString();
      Serial.println(LOCAL_IP);
    }
  }

  File f = SPIFFS.open(s_data_file, "r");   //Open File for reading
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("Reading Data first in loop from File:");

  if (!f) {
    Serial.println("Count file open failed on read.");
  }
  else
  {
    size_t size = f.size();
    std::unique_ptr<char[]> buf(new char[size]);
    f.readBytes(buf.get(), size);

    const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
    DynamicJsonDocument  root(1024);
    deserializeJson(root,buf.get()); //doc.parseObject(buf.get());

    Service = root["service"].as<String>();
    host_ip = root["host_ip"].as<String>();
    port = root["port"];
    uinterval = root["uinterval"];
    u_time = root["u_time"].as<String>(); //
    c_id = root["c_id"].as<String>(); // "Abcdefghijl"
    QOS = root["QOS"]; // 0
    U_name = root["U_name"].as<String>();
    s_pass = root["s_pass"].as<String>();
    p_topic = root["p_topic"].as<String>();
    Http_requestpath = root["Http_requestpath"].as<String>();

    Serial.println(Service);
    Serial.println(host_ip);
    Serial.println(port);
    Serial.println(p_topic);
    Serial.println(uinterval);
    Serial.println(u_time);
    Serial.println(c_id);
    Serial.println(QOS);
    Serial.println(U_name);
    Serial.println(s_pass);
    Serial.println(Http_requestpath);
  }
  f.close();

  if (Service == NULL) {
    service_s = "Not Set";
  }
  else {
    service_s = Service + "(SET)";
  }
  server.begin();
  client.setClient(espClient);
client.setServer(mqtt_server, 1883);
      client.setCallback(callback);
  FastLED.addLeds<WS2812, DATA_PIN1, RGB>(leds1, NUM_LEDS1);
  FastLED.addLeds<WS2812, DATA_PIN2, RGB>(leds2, NUM_LEDS2);
  FastLED.setBrightness(254);

   Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("WiFi connected");
  Serial.print("Netmask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  myservo.write(10);

  mac = getMacAddress();
}

void loop() {

 if (!client.connected()) {
    reconnect();
  }
  client.loop();
   Measure();
    delay(1000);
    
  if (Serial2.available()) {
      mystring = "";
      Serial.println("serial detected");
      while (Serial2.available() )//wait for data available
      {
        char teststr =  Serial2.read();  //read until timeout
        //Serial.println(teststr);
        // Serial.println(mystring);
        mystring += String(teststr);
        //   mystring = String(teststr);
  
      }
    mystring.trim();
    Serial.println(mystring);
 client.loop();
    if (mystring == "765756931175") {
        Serial.println("Lock open ");
        digitalWrite(Lock, HIGH);
          delay(2000);
          digitalWrite(Lock, LOW);
          delay(500);
           mystring = "";

           String payload = "{";
          payload += "\"Mac_Id\":\""; payload += getMacAddress(); payload += "\",";
          payload += "\"User_Id\":\""; payload += Login_id; payload += "\",";
          payload += "\"OR_DATA\":\""; payload += mystring; payload += "\",";
          payload += "\"Count\":1"; //payload += 9999;
          payload += "}";
      
          char attributes[800];
          payload.toCharArray(attributes, 800 );
          client.publish(PublishTopic, attributes );
          Serial.println( attributes );
          Serial.println("Data sent successfully");
          service_s = "MQTT(CONNECTED)";
          lastSend = millis();
          Login_Time = millis();
          }
    
        else if (mystring == "7630049200067") {
        Serial.println("US Readng ");
        Measure();
          delay(2000);
        Measure();
          delay(2000);
           mystring = "";

           
           String payload = "{";
          payload += "\"Mac_Id\":\""; payload += getMacAddress(); payload += "\",";
           payload += "\"Pub_Type\":\""; payload += "Drop_Bottle"; payload += "\",";
          payload += "\"User_Id\":\""; payload += Login_id; payload += "\",";
          payload += "\"OR_DATA\":\""; payload += mystring; payload += "\",";
          payload += "\"Count\":1"; //payload += 9999;
          payload += "\"IP address\":\""; payload += WiFi.localIP(); payload += "\",";
     payload += "\"Netmask\":\""; payload += WiFi.subnetMask(); payload += "\",";
     payload += "\"Gateway\":\""; payload += WiFi.gatewayIP(); payload += "\",";
          payload += "}";
      
          char attributes[800];
          payload.toCharArray(attributes, 800 );
          client.publish(PublishTopic, attributes );
          Serial.println( attributes );
          Serial.println("Data sent successfully");
          service_s = "MQTT(CONNECTED)";
          lastSend = millis();
          Login_Time = millis();
          }
//   client.loop();
  
  
    else{
        for (int i = 0; i <= 40; i++)
          {
            leds2[i] = CRGB(255,0,0);
            FastLED.show();
            delay(50);
          }
          myservo.write(90);
          delay(1000);
            for (int i = 0; i <= 4; i++)
          {
          Measure();
          delay(1000);
          }
           client.loop();
           String payload = "{";
          payload += "\"Mac_Id\":\""; payload += getMacAddress(); payload += "\",";
           payload += "\"Pub_Type\":\""; payload += "Drop_Bottle"; payload += "\",";
          payload += "\"User_Id\":\""; payload += Login_id; payload += "\",";
          payload += "\"OR_DATA\":\""; payload += 123; payload += "\",";
          payload += "\"Count\":1"; //payload += 9999;
          payload += "\"IP address\":\""; payload += WiFi.localIP().toString(); payload += "\",";
     payload += "\"Netmask\":\""; payload += WiFi.subnetMask().toString(); payload += "\",";
     payload += "\"Gateway\":\""; payload += WiFi.gatewayIP().toString(); payload += "\",";
          payload += "}";
      
          char attributes[800];
          payload.toCharArray(attributes, 800 );
          client.publish(PublishTopic, attributes );
          Serial.println( attributes );
          Serial.println("Data sent successfully");
          service_s = "MQTT(CONNECTED)";
          lastSend = millis();
          Login_Time = millis();
     
          mystring = "";
          for (int i = 0; i <= 40; i++)
          {
            leds2[i] = CRGB(0,255,0);
            FastLED.show();
            delay(50);
          }
           myservo.write(10);
    
          
        }
         client.loop();
    } 

  if ( millis() - lastSend > MINUTE ) { // Update and send only after 1 seconds
    String payload = "{";
          payload += "\"Mac_Id\":\""; payload += getMacAddress(); payload += "\",";
           payload += "\"Pub_Type\":\""; payload += "1_minute_pub"; payload += "\",";
          payload += "\"User_Id\":\""; payload += Login_id; payload += "\",";
          payload += "\"Size\":\""; payload +=distance; payload += "\",";
          payload += "\"Count\":1"; //payload += 9999;
          payload += "\"IP address\":\""; payload += WiFi.localIP().toString(); payload += "\",";
     payload += "\"Netmask\":\""; payload += WiFi.subnetMask().toString(); payload += "\",";
     payload += "\"Gateway\":\""; payload += WiFi.gatewayIP().toString(); payload += "\",";
          payload += "}";

    char attributes[800];
    payload.toCharArray(attributes, 800 );
    client.publish(PublishTopic, attributes );
    Serial.println( attributes );
    Serial.println("Data sent successfully");
    service_s = "MQTT(CONNECTED)";
    lastSend = millis();
  }


  
  if ( millis() - lastKeepalive > MINUTE *5 ) { // Update and send only after 1 seconds
    String payload = "{";
    payload += "\"Keepalive\":"; payload += 0000; payload += ",";\
     payload += "\"IP address\":\""; payload +=  WiFi.localIP().toString(); payload += "\",";
     payload += "\"Netmask\":\""; payload +=  WiFi.subnetMask().toString(); payload += "\",";
     payload += "\"Gateway\":\""; payload +=  WiFi.gatewayIP().toString(); payload += "\",";
      payload += "\"Pub_Type\":\""; payload += "Keep_Alive"; payload += "\",";
    payload += "\"MAC_Id\":\""; payload += getMacAddress();
    payload += "\"}";

    char attributes[800];
     client.loop();
    payload.toCharArray(attributes, 800 );
    client.publish(KeepAliveTopic, attributes );
    Serial.println( attributes );
    Serial.println("Data sent successfully");
    service_s = "MQTT(CONNECTED)";
    lastKeepalive = millis();
     client.loop();
  }

  if ( (millis() - Login_Time > MINUTE )  && (Login_id != "0")) {
//       Login_Time = millis();
       Login_id ="0";
  }
  client.loop();
  if (!client.connected()) {
     Serial.println("Mqtt connection state");
     Serial.print(client.state());
    reconnect();
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
   
//    WiFi.disconnect();
//     WiFi.mode(WIFI_AP_STA);
    if ( status != WL_CONNECTED) {
      WiFi.begin(wsid.c_str(), wpass.c_str());
      WiFi.mode(WIFI_AP_STA);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
   
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client";
    clientId += String(random(0xffff), HEX);
    WiFi.mode(WIFI_STA);
    if  (client.connect(clientId.c_str()),"","","",0,false,"",true) {
          Serial.println("connected");
          // Once connected, publish an announcement...
          client.publish(PublishTopic, "hello world");
          // ... and resubscribe
          client.subscribe(SubTopic);
          client.subscribe(FwTopic);
          client.subscribe(Update_topic);
          return;
        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          // Wait 5 seconds before retrying
          delay(5000);
        }

  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");


  if(strcmp(topic,SubTopic) ==0)
  {
    const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
    DynamicJsonDocument  root(1024);
    deserializeJson(root,payload);
//    if()
    String  user = root["user_id"].as<String>();
      String  device = root["device_id"].as<String>();
      String macid =getMacAddress();
//      if(strcmp(device,macid) ==0)
//      {
      if(device == macid){
        Login_id = user;
        Login_Time = millis();
        Serial.println(" mac id matched for devicedevice");Serial.println(device);
      }
      Serial.println(" user");Serial.println(user);Serial.println(" device");Serial.println(device);
  }

  else if(strcmp(topic,Update_topic) ==0)
  {
    const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
    DynamicJsonDocument  root(1024);
    deserializeJson(root,payload);
//    if()
      String  device = root["device_id"].as<String>();
      String macid =getMacAddress();
//      if(strcmp(device,macid) ==0)
//      {
      if(device == macid){
        Serial.println(" mac id matched for devicedevice");Serial.println(device);
        if (FirmwareVersionCheck()) 
        {
            firmwareUpdate();
        }
      }
  }

  else if(strcmp(topic,FwTopic) ==0)
  {
    const size_t capacity = JSON_OBJECT_SIZE(11) + 240;
    DynamicJsonDocument  root(1024);
    deserializeJson(root,payload);
//    if()
      String  device = root["device_id"].as<String>();
      String macid =getMacAddress();
//      if(strcmp(device,macid) ==0)
//      {
      if(device == macid){
        Serial.println(" mac id matched for devicedevice");Serial.println(device);
        if (FirmwareVersionCheck()) 
          {
//              firmwareUpdate();
          }
      }
  }

  
 



  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

   client.loop();

}

void Measure()
{ 
   
  digitalWrite(sensor_trig, LOW); // Set the trigger pin to low for 2uS
  delayMicroseconds(2);
  digitalWrite(sensor_trig, HIGH); // Send a 10uS high to trigger ranging
  delayMicroseconds(10);
  digitalWrite(sensor_trig, LOW); // Send pin low again
   distance = pulseIn(sensor_echo, HIGH, 26000); // Read in times pulse
  distance = distance / 58;
  Serial.print(distance);
  Serial.println(" cm");
  delay(50);
   client.loop();
}



void Test()
{
  Serial.println("Servo sets 0 deg");
  delay(500);

  for (int i = 0; i <= 108; i++)
  {
    leds1[i] = CRGB(45,111,168);
    FastLED.show();
    delay(100);
  }
  

  Serial.println("Ensure all Bottom led glow");
  for (int i = 0; i <= 40; i++)
  {
    leds2[i] = CRGB(0,255,0);
    FastLED.show();
    delay(100);
  }
  Serial.println("Ensure all Door led glow");

  digitalWrite(Lock, HIGH);
  delay(500);
  digitalWrite(Lock, LOW);
  delay(500);
  Serial.println("Lock open and close");

  Measure();

  delay(1000);

}



void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(2, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure  client1 ;
//  std::unique_ptr<BearSSL::WiFiClientSecure>client1(new BearSSL::WiFiClientSecure);
//   client1->setInsecure();

//  if (client1) 
//  {
    client1.setCACert(rootCACertificate);
//    client -> setInsecure();
    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( client1, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(200);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
//  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      return 0;
    } 
    else 
    {
      Serial.println(payload);
      Serial.println("New firmware detected");
      return 1;
    }
  } 
  return 0;  
}
