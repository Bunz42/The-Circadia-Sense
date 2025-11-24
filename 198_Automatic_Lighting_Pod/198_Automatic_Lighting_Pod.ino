/*
  HID Circadian Light Controller (LEDS + TIME + LCD)
  Authors: Raymond Hao, Alex Li, Ethan Sun, Sidney Ruan

  1. Tracks time with built-in RTC (Real Time Clock) in the Arduino R4 Minima.
  2. Fades LEDs (NeoPixel WS2812b LED Strip) based on time of day (Daytime = White, Afternoon = Amber, Night = Cool Blue).
  3. LCD: Displays current Time (HH:MM) and LED Mode.

  *** PIN CONFIGURATION ***
  - LCD Parallel Data Pins: 7 (RS), 8 (E), 9 (D4), 10 (D5), 11 (D6), 12 (D7)
  - LED Data Pin: 6
*/

// --- INCLUDE LIBRARIES (RTC, NEOPIXEL, LIQUID CRYSTAL, ULTRASONIC SENSOR)---
#include "RTC.h"
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

// --- LED STRIP CONFIG ---
#define LED_PIN 6
#define LED_COUNT 60
#define MAX_BRIGHTNESS 255

// --- ULTRASONIC SENSOR CONFIG
#define TRIG_PIN 5
#define ECHO_PIN 3
#define DIST_MIN 5
#define DIST_MAX 25

// --- Fading Configuration ---
#define FADE_DURATION_MS 1800000UL // 30 minute gradual fade
// #define FADE_DURATION_MS 10000UL // UNCOMMENT FOR FAST TESTING (10 second fade)

// --- BRIGHTNESS SMOOTHING CONFIG ---
#define BRIGHTNESS_FADE_DURATION 2000.0 // Time in ms to go from 0 to 100% brightness (used to fade smoothly into the brightness the user is adjusting the LEDs to)

// --- LED MODE TIMINGS (24-hour format) ---
#define HOUR_DAY_START 7        // 7:00 AM
#define HOUR_AFTERNOON_START 17 // 5:00 PM
#define HOUR_NIGHT_START 21     // 9:00 PM

// --- STRUCTURE FOR LED COLOR ---
struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

// --- COLOR DEFINITIONS (BY MODE) ---
const RGBColor COLOR_DAYTIME = {255, 255, 255}; // Bright Cool White
const RGBColor COLOR_AFTERNOON = {255, 140, 0}; // Warm Amber/Orange
const RGBColor COLOR_NIGHT = {0, 10, 50}; // Calm Blue

// --- GLOBAL STATE VARIABLES (Tracks fading state, update timers, LED color states, and the mode to be printed to the LCD) ---
unsigned long fadeStartTime = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastBrightUpdate = 0;

RGBColor currentColor = COLOR_NIGHT;
RGBColor startColor = COLOR_NIGHT;
RGBColor endColor = COLOR_NIGHT;

// --- COLOR DEFINITIONS (BY MODE) ---
const RGBColor COLOR_DAYTIME = {0, 10, 50};       // Calm Blue (morning)
const RGBColor COLOR_AFTERNOON = {255, 255, 255}; // Bright White
const RGBColor COLOR_NIGHT = {255, 140, 0};       // Warm Amber/Orange

String currentMode = "NIGHT";

// --- BRIGHTNESS VARIABLES ---
uint8_t targetBrightness = 255;         // Where we WANT to be
float currentBrightnessPrecise = 255.0; // Where we ARE (float for smooth steps)
uint8_t globalBrightness = 255;         // The integer value sent to LEDs

// --- INITIALIZE OBJECTS (LED Strip and LCD) ---
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// --- FUNCTION DECLARATIONS ---
void checkMode(int currentHour);
void startFade(RGBColor newEndColor, String newMode);
void updateFade();
void applyCurrentColor();
void updateLCD(String mode, RTCTime& time);
long getDistance();
void updateBrightnessSmoothly();

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  strip.begin(); //Initialize the LED strip
  strip.show();
  RTC.begin(); //Initialize the Real-Time-Clock (RTC)

  //Initialize the Liquid Crystal Display
  lcd.begin(16, 2);
  lcd.setCursor(0, 1);

  /*
    RTC Time Set (CRITICAL):
    Uncomment, Edit Time, Upload, Re-comment, Upload.
  */
  //Args: day, Month::<name>, year, hour, minute, second, DayOfWweek::<name>, SaveLight::<name>
  // RTCTime startTime(21, Month::NOVEMBER, 2025, 16, 00, 00, DayOfWeek::FRIDAY, SaveLight::SAVING_TIME_ACTIVE);
  // RTC.setTime(startTime); // Set the time to whatever the current time and date is manually.

  //Initial mode check
  RTCTime now;
  RTC.getTime(now);
  checkMode(now.getHour());

  lastBrightUpdate = millis();
}

