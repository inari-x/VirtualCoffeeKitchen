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

int PrevParticipants = 0;
int statusCode;                                //for config result
unsigned long pressedTime  = 0;
unsigned long totalPressTime = 0;
int buttonPressed = 0;
String content;
String esid;
String epass = "";
String eemail = "";
bool leaveWebUi;
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
void print_wakeup_reason()
{
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

int getCurrentParticipants()
{
  int participants;  
  HTTPClient http;
  http.begin("http://141.45.146.242:80/participant-count");
  int httpCode = http.GET();
  Serial.println(httpCode);
  // if (httpCode != 200) {
  //   Serial.println("Error making API request, HTTP code: " + String(httpCode));
  //   return -1;
  // }
  Serial.println("Server response: " + http.getStream());
  String payload = http.getString();
  // if (payload.length() == 0) {
  //   Serial.println("Error: Payload is empty");
  //   return -1;
  // }
  Serial.println("Payload: " + payload);
  payload.trim();
  participants = atoi(payload.c_str());
  // if (participants == 0 && payload != "0") {
  //   Serial.println("Error: Payload is not a valid integer");
  //   return -1;
  // }
  Serial.println("Participants count: " + String(participants));
  return participants;
}

void displayParticipantsCount(int count)
{
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(50, 25, String(count));
    Heltec.display->display();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display -> drawXbm(110,0,logo_width,logo_height,(const unsigned char *)logo_bits);
    Heltec.display -> display();
}

void flash()
{
  for (int i = 0; i < 10; i++){
      Heltec.display -> setBrightness(255);
      Heltec.display -> drawXbm(0,0,flasher_width,flasher_height,(const unsigned char *)flasher);
      Heltec.display -> display();
      delay(250);
      Heltec.display -> setBrightness(0);
      delay(250);
    }
    Heltec.display -> setBrightness(255);
    Heltec.display -> clear();
}

void displayBatteryAndWifi() 
{
  int batteryLevel = round((analogRead(37) / 4095.0) * 100);
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

void loop() 
{
  if(totalPressTime>0 && totalPressTime<2500 && buttonPressed==1)
  {
    buttonPressed=0;
    totalPressTime=0;
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16);    
    Heltec.display->drawString(10, 23, "please wait");
    Heltec.display->display();
    HTTPClient http;
    http.begin("http://141.45.146.242:80/join-meeting");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String payload = eemail.c_str();
    int httpCode = http.POST(payload);
    Serial.println("Joining the zoom meeting");
    // Check if the request was successsful
    if (httpCode == HTTP_CODE_OK) {
      Heltec.display->clear();   
      Heltec.display->drawString(10, 23, "Joining zoom");
      Heltec.display->display();
      // Read the response data
      String response = http.getString();
      Serial.println(response);
    }
    Heltec.display->setFont(ArialMT_Plain_10);    
    // Cleanup
    http.end();
  }
  else if(totalPressTime>=3000 &&totalPressTime<=8000 && buttonPressed==1){
    buttonPressed = 0;
    totalPressTime=0;  
    WiFi.disconnect();
    setupAP();// Setup HotSpot
    Serial.println("The access point is set up");
    leaveWebUi = false;
    while (!leaveWebUi)
    {
      delay(10);
      server.handleClient();
    }
  }
  else if (totalPressTime>9000 && buttonPressed==1)
  {
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16); 
    Heltec.display->drawString(10, 20, "Going to sleep..");
    Heltec.display->display();
    delay(2000);   
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
  }
  displayBatteryAndWifi();
}

void doRising()
{
  totalPressTime = millis()-pressedTime;
  pressedTime=0;
  buttonPressed=1;
  Serial.printf("total pressed time %lu \n",totalPressTime);
}

void doFalling()
{
  pressedTime = millis();
  Serial.printf("millis in interrupt %lu \n",millis());
}
//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void) 
{
  int c = 0;
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
  // if (WiFi.status() == WL_CONNECTED)
  // Serial.println(WiFi.localIP());
  // Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.softAP("Virtual Coffe Kitchen");
  Heltec.display-> clear();
  Heltec.display->drawString(15, 25, "Please connect to:");
  Heltec.display->drawString(13, 34, "Virtual Coffe Kitchen");
  Heltec.display->display();
  launchWeb();
}

String scanNetwork()
{
  delay(100);
  String  st;
  int n = WiFi.scanNetworks();
  if (n == 0){
    Serial.println("no networks found");
  }
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
    st += "</li>";
  }
  st += "</ol>";
  return st;  
  delay(100);  
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
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<head><style>body { font-family: Arial, sans-serif; text-align: center; }</style>";
      content += "<style>h1 { background-color: darkblue; color: white; padding: 20px; }</style></head>";
      content += "<h1>Welcome to the Wifi Credentials Update page</h1>";
      content += "<p>To update your wifi credentials, please click the 'Save' button below:</p>";
      content += "<p style='text-align: center;'>" + scanNetwork() + "</p>";
      content += "<p>Enter your new wifi credentials:</p><form method='get' action='setting'>";
      content += "<label>SSID: </label><input name='ssid' length=32><br><br>";
      content += "<label>Password: </label><input name='pass' length=64><br><br>";
      content += "<label>Email: </label><input name='email' length=64><br><br>";
      content += "<button style='background-color: darkblue; color: white; padding: 10px 20px; border: none; border-radius: 5px;'>Save</button></form>";
      content += "</div></body></html>";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qemail = server.arg("email");
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
        for (int i = 0; i < qemail.length(); ++i)
        {
          EEPROM.write(96 + i, qemail[i]);
          Serial.print("Wrote: ");
          Serial.println(qemail[i]);
        }
        EEPROM.commit();
        leaveWebUi = true;
        content = "{\"Success\":\"Saved to EEPROM. Resetting to boot into new wifi wifi\"}";
        statusCode = 200;
        ESP.restart();
      } 
      else {
        leaveWebUi = true;
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
      }
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(statusCode, "application/json", content);
        Heltec.display-> clear();
      });
  }
}