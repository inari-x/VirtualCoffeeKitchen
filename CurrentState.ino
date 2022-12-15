#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include "Arduino.h"
#include "heltec.h"
#include "images.h"

#define BUTTON_PIN 0

//Variables
int i = 0;
int statusCode;
const char* ssid = "Default SSID";
const char* passphrase = "Default passord";
unsigned long pressedTime  = 0;
unsigned long totalPressTime = 0;
int buttonPressed=0;
String st;
String content;
String esid;
String epass = "";
//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
//Establishing Local server at port 80
WebServer server(80);

void isr()
{
  if (digitalRead(0) == 0)
  { 
    doFalling();
  }
  else {
    doRising();
  }
}


void setup()
{
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor

    // SCREEN -----------------------------------------------------------------------------------------
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  delay(300);

  //---------------------------------------------------------------------------------------------------

  // Serial.println();
  // Serial.println("Disconnecting current wifi connection");

  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  pinMode(0, INPUT_PULLUP);
  attachInterrupt(0, isr, CHANGE);
  //---------------------------------------- Read eeprom for ssid and pass
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  // Serial.print("SSID: ");
  // Serial.println(esid);
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  // Serial.print("PASS: ");
  // Serial.println(epass);
  WiFi.begin(esid.c_str(), epass.c_str());
}

void drawWifiConnection() {
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(10, 0, "connected to:" + esid);
    Heltec.display->display();
}

void drawParticipants() {
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->drawString(0, 20, "zoom: ");
    Heltec.display->display();
}


void loop() {
  if(pressedTime>4000 && buttonPressed==1){
    buttonPressed = 0;
    totalPressTime = 0;
    WiFi.disconnect();
    launchWeb();
    setupAP();// Setup HotSpot
    while ((WiFi.status() != WL_CONNECTED))
    {
      Heltec.display->clear();
      Heltec.display->display();
      delay(10);
      server.handleClient();
    }
  }
  else if(totalPressTime>0 && totalPressTime<2000 && buttonPressed==1)
  {
    buttonPressed=0;
    totalPressTime=0;
    Serial.println("Joining the zoom meeting");
    drawParticipants();
  }
  
  if ((WiFi.status() == WL_CONNECTED))
  {
    for (int i = 0; i < 10; i++)
    {
      // Serial.print("Connected to ");
      drawWifiConnection();
      // Serial.print(esid);
      delay(100);
    }
  }
  else
  {
    Heltec.display->clear();
    Heltec.display->drawStringMaxWidth(0, 0, 128,"please switch to Access point mode to reconfigure the Wifi");
    Heltec.display->display();
    // Serial.println("please switch to Access point mode to reconfigure the WiFi");
    return;
  }
  delay(100);
}

void doRising()
{
  totalPressTime = millis()-pressedTime;
  pressedTime = 0;
  Serial.printf("total pressed time %lu \n",totalPressTime);
}

void doFalling()
{
  pressedTime = millis();
  buttonPressed=1;
  Serial.printf("millis in interrupt %lu \n",millis());
 
}



//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void) {
  int c = 0;
  //Serial.println("Waiting for Wifi to connect");
  while ( c < 10 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(100);
    Serial.print("*");
    c++;
  }
  return false;
}
void launchWeb()
{
  if (WiFi.status() == WL_CONNECTED)
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
}
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      //Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    //st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("Virtual Coffe Kitchen", "12345678");
  launchWeb();
}
void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.restart();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}



  
