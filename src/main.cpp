#include <Arduino.h>

#include "ADS1X15.h"
ADS1115 ADS(0x48);

#include <SdFat.h>
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROMex.h>

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

#include "PCF8575.h"
PCF8575 pcf8575(0x25);

RTC_DS1307 RTC; // The real time clock object is "RTC"
#define DS1307_I2C_ADDRESS 0x68
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
uint8_t recentSec;
DateTime currentTime;
DateTime lastSave;
char alarm1String[12] = "hh:mm:ss";

SdFat SD;                  // The SdFat object is "SD"
#define MOSIpin 11         // For SD card
#define MISOpin 12         // For SD card
const int chipSelect = 10; // CS pin for the SD card

int temp;
int ph;
int pressure;
int gasCount;
int recentCount;

#define hall 5

// Data to save in SD Card
String dataToSave;

String compileData(DateTime TimeOfRecording, int PH, int TEMP, int PRESSURE, int COUNT)
{
  String DataToSave = String(TimeOfRecording.timestamp(DateTime::TIMESTAMP_FULL)) + "," +
                      String(PH) + "," +
                      String(TEMP) + "," +
                      String(PRESSURE) + "," +
                      String(COUNT);
  return DataToSave;
}

// |--------------------------------------------------------------------------------------------------------------------------------------------|
// |                                                         MENU                                                                               |
// |--------------------------------------------------------------------------------------------------------------------------------------------|
// Declaration of LCD Variables
const int NUM_MAIN_ITEMS = 4;
const int NUM_SETTING_ITEMS = 5;
const int NUM_TESTMACHINE_ITEMS = 4;

int currentMainScreen;
int currentSettingScreen;
int currentTestMenuScreen;
bool menuFlag, settingFlag, settingEditFlag, testMenuFlag, refreshScreen = false;

String menu_items[NUM_MAIN_ITEMS][2] = { // array with item names
    {"SETTING", "ENTER TO EDIT"},
    {"TEST MACHINE", "ENTER TO TEST"},
    {"RESET CURRENT COUNT", "ENTER TO RESET"},
    {"EXIT MENU", "ENTER TO RUN AUTO"}};

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"MOTOR RUN", "SEC"},
    {"SET TIME", "24 HR SETTING"},
    {"SET MAX PRESSURE", "PSI RELEASE"},
    {"SAVING INTERVAL", "MIN"},
    {"SAVE"}};

int parametersTimer[NUM_SETTING_ITEMS] = {1, 1, 1, 1};
int parametersTimerMaxValue[NUM_SETTING_ITEMS] = {1200, 24, 10, 60};

String testmachine_items[NUM_TESTMACHINE_ITEMS] = { // array with item names
    "MAIN CONTACTOR",
    "MOTOR RUN",
    "VALVE",
    "EXIT"};

int runTimeAdd = 20;
int setTimeAdd = 30;
int maxPresTimeAdd = 40;
int saveIntervalTimeAdd = 50;
int countAdd = 60;

char *secondsToHHMMSS(int total_seconds)
{
  int hours, minutes, seconds;

  hours = total_seconds / 3600;         // Divide by number of seconds in an hour
  total_seconds = total_seconds % 3600; // Get the remaining seconds
  minutes = total_seconds / 60;         // Divide by number of seconds in a minute
  seconds = total_seconds % 60;         // Get the remaining seconds

  // Format the output string
  static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
  sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
  return hhmmss_str;
}

void saveCount(int CountCurrent)
{
  EEPROM.writeInt(countAdd, CountCurrent);
}
void readCount()
{
  int currentCountFromEEPROM = EEPROM.readInt(countAdd);
  Serial.println(currentCountFromEEPROM);
  // sensor.setCount(currentCountFromEEPROM);
  recentCount = currentCountFromEEPROM;
}

