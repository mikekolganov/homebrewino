// RECIPES
// Syntax: <NAME>|<PAUSE TEMP>*<PAUSE DURATION>(<REMINDER NAME>-<MINUTES SINCE PAUSE START>)
// Notes:
// * Mash pauses are delimited by "|", arbitrary amount of mash pauses, must be terminated with "|"
// * Reminders are optional and specified in round brackets at end of mash pause definition
const char RECIPE_1[] PROGMEM = "CHECH|54*20|65*15|72*30|78*10|100*60(TRADITION@1;ZHATETSKIY@55)";
const char RECIPE_2[] PROGMEM = "HOBGOBLIN|68*60|78*5|100*60(CASCADE@1;GOLDINGS@30;FUGGLE@55)";
const char RECIPE_3[] PROGMEM = "ZHIGULEVSKOE|54*20|64*15|72*20|78*10|100*90(ISTRA@20;MOSKOW@85)";
const char RECIPE_4[] PROGMEM = "APA|64*20|72*25|78*5|100*60(Centennial@5;Chinook@55)";
const char RECIPE_5[] PROGMEM = "VELVET|64*20|72*40|78*10|100*60(Hallertau 1/2@1;Hallertau 1/2@55)";
const char RECIPE_6[] PROGMEM = "PALE ALE|52*10|63*10|72*20|78*10|100*60(Tettnang@30;Blank@55)";

const int RECIPES_COUNT = 6;

const char* const RECIPES[] PROGMEM = {
  RECIPE_1,
  RECIPE_2,
  RECIPE_3,
  RECIPE_4,
  RECIPE_5,
  RECIPE_6,
};

const char SEGMENTS_DELIMITER = '|';
const char SEGMENT_VALUES_DELIMITER = '*';
const char REMINDERS_DELIMITER = ';';
const char REMINDER_VALUES_DELIMITER = '@';
const char REMINDER_BEGIN = '(';
const char REMINDER_END = ')';

// INCLUDES
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROMex.h>

// PINS
#define D_PIN_ONE_WIRE  12 // Temp sensors
#define D_PIN_BACKLIGHT 10 // Display backlight control
#define D_PIN_BUZZER    11 // Piezo buzzer

#define A_PIN_KEYS 0   // Arrow keys
#define A_PIN_ESCAPE 1 // Reset key, used as Esc

// UI STATE
int ACTIVE_SCREEN = 1;

