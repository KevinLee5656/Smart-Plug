#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ThingSpeak.h"


#define CURRENT_SENSOR_PIN 34
#define RELAY_PIN 23
#define SCL_PIN 22
#define SDA_PIN 21
#define SUPPLY_VOLTAGE 5.0
#define MAX_ADC_COUNT 4095.0
#define CURRENT_SENSOR_SENSITIVITY 0.1  // [V/A]
#define NUM_CURRENT_ADC_SAMPLES 100
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define ADC_OFFSET 740

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient  client;

double countToCurrent(int adcCount);
float getVPP();

unsigned long myChannelNumber = 1372693;
const char* myWriteAPIKey = "JTU276A6ZM3O5YEI";
const char* myReadAPIKey = "";////////////////////////////////////////////////////////////ADD READ API KEY HERE///////
double measuredI, squaredI, sumI = 0, Irms, Vrms, Vpp, power = 0;
int num_current_adc_sample = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial) {}
 
   if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  pinMode(RELAY_PIN, OUTPUT);
  
  char *ssid = "myREZ - Residents";   // your network SSID (name) 
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
  Serial.print("ADC :");
  Serial.println(analogRead(CURRENT_SENSOR_PIN) - ADC_OFFSET);
  squaredI = pow(measuredI, 2);
  sumI += squaredI;
  num_current_adc_sample++;
  if(num_current_adc_sample >= NUM_CURRENT_ADC_SAMPLES) {
    Irms = sqrt(sumI / NUM_CURRENT_ADC_SAMPLES);
    Irms = (float)((int)(Irms * 100.0 + .5) / 100.0);
    power = Irms * 120;
    Serial.print("Irms :");
    Serial.println(Irms);
    
    int powerOn = ThingSpeak.readLongField(myChannelNumber, 3, myReadAPIKey);

    if (powerOn == 0) {
      return;
    }

    
    ThingSpeak.writeField(myChannelNumber, 1, (long)Irms, myWriteAPIKey);
    ThingSpeak.writeField(myChannelNumber, 2, (long)power, myWriteAPIKey);
    num_current_adc_sample = 0;
    sumI = 0;
    
    display.clearDisplay();
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(F("Irms: "));
    display.print(Irms);
    display.println(F(" A"));
    display.setCursor(0, 20);
    display.print(F("Power: "));
    display.print(power);
    display.println(F(" W"));
    display.display();      // Show initial text
    delay(100);
  }
}

// 0V = 0, 5V = 4095
// 2.5V = 0A, 0.5V = -20A, 4.5V = 20A due to 100 mV/A sensitivity
// 0-4095 for adc count due to 12 bit resolution
double countToCurrent(int adcCount) {
  double voltage = (adcCount-ADC_OFFSET) / MAX_ADC_COUNT * SUPPLY_VOLTAGE;
  double current = abs(voltage - SUPPLY_VOLTAGE/2) / CURRENT_SENSOR_SENSITIVITY;
  return current;
}

void turnOn() {
  ThingSpeak.writeField(myChannelNumber, 3, 1, myWriteAPIKey);
}

void turnOff() {
  ThingSpeak.writeField(myChannelNumber, 3, 0, myWriteAPIKey);
}
