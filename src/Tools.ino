#include <Button2.h>

#include <HTTP_Method.h>
#include <Uri.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <WebSocketsServer.h> // Install WebSockets by Markus Sattler from IDE Library manager

#include <TFT_eSPI.h>

#include <DHT.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiSTA.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiType.h>
#include <WiFiUdp.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>
#include <Blynk.h>
#include <BlynkSimpleEsp32.h>
// TODO: Make sure to add approximate.h library to calculate distance of devices using wifi
// TODO: Add Bluetooth
// WARNING: BluetoothSerial does not work on ESP32S3

#define DEBUG_ON  0
#if DEBUG_ON
#define DBG_begin(...)    Serial.begin(__VA_ARGS__)
#define DBG_print(...)    Serial.print(__VA_ARGS__)
#define DBG_println(...)  Serial.println(__VA_ARGS__)
#define DBG_printf(...)   Serial.printf(__VA_ARGS__)
#else
#define DBG_begin(...)
#define DBG_print(...)
#define DBG_println(...)
#define DBG_printf(...)
#endif

#define BLYNK_TEMPLATE_ID "TMPLf-rVGNZE"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "rHmb2QrPJR5m-XiTh0tpa5jT7-_L6332"

BLEScan* blescan;
int scanTime = 5; //In seconds

#include <ArduinoOTA.h>

using namespace httpsserver;

SSLCert * cert;
HTTPSServer * secureServer;

HTTPClient redditClient;

uint32_t BG_THEME = TFT_BLACK;
uint32_t FG_THEME = TFT_WHITE;
String SSIDs[15];
String Passwords[15];
const char* WiFiMenuOptions[] = {"Scan WiFi", "Connect", "Disconnect", "STA info", "Erase WiFi", "Start AP", "Stop AP", "AP Info"};
const char* BTMenuOptions[] = {"BLE Scan"};
const char* HomeOptions[] = {"WiFi", "Bluetooth", "ESP32 mDNS", "Webserver","Theme", "Display", "Sensors", "Reddit", "Keyboard", "Unlock", "Factory Reset"};
String reddit_json = "";
bool optionsEnabled[] = {true, true, true, true, true, true, true, true};
char buff[512];
bool isAppOpen = false;
bool staapmodepublic = false;
char* publicSelectedOption = "WiFi";
int publicSelectedIndex = 0;
#define TXT_SIZE 2
#define LINE_HEIGHT 20

// Get number of items in string array
template< typename T, size_t N > size_t ArraySize (T (&) [N]){ return N; }
 
TFT_eSPI tft = TFT_eSPI();

DynamicJsonDocument doc(49152);

#define btn1_pin 0
#define btn2_pin 14


Button2 btn1 = Button2(btn1_pin);
Button2 btn2 = Button2(btn2_pin);

#define DHT_PIN 2
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);


USBHIDKeyboard Keyboard;

MDNSResponder mdns;

#include "index_html.h"

Preferences prefs;



TaskHandle_t BGProcess;

const int MAX_ROWS = 6;
const int MAX_COLS = 17;
const uint8_t Keycodes[MAX_ROWS][MAX_COLS] = {
  // Row 0 (top row)
  {KEY_TAB, KEY_F1 , KEY_F2 , KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12},
  // Row 1
  {'`',     '1',    '2',     '3',    '4',    '5',    '6',    '7',    '8',    '9',    '0',     '-',     '+',     '\b', KEY_INSERT, KEY_HOME, KEY_PAGE_UP},
  // Row 2
  {KEY_TAB, 'q',    'w',     'e',    'r',    't',    'y',    'u',    'i',    'o',    'p',     '[',     ']',     '\\', KEY_DELETE, KEY_END,  KEY_PAGE_DOWN},
  // Row 3
  {KEY_CAPS_LOCK, 'a', 's',  'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',     '\'',     KEY_RETURN},
  // Row 4
  {KEY_LEFT_SHIFT,'z', 'x',  'c',    'v',    'b',    'n',    'm',    ',',    '.',    '/',     KEY_RIGHT_SHIFT, 0, KEY_UP_ARROW},
  // Row 5
  {KEY_LEFT_CTRL, KEY_LEFT_GUI , KEY_LEFT_ALT, ' ', KEY_RIGHT_ALT, KEY_RIGHT_GUI, 0, KEY_RIGHT_CTRL, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW},
};

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  DBG_printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      DBG_printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DBG_printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      {
        DBG_printf("[%u] get Text: [%d] %s \r\n", num, length, payload);

        StaticJsonDocument<96> doc;
        DeserializationError error = deserializeJson(doc, payload);;

        if (error) {
          DBG_print(F("deserializeJson() failed: "));
          DBG_println(error.f_str());
          return;
        }
        const char* event = doc["event"];
        int row = doc["row"];
        if (row < 0) {
          DBG_printf("row negative %d\n", row);
          row = 0;
        }
        if (row >= MAX_ROWS) {
          DBG_printf("row too high %d\n", row);
          row = MAX_ROWS - 1;
        }

        int col = doc["col"];
        if (col < 0) {
          DBG_printf("col negative %d\n", col);
          col = 0;
        }
        if (col >= MAX_COLS) {
          DBG_printf("col too high %d\n", col);
          col = MAX_COLS - 1;
        }
        if (strcmp(event, "touch start") == 0) {
          Keyboard.press(Keycodes[row][col]);
          // tft.println(Keycodes[row][col]);
        }
        else if (strcmp(event, "touch end") == 0) {
          Keyboard.release(Keycodes[row][col]);
        }
      }
      break;
    case WStype_BIN:
      DBG_printf("[%u] get binary length: %u\r\n", num, length);
      //      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      DBG_printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRootU()
{
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFoundU()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
bool messageReceived = false;
String receivedText = "";
bool capsOn = false;
void webSocketEventKI(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  DBG_printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
  switch(type) {
    case WStype_DISCONNECTED:
      DBG_printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        DBG_printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      {
        DBG_printf("[%u] get Text: [%d] %s \r\n", num, length, payload);

        StaticJsonDocument<96> doc;
        DeserializationError error = deserializeJson(doc, payload);;

        if (error) {
          DBG_print(F("deserializeJson() failed: "));
          DBG_println(error.f_str());
          return;
        }
        const char* event = doc["event"];
        int row = doc["row"];
        if (row < 0) {
          DBG_printf("row negative %d\n", row);
          row = 0;
        }
        if (row >= MAX_ROWS) {
          DBG_printf("row too high %d\n", row);
          row = MAX_ROWS - 1;
        }

        int col = doc["col"];
        if (col < 0) {
          DBG_printf("col negative %d\n", col);
          col = 0;
        }
        if (col >= MAX_COLS) {
          DBG_printf("col too high %d\n", col);
          col = MAX_COLS - 1;
        }
        if (strcmp(event, "touch start") == 0) {
          if(Keycodes[row][col] != KEY_RETURN) {
            String tempchar = "";
            if(String((const char*)doc["name"]).indexOf("<br>") == -1 && String((const char*)doc["name"]) != "Backspace" && String((const char*)doc["name"]) != "Caps<br>Lock" && String((const char*)doc["name"]) != "Space") {
              if(capsOn) {
                tempchar = String((const char*)doc["name"]);
                tempchar.toUpperCase();
                receivedText += tempchar;
              }
              else {
                tempchar = String((const char*)doc["name"]);
                tempchar.toLowerCase();
                receivedText += tempchar;
              }
              tft.print(tempchar);
            }
            else if(String((const char*)doc["name"]).indexOf("<br>") > -1 && String((const char*)doc["name"]) != "Caps<br>Lock" && String((const char*)doc["name"]) != "Space") {
              if(capsOn) {
                tempchar = String((const char*)doc["name"]).substring(String((const char*)doc["name"]).indexOf("<br>")+4);
                receivedText += tempchar;
              }
              else {
                tempchar = String((const char*)doc["name"]).substring(0, 1);
                receivedText += tempchar;
              }
              tft.print(tempchar);
            }
            else if(String((const char*)doc["name"]) == "Caps<br>Lock") {
              if(capsOn) {
                capsOn = false;
                tft.fillCircle(10, TFT_HEIGHT-20, 10, TFT_RED);
                delay(200);
              }
              else {
                capsOn = true;
                tft.fillCircle(10, TFT_HEIGHT-20, 10, TFT_GREEN);
                delay(200);
              }
            }
            else if(String((const char*)doc["name"]) == "Backspace") {
              receivedText.remove(receivedText.length()-1);
              tft.print(receivedText);
              Serial.println(receivedText);
            }
            else if(String((const char*)doc["name"]) == "Space") {
              receivedText += " ";
              tft.print(" ");
            }
            
          }
          else if(Keycodes[row][col] == KEY_BACKSPACE) {
            
          }
          else {
            if(Keycodes[row][col] == KEY_RETURN) {
              tft.fillScreen(TFT_BLACK);
              tft.setTextColor(TFT_WHITE);
              tft.setTextSize(2);
              tft.setCursor(0,0);
              tft.println(receivedText);
              delay(2000);
              messageReceived = true;
              Serial.println("Keyboard Input Event Done");
            }
          }
        }
        else if (strcmp(event, "touch end") == 0) {
          
        }
      }
      break;
    case WStype_BIN:
      DBG_printf("[%u] get binary length: %u\r\n", num, length);
      //      hexdump(payload, length);

      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    default:
      DBG_printf("Invalid WStype [%d]\r\n", type);
      break;
  }
}

void handleRootKI()
{
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFoundKI()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // tft.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      tft.print("Name: ");
      tft.println(advertisedDevice.getName().c_str());
      tft.print("Address: ");
      tft.println(advertisedDevice.getAddress().toString().c_str());
      tft.print("Icon: ");
      tft.println((char*)advertisedDevice.getAppearance());
      // tft.print("RSSI: ");
      // tft.println((char *)advertisedDevice.getRSSI());
      
    }
};

