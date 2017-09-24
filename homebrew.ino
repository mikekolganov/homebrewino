// RECIPES

const char RECIPE_0[] PROGMEM = "CHECH|54*20|65*15|72*30|78*10|100*60(TRADITION-1;ZHATETSKIY-55)";
const char RECIPE_1[] PROGMEM = "HOBGOBLIN|68*60|78*5|100*60(CASCADE;1)(GOLDINGS-30;FUGGLE-55)";
const char RECIPE_2[] PROGMEM = "ZHIGULEVSKOE|54*20|64*15|72*20|78*10|100*90(ISTRA-20;MOSKOW-85)";

const int RECIPES_COUNT = 3;

const char* const RECIPES[] PROGMEM = {
  RECIPE_0,
  RECIPE_1,
  RECIPE_2
};

// INCLUDES

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROMex.h>

// PINS

#define D_PIN_ONE_WIRE  12
#define D_PIN_BACKLIGHT 10
#define D_PIN_BUZZER 11

#define A_PIN_KEYS 0
#define A_PIN_ESCAPE 1

// UI SCREENS

#define SCREEN_WELCOME            1
#define SCREEN_MAIN_MENU          2
#define SCREEN_CHOOSE_RECIPE_MENU 3
#define SCREEN_SETTINGS_MENU      4
#define SCREEN_CREDITS            5
#define SCREEN_DEBUG              6
#define SCREEN_RECIPE_BREWING            7
#define SCREEN_MANUAL_BREWING     8

// UI DELAYS

#define WELCOME_DELAY        300
#define SENSORS_DELAY        1000
#define RENDER_DELAY         200
#define BACKLIGHT_DELAY      200
#define BUTTONS_DELAY        200

// SETTINGS

#define SETTING_ADDRESS_HEATER_POWER 100
#define SETTING_ADDRESS_TANK_VOLUME  102
#define SETTING_ADDRESS_BACKLIGHT    104

#define SETTING_HEATER_POWER_DEFALUT 2000; // Watt
#define SETTING_TANK_VOLUME_DEFALUT  25;   // Liters
#define SETTING_BACKLIGHT_DEFALUT    80;   // Percents

int SETTING_HEATER_POWER;
int SETTING_TANK_VOLUME;
int SETTING_BACKLIGHT;

// ENVIRONMENT

OneWire oneWire(D_PIN_ONE_WIRE);
DallasTemperature sensors(&oneWire);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

float THERMOMETER_ONE;
float THERMOMETER_TWO;

// CHARACTER CONSTANTS

const char POINTER_SYMBOL = char(165);
const char DEGREE_SYMBOL = char(223);

// BREWING!

boolean BREWING_IN_PROGRESS;
String BREWING_RECIPE;
unsigned int BREWING_PASSED;

// UI STATE

int ACTIVE_SCREEN     = 1;
int ACTIVE_ITEM_INDEX = 0;
int PREVIOUS_ACTIVE_ITEM_INDEX = 0;

// INPUT STATE

boolean BUTTON_UP     = false;
boolean BUTTON_RIGHT  = false;
boolean BUTTON_DOWN   = false;
boolean BUTTON_LEFT   = false;
boolean BUTTON_SELECT = false;
boolean BUTTON_ESCAPE = false;

// LOOP THROTTLE

unsigned long fnDelay_render = 0;
unsigned long fnDelay_requestTemperatures = 0;
unsigned long fnDelay_requestButtonsState = 0;
unsigned long fnDelay_adjustBacklight = 0;

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void adjustBacklight() {
  if (millis() - fnDelay_adjustBacklight < BACKLIGHT_DELAY) return;
  fnDelay_adjustBacklight = millis();

  analogWrite(D_PIN_BACKLIGHT, round(255 / 100 * SETTING_BACKLIGHT));
}

void requestTemperatures() {
  if (millis() - fnDelay_requestTemperatures < SENSORS_DELAY) return;
  fnDelay_requestTemperatures = millis();

  sensors.requestTemperatures();
  THERMOMETER_ONE = sensors.getTempCByIndex(0);
  THERMOMETER_TWO = sensors.getTempCByIndex(1);
}

void buttonSound() {
  tone(D_PIN_BUZZER, 2550, 15);
  delay(15);
  pinMode(D_PIN_BUZZER, INPUT);
}

