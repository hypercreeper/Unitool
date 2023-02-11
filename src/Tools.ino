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
#include <BluetoothSerial.h>
// TODO: Make sure to add approximate.h library to calculate distance of devices using wifi
// TODO: Add Bluetooth

BLEScan* blescan;
int scanTime = 5; //In seconds

#include <ArduinoOTA.h>

using namespace httpsserver;

SSLCert * cert;
HTTPSServer * secureServer;

HTTPClient redditClient;

uint32_t BG_THEME = TFT_BLACK;
uint32_t FG_THEME = TFT_WHITE;
char* SSIDs[] = {"Moukayed", "Belkin.069", "Mohamad's Room", "Diff. Network"};
char* Passwords[] = {"0566870554", "0566870554", "Mohamad2008!"};
String options[] = {"BLE Scan", "Scan Wifi", "Connect", "Disconnect", "STA Info", "Erase Wifi", "Start AP", "Stop AP", "AP Info", "Webserver","Theme", "Display", "Sensors", "Reddit"};
String reddit_json = "";
bool optionsEnabled[] = {true, true, true, true, true, true, true, true, true, true, true, false};
int selectedIndex = 0;
String selectedOption = "BLE Scan";
char buff[512];
bool isAppOpen = false;
bool staapmodepublic = false;
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

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      // tft.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      tft.print("Name: ");
      tft.println(advertisedDevice.getName().c_str());
      tft.print("Address: ");
      tft.println(advertisedDevice.getAddress().toString().c_str());
      tft.print("?: ");
      tft.println((char*)advertisedDevice.getAppearance());
      // tft.print("RSSI: ");
      // tft.println((char *)advertisedDevice.getRSSI());
      
    }
};