void loop() {
  //Retrieve the current time from the RTC
  RTCTime currentTime;
  RTC.getTime(currentTime);

  //For performance, only perform a mode check every minute
  if(millis() - lastTimeUpdate >= 60000){
    checkMode(currentTime.getHour());
    lastTimeUpdate = millis();
  }

  long dist = getDistance();

  // If hand is within range (5cm - 25cm), update the TARGET brightness
  if(dist < DIST_MAX){
    int mappedBrightness = map(dist, DIST_MIN, DIST_MAX, 255, 0); 
    targetBrightness = constrain(mappedBrightness, 0, 255); 
  }

  updateBrightnessSmoothly();

  fade(); //Begin the fade to the color associated with the current mode
  applyCurrentColor(); //Apply the associated color to the LED strip

  //Update the LCD every second to avoid flickering
  if(millis() - lastLCDUpdate >= 1000){
    updateLCD(currentMode, currentTime);
    lastLCDUpdate = millis();
  }

  delay(50); //For stability
}

void checkMode(int currentHour){
  String mode = currentMode;
  RGBColor color = currentColor;

  // Check which mode to set the LEDs to based on the current time of day.
  if (currentHour >= HOUR_DAY_START && currentHour < HOUR_AFTERNOON_START) {
    mode = "DAYTIME";
    color = COLOR_DAYTIME;
  } else if (currentHour >= HOUR_AFTERNOON_START && currentHour < HOUR_NIGHT_START) {
    mode = "AFTERNOON";
    color = COLOR_AFTERNOON;
  } else {
    mode = "NIGHT";
    color = COLOR_NIGHT;
  }

  //Only perform the fade setup and mode change if the mode ACTUALLY changed during a mode check
  if (mode != currentMode) {
      setupFade(color, mode);
  }
}

//Changes the current color to an argument "color" and the mode to an argument "mode", then sets up the fade by assigning the fadeStartTime state variable
void setupFade(RGBColor color, String mode){
  startColor = currentColor;
  endColor = color;
  currentMode = mode;

  //Makes the fade start time the current number of milliseconds that have passed since the program started running.
  fadeStartTime = millis();

  Serial.print("Mode Change: ");
  Serial.println(mode);
}

//Performs the gradual fade to a new color through a linear interpolation algorithm (Credit: Google Gemini Pro)
void fade() {
  unsigned long timeElapsed = millis() - fadeStartTime;
  
  if (timeElapsed < FADE_DURATION_MS) {
    // Calculate how far through the fade we are (0.0 to 1.0)
    float factor = (float)timeElapsed / FADE_DURATION_MS;

    // Linear interpolation algorithm to fade the colors
    currentColor.r = (uint8_t)((startColor.r * (1.0 - factor)) + (endColor.r * factor));
    currentColor.g = (uint8_t)((startColor.g * (1.0 - factor)) + (endColor.g * factor));
    currentColor.b = (uint8_t)((startColor.b * (1.0 - factor)) + (endColor.b * factor));
    
  } else {
    // Fade complete
    currentColor = endColor;
  }
}

//Applies the current color to the LED strip
void applyCurrentColor(){
  for(int i = 0; i < strip.numPixels(); ++i){
    strip.setPixelColor(i, currentColor.r, currentColor.g, currentColor.b);
  }
  strip.show();
}

//Updates the LCD accordingly by printing the current time and mode and 2 separate lines
void updateLCD(String mode, RTCTime& time){
  //Line 1: display the current time.
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  if(time.getHour() < 10) lcd.print("0");
  lcd.print(time.getHour());
  lcd.print(":");
  if(time.getMinutes() < 10) lcd.print("0");
  lcd.print(time.getMinutes());
  lcd.print("      "); //Clear any trailing text

  //Line 2: display the current mode (centered)
  int screenWidth = 16;
  int textWidth = mode.length() + 6;
  int startColumn = (screenWidth - textWidth)/2;

  lcd.setCursor(0, 1);
  lcd.print("                "); //Clear the line
  lcd.setCursor(startColumn, 1);
  lcd.print("MODE: ");
  lcd.print(mode);
}
