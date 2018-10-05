#include <ArduinoHttpClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <string>
#include <map>
#include "time.h"
#include "math.h"
#include "Arduino.h"

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

WiFiMulti WiFiMulti;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, "205.166.161.233", 80);

struct routeStopInfo {
  std::string longName;
  int stopID;
};

routeStopInfo newRouteStopInfo(const char* longName, int stopID) {
  routeStopInfo r = { longName, stopID };
  return r;
}

std::map<char*, routeStopInfo> routeStops;

void setupRouteStopInfo(void) {
  routeStops["09N"] = newRouteStopInfo("BCT09_North", 4196);
  routeStops["09S"] = newRouteStopInfo("BCT09_South", 667);
  routeStops["07E"] = newRouteStopInfo("BCT07_East", 498);
  routeStops["07W"] = newRouteStopInfo("BCT07_West", 499);
  routeStops["01N"] = newRouteStopInfo("BCT01_North", 96);
  routeStops["01S"] = newRouteStopInfo("BCT01_South", 31);
  routeStops["101N"] = newRouteStopInfo("BCT101-US1 Breeze _North", 96);
  routeStops["101S"] = newRouteStopInfo("BCT101-US1 Breeze _South", 31);
}
/*hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}*/


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
 // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  display.fillRect(0,0,128,16,BLACK);
  display.setCursor(0,0);
  display.println(&timeinfo, "%a %b %d %Y");
  display.println(&timeinfo, "%H:%M:%S %z");
  //display.display();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);

  setupRouteStopInfo();
  WiFiMulti.addAP("Linchlonst-2.4", "Staysober247");

  Serial.print("Wait...");
  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
  delay(100);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  WiFi.disconnect(true);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
}

String response;
int statusCode = 0;

int getRouteETA(char* route) {
  char postData[10];
  DynamicJsonBuffer jsonBuffer(4096);
  int stopID = routeStops[route].stopID;
  std::string longName = routeStops[route].longName;
  sprintf(postData, "[\"%04d\"]", stopID);
  client.post("/TransitAPI/ETA/", "application/json", postData);
  statusCode = client.responseStatusCode();
  response = client.responseBody();
  client.stop();
  Serial.printf("%d\n%s\n\n", statusCode, response.c_str());
  if (statusCode == 200) {
    JsonObject& root = jsonBuffer.parseObject(response);
    JsonArray& arrivals = root["ArrivalEstimates"];
    float nextETA = 0.0;
    for (JsonObject& estimate : arrivals) {
      if (strcmp(estimate["Route"]["RouteName"], longName.c_str()) == 0) {
        nextETA = estimate["NextArrivalCountdown"];
        break;
      }
    }
    jsonBuffer.clear();
    return (int) round(nextETA);
  }
  return 0;
}

void printRouteETA(char* route) {
  int routeETA = getRouteETA(route);
  int etaMinutes = ceil((float)routeETA / 60.0);
  display.printf("%s: ", route);
  if (routeETA != 0) {
    display.printf("%d min\n", etaMinutes);
  } else {
    display.printf("----\n");
  }
  //display.display();
}


void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  if (WiFiMulti.run() == WL_CONNECTED) {
    display.setCursor(0, 16);
    printRouteETA("09S");
    printRouteETA("07E");
    printRouteETA("07W");
    printRouteETA("01N");
    printRouteETA("101N");
    printLocalTime();
    display.display();
  }
  delay(15000);
}