int selectedIndexPrivate = 0;
void createList(const char* options[], int length, void (*callback)(char* selectedOption, int selectedIndex), uint32_t BG, uint32_t FG, int txtSize, int lineHeight) {
    selectedIndexPrivate = 0;
    drawList(options, length, BG, FG, txtSize, lineHeight);
    btn2.setClickHandler([options, BG, FG, txtSize, lineHeight, length](Button2 & b) {
        if(selectedIndexPrivate >= length-1) {
            selectedIndexPrivate = 0;
        }
        else {
            selectedIndexPrivate++;
        }
        drawList(options, length, BG, FG, txtSize, lineHeight);
    });
    btn1.setClickHandler([options, BG, FG, txtSize, lineHeight, length](Button2 & b) {
            if(selectedIndexPrivate <= 0) {
                selectedIndexPrivate = length-1;
            }
            else {
                selectedIndexPrivate--;
            }
            drawList(options, length, BG, FG, txtSize, lineHeight);
    });
    btn2.setLongClickHandler([callback, options](Button2 & b) {
      char* selectedOption = (char*)options[selectedIndexPrivate];
      callback(selectedOption, selectedIndexPrivate);
      selectedIndexPrivate = 0;
    });
}
void createList(String options[], int length, void (*callback)(String selectedOption, int selectedIndex), uint32_t BG, uint32_t FG, int txtSize, int lineHeight) {
    selectedIndexPrivate = 0;
    drawList(options, length, BG, FG, txtSize, lineHeight);
    btn2.setClickHandler([options, BG, FG, txtSize, lineHeight, length](Button2 & b) {
        if(selectedIndexPrivate >= length-1) {
            selectedIndexPrivate = 0;
        }
        else {
            selectedIndexPrivate++;
        }
        drawList(options, length, BG, FG, txtSize, lineHeight);
    });
    btn1.setClickHandler([options, BG, FG, txtSize, lineHeight, length](Button2 & b) {
            if(selectedIndexPrivate <= 0) {
                selectedIndexPrivate = length-1;
            }
            else {
                selectedIndexPrivate--;
            }
            drawList(options, length, BG, FG, txtSize, lineHeight);
    });
    btn2.setLongClickHandler([callback, options](Button2 & b) {
      String selectedOption = (String)options[selectedIndexPrivate];
      callback(selectedOption, selectedIndexPrivate);
      selectedIndexPrivate = 0;
    });
}
void drawList(String options[], int length, uint32_t BG, uint32_t FG, int txtSize, int lineHeight){
    tft.setRotation(0);
    tft.fillScreen(BG);
    tft.setTextSize(txtSize);
    for(int i = 0; i < length; i++) {
        tft.drawRect(0, 0, TFT_WIDTH, lineHeight+(lineHeight*i), FG);
        if(i == selectedIndexPrivate) {
            tft.setTextColor(BG, FG);
        }
        else {
            tft.setTextColor(FG, BG);
        }
        tft.setCursor(0,(lineHeight*i)+1);
        tft.println(options[i]);
    }
}
void drawList(const char* options[], int length, uint32_t BG, uint32_t FG, int txtSize, int lineHeight){
    tft.setRotation(0);
    tft.fillScreen(BG);
    tft.setTextSize(txtSize);
    for(int i = 0; i < length; i++) {
        tft.drawRect(0, 0, TFT_WIDTH, lineHeight+(lineHeight*i), FG);
        if(i == selectedIndexPrivate) {
            tft.setTextColor(BG, FG);
        }
        else {
            tft.setTextColor(FG, BG);
        }
        tft.setCursor(0,(lineHeight*i)+1);
        tft.println(options[i]);
    }
}





void homeMenuCallback(char* selectedOption, int selectedIndex) {
  publicSelectedOption = selectedOption;
  if(isAppOpen == false) {
      if(selectedOption == "Theme") {
        openThemeApp();
      }
      else if(selectedOption == "WiFi") {
        openWiFiSubmenu();
      }
      else if(selectedOption == "Bluetooth") {
        openBTSubmenu();
      }
      else if(selectedOption == "Webserver") {
        openWebServerApp();
      }
      else if(selectedOption == "Display") {
        openDisplayApp();
      }
      else if(selectedOption == "Sensors") {
        openSensorApp();
      }
      else if(selectedOption == "Reddit") {
        openRedditApp(false);
      }
      else if(selectedOption == "Keyboard") {
        openKeyboardApp();
      }
      else if(selectedOption == "ESP32 mDNS") {
        openmDNSApp();
      }
      else if(selectedOption == "Unlock") {
        openUnlockApp();
      }
      else if(selectedOption == "Factory Reset") {
        openFactoryResetApp();
      }
    }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  tft.print("Starting prefs...");
  prefs.begin("unitool", false);
  tft.println("done");
  tft.print("Starting Serial...");
  Serial.begin(115200);
  tft.println("done");
  tft.print("Setting Theme...");
  setTheme(false);
  tft.println("done");
  WiFi.setAutoReconnect(true);
  tft.print("Starting DHT...");
  tft.println("done");
  dht.begin();
  tft.print("Starting BT...");
  BLEDevice::init("ESP32");
  tft.println("done");
  tft.println("Starting WiFi & Blynk...");
  WiFi.mode(WIFI_STA);
  tft.println("Mode Set");
  int16_t n = WiFi.scanNetworks();
  if (n == 0) {
      tft.setTextColor(TFT_RED);
      tft.println("no networks found [FATAL]");
      tft.setTextColor(TFT_WHITE);
      return;
  } else {
      for (int i = 0; i < n; i++) {
        tft.print("Attempting ");
        tft.println(WiFi.SSID(i).c_str());
        tft.println(" With pwd: ");
        tft.println(prefs.getString(WiFi.SSID(i).c_str()).c_str()); 
        Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID(i).c_str(), prefs.getString(WiFi.SSID(i).c_str()).c_str());
        delay(2000);
        if(Blynk.CONNECTED) {
          tft.println("CONNECTED");
          break;
        }
      }
      if(!WiFi.isConnected()) {
        tft.setTextColor(TFT_RED);
        tft.println("failed [FATAL]");
        tft.setTextColor(TFT_WHITE);
        return;
      }
      else {
        tft.println("done");
      }
  }
  tft.println("Launching...");
  delay(500);
  DisplayHomeScreen();
  btn1.setDoubleClickHandler([](Button2 & b) {
    isAppOpen = false;
    DisplayHomeScreen();
  });
  xTaskCreatePinnedToCore(
      BackgroundProcesses, /* Function to implement the task */
      "Background Processes", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &BGProcess,  /* Task handle. */
      0); /* Core where the task should run */
}