void saveSettings()
{
  EEPROM.writeInt(runTimeAdd, parametersTimer[0]);
  EEPROM.writeInt(setTimeAdd, parametersTimer[1]);
  EEPROM.writeInt(maxPresTimeAdd, parametersTimer[2]);
  EEPROM.writeInt(saveIntervalTimeAdd, parametersTimer[3]);
}

void loadSettings()
{
  parametersTimer[0] = EEPROM.readInt(runTimeAdd);
  parametersTimer[1] = EEPROM.readInt(setTimeAdd);
  parametersTimer[2] = EEPROM.readInt(maxPresTimeAdd);
  parametersTimer[3] = EEPROM.readInt(saveIntervalTimeAdd);

  // RunVFD.setTimer(secondsToHHMMSS(parametersTimer[0]));
}

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

unsigned long minButtonLongPressDuration = 2000;
unsigned long buttonLongPressUpMillis;
unsigned long buttonLongPressDownMillis;
unsigned long buttonLongPressEnterMillis;
bool buttonStateLongPressUp = false;
bool buttonStateLongPressDown = false;
bool buttonStateLongPressEnter = false;

const int intervalButton = 50;
unsigned long previousButtonMillis;
unsigned long buttonPressDuration;
unsigned long currentMillis;

const int intervalButton2 = 50;
unsigned long previousButtonMillis2;
unsigned long buttonPressDuration2;
unsigned long currentMillis2;

const int intervalButton3 = 50;
unsigned long previousButtonMillis3;
unsigned long buttonPressDuration3;
unsigned long currentMillis3;

void InitializeButtons()
{
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buttonPin2, INPUT_PULLUP);
  pinMode(buttonPin3, INPUT_PULLUP);
}

void readButtonUpState()
{
  if (currentMillis - previousButtonMillis > intervalButton)
  {
    int buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && buttonStatePrevious == HIGH && !buttonStateLongPressUp)
    {
      buttonLongPressUpMillis = currentMillis;
      buttonStatePrevious = LOW;
    }
    buttonPressDuration = currentMillis - buttonLongPressUpMillis;
    if (buttonState == LOW && !buttonStateLongPressUp && buttonPressDuration >= minButtonLongPressDuration)
    {
      buttonStateLongPressUp = true;
    }
    if (buttonStateLongPressUp == true)
    {
      // Insert Fast Scroll Up
      if (menuFlag == true)
      {
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
            {
              parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
            }
            else
            {
              parametersTimer[currentSettingScreen] += 1;
            }
          }
          else
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              currentSettingScreen = 0;
            }
            else
            {
              currentSettingScreen++;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
          {
            currentTestMenuScreen = 0;
          }
          else
          {
            currentTestMenuScreen++;
          }
        }
        else
        {
          if (currentMainScreen == NUM_MAIN_ITEMS - 1)
          {
            currentMainScreen = 0;
          }
          else
          {
            currentMainScreen++;
          }
        }
      }
    }

    if (buttonState == HIGH && buttonStatePrevious == LOW)
    {
      buttonStatePrevious = HIGH;
      buttonStateLongPressUp = false;
      if (buttonPressDuration < minButtonLongPressDuration)
      {
        // Short Scroll Up
        if (menuFlag == true)
        {
          refreshScreen = true;
          if (settingFlag == true)
          {
            if (settingEditFlag == true)
            {
              if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
              {
                parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
              }
              else
              {
                parametersTimer[currentSettingScreen] += 1;
              }
            }
            else
            {
              if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
              {
                currentSettingScreen = 0;
              }
              else
              {
                currentSettingScreen++;
              }
            }
          }
          else if (testMenuFlag == true)
          {
            if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
            {
              currentTestMenuScreen = 0;
            }
            else
            {
              currentTestMenuScreen++;
            }
          }
          else
          {
            if (currentMainScreen == NUM_MAIN_ITEMS - 1)
            {
              currentMainScreen = 0;
            }
            else
            {
              currentMainScreen++;
            }
          }
        }
      }
    }
    previousButtonMillis = currentMillis;
  }
}

