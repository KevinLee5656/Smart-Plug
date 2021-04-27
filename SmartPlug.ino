#include <WiFi.h>
#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

#define CURRENT_SENSOR_PIN 36
#define SUPPLY_VOLTAGE 5.0
#define MAX_ADC_COUNT 4095.0
#define CURRENT_SENSOR_SENSITIVITY 0.1  // [V/A]
#define NUM_CURRENT_ADC_SAMPLES 100

WiFiClient  client;
double countToCurrent(int adcCount);
unsigned long myChannelNumber = 1372693;
const char * myWriteAPIKey = "JTU276A6ZM3O5YEI";
double measuredI, squaredI, sumI = 0, Irms = 0;
int num_current_adc_sample = 0;

void setup() {
  Serial.begin(115200);  // Initialize serial
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  char *ssid = "";   // your network SSID (name) 
  char *pass = "";   // your network password
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
    ThingSpeak.writeField(myChannelNumber, 1, number, myWriteAPIKey);
    num_current_adc_sample = 0;
    sumI = 0;
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