void requestButtonsState() {
  if (millis() - fnDelay_requestButtonsState < BUTTONS_DELAY) return;
  fnDelay_requestButtonsState = millis();

  if (BUTTON_UP || BUTTON_RIGHT || BUTTON_DOWN || BUTTON_LEFT || BUTTON_SELECT || BUTTON_ESCAPE) return;

  int keysValue = analogRead(A_PIN_KEYS);
  if      (keysValue < 100) { buttonSound(); BUTTON_RIGHT = true;  }
  else if (keysValue < 200) { buttonSound(); BUTTON_UP = true;     }
  else if (keysValue < 400) { buttonSound(); BUTTON_DOWN = true;   }
  else if (keysValue < 600) { buttonSound(); BUTTON_LEFT = true;   }
  else if (keysValue < 800) { buttonSound(); BUTTON_SELECT = true; }

  int escapeValue = analogRead(A_PIN_ESCAPE);
  if    (escapeValue < 100) { buttonSound(); BUTTON_ESCAPE = true; }
}

void releaseButtonsState() {
  BUTTON_RIGHT = false;
  BUTTON_UP = false;
  BUTTON_DOWN = false;
  BUTTON_LEFT = false;
  BUTTON_SELECT = false;
  BUTTON_ESCAPE = false;
}

void renderIterableList(String items[], int length) {
  if (BUTTON_UP || BUTTON_DOWN) {
    PREVIOUS_ACTIVE_ITEM_INDEX = ACTIVE_ITEM_INDEX;
    BUTTON_UP ? ACTIVE_ITEM_INDEX-- : ACTIVE_ITEM_INDEX++;

    if (ACTIVE_ITEM_INDEX >= length) { ACTIVE_ITEM_INDEX = 0; }
    if (ACTIVE_ITEM_INDEX < 0) { ACTIVE_ITEM_INDEX = length - 1; }
    releaseButtonsState();
  }

  String firstRow;
  String secondRow;

  for(int i = 0; i < length; i++) {
    if (i != ACTIVE_ITEM_INDEX) continue;
    boolean upDirection = true;
    if (ACTIVE_ITEM_INDEX == PREVIOUS_ACTIVE_ITEM_INDEX) {
      upDirection = true;
    }
    else if (ACTIVE_ITEM_INDEX == 0 && PREVIOUS_ACTIVE_ITEM_INDEX == (length - 1)) {
      upDirection = false;
    }
    else if (ACTIVE_ITEM_INDEX == length -1 && PREVIOUS_ACTIVE_ITEM_INDEX == 0) {
      upDirection = true;
    }
    else {
      upDirection = (PREVIOUS_ACTIVE_ITEM_INDEX > ACTIVE_ITEM_INDEX);
    }

    if (upDirection) {
      firstRow = POINTER_SYMBOL + items[i];
      secondRow = (i == length - 1) ? " " + items[0] : " " + items[i+1];
    }
    else {
      firstRow = (i == 0) ? " " + items[length - 1] : " " + items[i-1];
      secondRow = POINTER_SYMBOL + items[i];
    }
  }

  lcd.clear();
  lcd.print(firstRow);
  lcd.setCursor(0, 1);
  lcd.print(secondRow);
}

void renderWelcome() {
  lcd.clear();
  lcd.print("WELCOME TO");
  lcd.setCursor(0, 1);
  lcd.print("HOMEBREW ROBOT!");
  delay(WELCOME_DELAY);

  ACTIVE_SCREEN = 2;
}

void renderMainMenu() {
  String items[5];
  int itemIndex = 0;

  items[itemIndex++] = "CHOOSE RECIPE";
  items[itemIndex++] = "MANUAL BREWING";
  items[itemIndex++] = "SETTINGS";
  items[itemIndex++] = "CREDITS";
  items[itemIndex++] = "DEBUG";

  renderIterableList(items, itemIndex);

  if (BUTTON_SELECT) {
    releaseButtonsState();

    if (items[ACTIVE_ITEM_INDEX] == "CHOOSE RECIPE") {
      ACTIVE_SCREEN = SCREEN_CHOOSE_RECIPE_MENU;
    }
    else if (items[ACTIVE_ITEM_INDEX] == "MANUAL BREWING") {
      ACTIVE_SCREEN = SCREEN_MANUAL_BREWING;
    }
    else if (items[ACTIVE_ITEM_INDEX] == "SETTINGS") {
      ACTIVE_SCREEN = SCREEN_SETTINGS_MENU;
    }
    else if (items[ACTIVE_ITEM_INDEX] == "CREDITS") {
      ACTIVE_SCREEN = SCREEN_CREDITS;
    }
    else if (items[ACTIVE_ITEM_INDEX] == "DEBUG") {
      ACTIVE_SCREEN = SCREEN_DEBUG;
    }

    ACTIVE_ITEM_INDEX = 0;
  }
}