void readButtonDownState()
{
  if (currentMillis2 - previousButtonMillis2 > intervalButton2)
  {
    int buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 == LOW && buttonStatePrevious2 == HIGH && !buttonStateLongPressDown)
    {
      buttonLongPressDownMillis = currentMillis2;
      buttonStatePrevious2 = LOW;
    }
    buttonPressDuration2 = currentMillis2 - buttonLongPressDownMillis;
    if (buttonState2 == LOW && !buttonStateLongPressDown && buttonPressDuration2 >= minButtonLongPressDuration)
    {
      buttonStateLongPressDown = true;
    }
    if (buttonStateLongPressDown == true)
    {
      if (menuFlag == true)
      {
        refreshScreen = true;
        if (settingFlag == true)
        {
          if (settingEditFlag == true)
          {
            if (parametersTimer[currentSettingScreen] <= 0)
            {
              parametersTimer[currentSettingScreen] = 0;
            }
            else
            {
              parametersTimer[currentSettingScreen] -= 1;
            }
          }
          else
          {
            if (currentSettingScreen == 0)
            {
              currentSettingScreen = NUM_SETTING_ITEMS - 1;
            }
            else
            {
              currentSettingScreen--;
            }
          }
        }
        else if (testMenuFlag == true)
        {
          if (currentTestMenuScreen == 0)
          {
            currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
          }
          else
          {
            currentTestMenuScreen--;
          }
        }
        else
        {
          if (currentMainScreen == 0)
          {
            currentMainScreen = NUM_MAIN_ITEMS - 1;
          }
          else
          {
            currentMainScreen--;
          }
        }
      }
    }

    if (buttonState2 == HIGH && buttonStatePrevious2 == LOW)
    {
      buttonStatePrevious2 = HIGH;
      buttonStateLongPressDown = false;
      if (buttonPressDuration2 < minButtonLongPressDuration)
      {
        if (menuFlag == true)
        {
          refreshScreen = true;
          if (settingFlag == true)
          {
            if (settingEditFlag == true)
            {
              if (currentSettingScreen == 2)
              {
                if (parametersTimer[currentSettingScreen] <= 2)
                {
                  parametersTimer[currentSettingScreen] = 2;
                }
                else
                {
                  parametersTimer[currentSettingScreen] -= 1;
                }
              }
              else
              {
                if (parametersTimer[currentSettingScreen] <= 0)
                {
                  parametersTimer[currentSettingScreen] = 0;
                }
                else
                {
                  parametersTimer[currentSettingScreen] -= 1;
                }
              }
            }
            else
            {
              if (currentSettingScreen == 0)
              {
                currentSettingScreen = NUM_SETTING_ITEMS - 1;
              }
              else
              {
                currentSettingScreen--;
              }
            }
          }
          else if (testMenuFlag == true)
          {
            if (currentTestMenuScreen == 0)
            {
              currentTestMenuScreen = NUM_TESTMACHINE_ITEMS - 1;
            }
            else
            {
              currentTestMenuScreen--;
            }
          }
          else
          {
            if (currentMainScreen == 0)
            {
              currentMainScreen = NUM_MAIN_ITEMS - 1;
            }
            else
            {
              currentMainScreen--;
            }
          }
        }
      }
    }
    previousButtonMillis2 = currentMillis2;
  }
}

