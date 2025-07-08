#include <BleKeyboard.h>

BleKeyboard bleKeyboard;
// ZZ change the name.

// Pin layout from https://benjamin.computer/posts/2018-10-15-powerglove.html

// Define pins
// DB9 Pin 7 -> P25 - Latch
const int LATCH = 25;
// DB9 Pin 8 -> P26 - Clock
const int CLOCK = 26;
// DB9 Pin 9 -> P27 - Data
const int DATA = 27;

// Define controller keys.
const int A_BUTTON = 0;
const int B_BUTTON = 1;
const int SELECT_BUTTON = 2;
const int START_BUTTON = 3;
const int UP_BUTTON = 4;
const int DOWN_BUTTON = 5;
const int LEFT_BUTTON = 6;
const int RIGHT_BUTTON = 7;

// Define delay
const int DELAY = 300;

// Button mask.
typedef enum {
  BUTTON_A      = 0x01,
  BUTTON_B      = 0x02,
  BUTTON_SELECT = 0x04,
  BUTTON_START  = 0x08,
  BUTTON_UP     = 0x10,
  BUTTON_DOWN   = 0x20,
  BUTTON_LEFT   = 0x40,
  BUTTON_RIGHT  = 0x80
} ButtonMask;

typedef struct {
  uint8_t buttonMask;
  uint8_t keyboardEvent;
} ButtonMapping;

// MediaKeyReport definition (assuming this comes from BleKeyboard.h or another included header)
typedef uint8_t MediaKeyReport[2];

// MediaButtonMapping struct: keyboardEvent is of type MediaKeyReport
typedef struct {
  uint8_t buttonMask;
  MediaKeyReport keyboardEvent;
} MediaButtonMapping;

// Change key mappings here.

// Right: KEY_RIGHT_ARROW
// Left: KEY_LEFT_ARROW
// Up: KEY_UP_ARROW
// Down: KEY_DOWN_ARROW
ButtonMapping arrowMappings[] = {
  {BUTTON_RIGHT,    KEY_RIGHT_ARROW},
  {BUTTON_LEFT,     KEY_LEFT_ARROW},
  {BUTTON_UP,       KEY_UP_ARROW},
  {BUTTON_DOWN,     KEY_DOWN_ARROW},
};

// Change key mappings here.
// Note media keys are mapped to a two byte array.
MediaButtonMapping mediaMappings[] = {
  {BUTTON_A,        {0, 8}}, // KEY_MEDIA_WWW_SEARCH
  {BUTTON_B,        {0, 1}}, // KEY_MEDIA_LOCAL_MACHINE_BROWSER - opens my computer on Windows
  {BUTTON_SELECT,   {0, 2}}, // KEY_MEDIA_CALCULATOR
  {BUTTON_START,    {0, 4}}, // CKEY_MEDIA_WWW_BOOKMARKS == doesn't work on Windows :(
};


// Delay functions
// Arduino has built-in delay functions
// delay(ms) for milliseconds
// delayMicroseconds(us) for microseconds

// Bit manipulation functions
// Arduino has built-in bit manipulation functions
// These are bitRead(), bitWrite(), bitSet(), bitClear(), and bit()

// Example usage:
// uint8_t byte = 0;
// bitSet(byte, 3);    // Sets the 3rd bit of byte
// bitClear(byte, 3);  // Clears the 3rd bit of byte

uint8_t readController() {
  // Preload a variable with all 1s. This means nothing is pressed.
  uint8_t tempData = 255;

  // Pulse the LATCH pin.
  digitalWrite(LATCH, LOW);
  digitalWrite(CLOCK, LOW);
  delayMicroseconds(12);

  digitalWrite(LATCH, HIGH); // Set LATCH to HIGH.
  delayMicroseconds(12);

  // After latching, we can read the first bit (A_BUTTON).
  if (digitalRead(DATA) == LOW)
  {
    bitClear(tempData, 0);
  }

  // Start reading the rest.
  digitalWrite(LATCH, LOW);
  delayMicroseconds(6);
  // We have to read 7 more bits. So PULSE the clock and read the bits.
  for (int i = 1; i < 8; i ++) {
    digitalWrite(CLOCK, HIGH);
    delayMicroseconds(6);
    // If the data is low, it means the bit is 0 and the button was pressed.
    if (digitalRead(DATA) == LOW) {
      bitClear(tempData, i);
    }
    digitalWrite(CLOCK, LOW);
    delayMicroseconds(6);
  }
  // Return the data.
  return tempData;
}

// Makes it easier to print the values.
const char* buttonLabels[] = {"A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT"};

// Read the bitmask and print the values of the pressed buttons (bits that are 0).
void processButtons(uint8_t bitMask) {

  // Skip if input is all zeroes or all ones. We really do not press all keys together.
  if ((bitMask == 0x00) || (bitMask == 0xFF) ) {
    return;
  }

  for(int i=0; i<8; i++) {
    ButtonMask mask = (ButtonMask)(1 << i);
    if (!(bitMask & mask)) {
      Serial.print(buttonLabels[i]);
      Serial.print(" - ");
    }
  }
  Serial.println();
}


void setup() {
  Serial.begin(115200);

  Serial.println("Setting up reading from the Power Glove.");

  // Setup reading from the Power Glove.

  // Set LATCH to output.
  pinMode(LATCH, OUTPUT);
  // Set CLOCK to output.
  pinMode(CLOCK, OUTPUT);

  // Set DATA to input.
  pinMode(DATA, INPUT);

  // Wait 60 milliseconds.
  delay(60);

  // Clear the pins.
  digitalWrite(LATCH, HIGH);
  digitalWrite(CLOCK, HIGH);

  Serial.println("Starting BLE work!");
  bleKeyboard.begin();
}

void loop() {

    if(bleKeyboard.isConnected()) {

    uint8_t incomingData = readController();

    if ((incomingData == 0x00) || (incomingData == 0xFF) ) {
      delay(DELAY);
      return;
    }

    processButtons(incomingData);

    // Check for arrow keys.
    for (int i = 0; i < sizeof(arrowMappings) / sizeof(arrowMappings[0]); i++) {
      if (!(incomingData & arrowMappings[i].buttonMask)) {
        bleKeyboard.write(arrowMappings[i].keyboardEvent);
      }
    }

    // Check the button mappings.
    for (int i = 0; i < sizeof(mediaMappings) / sizeof(mediaMappings[0]); i++) {
      if (!(incomingData & mediaMappings[i].buttonMask)) {
        bleKeyboard.write(mediaMappings[i].keyboardEvent);
      }
    }

    delay(300);
  }
}