void setTheme(bool Light) {
  if(Light) {
    BG_THEME = TFT_WHITE;
    FG_THEME = TFT_BLACK;
  }
  else {
    BG_THEME = TFT_BLACK;
    FG_THEME = TFT_WHITE;
  }
}
bool isThemeLight() {
  if(BG_THEME == TFT_BLACK && FG_THEME == TFT_WHITE) {
    return false;
  }
  else {
    return true;
  }
}
void DisplayHomeScreen() {
  createList(HomeOptions, ArraySize(HomeOptions), homeMenuCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}
// Apps Start here
// Define an app using void openAppName() {}
// Make sure to always have isAppOpen = true; in your app at the first line
// if you need btn clicks use function btn#.setClickHandler([](Button2 & b) { // code goes here});
// btn#.setLongClickHandler is also allowed but double click on the left btn (btn1) is not allowed because it is used as an app exit btn
// void openThemeApp() {
//   // isAppOpen = true;
//   if(isThemeLight()) {
//         setTheme(false);
//       }
//       else {
//         setTheme(true);
//       }
//       DisplayHomeScreen();
// }


const char* themeOptions[] = {"Dark", "Light", "Red", "Orange", "Yellow", "Green", "Blue", "Purple"};

void themeCallback(char* selectedThemeOption, int selectedThemeIndex) {
  if(selectedThemeOption == "Red") {
    BG_THEME = TFT_RED;
    FG_THEME = TFT_WHITE;
  }
  else if(selectedThemeOption == "Orange") {
    BG_THEME = TFT_ORANGE;
    FG_THEME = TFT_BLACK;
  }
  else if(selectedThemeOption == "Yellow") {
    BG_THEME = TFT_YELLOW;
    FG_THEME = TFT_BLACK;
  }
  else if(selectedThemeOption == "Green") {
    BG_THEME = TFT_GREEN;
    FG_THEME = TFT_BLACK;
  }
  else if(selectedThemeOption == "Blue") {
    BG_THEME = TFT_BLUE;
    FG_THEME = TFT_WHITE;
  }
  else if(selectedThemeOption == "Purple") {
    BG_THEME = TFT_PURPLE;
    FG_THEME = TFT_WHITE;
  }
  else if(selectedThemeOption == "Dark") {
    BG_THEME = TFT_BLACK;
    FG_THEME = TFT_WHITE;
  }
  else if(selectedThemeOption == "Light") {
    BG_THEME = TFT_WHITE;
    FG_THEME = TFT_BLACK;
  }
}
void openThemeApp() {
  isAppOpen = true;
  createList(themeOptions, ArraySize(themeOptions), themeCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}

void WiFiSubmenuCallback(char* SelectedWiFiSubmenuOption, int SelectedWiFiSubmenuIndex) {
  if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Scan WiFi") {
      openScanWifiApp();
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Connect") {
      openConnectApp();
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Disconnect") {
      openDisconnectApp(false);
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "STA info") {
      openSTAInfoApp();
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Erase WiFi") {
      openDisconnectApp(true);
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Start AP") {
      openStartAPApp();
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "Stop AP") {
      openDisconnectAPApp();
    }
    else if(WiFiMenuOptions[SelectedWiFiSubmenuIndex] == "AP Info") {
      openAPInfoApp();
    }
    else {
      tft.setCursor(0,0);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("An Error Occurred");
    }
}
void openWiFiSubmenu() {
  isAppOpen = true;
  createList(WiFiMenuOptions, ArraySize(WiFiMenuOptions), WiFiSubmenuCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}

void BTMenuCallback(char* BTMenuSelectedOption, int BTMenuSelectedIndex) {
  if(BTMenuOptions[BTMenuSelectedIndex] == "BLE Scan") {
      openBLEScanApp();
  }
  else if(BTMenuOptions[BTMenuSelectedIndex] == "Connect") {
     
  }
}
void openBTSubmenu() {
  isAppOpen = true;
  createList(BTMenuOptions, ArraySize(BTMenuOptions), BTMenuCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}

void openScanWifiApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(TXT_SIZE);
  tft.drawString("Scanning...",0, tft.height()/2);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int16_t n = WiFi.scanNetworks();
  tft.fillScreen(BG_THEME);
  if (n == 0) {
      tft.drawString("no networks found", tft.width()/2, tft.height()/2);
  } else {
      tft.setCursor(0, 0);
      for (int i = 0; i < n; ++i) {
          sprintf(buff,
                  "[%d]:%s(%d)",
                  i + 1,
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i));
          tft.println(buff);
      }
  }
}
void openBLEScanApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextColor(FG_THEME);
  tft.setTextSize(1);
  tft.setCursor(0,0);
  blescan = BLEDevice::getScan(); //create new scan
  blescan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  blescan->setActiveScan(true); //active scan uses more power, but get results faster
  blescan->setInterval(100);
  blescan->setWindow(99);  // less or equal setInterval value
  BLEScanResults foundDevices = blescan->start(scanTime, false);
  tft.print("Devices found: ");
  tft.println(foundDevices.getCount());
  tft.println("Scan done!");
}

String getKeyboardInput(String prompt, bool shutDownAPAfterUse) {
  messageReceived = false;
  bool keyboardRunning = true;
  receivedText = "";
  tft.fillScreen(BG_THEME);
  tft.setCursor(0,0);
  tft.setTextColor(FG_THEME);
  // if(WiFi.softAPIP() == IPAddress(0,0,0,0)) {
    if(!WiFi.isConnected()) {
      // WiFi.mode(WIFI_STA);
      // WiFi.begin("Moukayed", "0566870554");
      WiFi.mode(WIFI_AP); // explicitly set mode, esp defaults to STA+AP
      WiFi.softAP("ESP32Input");
    }
  // }
  delay(3000);
  if (mdns.begin("esp32keyboardinput")) {
    // tft.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    tft.println("MDNS.begin failed");
  }
  DBG_print(F("Connect to http://esp32keyboardinput.local or http://"));
  DBG_println(WiFi.localIP());
  tft.setTextColor(FG_THEME);
  tft.setTextSize(TXT_SIZE);
  // tft.println("http://esp32keyboardinput.local/");
  if(WiFi.getMode() == WIFI_STA) {
    // tft.println(WiFi.localIP());
  }
  else {
    // tft.println(WiFi.softAPIP());
  }
  server.on("/", handleRootKI);
  server.onNotFound(handleNotFoundKI);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEventKI);
  tft.println(prompt);
  tft.print("> ");
  while(keyboardRunning) {
        if(messageReceived) {
          keyboardRunning = false;
          server.close();
          webSocket.close();
          if(shutDownAPAfterUse) {
            WiFi.softAPdisconnect();
          }
          return receivedText;
        }
        else {
          server.handleClient();
          webSocket.loop();
        }
    }
}