String getRecipeName(String recipeCode) {
  return recipeCode.substring(0, recipeCode.indexOf('|'));
}

int getRecipeSegmentsCount(String recipeCode) {
  int segmentsCount = 0;
  for (int i = 0; i < recipeCode.length(); i++) {
    if (recipeCode.charAt(i) == '|') {
      segmentsCount += 1;
    }
  }
  return segmentsCount;
}

int getRecipeSegmentDuration(String recipeCode, int segmentIndex) {

}

int getRecipeSegmentTemperature(String recipeCode, int segmentIndex) {

}

void getSegmentReminders(String recipeCode, int segmentIndex) {

}

// vvv MAGIC GOES HERE vvv

void renderRecipeBrewing() {
  lcd.clear();
  lcd.print(String(THERMOMETER_ONE, 1) + DEGREE_SYMBOL + "/" + String(THERMOMETER_TWO, 1) + DEGREE_SYMBOL);
  lcd.setCursor(0, 1);
  lcd.print("RECIPE BREWING");
}

void renderManualBrewing() {
  lcd.clear();
  lcd.print(String(THERMOMETER_ONE, 1) + DEGREE_SYMBOL + "/" + String(THERMOMETER_TWO, 1) + DEGREE_SYMBOL);
  lcd.setCursor(0, 1);
  lcd.print("MANUAL BREWING");
}

// ^^^ MAGIC GOES HERE ^^^

void renderChooseRecipeMenu() {
  String items[RECIPES_COUNT];
  char buffer[128];
  String recipe;

  for (int i = 0; i < RECIPES_COUNT; i++) {
    strcpy_P(buffer, (char*)pgm_read_word(&(RECIPES[i])));
    recipe = String(buffer);
    items[i] = getRecipeName(recipe) + "(" + getRecipeSegmentsCount(recipe) + ")";
  }

  renderIterableList(items, RECIPES_COUNT);

  if (BUTTON_SELECT) {
    ACTIVE_SCREEN = SCREEN_RECIPE_BREWING;
    releaseButtonsState();
  }
  else if (BUTTON_ESCAPE) {
    ACTIVE_ITEM_INDEX = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    releaseButtonsState();
  }
}

String blinkOnCondition(String valueToBlink, boolean condition) {
  if (condition && (millis() / 700 % 2 == 0)) {
    String result;
    for (int i = 0; i < valueToBlink.length(); i++) {
      result += "_";
    }
    return result;
  }
  return valueToBlink;
}

String wrapAlign(String firstPart, String secondPart, int rowLength) {
  String wrapped;
  for (int i = 0; i < rowLength; i++) {
    if (i < firstPart.length()) {
      wrapped += firstPart.charAt(i);
    }
    else if (rowLength - i > secondPart.length()) {
      wrapped += " ";
    }
    else {
      wrapped += secondPart;
      break;
    }
  }

  return wrapped;
}

void renderSettingsMenu() {
  String items[3];

  items[0] = wrapAlign("POWER", blinkOnCondition(String(SETTING_HEATER_POWER), ACTIVE_ITEM_INDEX == 0) + "W", 15);
  items[1] = wrapAlign("TANK", blinkOnCondition(String(SETTING_TANK_VOLUME), ACTIVE_ITEM_INDEX == 1) + "L", 15);
  items[2] = wrapAlign("BACKLIGHT", blinkOnCondition(String(SETTING_BACKLIGHT), ACTIVE_ITEM_INDEX == 2) + "%", 15);

  renderIterableList(items, 3);

  if (BUTTON_ESCAPE) {
    ACTIVE_ITEM_INDEX = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    writeSettings();
    releaseButtonsState();
  }
  else if (BUTTON_LEFT || BUTTON_RIGHT) {
    switch (ACTIVE_ITEM_INDEX) {
        case 0:
          if ((SETTING_HEATER_POWER == 4000 && BUTTON_RIGHT) || (SETTING_HEATER_POWER == 500 && BUTTON_LEFT)) return;
          BUTTON_LEFT ? SETTING_HEATER_POWER -= 100 : SETTING_HEATER_POWER += 100;
          break;
        case 1:
          if ((SETTING_TANK_VOLUME == 60 && BUTTON_RIGHT) || (SETTING_TANK_VOLUME == 5 && BUTTON_LEFT)) return;
          BUTTON_LEFT ? SETTING_TANK_VOLUME -= 1 : SETTING_TANK_VOLUME += 1;
          break;
        case 2:
          if ((SETTING_BACKLIGHT == 100 && BUTTON_RIGHT) || (SETTING_BACKLIGHT == 0 && BUTTON_LEFT)) return;
          BUTTON_LEFT ? SETTING_BACKLIGHT -= 5 : SETTING_BACKLIGHT += 5;
          break;
    }
    releaseButtonsState();
  }
}

