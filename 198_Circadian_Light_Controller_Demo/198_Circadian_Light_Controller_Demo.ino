/*
  HID Circadian Light Controller (LEDS + TIME + LCD + ULTRASONIC SENSOR) DEMO
  Authors: Raymond Hao, Alex Li, Ethan Sun, Sidney Ruan

  1. Fades LEDs (NeoPixel WS2812b LED Strip) based on time of day (Daytime = White, Afternoon = Amber, Night = Cool Blue).
  2. Cycles through each mode every 10 seconds (for demo purposes only)
  3. LCD: Displays current Time (HH:MM), brightness and LED Mode.
  4. Ultrasonic Sensor: Used to adjust brightness by moving hand back and forth.

  *** PIN CONFIGURATION ***
  - LCD Parallel Data Pins: 7 (RS), 8 (E), 9 (D4), 10 (D5), 11 (D6), 12 (D7)
  - LED Data Pin: 6
  - Ultrasonic Sensor Pins: 5 (TRIG), 3 (ECHO)
*/

// --- INCLUDE LIBRARIES (RTC, NEOPIXEL, LIQUID CRYSTAL, ULTRASONIC SENSOR) ---
#include "RTC.h"
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

// --- LED STRIP CONFIG ---
#define LED_PIN 6
#define LED_COUNT 60

// --- ULTRASONIC SENSOR CONFIG ---
#define TRIG_PIN 5
#define ECHO_PIN 3
#define DIST_MIN 2
#define DIST_MAX 35

// --- FADING CONFIG ---
#define FADE_DURATION_MS 3000UL // Fast, 3 second fade (for demo purposes)
#define HOLD_MODE 10000UL // Maintain the current mode for 10s before automatically switching

// --- BRIGHTNESS SMOOTHING CONFIG ---
#define BRIGHTNESS_FADE_DURATION 2000.0 // Time in ms to go from 0 to 100% brightness (used to fade smoothly into the brightness the user is adjusting the LEDs to)

// --- LED MODE TIMINGS (24-hour format) ---
#define HOUR_DAY 10         // Simulate 10:00 AM (for daytime demo)
#define HOUR_AFTERNOON 18   // Simulate 6:00 PM (for afternoon demo)
#define HOUR_NIGHT 22       // Simulate 10:00 PM (for night demo)

// --- STRUCTURE FOR LED COLOR ---
struct RGBColor { uint8_t r; uint8_t g; uint8_t b; };

// --- COLOR DEFINITIONS (BY MODE) ---
const RGBColor COLOR_DAYTIME = {0, 10, 50};       // Calm Blue (morning)
const RGBColor COLOR_AFTERNOON = {255, 255, 255}; // Bright White
const RGBColor COLOR_NIGHT = {255, 140, 0};       // Warm Amber/Orange

// --- GLOBAL STATE VARIABLES (Tracks fading state, update timers, LED color states, and the mode to be printed to the LCD) ---
unsigned long fadeStartTime = 0;
unsigned long lastBrightUpdate = 0;
int simHour = 0; //The simulated current hour

RGBColor currentColor = COLOR_NIGHT; //For testing, just set to NIGHT for now.
RGBColor startColor = COLOR_NIGHT;   
RGBColor endColor = COLOR_NIGHT;  

String currentMode = "NIGHT";

//Brightness variables
uint8_t targetBrightness = 255;    // Where we WANT to be
float currentBrightnessPrecise = 255.0; // Where we ARE (float for smooth steps)
uint8_t globalBrightness = 255;    // The integer value sent to LEDs

// --- INITIALIZE OBJECTS (LED Strip and LCD) ---
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// --- FUNCTION DECLARATIONS (DEMO EDITION) ---
void setDemoMode(String modeName, int hour, RGBColor color);
void setupFade(RGBColor newEndColor);
void fade();
void applyCurrentColor();
void updateLCD(String mode, int hour);
long getDistance();
void updateBrightnessSmoothly();

void setup() {
  Serial.begin(9600);

  //Ultrasonic sensor pin setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  strip.begin(); //Initialize the LED strip
  strip.show();

  RTC.begin(); //Initialize the Real-Time-Clock (RTC)
  lcd.begin(16, 2); //Initialize the Liquid Crystal Display
}