void setup() {
  // put your setup code here, to run once:
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0);
  tft.setTextColor(TFT_BLACK);
  setTheme(false);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  dht.begin();
  BLEDevice::init("ESP32");
  DisplayHomeScreen();
  btn2.setLongClickHandler([](Button2 & b) {
    if(isAppOpen == false) {
      if(selectedOption == "Theme") {
        openThemeApp();
      }
      else if(selectedOption == "Scan Wifi") {
        openScanWifiApp();
      }
      else if(selectedOption == "BLE Scan") {
        openBLEScanApp();
      }
      else if(selectedOption == "Connect") {
        openConnectApp();
      }
      else if(selectedOption == "Disconnect") {
        openDisconnectApp(false);
      }
      else if(selectedOption == "Erase Wifi") {
        openDisconnectApp(true);
      }
      else if(selectedOption == "STA Info") {
        openSTAInfoApp();
      }
      else if(selectedOption == "Start AP") {
        openStartAPApp();
      }
      else if(selectedOption == "Stop AP") {
        openDisconnectAPApp();
      }
      else if(selectedOption == "AP Info") {
        openAPInfoApp();
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
      
    }
  });
  btn2.setClickHandler([](Button2 & b) {
      if(isAppOpen == false) {
        if(selectedIndex >= ArraySize(options)-1) {
          selectedIndex = 0;
        }
        else {
          selectedIndex++;
        }
        selectedOption = options[selectedIndex];
        DisplayHomeScreen();
      }
  });
  btn1.setClickHandler([](Button2 & b) {
      if(isAppOpen == false) {
        if(selectedIndex <= 0) {
          selectedIndex = ArraySize(options)-1;
        }
        else {
          selectedIndex--;
        }
        selectedOption = options[selectedIndex];
        DisplayHomeScreen();
      }
  });
  btn1.setDoubleClickHandler([](Button2 & b) {
    isAppOpen = false;
    btn2.setClickHandler([](Button2 & b) {
      if(isAppOpen == false) {
        if(selectedIndex >= ArraySize(options)-1) {
          selectedIndex = 0;
        }
        else {
          selectedIndex++;
        }
        selectedOption = options[selectedIndex];
        DisplayHomeScreen();
      }
  });
  btn1.setClickHandler([](Button2 & b) {
      if(isAppOpen == false) {
        if(selectedIndex <= 0) {
          selectedIndex = ArraySize(options)-1;
        }
        else {
          selectedIndex--;
        }
        selectedOption = options[selectedIndex];
        DisplayHomeScreen();
      }
  });
    btn2.setLongClickHandler([](Button2 & b) {
    if(isAppOpen == false) {
      if(selectedOption == "Theme") {
        openThemeApp();
      }
      else if(selectedOption == "Scan Wifi") {
        openScanWifiApp();
      }
      else if(selectedOption == "BLE Scan") {
        openBLEScanApp();
      }
      else if(selectedOption == "Connect") {
        openConnectApp();
      }
      else if(selectedOption == "Disconnect") {
        openDisconnectApp(false);
      }
      else if(selectedOption == "Erase Wifi") {
        openDisconnectApp(true);
      }
      else if(selectedOption == "STA Info") {
        openSTAInfoApp();
      }
      else if(selectedOption == "Start AP") {
        openStartAPApp();
      }
      else if(selectedOption == "Stop AP") {
        openDisconnectAPApp();
      }
      else if(selectedOption == "AP Info") {
        openAPInfoApp();
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

    }
  });
    DisplayHomeScreen();
  });
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
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(options); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(options[i].indexOf("|") != -1) {
      tft.setTextColor(TFT_BLACK, TFT_RED);
    }
    else if(i == selectedIndex) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(options[i]);
  }
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


char* themeOptions[] = {"Dark", "Light", "Red", "Orange", "Yellow", "Green", "Blue", "Purple"};
int selectedThemeOptions = 0;
void openThemeApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(themeOptions); i++) {
    if(i == selectedThemeOptions) {
      if(themeOptions[i] == "Red") {
        tft.fillScreen(TFT_RED);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      else if(themeOptions[i] == "Orange") {
        tft.fillScreen(TFT_ORANGE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      }
      else if(themeOptions[i] == "Yellow") {
        tft.fillScreen(TFT_YELLOW);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      }
      else if(themeOptions[i] == "Green") {
        tft.fillScreen(TFT_GREEN);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      }
      else if(themeOptions[i] == "Blue") {
        tft.fillScreen(TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      else if(themeOptions[i] == "Purple") {
        tft.fillScreen(TFT_PURPLE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      else if(themeOptions[i] == "Dark") {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }
      else if(themeOptions[i] == "Light") {
        tft.fillScreen(TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
      }
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
  }
  for(int i = 0; i < ArraySize(themeOptions); i++) {
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(themeOptions[i]);
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
  }
  
  btn1.setClickHandler([](Button2 & b) {
    if(selectedThemeOptions <= 0) {

    }
    else {
      selectedThemeOptions--;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(themeOptions); i++) {
      if(i == selectedThemeOptions) {
        if(themeOptions[i] == "Red") {
          tft.fillScreen(TFT_RED);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Orange") {
          tft.fillScreen(TFT_ORANGE);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Yellow") {
          tft.fillScreen(TFT_YELLOW);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Green") {
          tft.fillScreen(TFT_GREEN);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Blue") {
          tft.fillScreen(TFT_BLUE);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Purple") {
          tft.fillScreen(TFT_PURPLE);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Dark") {
          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Light") {
          tft.fillScreen(TFT_WHITE);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
    }
    for(int i = 0; i < ArraySize(themeOptions); i++) {
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(themeOptions[i]);
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
  }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedThemeOptions > ArraySize(themeOptions)) {
      selectedThemeOptions = 0;
    }
    else {
      selectedThemeOptions++;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(themeOptions); i++) {
      if(i == selectedThemeOptions) {
        tft.setTextColor(BG_THEME, FG_THEME);
        if(themeOptions[i] == "Red") {
          tft.fillScreen(TFT_RED);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Orange") {
          tft.fillScreen(TFT_ORANGE);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Yellow") {
          tft.fillScreen(TFT_YELLOW);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Green") {
          tft.fillScreen(TFT_GREEN);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
        else if(themeOptions[i] == "Blue") {
          tft.fillScreen(TFT_BLUE);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Purple") {
          tft.fillScreen(TFT_PURPLE);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Dark") {
          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        else if(themeOptions[i] == "Light") {
          tft.fillScreen(TFT_WHITE);
          tft.setTextColor(TFT_BLACK, TFT_WHITE);
        }
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
    }
    for(int i = 0; i < ArraySize(themeOptions); i++) {
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(themeOptions[i]);
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
  }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    for(int i = 0; i < ArraySize(themeOptions); i++) {
      if(i == selectedThemeOptions) {
        tft.setTextColor(BG_THEME, FG_THEME);
        if(themeOptions[i] == "Red") {
          BG_THEME = TFT_RED;
          FG_THEME = TFT_WHITE;
        }
        else if(themeOptions[i] == "Orange") {
          BG_THEME = TFT_ORANGE;
          FG_THEME = TFT_BLACK;
        }
        else if(themeOptions[i] == "Yellow") {
          BG_THEME = TFT_YELLOW;
          FG_THEME = TFT_BLACK;
        }
        else if(themeOptions[i] == "Green") {
          BG_THEME = TFT_GREEN;
          FG_THEME = TFT_BLACK;
        }
        else if(themeOptions[i] == "Blue") {
          BG_THEME = TFT_BLUE;
          FG_THEME = TFT_WHITE;
        }
        else if(themeOptions[i] == "Purple") {
          BG_THEME = TFT_PURPLE;
          FG_THEME = TFT_WHITE;
        }
        else if(themeOptions[i] == "Dark") {
          BG_THEME = TFT_BLACK;
          FG_THEME = TFT_WHITE;
        }
        else if(themeOptions[i] == "Light") {
          BG_THEME = TFT_WHITE;
          FG_THEME = TFT_BLACK;
        }
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
    }
    for(int i = 0; i < ArraySize(themeOptions); i++) {
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(themeOptions[i]);
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
  }
  });
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

int selectedWifiDetails = 0;
void openConnectApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(SSIDs); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedWifiDetails) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(SSIDs[i]);
  }
  btn1.setClickHandler([](Button2 & b) {
    if(selectedWifiDetails <= 0) {

    }
    else {
      selectedWifiDetails--;
    }
    for(int i = 0; i < ArraySize(SSIDs); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedWifiDetails) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(SSIDs[i]);
  }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedWifiDetails > ArraySize(options)) {
      selectedWifiDetails = 0;
    }
    else {
      selectedWifiDetails++;
    }
    for(int i = 0; i < ArraySize(SSIDs); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedWifiDetails) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(SSIDs[i]);
  }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    if(!staapmodepublic) {
      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
    }
    if(SSIDs[selectedWifiDetails] != "Diff. Network") {
      WiFi.begin(SSIDs[selectedWifiDetails], Passwords[selectedWifiDetails]);
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      int retrycon = 25;
      tft.fillScreen(BG_THEME);
      tft.print("Connecting to ");
      tft.print(SSIDs[selectedWifiDetails]);
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
      WiFi.beginSmartConfig();
      tft.fillScreen(BG_THEME);
      tft.setTextColor(FG_THEME);
      tft.setCursor(0,0);
      tft.setTextSize(TXT_SIZE);
      tft.println("Use ESPTOUCH to connect ESP32 to a new network");
      tft.print("Waiting...");
      while(!WiFi.smartConfigDone()) {
        
      }
    }

    tft.print("WiFi connected with IP: ");
    tft.println(WiFi.localIP());
    tft.println("DBL Click left to exit");
    if(staapmodepublic) {
      staapmodepublic = false;
    }
  });
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
  tft.println(WiFi.SSID());

  tft.print("\nPSK: \n");
  tft.println(WiFi.psk());

}
int selectedAPOption = 0;
String APOptions[] = {"Basic AP", "Dif. SSID AP", "Hidden AP", "One use AP", "AP IPv6", "STA+AP Mode"};
char* RandString[] = {"a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","1","2","3","4","5","6","7","8","9","0"};
char TempSSID[12];
char TempPWD[12];
const char* blank = "";
void openStartAPApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(APOptions); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedAPOption) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(APOptions[i]);
  }
  btn1.setClickHandler([](Button2 & b) {
    if(selectedAPOption <= 0) {

    }
    else {
      selectedAPOption--;
    }
    for(int i = 0; i < ArraySize(APOptions); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedAPOption) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(APOptions[i]);
  }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedAPOption > ArraySize(options)) {
      selectedAPOption = 0;
    }
    else {
      selectedAPOption++;
    }
    for(int i = 0; i < ArraySize(APOptions); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedAPOption) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(APOptions[i]);
  }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    WiFi.mode(WIFI_AP);
    if(APOptions[selectedAPOption] == "Basic AP") {
      WiFi.softAP("ESP32 AP", "1234567890");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
    }
    else if(APOptions[selectedAPOption] == "Dif. SSID AP") {
      WiFi.softAP("Notfreewifi", "dontaskforthepassword");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: Notfreewifi");
      tft.println("PWD: dontaskforthepassword");
    }
    else if(APOptions[selectedAPOption] == "One use AP") {
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
    else if(APOptions[selectedAPOption] == "Hidden AP") {
      WiFi.softAP("ESP32 AP", "1234567890", 1, 1);
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
      tft.setTextColor(TFT_RED);
      tft.println("\nNote: AP not visible on list, Press hidden network and type the details above. ");
      tft.setTextColor(FG_THEME);
    }
    else if(APOptions[selectedAPOption] == "AP IPv6") {
      WiFi.softAP("ESP32 AP", "1234567890");
      tft.fillScreen(BG_THEME);
      tft.println("AP On!");
      tft.println("SSID: ESP32 AP");
      tft.println("PWD: 123567890");
      tft.print("IPv6: ");
      tft.println(WiFi.softAPenableIpV6());
    }
    else if(APOptions[selectedAPOption] == "STA+AP Mode") {
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
  });
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
  tft.println(WiFi.softAPSSID());

  tft.print("\nPSK: \n");
  tft.println(TempPWD);

}
bool serverOn = false;
bool httpsServerOn = false;
char* webserverOptions[] = {"HTML Previewer", "Stop HTML Pre.", "HTTPS Server", "Stop HTTPS"};
int selectedWebserverOption = 0;
void openWebServerApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(webserverOptions); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedWebserverOption) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(webserverOptions[i]);
  }
  btn1.setClickHandler([](Button2 & b) {
    if(selectedWebserverOption <= 0) {

    }
    else {
      selectedWebserverOption--;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(webserverOptions); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedWebserverOption) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(webserverOptions[i]);
    }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedWebserverOption > ArraySize(webserverOptions)) {
      selectedWebserverOption = 0;
    }
    else {
      selectedWebserverOption++;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(webserverOptions); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedWebserverOption) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(webserverOptions[i]);
    }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    for(int i = 0; i < ArraySize(webserverOptions); i++) {
      if(i == selectedWebserverOption) {
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
  });
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
char* displayOptions[] = {"Display Test", "Red", "Orange", "Yellow", "Green", "Blue", "Purple", "Black", "White"};
int selectedDisplayOption = 0;
void openDisplayApp() {
  isAppOpen = true;
  tft.fillScreen(BG_THEME);
  tft.setTextSize(TXT_SIZE);
  for(int i = 0; i < ArraySize(displayOptions); i++) {
    tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
    if(i == selectedDisplayOption) {
      tft.setTextColor(BG_THEME, FG_THEME);
    }
    else {
      tft.setTextColor(FG_THEME, BG_THEME);
    }
    tft.setCursor(0,(LINE_HEIGHT*i)+1);
    tft.println(displayOptions[i]);
  }
  btn1.setClickHandler([](Button2 & b) {
    if(selectedDisplayOption <= 0) {

    }
    else {
      selectedDisplayOption--;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(displayOptions); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedDisplayOption) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(displayOptions[i]);
    }
  });
  btn2.setClickHandler([](Button2 & b) {
    if(selectedDisplayOption > ArraySize(displayOptions)) {
      selectedDisplayOption = 0;
    }
    else {
      selectedDisplayOption++;
    }
    tft.fillScreen(BG_THEME);
    for(int i = 0; i < ArraySize(displayOptions); i++) {
      tft.drawRect(0, 0, TFT_WIDTH, LINE_HEIGHT+(LINE_HEIGHT*i), FG_THEME);
      if(i == selectedDisplayOption) {
        tft.setTextColor(BG_THEME, FG_THEME);
      }
      else {
        tft.setTextColor(FG_THEME, BG_THEME);
      }
      tft.setCursor(0,(LINE_HEIGHT*i)+1);
      tft.println(displayOptions[i]);
    }
  });
  btn2.setLongClickHandler([](Button2 & b) {
    for(int i = 0; i < ArraySize(displayOptions); i++) {
      if(i == selectedDisplayOption) {
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
// Apps cannot be past this point
bool OTASetUP = false;
void loop() {
  btn1.loop();
  btn2.loop();
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
      tft.drawRoundRect(0, TFT_HEIGHT/2, (TFT_WIDTH/2)-100, 60, 5, FG_THEME);
      tft.drawCentreString("0%", TFT_WIDTH/2, TFT_HEIGHT/2, 0);
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      tft.fillRoundRect(0, TFT_HEIGHT/2, ((TFT_WIDTH/2)-100)+(progress / (total / 100)), 60, 5, FG_THEME);
      String temp = (progress / (total / 100)) + "%";
      tft.drawCentreString(temp, TFT_WIDTH/2, TFT_HEIGHT/2, 0);
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  OTASetUP = true;
  }
  if(selectedOption == "Sensors" && isAppOpen) {
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
      delay(2000);
    }


  }
  if(serverOn) {
    server.handleClient();
  }
  if(webserverOptions[selectedWebserverOption] == "HTTPS Server" && isAppOpen && httpsServerOn) {
    secureServer->loop();
  }
  ArduinoOTA.handle();
}