void connectCallback(String ConnectSelectedOption, int ConnectSelectedIndex) {
  if(!staapmodepublic) {
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
    }
    if(SSIDs[ConnectSelectedIndex] != "Other...") {
      String pwd = prefs.getString(SSIDs[ConnectSelectedIndex].c_str(), "");
      WiFi.begin(SSIDs[ConnectSelectedIndex].c_str(), pwd.c_str());
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      int retrycon = 25;
      tft.fillScreen(BG_THEME);
      tft.print("Connecting to ");
      tft.print(SSIDs[ConnectSelectedIndex]);
      tft.println("...");
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        if (--retrycon == 0)
        {
          tft.setTextColor(TFT_RED);
          tft.println("ERROR: CONNECTION FAILED\n\nPlease reconnect. ");
          // delay(1500);
          // ESP.restart();
          break;
        }
        tft.print(".");
      }
    }
    else {
      tft.fillScreen(BG_THEME);
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      tft.setTextSize(TXT_SIZE);
      Serial.println("Loading SSID Keyboard");
      getKeyboardInput("Enter SSID", false);
      String ssid = receivedText;
      tft.fillScreen(TFT_BLACK);
      getKeyboardInput("Enter Password", true);
      String pswd = receivedText;
      prefs.putString(ssid.c_str(), pswd.c_str());
      WiFi.disconnect();
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pswd.c_str());
      ssid = "";
      pswd = "";
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      int retrycon = 25;
      tft.fillScreen(BG_THEME);
      tft.print("Connecting to ");
      tft.print(ssid);
      tft.println("...");
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(500);
        if (--retrycon == 0)
        {
          tft.setTextColor(TFT_RED);
          tft.println("ERROR: CONNECTION FAILED\n\nPlease reconnect. ");
          // delay(1500);
          // ESP.restart();
          break;
        }
        tft.print(".");
      }
    }

    tft.print("WiFi connected with IP: ");
    tft.println(WiFi.localIP());
    tft.println("DBL Click left to exit");
    if(staapmodepublic) {
      staapmodepublic = false;
    }
}
void openConnectApp() {
  isAppOpen = true;
  char bufff[512];
  tft.fillScreen(BG_THEME);
  tft.setTextColor(FG_THEME);
  tft.setTextSize(TXT_SIZE);
  tft.setCursor(0,0);
  tft.println("Loading...");
  WiFi.mode(WIFI_STA);
  int16_t n = WiFi.scanNetworks();
  if (n == 0) {
      
  } else {
      for (int i = 0; i < n; i++) {
        SSIDs[i] = WiFi.SSID(i).c_str();
      }
      SSIDs[n] = "Other...";
  }
  createList(SSIDs, 15, connectCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
  //                ^SSIDs can only hold 15 items
  
}
void openDisconnectApp(bool eraseCred) {
  WiFi.disconnect(eraseCred);
  DisplayHomeScreen();
}
void openSTAInfoApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(2);
  tft.setTextColor(FG_THEME);
  tft.setCursor(0,0);

  tft.print("IP: ");
  tft.println(WiFi.localIP());

  tft.print("\nMac Address: \n");
  tft.println(WiFi.macAddress());

  tft.print("\nSubnet Mask: \n");
  tft.println(WiFi.subnetMask());

  tft.print("\nGateway IP: \n");
  tft.println(WiFi.gatewayIP());

  tft.print("\nDNS: ");
  tft.println(WiFi.dnsIP());

  tft.print("\nBroadcast IP: \n");
  tft.println(WiFi.broadcastIP());

  tft.print("\nSSID: ");
  tft.print(WiFi.SSID());

  tft.print("\nPSK: \n");
  tft.println(WiFi.psk());

}
int selectedAPOption = 0;
const char* APOptions[] = {"Basic AP", "Dif. SSID AP", "Hidden AP", "One use AP", "AP IPv6", "STA+AP Mode"};
char* RandString[] = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","1","2","3","4","5","6","7","8","9","0"};
char TempSSID[12];
char TempPWD[12];
const char* blank = "";

void StartAPCallback(char* selectedAPOption, int selectedAPIndex) {
  WiFi.mode(WIFI_AP);
    if(APOptions[selectedAPIndex] == "Basic AP") {
      WiFi.softAP("ESP32 AP", "1234567890");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
    }
    else if(APOptions[selectedAPIndex] == "Dif. SSID AP") {
      WiFi.softAP("Notfreewifi", "dontaskforthepassword");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: Notfreewifi");
      tft.println("PWD: dontaskforthepassword");
    }
    else if(APOptions[selectedAPIndex] == "One use AP") {
      strcpy(TempSSID, blank);
      strcpy(TempPWD, blank);
      for(int i = 0; i < 9; i++) {
        strcat(TempSSID, RandString[random(ArraySize(RandString))]);
      }
      for(int i = 0; i < 9; i++) {
        strcat(TempPWD, RandString[random(ArraySize(RandString))]);
      }
      WiFi.softAP(TempSSID, TempPWD);
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.print("SSID: ");
      tft.println(TempSSID);
      tft.print("PWD: ");
      tft.println(TempPWD);
    }
    else if(APOptions[selectedAPIndex] == "Hidden AP") {
      WiFi.softAP("ESP32 AP", "1234567890", 1, 1);
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
      tft.setTextColor(TFT_RED);
      tft.println("\nNote: AP not visible on list, Press hidden network and type the details above. ");
      tft.setTextColor(FG_THEME);
    }
    else if(APOptions[selectedAPIndex] == "AP IPv6") {
      WiFi.softAP("ESP32 AP", "1234567890");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
      tft.print("IPv6: ");
      tft.println(WiFi.softAPenableIpV6());
    }
    else if(APOptions[selectedAPIndex] == "STA+AP Mode") {
      WiFi.mode(WIFI_MODE_APSTA);
      strcpy(TempPWD, blank);
      for(int i = 0; i < 9; i++) {
        strcat(TempPWD, RandString[random(ArraySize(RandString))]);
      }
      WiFi.softAP("ESP32 STA+AP", TempPWD);
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
      staapmodepublic = true;
      tft.fillScreen(BG_THEME);
      tft.setCursor(0,0);
      tft.println("Please connect to WiFi...");
    }
}
void openStartAPApp() {
  isAppOpen = true;
  createList(APOptions, ArraySize(APOptions), StartAPCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}

void openDisconnectAPApp() {
  WiFi.softAPdisconnect (true);
  DisplayHomeScreen();
}
void openAPInfoApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(2);
  tft.setTextColor(FG_THEME);
  tft.setCursor(0,0);

  tft.print("IP: ");
  tft.println(WiFi.softAPIP());

  tft.print("\nMac Address: \n");
  tft.println(WiFi.softAPmacAddress());

  tft.print("\nBroadcast IP: \n");
  tft.println(WiFi.softAPBroadcastIP());

  tft.print("\nNetwork ID: \n");
  tft.println(WiFi.softAPNetworkID());

  tft.print("\nConnected: ");
  tft.println(WiFi.softAPgetStationNum());

  tft.print("\nSSID: ");
  tft.print(WiFi.softAPSSID());

  tft.print("\nPSK: \n");
  tft.println(TempPWD);

}
bool serverOn = false;
bool httpsServerOn = false;
const char* webserverOptions[] = {"HTML Previewer", "Stop HTML Pre.", "HTTPS Server", "Stop HTTPS"};
int selectedWebserverOption = 0;