void loop() {
  // --- DEMO-SPECIFIC LOGIC: JUST CYCLE THE MODES BASED ON THE TIME ELAPSED SINCE THE DEMO STARTED
  unsigned long currentMillis = millis();
  unsigned long cycleTime = currentMillis % 30000; //Cycles will be 30s long (this line ensures the cycle time value is always between 0 and 30000)

  // 0-10 Seconds: DAYTIME
  if (cycleTime < HOLD_MODE) {
    if (currentMode != "DAYTIME") {
       setDemoMode("DAYTIME", HOUR_DAY, COLOR_DAYTIME);
    }
  } 
  // 10-20 Seconds: AFTERNOON
  else if (cycleTime < (HOLD_MODE * 2)) {
    if (currentMode != "AFTERNOON") {
       setDemoMode("AFTERNOON", HOUR_AFTERNOON, COLOR_AFTERNOON);
    }
  } 
  // 20-30 Seconds: NIGHT
  else {
    if (currentMode != "NIGHT") {
       setDemoMode("NIGHT", HOUR_NIGHT, COLOR_NIGHT);
    }
  }

  // --- ULTRASONIC SENSOR LOGIC --- 
  long dist = getDistance();

  if(dist < DIST_MAX){
    int mappedBrightness = map(dist, DIST_MIN, DIST_MAX, 255, 0); //Credit: Google Gemini
    targetBrightness = constrain(mappedBrightness, 0, 255); //Smoothing of the brightness mapping (credit: google gemini)
  }

  updateBrightnessSmoothly(); //Smoothly fade the brightness when the user adjusts it

  fade(); //Fade to the color associated with the current mode
  applyCurrentColor(); //Apply the associated color to the LED strip
  updateLCD(currentMode, simHour); //Update the LCD

  delay(50); //For stability
}

// --- HELPER FUNCTIONS ---

//Helpter function to update the brightness smoothly when the user moves their hand back and forth
void updateBrightnessSmoothly() {
  unsigned long currentMillis = millis();
  unsigned long dt = currentMillis - lastBrightUpdate;
  lastBrightUpdate = currentMillis;

  // Calculate max step size based on elapsed time
  // Rate: 255 units / 2000ms
  float step = (255.0 / BRIGHTNESS_FADE_DURATION) * dt;

  if (currentBrightnessPrecise < targetBrightness) {
    currentBrightnessPrecise += step;
    if (currentBrightnessPrecise > targetBrightness) currentBrightnessPrecise = targetBrightness;
  } 
  else if (currentBrightnessPrecise > targetBrightness) {
    currentBrightnessPrecise -= step;
    if (currentBrightnessPrecise < targetBrightness) currentBrightnessPrecise = targetBrightness;
  }

  globalBrightness = (uint8_t)currentBrightnessPrecise;
}

//Helper function to read the current distance detected by the ultrasonic sensor
long getDistance(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout to prevent lag
  
  if (duration == 0) return 999; // If no reading, assume nothing is in front of the sensor so just return a big number that will never trigger the conditional in the loop
  return duration * 0.034 / 2;
}

// Helper function to switch the mode, and the current time associated with it, then setup the fading process
void setDemoMode(String mode, int hour, RGBColor color) {
  currentMode = mode;
  simHour = hour;
  setupFade(color);
}

//Changes the current color to an argument "color" and the mode to an argument "mode", then sets up the fade by assigning the fadeStartTime state variable
void setupFade(RGBColor color){
  startColor = currentColor;  
  endColor = color;

  //Makes the fade start time the current number of milliseconds that have passed since the program started running.
  fadeStartTime = millis();
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
  // Apply brightness scaling
  float scale = (float) globalBrightness / 255.0;
  
  uint8_t r = (uint8_t)(currentColor.r * scale);
  uint8_t g = (uint8_t)(currentColor.g * scale);
  uint8_t b = (uint8_t)(currentColor.b * scale);

  for(int i = 0; i < strip.numPixels(); ++i){
    strip.setPixelColor(i, r, g, b);
  }
  strip.show();
}

//Updates the LCD accordingly by printing the current time and mode and 2 separate lines
void updateLCD(String mode, int hour) {
  //Line 1: Print the time and brightness (denoted by B: )
  lcd.setCursor(0, 0); 
  lcd.print("Sim Time: ");
  if (hour < 10) lcd.print("0");
  lcd.print(hour);
  lcd.print(":00  "); 

  // lcd.print("B:");
  // lcd.print(map(brightness, 0, 255, 0, 100)); // Display % brightness
  // lcd.print("%  ");

  int screenWidth = 16;
  int textWidth = mode.length() + 6; 
  int startColumn = (screenWidth - textWidth) / 2;

  //Line 2: Print the mode
  lcd.setCursor(0, 1);
  lcd.print("                "); //Clear anything on the line before printing
  lcd.setCursor(startColumn, 1); 
  lcd.print("MODE: ");
  lcd.print(mode);
}