void renderCredits() {
  lcd.clear();
  lcd.print("Mike Kolganov");
  lcd.setCursor(0, 1);
  lcd.print("Omsk, 2017");

  if (BUTTON_ESCAPE) {
    ACTIVE_ITEM_INDEX = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    releaseButtonsState();
  }
}

void renderDebug() {
  lcd.clear();
  lcd.print("RAM: " + String(freeRam()));

  if (BUTTON_ESCAPE) {
    ACTIVE_ITEM_INDEX = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    releaseButtonsState();
  }
}

void render() {
  if (millis() - fnDelay_render < RENDER_DELAY) return;
  fnDelay_render = millis();

  switch (ACTIVE_SCREEN) {
    case SCREEN_WELCOME:
      renderWelcome();
      break;
    case SCREEN_MAIN_MENU:
      renderMainMenu();
      break;
    case SCREEN_CHOOSE_RECIPE_MENU:
      renderChooseRecipeMenu();
      break;
    case SCREEN_SETTINGS_MENU:
      renderSettingsMenu();
      break;
    case SCREEN_CREDITS:
      renderCredits();
      break;
    case SCREEN_DEBUG:
      renderDebug();
      break;
    case SCREEN_RECIPE_BREWING:
      renderRecipeBrewing();
      break;
    case SCREEN_MANUAL_BREWING:
      renderManualBrewing();
      break;
  }

  releaseButtonsState();
}

void readSettings() {
  SETTING_HEATER_POWER = EEPROM.readInt(SETTING_ADDRESS_HEATER_POWER);
  if (SETTING_HEATER_POWER == 0) SETTING_HEATER_POWER = SETTING_HEATER_POWER_DEFALUT;

  SETTING_TANK_VOLUME = EEPROM.readInt(SETTING_ADDRESS_TANK_VOLUME);
  if (SETTING_TANK_VOLUME == 0) SETTING_TANK_VOLUME = SETTING_TANK_VOLUME_DEFALUT;

  SETTING_BACKLIGHT = EEPROM.readInt(SETTING_ADDRESS_BACKLIGHT);
  if (SETTING_BACKLIGHT == 0) SETTING_BACKLIGHT = SETTING_BACKLIGHT_DEFALUT;
}

void writeSettings() {
  if (SETTING_HEATER_POWER != EEPROM.readInt(SETTING_ADDRESS_HEATER_POWER)) EEPROM.writeInt(SETTING_ADDRESS_HEATER_POWER, SETTING_HEATER_POWER);
  if (SETTING_TANK_VOLUME != EEPROM.readInt(SETTING_ADDRESS_TANK_VOLUME)) EEPROM.writeInt(SETTING_ADDRESS_TANK_VOLUME, SETTING_TANK_VOLUME);
  if (SETTING_BACKLIGHT != EEPROM.readInt(SETTING_ADDRESS_BACKLIGHT)) EEPROM.writeInt(SETTING_ADDRESS_BACKLIGHT, SETTING_BACKLIGHT);
}

void setup() {
  Serial.begin(9600);
  Serial.println();

  lcd.begin(16, 2);

  tone(D_PIN_BUZZER, 3830, 50);
  delay(50);
  pinMode(D_PIN_BUZZER, INPUT);

  EEPROM.setMemPool(0, EEPROMSizeUno);
  EEPROM.setMaxAllowedWrites(1000);
  readSettings();
}

void loop() {
  requestTemperatures();
  requestButtonsState();
  render();
  adjustBacklight();
}
