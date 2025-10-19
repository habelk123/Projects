
/*
---------------------------------------------------------------------NOTE:----------------------------------------------------------------------------- 
A custom key from OpenWeatherMap was used for this project to retrieve real time weather-data to display on oled screen (apiURL).
For ntpServer, "pool.ntp.org" was used.
This project was centred on developing FreeRTOS skills such as task creation, priority, and synchronization.
There may be small hiccups/delays when starting this program, due to wifi-connectivity and request times for servers.
--------------------------------------------------------------------------------------------------------------------------------------------------------
*/
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "private.h"
#include <time.h>

//Screen Parameters
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_RESET -1
#define SCREEN_ADD 0x3C


/*Globals*/

//NTP Data
long gmtOffset_sec = -5*3600;
int dayLightOffset_sec = 3600;
struct tm timeInfo;
const char* months[12]{"Jan.", "Feb.", "Mar.", "Apr.",
                       "May.", "Jun.", "Jul.", "Aug.", 
                       "Sept.", "Oct.", "Nov.", "Dec."};

//Synchronization Flags
volatile bool firstNtpSync = false;
volatile bool displayReady = false;
volatile bool firstHTTPReq = false;

//Weather data
float temp = 0.0 ;
String weather = "N/A";

Adafruit_SSD1306 screen(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);

/*RTOS Tasks*/
void WifiTask(void* parameter){ //Wifi Task Logic
  Serial.println("Wifi connecting...");
  WiFi.begin(ssid, password); //your password and ssid

  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Wifi connecting...");
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  Serial.println("Wifi connected!");

  for(;;){ //handle reconnections
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("Wifi reconnecting...");
      WiFi.reconnect();
    }
    vTaskDelay(3000/portTICK_PERIOD_MS);
  }
}

void ClockTask(void *parameter){ //Clock Task Logic
  while(WiFi.status() != WL_CONNECTED){ //check for wifi connection first
    Serial.println("Waiting for wifi connection to sync clock...");
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  Serial.println("Gathering time info....");
  configTime(gmtOffset_sec, dayLightOffset_sec, ntpServer);
  
  for(;;){
    if(WiFi.status() == WL_CONNECTED){ //while wifi is connected, retrieve time from NTP server
      if(getLocalTime(&timeInfo)){
        if(timeInfo.tm_year + 1900 >= 2020){ //checks if tm struct is filled with a valid timestamp (if Unix epoch or up to date time)
          firstNtpSync = true;
        }
        Serial.print(timeInfo.tm_hour);
        Serial.print(":");
        Serial.println(timeInfo.tm_min);
      }
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void ResyncNtpTask(void *parameter){ //Resync Task for NTP - every 6 hours reconfigure
  for(;;){
    if(firstNtpSync && WiFi.status() == WL_CONNECTED){
      configTime(gmtOffset_sec, dayLightOffset_sec, ntpServer);
    }
    vTaskDelay(21600000/portTICK_PERIOD_MS);
  }
}

void HttpTask(void *parameter){ //HTTP Task Logic - retrieving weather data from OpenWeatherMap API
  while(WiFi.status() != WL_CONNECTED){
    Serial.println("Waiting for wifi connection to fetch weather data...");
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
  for(;;){
    HTTPClient http;
    http.begin(apiURL); //send request to the API server using key and endpoint (e.g Toronto, Canada)
    if(http.GET() == 200){
      String weatherInfo = http.getString(); //retreive weather info from API (Json) and store in string
      StaticJsonDocument<1024> doc;
      deserializeJson(doc, weatherInfo); //parsing Json
      temp = doc["main"]["temp"]; //temperature field
      const char* w = doc["weather"][0]["main"]; //weather condition field
      firstHTTPReq = true; //valid weather data retreived
      weather = String(w); 
    }
    http.end(); //end request to free memory
    vTaskDelay(360000/portTICK_PERIOD_MS);
  }
}

void DisplayTask(void *parameter){ //OLED Display Task Logic
  //Buffers for storing display fields
  char dateBuffer[50];
  char timeBuffer[50];
  char tempBuffer[32];
  char conditionBuffer[50];

  while(!firstNtpSync || !firstHTTPReq){ //ensure display is called only after valid data is retrieved for time and weather
    vTaskDelay(50/portTICK_PERIOD_MS);
  }

  for(;;){
    //local copies of global data to ensure safe handling between tasks
    String localWeather = weather;
    float localTemp = temp;
    screen.clearDisplay();
    screen.setTextColor(SSD1306_WHITE);
    screen.setTextSize(0);
    
    
    //Convert time struct data into 12-hour format (AM):
    if(timeInfo.tm_hour < 12){
      if(timeInfo.tm_hour == 0 && timeInfo.tm_min < 10){
        sprintf(timeBuffer, "12:0%d AM", timeInfo.tm_min);
      }
      else if(timeInfo.tm_hour == 0){
        sprintf(timeBuffer, "12:%d AM", timeInfo.tm_min);
      }
      else if(timeInfo.tm_min < 10){
         sprintf(timeBuffer, "%d:0%d AM", timeInfo.tm_hour, timeInfo.tm_min);
      }
      else{
        sprintf(timeBuffer, "%d:%d AM", timeInfo.tm_hour, timeInfo.tm_min);
      }
    }
      //Convert time struct data into 12-hour format (PM):
    if(timeInfo.tm_hour >= 12){
      if(timeInfo.tm_hour == 12 && timeInfo.tm_min < 10){
        sprintf(timeBuffer, "12:0%d PM", timeInfo.tm_min);
      }
      else if(timeInfo.tm_hour == 12){
        sprintf(timeBuffer, "12:%d PM", timeInfo.tm_min);
      }
      else if(timeInfo.tm_min < 10){
        sprintf(timeBuffer, "%d:0%d PM", timeInfo.tm_hour - 12, timeInfo.tm_min);
      }
      else{
        sprintf(timeBuffer, "%d:%d PM", timeInfo.tm_hour - 12, timeInfo.tm_min);
      }
    }
    //Convert other display fields for printing
    sprintf(dateBuffer, "%s %d, %d", months[timeInfo.tm_mon], timeInfo.tm_mday, timeInfo.tm_year + 1900);
    sprintf(tempBuffer, "Temp: %dC", (int)round(localTemp));
    sprintf(conditionBuffer, "Cond: %s", localWeather.c_str());

    //Printing to display
    screen.setCursor(27,7);
    screen.println(dateBuffer);
    screen.setCursor(39, 20);
    screen.println(timeBuffer);
    screen.setCursor(25, 35);
    screen.println(tempBuffer);
    screen.setCursor(25, 45);
    screen.println(conditionBuffer);
    screen.display();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

void setup(){
  Serial.begin(115200);
  screen.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADD);
  //Display "Loading..." Message on startup
  screen.clearDisplay();
  screen.display();
  screen.setTextSize(2);
  screen.setCursor(5, 37);
  screen.setTextColor(SSD1306_WHITE);
  screen.println("Loading...");
  screen.display();

  //Task creation pinned to cores (Core 0 - Frequent Server tasks (Wifi, API, NTP), Core 1 - Display and occasional resync)
  xTaskCreatePinnedToCore(WifiTask, "Wifi Task", 4096, NULL, 4, NULL, 0);
  xTaskCreatePinnedToCore(ClockTask, "Clock Task", 4096, NULL, 3, NULL, 0);
  xTaskCreatePinnedToCore(HttpTask, "HTTP Task", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(ResyncNtpTask, "NTP Resync Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(DisplayTask, "OLED Display Task", 4096, NULL, 2, NULL, 1);
  
}

void loop(){
}
