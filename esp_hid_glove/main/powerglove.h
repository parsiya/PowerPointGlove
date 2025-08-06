#pragma once


// Button mask.
typedef enum {
    BUTTON_A = 0x01,
    BUTTON_B = 0x02,
    BUTTON_SELECT = 0x04,
    BUTTON_START = 0x08,
    BUTTON_UP = 0x10,
    BUTTON_DOWN = 0x20,
    BUTTON_LEFT = 0x40,
    BUTTON_RIGHT = 0x80
} ButtonMask;

// Define delay
const int DELAY = 300;
// Define mouse delay
const int MOUSE_DELAY = 20;

// Mouse buttons
enum {
    MOUSE_LEFT = 1,
    MOUSE_RIGHT = 2,
    MOUSE_MIDDLE = 4
};

// STEP = Step for moving the mouse. E.g., one click moves by 10.
// Higher = higher mouse sensitivity/speed.
#define STEP 10

// Function signatures
void delayMiliseconds(uint32_t ms);
void delayMicroseconds(uint32_t us);
void bitClear(uint8_t* byte, uint8_t bit);
void bitSet(uint8_t* byte, uint8_t bit);
uint8_t readController();
void processButtons(uint8_t bitMask);
void powerglove_startup();
void power_glove_to_mouse_task(void *pvParameters);
void send_mouse_glove(uint8_t buttons, char dx, char dy, char wheel);
void power_glove_to_mouse(void *pvParameters);