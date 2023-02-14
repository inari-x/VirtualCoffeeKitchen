#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <cstdlib>
#include "Arduino.h"
#include "heltec.h"
#include <Wire.h> 
#include <iostream>   
#include <string>
#include "images.h"

#define BUTTON_PIN 0
#define WIFI_ICON_16x16 "WIFI_ICON_16x16"

//Variables
int CurrentParticipants = 10;
int PrevParticipants = 0;
int i = 0;
int statusCode;
const char* ssid = "Default SSID";
const char* passphrase = "Default passord";
const char* email = "Default email";
unsigned long pressedTime  = 0;
unsigned long totalPressTime = 0;
int buttonPressed=0;
int participants = 0;
String st;
String content;
String esid;
String epass = "";
String eemail = "";
IPAddress ip;
String ipStr;
//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
//Establishing Local server at port 80
WebServer server(80);

#include "driver/rtc_io.h"
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void isr()
{
  if (digitalRead(0) == 0)
  { 
    doFalling();
    delay(10);
  }
  else {
    doRising();
    delay(10);
  }
}
int getCurrentParticipants(){
  //send the api request here
  // HTTPClient http;
  // http.begin("http://141.45.146.242:80/participant-count");
  // int httpCode = http.GET();
  // Serial.println("i am HERE: " + http.getString());
  // String response = http.getString();
  // int payload = atoi(response.c_str());
  // participants = payload;
  // Heltec.display->clear();
  // Heltec.display->setFont(ArialMT_Plain_10);
  // Heltec.display->drawString(50, 25, String(payload));
  // Heltec.display->display();
  // participants = payload;
  // return participants;  

  // int participants = payload.toInt();
  // participants = participants + 1;
  // CurrentParticipants = participants;
  // return participants;

  HTTPClient http;
  http.begin("http://141.45.146.242:80/participant-count");
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.println("Error making API request, HTTP code: " + String(httpCode));
    return -1;
  }
  Serial.println("Server response: " + http.getStream());
  String payload = http.getString();
  if (payload.length() == 0) {
    Serial.println("Error: Payload is empty");
    return -1;
  }
  Serial.println("Payload: " + payload);
  payload.trim();
  participants = atoi(payload.c_str());
  if (participants == 0 && payload != "0") {
    Serial.println("Error: Payload is not a valid integer");
    return -1;
  }
  Serial.println("Participants count: " + String(participants));
  return participants;
  }


void displayParticipantsCount(int count){
  // Heltec.display->setFont (ArialMT_Plain_24);
  //   Heltec.display->drawString(50, 25,String(count));
  //   Heltec.display->display();

    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(50, 25, String(participants));
    Heltec.display->display();

    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display -> drawXbm(110,0,logo_width,logo_height,(const unsigned char *)logo_bits);
    Heltec.display -> display();
}

void flash(){
  for (int i = 0; i < 5; i++){
      Heltec.display -> setBrightness(255);
      Heltec.display -> drawXbm(0,0,flasher_width,flasher_height,(const unsigned char *)flasher);
      Heltec.display -> display();
      delay(500);
      Heltec.display -> setBrightness(0);
      delay(500);
    }
    Heltec.display -> setBrightness(255);
    Heltec.display -> clear();
}


void displayBatteryAndWifi() {
  int Millivolts = map (analogRead(37), 0, 3000, 150, 2450);
  int batteryVoltage = Millivolts;
  int batteryLevel = (batteryVoltage / 1024) * 100;
  String batteryString = String(batteryLevel) + "%";
  Heltec.display->clear();
  if (testWifi()) {
    int current = getCurrentParticipants();
    if (current > PrevParticipants){
      Serial.println("inside the if");
      flash();
    }
    PrevParticipants = current;
    displayParticipantsCount(current);
  } else {
    Heltec.display->drawString(7, 25, "Device is not connected");
    Heltec.display->drawString(5, 34, "Switch to AP by pressing");
    Heltec.display->drawString(13, 43, "5 SECs on the button");
  }
  Heltec.display->drawString(0, 0, batteryString);
  Heltec.display->display();
}