void readButtonEnterState()
{
  if (currentMillis3 - previousButtonMillis3 > intervalButton3)
  {
    int buttonState3 = digitalRead(buttonPin3);
    if (buttonState3 == LOW && buttonStatePrevious3 == HIGH && !buttonStateLongPressEnter)
    {
      buttonLongPressEnterMillis = currentMillis3;
      buttonStatePrevious3 = LOW;
    }
    buttonPressDuration3 = currentMillis3 - buttonLongPressEnterMillis;
    if (buttonState3 == LOW && !buttonStateLongPressEnter && buttonPressDuration3 >= minButtonLongPressDuration)
    {
      buttonStateLongPressEnter = true;
    }
    if (buttonStateLongPressEnter == true)
    {
      // Insert Fast Scroll Enter
      Serial.println("Long Press Enter");
      if (menuFlag == false)
      {
        refreshScreen = true;
        menuFlag = true;
      }
    }

    if (buttonState3 == HIGH && buttonStatePrevious3 == LOW)
    {
      buttonStatePrevious3 = HIGH;
      buttonStateLongPressEnter = false;
      if (buttonPressDuration3 < minButtonLongPressDuration)
      {
        if (menuFlag == true)
        {
          refreshScreen = true;
          if (currentMainScreen == 0 && settingFlag == true)
          {
            if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
            {
              settingFlag = false;
              saveSettings();
              loadSettings();
              currentSettingScreen = 0;
              // setTimers();
            }
            else
            {
              if (settingEditFlag == true)
              {
                settingEditFlag = false;
              }
              else
              {
                settingEditFlag = true;
              }
            }
          }
          else if (currentMainScreen == 1 && testMenuFlag == true)
          {
            if (currentTestMenuScreen == NUM_TESTMACHINE_ITEMS - 1)
            {
              currentMainScreen = 0;
              currentTestMenuScreen = 0;
              testMenuFlag = false;
              // stopAll();
            }
            else if (currentTestMenuScreen == 0)
            {
              // if (ContactorVFD.getMotorState() == false)
              // {
              //   ContactorVFD.relayOn();
              // }
              // else
              // {
              //   ContactorVFD.relayOff();
              // }
            }
            else if (currentTestMenuScreen == 1)
            {
              // if (RunVFD.getMotorState() == false)
              // {
              //   RunVFD.relayOn();
              // }
              // else
              // {
              //   RunVFD.relayOff();
              // }
            }
            else if (currentTestMenuScreen == 2)
            {
              // if (GasValve.getMotorState() == false)
              // {
              //   GasValve.relayOn();
              // }
              // else
              // {
              //   GasValve.relayOff();
              // }
            }
          }
          else
          {
            if (currentMainScreen == 0)
            {
              settingFlag = true;
            }
            else if (currentMainScreen == 1)
            {
              testMenuFlag = true;
            }else if(currentMainScreen == 2){
              // sensor.resetCount();
              saveCount(0);
            }
            else
            {
              menuFlag = false;
            }
          }
        }
      }
    }
    previousButtonMillis3 = currentMillis3;
  }
}


void ReadButtons()
{
  currentMillis = millis();
  currentMillis2 = millis();
  currentMillis3 = millis();
  readButtonEnterState();
  readButtonUpState();
  readButtonDownState();
}

// |--------------------------------------------------------------------------------------------------------------------------------------------|
// |                                                         INITIALIZE METHOD                                                                  |
// |--------------------------------------------------------------------------------------------------------------------------------------------|
void printMainMenu(String MenuItem, String Action)
{
  lcd.clear();
  lcd.print(MenuItem);
  lcd.setCursor(0, 3);
  lcd.write(0);
  lcd.setCursor(2, 3);
  lcd.print(Action);
  refreshScreen = false;
}

void printSettingScreen(String SettingTitle, String Unit, double Value, bool EditFlag, bool SaveFlag)
{
  lcd.clear();
  lcd.print(SettingTitle);
  lcd.setCursor(0, 1);

  if (SaveFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    lcd.print("ENTER TO SAVE ALL");
  }
  else
  {
    lcd.print(Value);
    lcd.print(" ");
    lcd.print(Unit);
    lcd.setCursor(0, 3);
    lcd.write(0);
    lcd.setCursor(2, 3);
    if (EditFlag == false)
    {
      lcd.print("ENTER TO EDIT");
    }
    else
    {
      lcd.print("ENTER TO SAVE");
    }
  }
  refreshScreen = false;
}

