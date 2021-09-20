// Cool ternary I found online, to correctly round positive or
// negative numbers, because C code truncates.
#define FLOAT_TO_INT(x) ((x)>=0?(int)((x)+0.5):(int)((x)-0.5));


// eInk init
#include "Adafruit_ThinkInk.h"

#define EPD_CS     10
#define EPD_DC      9
#define SRAM_CS     8
#define EPD_RESET   7 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    6 // can set to -1 to not use a pin (will wait a fixed delay)


// 2.9" Tricolor Featherwing or Breakout with IL0373 chipset
ThinkInk_290_Tricolor_Z10 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);


// SHT31 Humidity & Temp Sensor init
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

// CCS811 voc sensor init
#include "Adafruit_CCS811.h"

Adafruit_CCS811 ccs;
uint16_t baselineValue = 400;

// PMSA0031 particulate sensor init
#include "Adafruit_PM25AQI.h"

Adafruit_PM25AQI aqi = Adafruit_PM25AQI();

// TODO: The AQI code needs to be made into its own file.

//For PM2pt5 and PM10:
// Air Quality naming (Vertical Schema):
// [1][anything] = Good
// [1][anything] = Moderate
// [1][anything] = Unhealthy for Sensitive Groups
// [1][anything] = Unhealthy
// [1][anything] = Very Unhealthy
// [1][anything] = Hazardous

// Air Quality Data (Horizontal Schema):
// [anything][1] = Lowest Concentration (for given AQI)
// [anything][2] = Highest Concentration
// [anything][3] = Lowest AQI (in bracket)
// [anything][4] = Highest AQI

// note: The AQI data I get for pm2.5 is in integers, so I've trunkated.
// I may want to make go back to 1 decimal place when I have everything working
// but I'm not sure, as the data I get is in integers anyway -- so unless I was
// averaging over time, that wouldn't be very useful.

int PM2pt5[6][4] = {  
   {0, 12, 0, 50} ,
   {13, 35, 51, 100} ,
   {36, 55, 101, 150},
   {56, 150, 151, 200},
   {151, 250, 201, 300},
   {151, 501, 301, 500}
};

int PM10[6][4] = {  
   {0, 54, 0, 50},
   {55, 154, 51, 100},
   {155, 254, 101, 150},
   {255, 354, 151, 200},
   {355, 424, 201, 300},
   {425, 604, 301, 500}
};

// Reference: https://forum.airnowtech.org/t/the-aqi-equation/169
// Calculation for AQI is:
// AQI = (Hightest AQI (in bracket) - lowest AQI)
//       / (highest concentration - lowest concentration)
//       * input concentration
//       - lowest concentration
//       + lowest AQI

int findAQIbracket(int inputConcentration, int AQItable[6][4]) {

   for(int count = 0; count < 6; count++) {
//      if (AQItable[count][0] <= inputConcentration && AQItable[count][1] >= inputConcentration){ 
         Serial.print("LowIndex: ");
         Serial.println(AQItable[count][0]);
         Serial.print("Concentration: ");
         Serial.println(inputConcentration);
         Serial.print("HighIndex: ");
         Serial.println(AQItable[count][1]);
         Serial.print("Count: ");
         Serial.println(count);
//      }
   }
}

// this code is currently testing pieces of what it will be.
int calculateAQI() {
    PM25_AQI_Data data;
  
  if (aqi.read(&data)) {
    Serial.print("PM2.5: ");
    Serial.print(data.pm25_standard);
    Serial.print(", PM10: ");
    Serial.println(data.pm100_standard);

    Serial.print("PM2.5 AQI bracket: ");
    findAQIbracket(data.pm25_standard, PM2pt5);
    
    Serial.print("PM10 AQI bracket: ");
    findAQIbracket(data.pm100_standard, PM10);
  }
}



// define variables
int loopCount = 0;


void setup() {
  Serial.begin(115200);
//  while (!Serial) { delay(100); }
  Serial.println("Metro M4 is on");
  delay(1000);
  
  display.begin(THINKINK_TRICOLOR);


  if (! sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }


  //Set up CCS811
  if(!ccs.begin()){
    Serial.println("Failed to start CCSS811! Please check your wiring.");
    while(1);
  }

  // Wait for the sensor to be ready
  while(!ccs.available());

  if (!aqi.begin_I2C()) {
    Serial.println("Could not find PM 2.5 sensor!");
    while (1) delay(10);
  }

}

void loop() {
  if (loopCount == 99990) {
      ccs.setEnvironmentalData(sht31.readHumidity(), sht31.readTemperature());
    
      display.clearBuffer();
      display.setTextSize(3);
      display.setTextColor(EPD_BLACK);
      

      display.setCursor(1, 1);


    
      if(ccs.available()){
        if(!ccs.readData()){
          display.setCursor(1, 27);
          display.print("CO2: ");
          display.print(ccs.geteCO2());
          display.print(" ppm");
          display.setCursor(1, 54);
          display.print("TVOC: ");
          display.print(ccs.getTVOC());
          display.print(" ppb");
        }
      }
    
      PM25_AQI_Data data;
      
      if (! aqi.read(&data)) {
        Serial.println("Could not read from AQI");
        delay(500);  // try again in a bit!
        return;
      }
      display.setCursor(1, 80);
      display.print(F("PM1:"));
      display.print(data.pm10_standard);
      display.print(F(" PM2.5:"));
      display.print(data.pm25_standard);
      display.setCursor(1, 106);
      display.print(F("PM10:"));
      display.print(data.pm100_standard);
      
      display.display();
  }

  Serial.println();
  Serial.print("loopCount: ");
  Serial.println(loopCount);

  Serial.print("Raw: ");
  Serial.print(ccs.getRawADCreading());
  Serial.print(", Base: ");
  Serial.println(ccs.getBaseline());
  ccs.setBaseline(baselineValue);
  
  if(ccs.available()){
    if(!ccs.readData()){
      Serial.print("CO2: ");
      Serial.print(ccs.geteCO2());
      Serial.print(" ppm, ");
      Serial.print("TVOC: ");
      Serial.print(ccs.getTVOC());
      Serial.println(" ppb");
    }
  }

  calculateAQI();
  
  delay(1000);

  loopCount++;
  if (loopCount == 180) {
    loopCount = 0;
  }
}