void webserverCallback(char* selectedWebserverOptions, int selectedWebserverIndex) {
  for(int i = 0; i < ArraySize(webserverOptions); i++) {
      if(i == selectedWebserverIndex) {
        if(webserverOptions[i] == "HTML Previewer") {
          server.on("/", handleRoot);
          server.on("/view", HTTP_POST, handlePOST);
          server.begin();
          serverOn = true;
          tft.fillScreen(BG_THEME);
          tft.setTextColor(FG_THEME);
          tft.setCursor(0,0);
          tft.println("Server On!");
          tft.print("Connect at: \nhttp://");
          tft.print(WiFi.localIP());
          tft.println("/");
          tft.print("http://");
          tft.print(WiFi.localIP());
          tft.println("/view POST");
        }
        else if(webserverOptions[i] == "Stop HTML Pre.") {
          server.stop();
          if(BG_THEME != TFT_RED) {
            tft.setTextColor(TFT_RED);
          }
          else {
            tft.setTextColor(FG_THEME);
          }
          tft.fillScreen(BG_THEME);
          tft.setCursor(0,0);
          tft.println("HTML Server Stopped");
          serverOn = false;
        }
        else if(webserverOptions[i] == "HTTPS Server") {
          tft.fillScreen(BG_THEME);
          tft.setTextColor(FG_THEME);
          tft.setCursor(0,0);
          tft.setTextSize(TXT_SIZE);
          tft.println("Generating Certificate...");
          cert = new SSLCert();
          int createCertResult = createSelfSignedCert(
              *cert,
              KEYSIZE_2048,
              "CN=myesp.local,O=acme,C=US");

          if (createCertResult != 0) {
              if(BG_THEME != TFT_RED) {
                tft.setTextColor(TFT_RED);
              }
              else {
                tft.setTextColor(FG_THEME);
              }
              tft.fillScreen(BG_THEME);
              tft.setCursor(0,0);
              tft.println("Error generating certificate");
              return; 
          }
          if(BG_THEME != TFT_GREEN) {
            tft.setTextColor(TFT_GREEN);
          }
          else {
            tft.setTextColor(FG_THEME);
          }
          tft.fillScreen(BG_THEME);
          tft.setCursor(0,0);
          tft.println("Certificate created");
          tft.setTextColor(FG_THEME);
          tft.println("Starting Server...");
          secureServer = new HTTPSServer(cert);
          ResourceNode * notFoundRoute = new ResourceNode("/notfound", "GET", [](HTTPRequest * req, HTTPResponse * res){
            res->setStatusCode(404);
            res->println("Not found");
          });
          ResourceNode * rootRoute = new ResourceNode("/", "GET", [](HTTPRequest * req, HTTPResponse * res){
            res->setStatusCode(201);
            String body = "<html> <head> <style> body { transition-duration: 0.3s } button { cursor: pointer; margin: 5px; } @media (prefers-color-scheme: dark) { body { background-color: rgb(38, 38, 38); color: white; } button { background-color:rgb(51, 51, 51); color: white; border: none; } .range::-webkit-slider-thumb { background-color: rgb(46, 136, 184); } .range::-ms-thumb { background-color: rgb(46, 136, 184); } } @media (prefers-color-scheme: light) { body { background-color: rgb(255, 255, 255); } button { background-color: rgb(240, 240, 240); color: rgb(38, 38, 38); border: none; } .range::-webkit-slider-thumb { background-color: rgb(86, 176, 224); } .range::-ms-thumb { background-color: rgb(86, 176, 224); } } </style> </head> <body id='b'> <select id='camerasel' onchange='changeCams(this)'></select> <select id='micsel' onchange='changeMics(this)'></select> <button onclick='startMedia()'>Start Media</button> <hr> <br> <div style='height: 40px; width: 123px; background-color: red;'></div> <div> <div> <video id='video' height='400' width='780' autoplay style='border: 1px solid black;'> Your browser does not support playing videos. :( </video> <div style='margin-top: -30px; position: absolute;'> <button>|></button> <input type='range' class='range' min='0' value='0' max='100' id='seek' onpointermove='windVideo(this)'> <button onclick='' hidden>Volume</button> <button onclick='' hidden>Options</button> <button onclick='vidFullscreen()'>Fullscreen</button> <button onclick='vidPip()'>Picture in Picture</button> <button onclick='vidDownload()' hidden>Download</button> </div> </div> </div> <script> function loop() { document.getElementById('seek').style.width = document.getElementById('video').width-100; } setInterval(loop, 1); document.body.addEventListener('keydown', function(e) { e.preventDefault(); if(e.key == 'p' || e.key == ' ') { var video = document.querySelector('video'); if(video.paused == true) { video.play(); } else { video.pause(); } } if(e.key == 'f') { vidFullscreen(); } }); var vidconstraints; var micconstraints; navigator.mediaDevices.getUserMedia({audio:true,video:true}).then(fake => { navigator.mediaDevices.enumerateDevices() .then(devices => { var videoDevices = [0,0]; var micDevices = [0,0]; var videoDeviceIndex = 0-1; var micDeviceIndex = 0-1; devices.forEach(function(device) { console.log(device.kind + ': ' + device.label + ' id = ' + device.deviceId); if (device.kind == 'videoinput') { videoDevices[videoDeviceIndex++] =  device; } if (device.kind == 'audioinput') { micDevices[micDeviceIndex++] =  device; } }); var camsel = document.getElementById('camerasel'); videoDevices.forEach(item => { var cam = document.createElement('option'); cam.value = item.deviceId; cam.innerText = item.label; camsel.appendChild(cam); }); var micsel = document.getElementById('micsel'); micDevices.forEach(item => { var mic = document.createElement('option'); mic.value = item.deviceId; mic.innerText = item.label; micsel.appendChild(mic); }); }); }); function changeCams(sender) { vidconstraints =  {width: 1280, height: 720, deviceId: { exact: sender.value  } }; startMedia(); } function changeMics(sender) { micconstraints =  {echoCancellation:true, noiseSuppression: true, autoGainControl:true, channelCount:{ideal: 2, min:1}, latency: 0.1, deviceId: { exact: sender.value  } }; startMedia(); } function vidFullscreen() { var video = document.querySelector('video'); video.requestFullscreen(); } function vidPip() { var video = document.querySelector('video'); video.requestPictureInPicture(); } function startMedia() { navigator.mediaDevices.getUserMedia({ video: vidconstraints, audio:true }) .then(stream => { if (window.webkitURL) { var video = document.querySelector('video'); video.srcObject = stream; localMediaStream = stream; } else if (video.mozSrcObject !== undefined) { video.mozSrcObject = stream; } else if (video.srcObject !== undefined) { video.srcObject = stream; } else { video.src = stream; }}) .catch(e => console.error(e)); } function windVideo(el) { var video = document.querySelector('video'); video.currentTime = el.value; el.title = video.currentTime; } </script> </body> </html>";
            res->println(body);
          });
          secureServer->registerNode(notFoundRoute);
          secureServer->registerNode(rootRoute);
          secureServer->start();
           if (secureServer->isRunning()) {
              tft.println("Server Started");
              tft.print("Connect: \nhttps://");
              tft.print(WiFi.localIP());
              httpsServerOn = true;
          }else{
              if(BG_THEME != TFT_RED) {
                tft.setTextColor(TFT_RED);
              }
              else {
                tft.setTextColor(FG_THEME);
              }
              tft.fillScreen(BG_THEME);
              tft.setCursor(0,0);
              tft.println("Failed To Start");
          }
          
        }
        else if(webserverOptions[i] == "Stop HTTPS") {
          secureServer->stop();
          if(BG_THEME != TFT_RED) {
            tft.setTextColor(TFT_RED);
          }
          else {
            tft.setTextColor(FG_THEME);
          }
          tft.fillScreen(BG_THEME);
          tft.setCursor(0,0);
          tft.println("HTTPS Server Stopped");
          httpsServerOn = false;
        }
        
      }
    }
}