void printTestScreen(String TestMenuTitle, String Job, bool Status, bool ExitFlag)
{
  lcd.clear();
  lcd.print(TestMenuTitle);
  if (ExitFlag == false)
  {
    lcd.setCursor(0, 2);
    lcd.print(Job);
    lcd.print(" : ");
    if (Status == true)
    {
      lcd.print("ON");
    }
    else
    {
      lcd.print("OFF");
    }
  }

  if (ExitFlag == true)
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Exit Test");
  }
  else
  {
    lcd.setCursor(0, 3);
    lcd.print("Click to Run Test");
  }
  refreshScreen = false;
}

void printRunAuto(int PH, int PRESSURE, int TEMP, int COUNT)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  char buff[] = "hh:mm:ss DD-MM-YYYY";
  lcd.print(currentTime.toString(buff));
  lcd.setCursor(0, 1);
  char buff2[] = "hh:mm:ss DD-MM-YYYY";
  lcd.print(lastSave.toString(buff2));
  lcd.setCursor(0, 2);
  lcd.print("PH");
  lcd.setCursor(6, 2);
  lcd.print("TEMP");
  lcd.setCursor(11, 2);
  lcd.print("PSI");
  lcd.setCursor(16, 2);
  lcd.print("GAS");

  lcd.setCursor(0, 3);
  lcd.print(PH);
  lcd.setCursor(6, 3);
  lcd.print(TEMP);
  lcd.setCursor(11, 3);
  lcd.print(PRESSURE);
  lcd.setCursor(16, 3);
  lcd.print(COUNT);
}

void printScreens()
{
  if (menuFlag == false)
  {
    // printRunAuto(ph, pressure, temp, sensor.getCount());
    // Serial.println(compileData(currentTime, ph, temp, pressure, sensor.getCount()));
    refreshScreen = false;
  }
  else
  {
    if (settingFlag == true)
    {
      if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
      {
        printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, true);
      }
      else
      {
        printSettingScreen(setting_items[currentSettingScreen][0], setting_items[currentSettingScreen][1], parametersTimer[currentSettingScreen], settingEditFlag, false);
      }
    }
    else if (testMenuFlag == true)
    {
      switch (currentTestMenuScreen)
      {
      case 0:
        // printTestScreen(testmachine_items[currentTestMenuScreen], "Status", ContactorVFD.getMotorState(), false);
        break;
      case 1:
        // printTestScreen(testmachine_items[currentTestMenuScreen], "Status", RunVFD.getMotorState(), false);
        break;
      case 2:
        // printTestScreen(testmachine_items[currentTestMenuScreen], "Status", GasValve.getMotorState(), false);
        break;
      case 3:
        printTestScreen(testmachine_items[currentTestMenuScreen], "", true, true);
        break;

      default:
        break;
      }
    }
    else
    {
      printMainMenu(menu_items[currentMainScreen][0], menu_items[currentMainScreen][1]);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);

  Wire.begin();                                 // initialize the I2C interface
  RTC.begin();                                  // initialize the RTC
  RTC.adjust(DateTime((__DATE__), (__TIME__))); // sets the RTC to the time the sketch was compiled.

  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
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

  if (digitalRead(buttonPin) == false)
  {
    pcf8575.digitalWrite(rVFD, true);
  }
  else
  {
    pcf8575.digitalWrite(rVFD, false);
  }

  if (digitalRead(buttonPin2) == false)
  {
    pcf8575.digitalWrite(rOpenValve, true);
  }
  else
  {
    pcf8575.digitalWrite(rOpenValve, false);
  }

  if (digitalRead(buttonPin3) == false)
  {
    pcf8575.digitalWrite(rCloseVlave, true);
  }
  else
  {
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