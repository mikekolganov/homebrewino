////////////////////////////////////////////////////////////////////////////////
// RECIPES DECLARATION START
////////////////////////////////////////////////////////////////////////////////

const char RECIPE_0[] PROGMEM = "(Chech)(Mash 1:54*20)(Mash 2:65*15)(Mash 3:72*30)(Mash out:78*10)(Boiling:100*60{Tradition:1}{Zhatetskiy:55})";
const char RECIPE_1[] PROGMEM = "(Hobgoblin)(Mash 1:68*60)(Mash out:78*5)(Boiling:100*60{Cascade:1}{Goldings:30}{Fuggle:55})";
const char RECIPE_2[] PROGMEM = "(Zhigulevskoe)(Mash 1:54*20)(Mash 2:64*15)(Mash 3:72*20)(Mash out:78*10)(Boiling:100*90{Istra:20}{Moskow:85})";

const char* const RECIPES[] PROGMEM = {
  RECIPE_0,
  RECIPE_1,
  RECIPE_2
};

////////////////////////////////////////////////////////////////////////////////
// RECIPES DECLARATION END
////////////////////////////////////////////////////////////////////////////////

#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define D_PIN_ONE_WIRE  0
#define D_PIN_BACKLIGHT 10

#define A_PIN_KEYS 0
#define A_PIN_ESCAPE 1

#define SCREEN_WELCOME            1
#define SCREEN_MAIN_MENU          2
#define SCREEN_CHOOSE_RECIPE_MENU 3
#define SCREEN_SETTINGS_MENU      4
#define SCREEN_CREDITS            5
#define SCREEN_BREWING            6

#define WELCOME_DELAY        300
#define SENSORS_DELAY        1000
#define RENDER_DELAY         200
#define BUTTONS_DELAY        200

OneWire oneWire(D_PIN_ONE_WIRE);
DallasTemperature sensors(&oneWire);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int ACTIVE_SCREEN     = 1;
int ACTIVE_ITEM_INDEX = 0;
int PREVIOUS_ACTIVE_ITEM_INDEX = 0;

const char POINTER_SYMBOL = char(165);
const char DEGREE_SYMBOL = char(223);

float THERMOMETER_ONE;
float THERMOMETER_TWO;

boolean BUTTON_UP     = false;
boolean BUTTON_RIGHT  = false;
boolean BUTTON_DOWN   = false;
boolean BUTTON_LEFT   = false;
boolean BUTTON_SELECT = false;
boolean BUTTON_ESCAPE = false;

unsigned long fnDelay_render = 0;
unsigned long fnDelay_requestTemperatures = 0;
unsigned long fnDelay_requestButtonsState = 0;

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void requestTemperatures(unsigned long now) {
  if (now - fnDelay_requestTemperatures < SENSORS_DELAY) return;
  fnDelay_requestTemperatures = now;

  sensors.requestTemperatures();
  THERMOMETER_ONE = sensors.getTempCByIndex(0);
  THERMOMETER_TWO = sensors.getTempCByIndex(1);
}

void requestButtonsState(unsigned long now) {
  if (now - fnDelay_requestButtonsState < BUTTONS_DELAY) return;
  fnDelay_requestButtonsState = now;

  if (BUTTON_UP || BUTTON_RIGHT || BUTTON_DOWN || BUTTON_LEFT || BUTTON_SELECT || BUTTON_ESCAPE) return;

  int keysValue = analogRead(A_PIN_KEYS);
  if      (keysValue < 100) { BUTTON_RIGHT = true;  }
  else if (keysValue < 200) { BUTTON_UP = true;     }
  else if (keysValue < 400) { BUTTON_DOWN = true;   }
  else if (keysValue < 600) { BUTTON_LEFT = true;   }
  else if (keysValue < 800) { BUTTON_SELECT = true; }

  int escapeValue = analogRead(A_PIN_ESCAPE);
  if    (escapeValue < 100) { BUTTON_ESCAPE = true; }
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
  String items[4];
  int itemIndex = 0;

  items[itemIndex++] = "CHOOSE RECIPE";
  items[itemIndex++] = "MANUAL BREWING";
  items[itemIndex++] = "SETTINGS";
  items[itemIndex++] = "CREDITS";

  if (BUTTON_SELECT || BUTTON_RIGHT) {
    releaseButtonsState();

    if (items[ACTIVE_ITEM_INDEX] == "CREDITS") {
      ACTIVE_SCREEN = SCREEN_CREDITS;
    }
    else if (items[ACTIVE_ITEM_INDEX] == "CHOOSE RECIPE") {
      ACTIVE_SCREEN = SCREEN_CHOOSE_RECIPE_MENU;
    }
  }

  renderIterableList(items, itemIndex);
}

void renderChooseRecipeMenu() {
  if (BUTTON_ESCAPE || BUTTON_LEFT) {
    releaseButtonsState();
    ACTIVE_ITEM_INDEX = 0;
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
  }

  String items[5];
  int itemIndex = 0;

  items[itemIndex++] = "BAVARSKOE";
  items[itemIndex++] = "CHESHSKOE";
  items[itemIndex++] = "WEISSBIER";
  items[itemIndex++] = "IPA";
  items[itemIndex++] = "APA";

  renderIterableList(items, itemIndex);
}

void renderSettingsMenu() {

}

void renderCredits() {
  if (BUTTON_ESCAPE || BUTTON_LEFT) {
    releaseButtonsState();
    ACTIVE_SCREEN = SCREEN_MAIN_MENU;
  }

  lcd.clear();
  lcd.print("Mike Kolganov");
  lcd.setCursor(0, 1);
  lcd.print("Omsk, 2017");
}

void renderBrewing() {

}

void render(unsigned long now) {
  if (now - fnDelay_render < RENDER_DELAY) return;
  fnDelay_render = now;

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
    case SCREEN_BREWING:
      renderBrewing();
      break;
  }

  releaseButtonsState();
}

void setup() {
  lcd.begin(16, 2);
  analogWrite(D_PIN_BACKLIGHT, 64);
}

void loop() {
  unsigned long now = millis();

  requestTemperatures(now);
  requestButtonsState(now);
  render(now);
}