void openWebServerApp() {
  isAppOpen = true;
  createList(webserverOptions, ArraySize(webserverOptions), webserverCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}
void handlePOST() {
  if (server.hasArg("index") == true) {
  String index = server.arg("index");
  String body = "<html><head><title>Forbidden</title></head><body><h1>Forbidden</h1><hr><p>Your request was: <span>" + index + "</span></p></body></html>";
  server.send(403, "text/html", body);
  }
}
void handleRoot() {
  String body = "<html> <head> <style> body { transition-duration: 0.3s } button { cursor: pointer; margin: 5px; } @media (prefers-color-scheme: dark) { body { background-color: rgb(38, 38, 38); color: white; } button { background-color:rgb(51, 51, 51); color: white; border: none; } .range::-webkit-slider-thumb { background-color: rgb(46, 136, 184); } .range::-ms-thumb { background-color: rgb(46, 136, 184); } } @media (prefers-color-scheme: light) { body { background-color: rgb(255, 255, 255); } button { background-color: rgb(240, 240, 240); color: rgb(38, 38, 38); border: none; } .range::-webkit-slider-thumb { background-color: rgb(86, 176, 224); } .range::-ms-thumb { background-color: rgb(86, 176, 224); } } </style> </head> <body id='b'> <select id='camerasel' onchange='changeCams(this)'></select> <select id='micsel' onchange='changeMics(this)'></select> <button onclick='startMedia()'>Start Media</button> <hr> <br> <div style='height: 40px; width: 123px; background-color: red;'></div> <div> <div> <video id='video' height='400' width='780' autoplay style='border: 1px solid black;'> Your browser does not support playing videos. :( </video> <div style='margin-top: -30px; position: absolute;'> <button>|></button> <input type='range' class='range' min='0' value='0' max='100' id='seek' onpointermove='windVideo(this)'> <button onclick='' hidden>Volume</button> <button onclick='' hidden>Options</button> <button onclick='vidFullscreen()'>Fullscreen</button> <button onclick='vidPip()'>Picture in Picture</button> <button onclick='vidDownload()' hidden>Download</button> </div> </div> </div> <script> function loop() { document.getElementById('seek').style.width = document.getElementById('video').width-100; } setInterval(loop, 1); document.body.addEventListener('keydown', function(e) { e.preventDefault(); if(e.key == 'p' || e.key == ' ') { var video = document.querySelector('video'); if(video.paused == true) { video.play(); } else { video.pause(); } } if(e.key == 'f') { vidFullscreen(); } }); var vidconstraints; var micconstraints; navigator.mediaDevices.getUserMedia({audio:true,video:true}).then(fake => { navigator.mediaDevices.enumerateDevices() .then(devices => { var videoDevices = [0,0]; var micDevices = [0,0]; var videoDeviceIndex = 0-1; var micDeviceIndex = 0-1; devices.forEach(function(device) { console.log(device.kind + ': ' + device.label + ' id = ' + device.deviceId); if (device.kind == 'videoinput') { videoDevices[videoDeviceIndex++] =  device; } if (device.kind == 'audioinput') { micDevices[micDeviceIndex++] =  device; } }); var camsel = document.getElementById('camerasel'); videoDevices.forEach(item => { var cam = document.createElement('option'); cam.value = item.deviceId; cam.innerText = item.label; camsel.appendChild(cam); }); var micsel = document.getElementById('micsel'); micDevices.forEach(item => { var mic = document.createElement('option'); mic.value = item.deviceId; mic.innerText = item.label; micsel.appendChild(mic); }); }); }); function changeCams(sender) { vidconstraints =  {width: 1280, height: 720, deviceId: { exact: sender.value  } }; startMedia(); } function changeMics(sender) { micconstraints =  {echoCancellation:true, noiseSuppression: true, autoGainControl:true, channelCount:{ideal: 2, min:1}, latency: 0.1, deviceId: { exact: sender.value  } }; startMedia(); } function vidFullscreen() { var video = document.querySelector('video'); video.requestFullscreen(); } function vidPip() { var video = document.querySelector('video'); video.requestPictureInPicture(); } function startMedia() { navigator.mediaDevices.getUserMedia({ video: vidconstraints, audio:true }) .then(stream => { if (window.webkitURL) { var video = document.querySelector('video'); video.srcObject = stream; localMediaStream = stream; } else if (video.mozSrcObject !== undefined) { video.mozSrcObject = stream; } else if (video.srcObject !== undefined) { video.srcObject = stream; } else { video.src = stream; }}) .catch(e => console.error(e)); } function windVideo(el) { var video = document.querySelector('video'); video.currentTime = el.value; el.title = video.currentTime; } </script> </body> </html>";
  server.send(200, "text/html", body);
  tft.fillRect(0,TFT_HEIGHT-60, TFT_WIDTH, 60, BG_THEME);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(FG_THEME);
  tft.setCursor(TFT_HEIGHT-40, 20);
  tft.print("Client Connected");
  tft.setTextDatum(TL_DATUM);
  delay(2000);
  tft.fillRect(0,TFT_HEIGHT-60, TFT_WIDTH, 60, BG_THEME);
}
void openmDNSApp() {
  isAppOpen = true;
  tft.setRotation(1);
  tft.fillScreen(BG_THEME);
  tft.setTextColor(FG_THEME);
  tft.setCursor(0,0);
  if(MDNS.begin("ESP32")) {
    tft.println("mDNS Responder Started");
  }
  else {
    tft.println("mDNS Repsonder failed to start");
  }
  int n = MDNS.queryService("_http", "tcp");
  if (n == 0) {
    tft.println("No services found");
  } else {
    tft.print(n);
    tft.println(" service(s) found");

    for (int i = 0; i < n; i++) {
      tft.print(i + 1);
      tft.print(": ");
      tft.print(MDNS.hostname(i));
      tft.print(" (");
      tft.print(MDNS.IP(i));
      tft.print(":");
      tft.print(MDNS.port(i));
      tft.println(")");
    }
  }
}

const char* displayOptions[] = {"Display Test", "Red", "Orange", "Yellow", "Green", "Blue", "Purple", "Black", "White"};
int selectedDisplayOption = 0;
void displayCallback(char* selectedDisplayCallback, int selectedDisplayIndex) {
  for(int i = 0; i < ArraySize(displayOptions); i++) {
      if(i == selectedDisplayIndex) {
        if(displayOptions[i] == "Display Test") {
          int elapsed = millis();
          for(int j = 0; j <= 10; j++) {
            tft.fillScreen(TFT_RED);
            tft.fillScreen(TFT_ORANGE);
            tft.fillScreen(TFT_YELLOW);
            tft.fillScreen(TFT_GREEN);
            tft.fillScreen(TFT_BLUE);
            tft.fillScreen(TFT_PURPLE);
            tft.fillScreen(TFT_BLACK);
            tft.fillScreen(TFT_WHITE);
          }
          tft.fillScreen(BG_THEME);
          tft.setTextSize(TXT_SIZE);
          tft.setTextColor(FG_THEME);
          tft.setCursor(0,0);
          tft.println("10 repeats. ");
          tft.println("Time Taken: ");
          elapsed = millis() - elapsed;
          tft.print(elapsed);
          tft.print("ms | ");
          tft.print(elapsed/1000);
          tft.println("s");
        }
        else if(displayOptions[i] == "Red") {
          tft.fillScreen(TFT_RED);
        }
        else if(displayOptions[i] == "Orange") {
          tft.fillScreen(TFT_ORANGE);
        }
        else if(displayOptions[i] == "Yellow") {
          tft.fillScreen(TFT_YELLOW);
        }
        else if(displayOptions[i] == "Green") {
          tft.fillScreen(TFT_GREEN);
        }
        else if(displayOptions[i] == "Blue") {
          tft.fillScreen(TFT_BLUE);
        }
        else if(displayOptions[i] == "Purple") {
          tft.fillScreen(TFT_PURPLE);
        }
        else if(displayOptions[i] == "Black") {
          tft.fillScreen(TFT_BLACK);
        }
        else if(displayOptions[i] == "White") {
          tft.fillScreen(TFT_WHITE);
        }
        
      }
    }
}

void openDisplayApp() {
  isAppOpen = true;
  createList(displayOptions, ArraySize(displayOptions), webserverCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
}
bool KeyboardOn = false;
void openKeyboardApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setCursor(0,0);
  tft.setTextColor(FG_THEME);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  USB.usbClass(0);
  USB.usbSubClass(0);
  USB.usbProtocol(0);
  Keyboard.begin();
  USB.begin();
  if(!WiFi.isConnected() || WiFi.softAPIP() == IPAddress(0,0,0,0)) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Please connect to the internet");
      return;
  }

  if (mdns.begin("usbkeyboard")) {
    tft.println("MDNS responder started");
    mdns.addService("http", "tcp", 80);
    mdns.addService("ws", "tcp", 81);
  }
  else {
    tft.println("MDNS.begin failed");
  }
  DBG_print(F("Connect to http://usbkeyboard.local or http://"));
  DBG_println(WiFi.localIP());
  tft.setTextColor(FG_THEME);
  tft.setTextSize(TXT_SIZE);
  tft.println("http://usbkeyboard.local/");
  tft.println("press right button to shutdown");
  server.on("/", handleRootU);
  server.onNotFound(handleNotFoundU);

  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  KeyboardOn = true;
  btn2.setClickHandler([](Button2 & b) {
    server.close();
    webSocket.close();
    tft.fillScreen(BG_THEME);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Keyboard offline");
    KeyboardOn = false;
  });
}

