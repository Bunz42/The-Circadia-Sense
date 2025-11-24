/*
  LCD Parallel Test Sketch

  This code tests a standard 16-pin 16x2 LCD display.
  It uses the built-in <LiquidCrystal.h> library.

  It will display "MODE: DAYTIME" in the center of the first line.

  Wiring (as defined in the code):
  - LCD RS pin to digital pin 12
  - LCD E pin to digital pin 11
  - LCD D4 pin to digital pin 5
  - LCD D5 pin to digital pin 4
  - LCD D6 pin to digital pin 3
  - LCD D7 pin to digital pin 2

  REMINDER: You MUST wire the V0 pin to a 10k potentiometer
  to control the screen contrast, otherwise you will see
  nothing but a blank screen or black boxes.
*/

// 1. INCLUDE THE LIBRARY
#include <LiquidCrystal.h>

// 2. DEFINE PINS & INITIALIZE
// LiquidCrystal lcd(rs, en, d4, d5, d6, d7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// 3. SETUP (runs once)
void setup() {
  // --- Centering Logic ---
  // Our display is 16 columns wide
  int screenWidth = 16;
  
  // Our text is 13 characters long
  String text = "MODE: DAYTIME";
  int textWidth = text.length(); // This will be 13

  // Calculate the starting column
  // (16 - 13) / 2 = 1.5. As an integer, this is 1.
  // So, we start at column 1 (the second position).
  int startColumn = (screenWidth - textWidth) / 2;

  // --- LCD Setup ---
  // Set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Set the cursor to our calculated start position
  // lcd.setCursor(column, row)
  lcd.setCursor(startColumn, 0); // Column 1, Row 0

  // Print the text
  lcd.print(text);
}

// 4. LOOP (runs forever)
void loop() {
  // We only need to print the text once,
  // so the loop can be empty.
}