byte MENU_LEVEL = 0;
char MENU_ACTIVE[10]          = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char MENU_ACTIVE_PREVIOUS[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int CAROUSEL_ACTIVE_SLIDE = 0;
int CAROUSEL_SLIDE_CHANGED_AT = 0;
boolean CAROUSEL_RESET_SLIDE = false;
boolean CAROUSEL_NEXT_SLIDE = false; 
boolean CAROUSEL_PREV_SLIDE = false; 

// UI SCREENS
#define SCREEN_WELCOME             1
#define SCREEN_MAIN_MENU           2
#define SCREEN_CHOOSE_RECIPE_MENU  3
#define SCREEN_PREVIEW_RECIPE_MENU 4
#define SCREEN_SETTINGS_MENU       5
#define SCREEN_CREDITS             6
#define SCREEN_RECIPE_BREWING      7
#define SCREEN_MANUAL_BREWING      8

// UI DELAYS
#define WELCOME_DELAY  300
#define BLINK_DELAY    700
#define CAROUSEL_DELAY 2500

// UI DIALOGS
#define DIALOG_MODE_QUESTION 1
#define DIALOG_MODE_MESSAGE  2

// SETTINGS
#define SETTING_ADDRESS_HEATER_POWER 100
#define SETTING_ADDRESS_TANK_VOLUME  102
#define SETTING_ADDRESS_BACKLIGHT    104

#define SETTING_HEATER_POWER_DEFALUT 2000 // Watt
#define SETTING_TANK_VOLUME_DEFALUT  25   // Liters
#define SETTING_BACKLIGHT_DEFALUT    80   // Percents

int SETTING_HEATER_POWER;
int SETTING_TANK_VOLUME;
int SETTING_BACKLIGHT;

// ENVIRONMENT
OneWire oneWire(D_PIN_ONE_WIRE);
DallasTemperature sensors(&oneWire);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

float THERMOMETER_ONE;
float THERMOMETER_TWO;

#define HEATER_MODE_OFF 0
#define HEATER_MODE_ON  1
byte    HEATER_MODE = HEATER_MODE_OFF;

// SYMBOLS
const char POINTER_SYMBOL = char(165);
const char DEGREE_SYMBOL = char(223);

// BREWING
unsigned int BREWIING_TIME_PROCESSED;

byte    BREWING_MODE;
#define BREWING_MODE_MANUAL 1
#define BREWING_MODE_RECIPE 2

byte    BREWING_STATE;
#define BREWING_STATE_IDLE      1
#define BREWING_STATE_WORKING   2
#define BREWING_STATE_PAUSED    3
#define BREWING_STATE_COMPLETED 4

String BREWING_RECIPE_STRING;
int    BREWING_MANUAL_TEMP;
int    BREWING_MANUAL_TIME;

// INPUT STATE
boolean BUTTON_UP     = false;
boolean BUTTON_RIGHT  = false;
boolean BUTTON_DOWN   = false;
boolean BUTTON_LEFT   = false;
boolean BUTTON_SELECT = false;
boolean BUTTON_ESCAPE = false;

// LOOP THROTTLE
#define RENDER_THROTTLE          200
#define SENSORS_THROTTLE         1000
#define BUTTONS_THROTTLE         200
#define BACKLIGHT_THROTTLE       200
#define PERSIST_BREWING_THROTTLE 60000 // Every minute, save current brewing state for recovering after unexpected power off / hanging
#define BREW_THROTTLE            1000

unsigned long fnThrottle_render              = 0;
unsigned long fnThrottle_requestTemperatures = 0;
unsigned long fnThrottle_requestButtonsState = 0;
unsigned long fnThrottle_adjustBacklight     = 0;
unsigned long fnThrottle_writeBrewingState   = 0;
unsigned long fnThrottle_brew                = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void adjustBacklight() {
  if (millis() - fnThrottle_adjustBacklight < BACKLIGHT_THROTTLE) return;
  fnThrottle_adjustBacklight = millis();

  analogWrite(D_PIN_BACKLIGHT, round(255 / 100 * SETTING_BACKLIGHT));
}

void requestTemperatures() {
  if (millis() - fnThrottle_requestTemperatures < SENSORS_THROTTLE) return;
  fnThrottle_requestTemperatures = millis();

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
  if (millis() - fnThrottle_requestButtonsState < BUTTONS_THROTTLE) return;
  fnThrottle_requestButtonsState = millis();

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

////////////////////////////////////////////////////////////////////////////////////////////////////

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

void readBrewingState() {
  // TODO: implement
}

void writeBrewingState() {
  if (millis() - fnThrottle_writeBrewingState < PERSIST_BREWING_THROTTLE) return;
  fnThrottle_writeBrewingState = millis();
  // TODO: implement
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String getRecipe(int index) {
  char buffer[128];
  strcpy_P(buffer, (char*)pgm_read_word(&(RECIPES[index])));
  return String(buffer);
}

String getRecipeName(String recipe) {
  return recipe.substring(0, recipe.indexOf(SEGMENTS_DELIMITER));
}

int getSegmentsCount(String recipe) {
  int segmentsCount = 0;
  for (int i = 0; i < recipe.length(); i++) {
    if (recipe.charAt(i) == SEGMENT_VALUES_DELIMITER) {
      segmentsCount += 1;
    }
  }
  return segmentsCount;
}

int getSegmentBegin(String recipe, int segmentIndex) {
  int segmentCounter = 0;
  int startPosition = recipe.indexOf(SEGMENTS_DELIMITER);
  while (startPosition != -1) {
    if (segmentCounter == segmentIndex) return startPosition + 1;
    startPosition = recipe.indexOf(SEGMENTS_DELIMITER, startPosition + 1);
    segmentCounter++;
  }
}

int getSegmentEnd(String recipe, int segmentIndex) {
  int segmentCounter = getSegmentsCount(recipe) - 1;
  int endPosition = recipe.length();
  while (segmentCounter >= 0) {
    if (segmentCounter == segmentIndex) return endPosition;
    endPosition = recipe.lastIndexOf(SEGMENTS_DELIMITER, getSegmentBegin(recipe, segmentCounter));
    segmentCounter--;
  }
}

String getSegment(String recipe, int index) {
  return recipe.substring(getSegmentBegin(recipe, index), getSegmentEnd(recipe, index));
}

int getSegmentDuration(String segment) {
  int from = segment.indexOf(SEGMENT_VALUES_DELIMITER) + 1;
  int to = segment.indexOf(REMINDER_BEGIN);
  if (to == -1) {
    to = segment.length();
  }
  return segment.substring(from, to).toInt();
}

int getSegmentTemperature(String segment) {
  int from = 0;
  int to = segment.indexOf(SEGMENT_VALUES_DELIMITER);
  return segment.substring(from, to).toInt();
}

int getRemindersCount(String segment) {
  if (segment.indexOf(REMINDER_BEGIN) == -1 || segment.indexOf(REMINDER_END) == -1) {
    return 0;
  }
  int reminderCounter = 0;
  int delimiterPosition = 0;
  while (delimiterPosition != -1) {
    reminderCounter++;
    delimiterPosition = segment.indexOf(REMINDERS_DELIMITER, delimiterPosition + 1);
  }
  return reminderCounter;
}

int getReminderBegin(String segment, int reminderIndex) {
  int reminderCounter = 0;
  int startPosition = segment.indexOf(REMINDER_BEGIN);
  while (startPosition != -1) {
    if (reminderCounter == reminderIndex) return startPosition + 1;
    startPosition = segment.indexOf(REMINDERS_DELIMITER, startPosition + 1);
    reminderCounter++;
  }
}

int getReminderEnd(String segment, int reminderIndex) {
  int reminderCounter = getRemindersCount(segment) - 1;
  int endPosition = segment.length() - 1;
  while (reminderCounter >= 0) {
    if (reminderCounter == reminderIndex) return endPosition;
    endPosition = segment.lastIndexOf(REMINDERS_DELIMITER, getReminderBegin(segment, reminderCounter));
    reminderCounter--;
  }
}

String getReminder(String segment, int reminderIndex) {
  return segment.substring(getReminderBegin(segment, reminderIndex), getReminderEnd(segment, reminderIndex));
}

String getReminderName(String reminder) {
  int from = 0;
  int to = reminder.indexOf(REMINDER_VALUES_DELIMITER);
  return reminder.substring(from, to);
}

int getReminderTime(String reminder) {
  int from = reminder.indexOf(REMINDER_VALUES_DELIMITER) + 1;
  int to = reminder.length();
  return reminder.substring(from, to).toInt();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

String formatTime(unsigned int time, boolean withSeconds) {
  // TODO: implement
}

String formatMinutes(unsigned int time) {
  return formatTime(time, false);
}

String formatSeconds(unsigned int time) {
  return formatTime(time, false);
}

String horizontalCarousel(String items[], int itemsCount, boolean autoPlay, int rowLength) {
  if (CAROUSEL_SLIDE_CHANGED_AT == 0) CAROUSEL_SLIDE_CHANGED_AT = millis();
  if (autoPlay && (millis() > CAROUSEL_SLIDE_CHANGED_AT + CAROUSEL_DELAY)) {
    CAROUSEL_NEXT_SLIDE = true;
  }

  if (CAROUSEL_NEXT_SLIDE || CAROUSEL_PREV_SLIDE) {
    if (CAROUSEL_NEXT_SLIDE) {
      CAROUSEL_ACTIVE_SLIDE = (CAROUSEL_ACTIVE_SLIDE == itemsCount - 1) ? 0 : CAROUSEL_ACTIVE_SLIDE + 1;
    }
    else if (CAROUSEL_PREV_SLIDE) {
      CAROUSEL_ACTIVE_SLIDE = (CAROUSEL_ACTIVE_SLIDE == 0) ? itemsCount - 1 : CAROUSEL_ACTIVE_SLIDE - 1;
    }
    CAROUSEL_SLIDE_CHANGED_AT = millis();
    CAROUSEL_NEXT_SLIDE = false;
    CAROUSEL_PREV_SLIDE = false;
  }

  if (CAROUSEL_RESET_SLIDE) {
    CAROUSEL_ACTIVE_SLIDE = 0;
    CAROUSEL_SLIDE_CHANGED_AT = millis();
    CAROUSEL_RESET_SLIDE = false;
  }


  String result;
  for (int i = 0; i < rowLength - 1; i++) {
    result += (items[CAROUSEL_ACTIVE_SLIDE].length() > i) ? String(items[CAROUSEL_ACTIVE_SLIDE].charAt(i)) : " ";
  }
  result += ">";
  return result;
}

String blinkOnCondition(String valueToBlink, boolean condition) {
  if (condition && (millis() / BLINK_DELAY % 2 == 0)) {
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

void renderIterableList(String items[], int length) {
  if (BUTTON_UP || BUTTON_DOWN) {
    MENU_ACTIVE_PREVIOUS[MENU_LEVEL] = MENU_ACTIVE[MENU_LEVEL];
    BUTTON_UP ? MENU_ACTIVE[MENU_LEVEL]-- : MENU_ACTIVE[MENU_LEVEL]++;

    if (MENU_ACTIVE[MENU_LEVEL] >= length) { MENU_ACTIVE[MENU_LEVEL] = 0; }
    if (MENU_ACTIVE[MENU_LEVEL] < 0) { MENU_ACTIVE[MENU_LEVEL] = length - 1; }
    releaseButtonsState();
  }

  String firstRow;
  String secondRow;

  for(int i = 0; i < length; i++) {
    if (i != MENU_ACTIVE[MENU_LEVEL]) continue;
    boolean upDirection = true;
    if (MENU_ACTIVE[MENU_LEVEL] == MENU_ACTIVE_PREVIOUS[MENU_LEVEL]) {
      upDirection = true;
    }
    else if (MENU_ACTIVE[MENU_LEVEL] == 0 && MENU_ACTIVE_PREVIOUS[MENU_LEVEL] == (length - 1)) {
      upDirection = false;
    }
    else if (MENU_ACTIVE[MENU_LEVEL] == length -1 && MENU_ACTIVE_PREVIOUS[MENU_LEVEL] == 0) {
      upDirection = true;
    }
    else {
      upDirection = (MENU_ACTIVE_PREVIOUS[MENU_LEVEL] > MENU_ACTIVE[MENU_LEVEL]);
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

////////////////////////////////////////////////////////////////////////////////////////////////////

void renderDialog(String message, byte mode) {

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
  MENU_LEVEL = SCREEN_MAIN_MENU;

  String items[4];
  int itemIndex = 0;

  items[itemIndex++] = "CHOOSE RECIPE";
  items[itemIndex++] = "MANUAL BREWING";
  items[itemIndex++] = "SETTINGS";
  items[itemIndex++] = "CREDITS";

  renderIterableList(items, itemIndex);

  if (BUTTON_SELECT) {
    releaseButtonsState();

    if (items[MENU_ACTIVE[MENU_LEVEL]] == "CHOOSE RECIPE") {
      ACTIVE_SCREEN = SCREEN_CHOOSE_RECIPE_MENU;
    }
    else if (items[MENU_ACTIVE[MENU_LEVEL]] == "MANUAL BREWING") {
      ACTIVE_SCREEN = SCREEN_MANUAL_BREWING;
    }
    else if (items[MENU_ACTIVE[MENU_LEVEL]] == "SETTINGS") {
      ACTIVE_SCREEN = SCREEN_SETTINGS_MENU;
    }
    else if (items[MENU_ACTIVE[MENU_LEVEL]] == "CREDITS") {
      ACTIVE_SCREEN = SCREEN_CREDITS;
    }
  }
}

void renderChooseRecipeMenu() {
  MENU_LEVEL = SCREEN_CHOOSE_RECIPE_MENU;

  String items[RECIPES_COUNT];

  for (int i = 0; i < RECIPES_COUNT; i++) {
    String recipe = getRecipe(i);
    items[i] = getRecipeName(recipe);
  }

  renderIterableList(items, RECIPES_COUNT);

  if (BUTTON_SELECT) {
    ACTIVE_SCREEN = SCREEN_PREVIEW_RECIPE_MENU;
    CAROUSEL_RESET_SLIDE = true;
    BREWING_RECIPE_STRING = getRecipe(MENU_ACTIVE[MENU_LEVEL]);
    releaseButtonsState();
  }
  else if (BUTTON_ESCAPE) {
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    MENU_LEVEL = 0;
    releaseButtonsState();
  }
}

void renderPreviewRecipeMenu() {
  String seats[10];
  int seatsCount = 0;

  seats[seatsCount++] = getRecipeName(BREWING_RECIPE_STRING);
  int segmentsCount = getSegmentsCount(BREWING_RECIPE_STRING);
  for (int i = 0; i < segmentsCount; i++) {
    String segment = getSegment(BREWING_RECIPE_STRING, i);
    int temperature = getSegmentTemperature(segment);
    int duration = getSegmentDuration(segment);
    seats[seatsCount++] = String(i + 1) + ": " + String(duration) + " " + String(temperature) + DEGREE_SYMBOL;
  }

  lcd.clear();
  lcd.print(horizontalCarousel(seats, seatsCount, true, 16));
  lcd.setCursor(0, 1);
  lcd.print(wrapAlign(String(POINTER_SYMBOL) + "YES", "NO", 16));

  if (BUTTON_SELECT) {
    ACTIVE_SCREEN = SCREEN_RECIPE_BREWING;
    BREWING_MODE = BREWING_MODE_RECIPE;
    releaseButtonsState();
  }
  else if (BUTTON_RIGHT) {
    CAROUSEL_NEXT_SLIDE = true;
  }
  else if (BUTTON_LEFT) {
    CAROUSEL_PREV_SLIDE = true;
  }
  else if (BUTTON_ESCAPE) {
    ACTIVE_SCREEN = SCREEN_CHOOSE_RECIPE_MENU;
    releaseButtonsState();
  }
}

void renderSettingsMenu() {
  MENU_LEVEL = SCREEN_SETTINGS_MENU;
  String items[3];

  items[0] = wrapAlign("POWER", blinkOnCondition(String(SETTING_HEATER_POWER), MENU_ACTIVE[MENU_LEVEL] == 0) + "W", 15);
  items[1] = wrapAlign("TANK", blinkOnCondition(String(SETTING_TANK_VOLUME), MENU_ACTIVE[MENU_LEVEL] == 1) + "L", 15);
  items[2] = wrapAlign("BACKLIGHT", blinkOnCondition(String(SETTING_BACKLIGHT), MENU_ACTIVE[MENU_LEVEL] == 2) + "%", 15);

  renderIterableList(items, 3);

  if (BUTTON_ESCAPE) {
    MENU_LEVEL = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    writeSettings();
    releaseButtonsState();
  }
  else if (BUTTON_LEFT || BUTTON_RIGHT) {
    switch (MENU_ACTIVE[MENU_LEVEL]) {
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
    MENU_LEVEL = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
    releaseButtonsState();
  }
}

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

void render() {
  if (millis() - fnThrottle_render < RENDER_THROTTLE) return;
  fnThrottle_render = millis();

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
    case SCREEN_PREVIEW_RECIPE_MENU:
      renderPreviewRecipeMenu();
      break;
    case SCREEN_SETTINGS_MENU:
      renderSettingsMenu();
      break;
    case SCREEN_CREDITS:
      renderCredits();
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

////////////////////////////////////////////////////////////////////////////////////////////////////

void heaterMode() {
  
}

void brewManual() {

}

void brewRecipe() {

}

void brew() {
  if (millis() - fnThrottle_brew < BREW_THROTTLE) return;
  fnThrottle_brew = millis();

  switch (BREWING_MODE) {
    case BREWING_MODE_RECIPE:
      brewRecipe();
      break;
    case BREWING_MODE_MANUAL:
      brewManual();
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
  adjustBacklight();
  writeBrewingState();
  render();
}
