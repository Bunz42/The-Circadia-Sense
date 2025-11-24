/*
  LED Strip Test Sketch
  
  This code is designed to test a WS2812B (NeoPixel) LED strip.
  It cycles through solid colors (Red, Green, Blue) and then
  runs a "theater chase" animation.

  This helps you verify:
  1. Your external 5V power supply is working.
  2. Your wiring (Power, Ground, Data) is correct.
  3. The Arduino is successfully sending data to the strip.

  REMINDER: You MUST use an external 5V power supply for the LED
  strip and connect its Ground to the Arduino's Ground.

  Required Library:
  1. "Adafruit NeoPixel" (by Adafruit)
*/

// 1. INCLUDE LIBRARY
#include <Adafruit_NeoPixel.h>

// 2. DEFINE CONSTANTS & PINS
#define LED_PIN 6     // Data-in pin (same as Project Horizon)
#define LED_COUNT 60  // How many LEDs are in your strip?

// 3. INITIALIZE OBJECT
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// 4. SETUP FUNCTION (runs once)
void setup() {
  Serial.begin(9600);
  Serial.println("LED Strip Test Starting...");

  strip.begin();           // INITIALIZE NeoPixel strip
  strip.setBrightness(150); // Set brightness (0-255). 150 is bright but safe.
  strip.show();            // Turn all pixels off
}

// 5. MAIN LOOP (runs forever)
void loop() {
  Serial.println("Setting color RED");
  colorWipe(strip.Color(255, 0, 0), 50); // Red
  delay(1000);

  Serial.println("Setting color GREEN");
  colorWipe(strip.Color(0, 255, 0), 50); // Green
  delay(1000);

  Serial.println("Setting color BLUE");
  colorWipe(strip.Color(0, 0, 255), 50); // Blue
  delay(1000);

  Serial.println("Running Theater Chase");
  theaterChase(strip.Color(255, 255, 255), 50); // White
  delay(1000);
}


// --- HELPER FUNCTIONS ---

/*
  colorWipe()
  Fills the strip one pixel at a time with a color.
*/
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

/*
  theaterChase()
  Lights up every third pixel in a repeating chase pattern.
*/
void theaterChase(uint32_t color, int wait) {
  for (int j = 0; j < 10; j++) { // Do 10 cycles of the chase
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, color); // Turn pixel on
      }
      strip.show();

      delay(wait);

      for (int i = 0; i < strip.numPixels(); i = i + 3) {
        strip.setPixelColor(i + q, 0); // Turn pixel off
      }
    }
  }
}