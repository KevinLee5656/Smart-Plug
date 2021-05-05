#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ThingSpeak.h"


#define CURRENT_SENSOR_PIN 36
#define RELAY_PIN 23
#define SCL_PIN 22
#define SDA_PIN 21
#define SUPPLY_VOLTAGE 5.0
#define MAX_ADC_COUNT 4095.0
#define CURRENT_SENSOR_SENSITIVITY 0.1  // [V/A]
#define NUM_CURRENT_ADC_SAMPLES 100
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

WiFiClient  client;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

double countToCurrent(int adcCount);
void writeDisplay(char* message, int x, int y);

unsigned long myChannelNumber = 1372693;
const char* myWriteAPIKey = "JTU276A6ZM3O5YEI";
double measuredI, squaredI, sumI = 0, Irms, Power = 0;
int num_current_adc_sample = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial) {}

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextSize(1);
  display.setTextColor(BLUE);
  writeDisplay("Booting up!", 0, 0); 
  delay(1000);
  display.clearDisplay();
  
  pinMode(RELAY_PIN, OUTPUT);

  char *ssid = "myREZ - Guest";   // your network SSID (name) 
  char *pass = "a12345678";   // your network password
  int keyIndex = 0;            // your network key Index number (needed only for WEP)

  WiFi.mode(WIFI_STA); 
  ThingSpeak.begin(client);  // Initialize ThingSpeak

  // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);        
    } 
    Serial.println("\nConnected.");
  }
}

void loop() {
  measuredI = countToCurrent(analogRead(CURRENT_SENSOR_PIN)); 
  squaredI = pow(measuredI, 2);
  sumI += squaredI;
  num_current_adc_sample++;
  if(num_current_adc_sample >= NUM_CURRENT_ADC_SAMPLES) {
    Irms = sqrt(sumI / NUM_CURRENT_ADC_SAMPLES);
    Power = Irms * 120;
    ThingSpeak.writeField(myChannelNumber, 1, Irms, myWriteAPIKey);
    ThingSpeak.writeField(myChannelNumber, 2, Power, myWriteAPIKey);
    num_current_adc_sample = 0;
    sumI = 0;
    writeDisplay("Irms", 0, 0); // kevinlee - add values
    writeDisplay("Power", 0, 1);
  }
  // receive MQTT configuration message for relay
  if(mqtt_message_received) {
    digitalWrite(RELAY_PIN, HIGH);
    display.clearDisplay();
    writeDisplay("OFF", 0, 0);
    while(digitalRead(RELAY_PIN)) {} // kevinlee - look for another mqtt message
  }
}

// 0V = 0, 5V = 4095
// 2.5V = 0A, 0.5V = -20A, 4.5V = 20A due to 100 mV/A sensitivity
// 0-4095 for adc count due to 12 bit resolution
double countToCurrent(int adcCount) {
  double voltage = adcCount / MAX_ADC_COUNT * SUPPLY_VOLTAGE;
  double current = abs(voltage - 2.5) / CURRENT_SENSOR_SENSITIVITY;
  return current;
}

void writeDisplay(char* message, int x, int y) {
  display.setCursor(x, y);
  display.println(message);
  display.display(); 
}