void setup()
{
  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  // SCREEN -----------------------------------------------------------------------------------------
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  delay(300);
  //---------------------------------------------------------------------------------------------------
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  attachInterrupt(0, isr, CHANGE);
  // pinMode(18, INPUT_PULLUP);
  //---------------------------------------- Read eeprom for ssid and pass
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  for (int i = 96; i < 150; ++i)
  {
    eemail += char(EEPROM.read(i));
  }
  Serial.print("EMAIL: ");
  Serial.println(eemail);
  WiFi.begin(esid.c_str(), epass.c_str());
  pinMode(17, INPUT_PULLUP);
  rtc_gpio_deinit(GPIO_NUM_2);
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  rtc_gpio_pullup_en(GPIO_NUM_2);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_2,1); //1 = High, 0 = Low
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
  displayBatteryAndWifi();
  if(totalPressTime>0 && totalPressTime<4000 && buttonPressed==1)
  {
    buttonPressed=0;
    totalPressTime=0;
    Serial.println("Joining the zoom meeting");
    drawParticipants();
  }
  
  else if(totalPressTime>=4000 &&totalPressTime<=10000 && buttonPressed==1){
    buttonPressed = 0;
    totalPressTime=0;
    WiFi.disconnect();
    setupAP();// Setup HotSpot
    Serial.println("The access point is set up");
    while ((WiFi.status() != WL_CONNECTED))
    {
      // Heltec.display->clear();
      // Heltec.display->drawString(10, 0, "Please go to the ip");
      // Heltec.display->display();
      delay(10);
      server.handleClient();
    }
    
  }
  else if (totalPressTime>10000 && buttonPressed==1)
  {
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
  }
  delay(100);
}

void doRising()
{
  totalPressTime = millis()-pressedTime;
  pressedTime=0;
  buttonPressed=1;
  Serial.printf("total pressed time %lu \n",totalPressTime);
  delay(2000);
}

void doFalling()
{
  pressedTime = millis();
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
  // const char apName = "Virtual Coffe Kitchen";
  // const char apPassword = "12345678";
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
  WiFi.softAP("Virtual Coffe Kitchen");
  Heltec.display-> clear();
  Heltec.display->drawString(15, 25, "Please connect to:");
  Heltec.display->drawString(13, 34, "Virtual Coffe Kitchen");
  Heltec.display->display();
  launchWeb();
}
void handleCSS() {
  String css = 
  "form {"
    "display: flex;"
    "flex-direction: column;"
    "align-items: flex-start;"
    "margin: 20px;"
  "}"
  "label, input[type='submit'] {"
    "margin-top: 10px;"
  "}"
  "input[type='text'] {"
    "width: 200px;"
    "height: 30px;"
    "padding: 5px;"
    "font-size: 16px;"
  "}"
  "input[type='submit'] {"
    "background-color: blue;"
    "color: white;"
    "width: 100px;"
    "height: 40px;"
    "font-size: 20px;"
  "}";

  server.send(200, "text/css", css);
}

void createWebServer()
{
  ip = WiFi.softAPIP();
  ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  while (WiFi.softAPgetStationNum()==0);   // do not forget to set a timeout
  Heltec.display-> clear();
  Heltec.display->drawString(13, 25, "Inter the following IP in");
  Heltec.display->drawString(35, 34, "your browser:");
  Heltec.display->drawString(35, 43, ipStr);
  Heltec.display->display();
  {
    server.on("/", []() {
      content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page <head><link rel='stylesheet' type='text/css' href='/style.css'></head>";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>  PASS: </label><input name='pass' length=64></br><label>EMAIL: </label><input name='email' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      // IPAddress ip = WiFi.softAPIP();
      // String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
   server.on("/", []() {
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  content = "<!DOCTYPE HTML>\r\n<html><h1>Welcome to the Wifi Credentials Update page</h1>";
  content += "<p>To update your wifi credentials, please click the 'Scan' button below:</p>";
  content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"Scan\"></form>";
  content += "<p>Your IP address: " + ipStr + "</p>";
  content += "<p>" + st + "</p>";
  content += "<p>Enter your new wifi credentials:</p><form method='get' action='setting'>";
  content += "<label>SSID: </label><input name='ssid' length=32><br><br>";
  content += "<label>Password: </label><input name='pass' length=64><br><br>";
  content += "<input type='submit' value='Save'></form>";
  content += "</html>";
  server.send(200, "text/html", content);
});

server.on("/scan", []() {
  //setupAP();
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  content = "<!DOCTYPE HTML>\r\n<html><p><a href='/'>Go back</a></p>";
  server.send(200, "text/html", content);
});

server.on("/setting", []() {
  String qsid = server.arg("ssid");
  String qpass = server.arg("pass");
  if (qsid.length() > 0 && qpass.length() > 0) {
    Serial.println("Clearing EEPROM");
    for (int i = 0; i < 96; ++i) {
      EEPROM.write(i, 0);
    }
    Serial.println(qsid);
    Serial.println("");
    Serial.println(qpass);
    Serial.println("");
    Serial.println("Writing EEPROM SSID:");
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
    content = "{\"Success\":\"Saved to EEPROM. Resetting to boot into new wifi wifi\"}";
        statusCode = 200;
        ESP.restart();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
      Heltec.display-> clear();
    });
  }
}