char* unlockOptions[] = {"Galaxy A30", "Chromebook", "Dell", "THISPC", "Lamar Phone"};
int selectedUnlockOption = 0;
void unlockCallback(char* selectedUnlockOption, int selectedUnlockIndex) {
  for(int i = 0; i < ArraySize(unlockOptions); i++) {
      if(i == selectedUnlockIndex) {
        if(unlockOptions[i] == "Galaxy A30") {
          Keyboard.println("42555299999");
          tft.setTextColor(TFT_GREEN);
          tft.println("UNLOCKED");
          tft.setTextColor(TFT_WHITE);
        }
        else if(unlockOptions[i] == "Chromebook") {
          Keyboard.println("256256");
          tft.setTextColor(TFT_GREEN);
          tft.println("UNLOCKED");
          tft.setTextColor(TFT_WHITE);
        }
        else if(unlockOptions[i] == "Dell") {
          Keyboard.println("GET OUT NOW");
          tft.setTextColor(TFT_GREEN);
          tft.println("UNLOCKED");
          tft.setTextColor(TFT_WHITE);
        }
        else if(unlockOptions[i] == "THISPC") {
          Keyboard.println("12347890");
          tft.setTextColor(TFT_GREEN);
          tft.println("UNLOCKED");
          tft.setTextColor(TFT_WHITE);
        }
        else if(unlockOptions[i] == "Lamar Phone") {
          Keyboard.println("3623");
          tft.setTextColor(TFT_GREEN);
          tft.println("UNLOCKED");
          tft.setTextColor(TFT_WHITE);
        }
      }
    }
}

void openUnlockApp() {
  USB.usbClass(0);
  USB.usbSubClass(0);
  USB.usbProtocol(0);
  Keyboard.begin();
  USB.begin();
  createList(displayOptions, ArraySize(displayOptions), webserverCallback, BG_THEME, FG_THEME, TXT_SIZE, LINE_HEIGHT);
  btn2.setLongClickHandler([](Button2 & b) {
    
  });
}

void openSensorApp() {
  isAppOpen = true;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(FG_THEME);
  tft.setTextSize(TXT_SIZE);
  tft.setCursor(0,0);
  tft.print("Loading...");
  delay(1000);
  tft.fillScreen(BG_THEME);
  tft.setCursor(0,0);
  tft.println("Reading Sensors...");
  tft.setCursor(0,0);
}
char* subreddits[] = {"r/esp32", "r/showerthoughts", "r/dadjokes", "r/dubai", "r/randomthoughts", "r/askreddit", "r/ask"};
int selectedSubreddit = 0;
String redditLink = "https://reddit.com/" + (String)subreddits[0] + "/top.json?limit=1&t=hour";
bool redirected = false;
void openRedditApp(bool redirect) {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  if(redirect) {
    for(int i = 0; i < ArraySize(subreddits); i++) {
      if(i == selectedSubreddit) {
        if(!redirected) {
          redditLink = "https://reddit.com/" + (String)subreddits[i] + "/top.json?limit=1&t=hour";
        }
        tft.setTextSize(TXT_SIZE);
        tft.fillScreen(BG_THEME);
        redditClient.begin(redditLink);
        Serial.println(redditLink);
        int resp = redditClient.GET();
        if(resp == 200) {
          reddit_json = redditClient.getString();
          redirected = false;
        }
        else if(resp == 301) {
          redditLink = redditClient.getLocation();
          redirected = true;
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.println("Cannot get reddit post");
          Serial.println(redditClient.getString());
          tft.print("ERROR CODE: ");
          tft.println(resp);
          tft.println("Hold right btn on next screen");
        }
        else {
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.println("Cannot get reddit post");
          Serial.println(redditClient.getString());
          tft.print("ERROR CODE: ");
          tft.println(resp);
          tft.println(redditLink);
          redirected = false;
        }
        redditClient.end();
        DeserializationError error = deserializeJson(doc, reddit_json);

        if (error) {
          delay(2000);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.setCursor(0,0);
          tft.fillScreen(BG_THEME);
          tft.print("deserializeJson() failed: ");
          tft.println(error.c_str());
          tft.println(redditLink);
          return;
        }
        tft.fillScreen(BG_THEME);
        tft.setTextColor(FG_THEME);
        tft.setTextSize(2);
        tft.setCursor(0,0);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println((const char*)doc["data"]["children"][0]["data"]["title"]);
        tft.println("");
        tft.setTextSize(1);
        tft.setTextColor(FG_THEME);
        tft.print((const char*)doc["data"]["children"][0]["data"]["selftext"]);
        return;
        // ^ stops the function from continuing
      }
    }
  }
  for(int i = 0; i < ArraySize(subreddits); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedSubreddit) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(subreddits[i]);
  }
  btn1.setClickHandler([](Button2 & b) {
    if(selectedSubreddit <= 0) {

    }
    else {
      selectedSubreddit--;
    }
    tft.fillScreen(BG_THEME);
    tft.setTextSize(TXT_SIZE);
    for(int i = 0; i < ArraySize(subreddits); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedSubreddit) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(subreddits[i]);
    }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedSubreddit > ArraySize(subreddits)) {
      selectedSubreddit = 0;
    }
    else {
      selectedSubreddit++;
    }
    tft.fillScreen(BG_THEME);
    tft.setTextSize(TXT_SIZE);
    for(int i = 0; i < ArraySize(subreddits); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedSubreddit) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(subreddits[i]);
    }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    for(int i = 0; i < ArraySize(subreddits); i++) {
      if(i == selectedSubreddit) {
        if(!redirected) {
          redditLink = "https://reddit.com/" + (String)subreddits[i] + "/top.json?limit=1&t=hour";
        }
        tft.setTextSize(TXT_SIZE);
        tft.fillScreen(BG_THEME);
        redditClient.begin(redditLink);
        Serial.println(redditLink);
        int resp = redditClient.GET();
        if(resp == 200) {
          reddit_json = redditClient.getString();
          redirected = false;
        }
        else if(resp == 301) {
          redditLink = redditClient.getLocation();
          redirected = true;
          tft.setTextColor(TFT_YELLOW, TFT_BLACK);
          tft.println("Redirecting...");
          openRedditApp(true);
        }
        else {
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.println("Cannot get reddit post");
          Serial.println(redditClient.getString());
          tft.print("ERROR CODE: ");
          tft.println(resp);
          tft.println(redditLink);
          redirected = false;
        }
        redditClient.end();
        DeserializationError error = deserializeJson(doc, reddit_json);

        if (error) {
          delay(2000);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.setCursor(0,0);
          tft.fillScreen(BG_THEME);
          tft.print("deserializeJson() failed: ");
          tft.println(error.c_str());
          tft.println(redditLink);
          return;
        }
        tft.fillScreen(BG_THEME);
        tft.setTextColor(FG_THEME);
        tft.setTextSize(2);
        tft.setCursor(0,0);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println((const char*)doc["data"]["children"][0]["data"]["title"]);
        tft.println("");
        tft.setTextColor(FG_THEME);
        tft.setTextSize(1);
        tft.print((const char*)doc["data"]["children"][0]["data"]["selftext"]);
      }
    }
  });
}
void openFactoryResetApp() {
  isAppOpen = true;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.drawCentreString("THIS ACTION CANNOT BE UNDONE. \nARE YOU SURE?", TFT_WIDTH/2, 30, 1);
  tft.setCursor(0, TFT_HEIGHT-20);
  tft.print("NO");
  tft.setCursor(TFT_WIDTH-60,TFT_HEIGHT-20);
  tft.print("YES");
  btn1.setClickHandler([](Button2 & b) {
    isAppOpen = false;
    DisplayHomeScreen();
  });
  btn2.setClickHandler([](Button2 & b) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0,0);
    tft.println("Resetting...");
    delay(500);
    prefs.clear();
    tft.println("Rebooting...");
    delay(500);
    ESP.restart();
  });
}

