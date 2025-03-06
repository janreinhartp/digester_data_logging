#include <Arduino.h>

#include "ADS1X15.h"
ADS1115 ADS(0x48);

#include <SdFat.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

#include "PCF8575.h"
PCF8575 pcf8575(0x25);

RTC_DS1307 RTC;                               // The real time clock object is "RTC"
#define DS1307_I2C_ADDRESS 0x68

SdFat SD;                                     // The SdFat object is "SD"
#define MOSIpin 11                            // For SD card
#define MISOpin 12                            // For SD card
const int chipSelect = 10;                    // CS pin for the SD card

void initializeLCD()
{
  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.print("HELLO WORLD");
}

int rVFD = P13;
int rOpenValve = P14;
int rCloseVlave = P15;

void initRelays()
{
  pcf8575.pinMode(rVFD, OUTPUT);
  pcf8575.digitalWrite(rVFD, LOW);

  pcf8575.pinMode(rOpenValve, OUTPUT);
  pcf8575.digitalWrite(rOpenValve, LOW);

  pcf8575.pinMode(rCloseVlave, OUTPUT);
  pcf8575.digitalWrite(rCloseVlave, LOW);

  pcf8575.begin();
}
static const int buttonPin = 6;
int buttonStatePrevious = HIGH;

static const int buttonPin2 = 7;
int buttonStatePrevious2 = HIGH;

static const int buttonPin3 = 8;
int buttonStatePrevious3 = HIGH;

void InitializeButtons()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
}

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);

  Wire.begin();                    // initialize the I2C interface
  RTC.begin();                     // initialize the RTC 
  RTC.adjust(DateTime((__DATE__), (__TIME__)));    //sets the RTC to the time the sketch was compiled.

  while (!Serial) {
    ;                              // wait for serial port to connect. Needed for native USB port only
  }
  // Serial.print("Find SD card: ");          // initialize and display the status of the SD card:
  // if (!SD.begin(chipSelect)) {
  //   Serial.println("Card failed");
  //   while(1);
  // }
  // Serial.println(" SD card OK");   
  // File dataFile = SD.open("datalog.txt", FILE_WRITE);
  // if (dataFile) {                          // if the file is available, write to it:
  //   dataFile.println("NanoDataLogger_bmp first tests");
  //   dataFile.print("Sealevel pressure used is ");
  //   dataFile.println("Date,Time,UTCtime,Temp_C,Pressure_hPa,Altitude_M");
  //   dataFile.close();
  // }
  // else {
  //   Serial.println("file error");          // if the file is not open, display an error:
  // }
  ADS.begin();
  initRelays();

  InitializeButtons();
}

void loop()
{

  Serial.print(digitalRead(buttonPin));
  Serial.print(digitalRead(buttonPin2));
  Serial.println(digitalRead(buttonPin3));

  if(digitalRead(buttonPin) == false){
    pcf8575.digitalWrite(rVFD, true);
  }else{
    pcf8575.digitalWrite(rVFD, false);
  }

  if(digitalRead(buttonPin2) == false){
    pcf8575.digitalWrite(rOpenValve, true);
  }else{
    pcf8575.digitalWrite(rOpenValve, false);
  }
  
  if(digitalRead(buttonPin3) == false){
    pcf8575.digitalWrite(rCloseVlave, true);
  }else{
    pcf8575.digitalWrite(rCloseVlave, false);
  }
  

  

  // ADS.setGain(0);

  // int16_t val_0 = ADS.readADC(0);
  // int16_t val_1 = ADS.readADC(1);
  // int16_t val_2 = ADS.readADC(2);
  // int16_t val_3 = ADS.readADC(3);

  // float f = ADS.toVoltage(1); //  voltage factor

  // Serial.print("\tAnalog0: ");
  // Serial.print(val_0);
  // Serial.print('\t');
  // Serial.println(val_0 * f, 3);
  // Serial.print("\tAnalog1: ");
  // Serial.print(val_1);
  // Serial.print('\t');
  // Serial.println(val_1 * f, 3);
  // Serial.print("\tAnalog2: ");
  // Serial.print(val_2);
  // Serial.print('\t');
  // Serial.println(val_2 * f, 3);
  // Serial.print("\tAnalog3: ");
  // Serial.print(val_3);
  // Serial.print('\t');
  // Serial.println(val_3 * f, 3);
  // Serial.println();

  // delay(1000);


}