// Apps cannot be past this point
bool OTASetUP = false;
void loop() {
  btn1.loop();
  btn2.loop();
  if(WiFi.isConnected()) {
    tft.fillCircle(TFT_WIDTH-5, 5, 5, TFT_BLUE);
  }
  else if(WiFi.getMode() == WIFI_AP) {
    tft.fillCircle(TFT_WIDTH-5, 5, 5, TFT_GREENYELLOW);
  }
  else {
    tft.fillCircle(TFT_WIDTH-5, 5, 5, TFT_RED);
  }
  if(WiFi.isConnected() && !OTASetUP) {
    
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      tft.fillScreen(BG_THEME);
      tft.setTextColor(FG_THEME);
      tft.setTextSize(TXT_SIZE);
      tft.setCursor(0,0);
      tft.println("Updating...");
      tft.setCursor(0,40);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Do Not Turn Off\nDo Not Disconnect WiFi");
      tft.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, FG_THEME);
      tft.drawRoundRect(0, TFT_HEIGHT/2, TFT_WIDTH, 60, 5, FG_THEME);
      tft.drawCentreString("0%", TFT_WIDTH/2, TFT_HEIGHT/2, 0);
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(0,0);
      tft.println("Update Complete");
      tft.println("Rebooting...");
      tft.fillRoundRect(0, TFT_HEIGHT/2, TFT_WIDTH, 60, 5, TFT_GREEN);
      delay(1000);
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      tft.fillRoundRect(0, TFT_HEIGHT/2, ((progress / (total / 100))*TFT_WIDTH)/100, 60, 5, FG_THEME);
      // tft.fillRect(0, TFT_HEIGHT, TFT_WIDTH, ((progress / (total / 100))*TFT_HEIGHT), FG_THEME);
      tft.setCursor(TFT_WIDTH/2, 20);
      tft.setTextColor(FG_THEME, BG_THEME);
      tft.print((progress / (total / 100)));
      tft.print("%");
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      tft.setCursor(0,0);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      if (error == OTA_AUTH_ERROR) tft.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) tft.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) tft.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) tft.println("Receive Failed");
      else if (error == OTA_END_ERROR) tft.println("End Failed");
    });

  ArduinoOTA.begin();
  OTASetUP = true;
  }
  if(publicSelectedOption == "Keyboard" && isAppOpen || KeyboardOn) {
    webSocket.loop();
    server.handleClient();
    if(BG_THEME != TFT_GREEN) {
      tft.fillCircle(TFT_WIDTH-35, 5, 5, TFT_GREEN);
    }
    else {
      tft.fillCircle(TFT_WIDTH-35, 5, 5, TFT_RED);
    }
  }
  if(publicSelectedOption == "Sensors" && isAppOpen) {
    // Time Section

    // Temp & Humidity Section
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      if(FG_THEME != TFT_RED) {
        tft.setTextColor(TFT_RED);
      }
      else {
        tft.setTextColor(TFT_PURPLE);
      }
      tft.println(F("Failed to read from DHT sensor!"));
    }
    else {
      tft.fillScreen(BG_THEME);
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      tft.print("Temp: ");
      tft.print(t);
      tft.println("*C");
      tft.print("Humi: ");
      tft.print(h);
      tft.println("%");
      Blynk.virtualWrite(V5, t);
      Blynk.virtualWrite(V4, h);
      delay(2000);
    }


  }
  if(serverOn) {
    server.handleClient();
    tft.fillCircle(TFT_WIDTH-20, 5, 5, TFT_GREEN);
  }
  else {
    tft.fillCircle(TFT_WIDTH-20, 5, 5, TFT_RED);
  }
  if(webserverOptions[selectedWebserverOption] == "HTTPS Server" || isAppOpen && httpsServerOn) {
    secureServer->loop();
  }
  ArduinoOTA.handle();
}

void BackgroundProcesses(void * parameter) {
  for(;;) {
    if(Blynk.CONNECTED) {
      if(WiFi.isConnected()) {
        Blynk.virtualWrite(V0, "Online [STA]");
        Blynk.virtualWrite(V7, WiFi.SSID());
        Blynk.virtualWrite(V6, WiFi.localIP().toString());
      }
      else {
        if(WiFi.getMode() == WIFI_AP) {
          Blynk.virtualWrite(V0, "Online [AP]");
          Blynk.virtualWrite(V7, WiFi.softAPSSID());
          Blynk.virtualWrite(V6, WiFi.softAPIP().toString());
        }
        else if(WiFi.getMode() == WIFI_AP_STA) {
          Blynk.virtualWrite(V0, "Online [STA+AP]");
          Blynk.virtualWrite(V7, WiFi.softAPSSID() + " | " + WiFi.SSID());
          Blynk.virtualWrite(V7, WiFi.localIP().toString() + " | " + WiFi.softAPIP().toString());
        }
        else {
          Blynk.virtualWrite(V0, "Offline");
        }
      }
      Blynk.run();
    }
  }
}