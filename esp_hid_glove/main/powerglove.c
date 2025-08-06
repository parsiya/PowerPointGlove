#include "esp_log.h"
// #include "esp_hidd_api.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"
#include "esp_bt.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
// #include "esp_gap_bt_api.h"
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/gpio.h"
#include "rom/ets_sys.h" // Needed for ets_delay_us

// #include "powerglove.h"


#define HIGH 1
#define LOW 0

// Pin layout from https://benjamin.computer/posts/2018-10-15-powerglove.html

// 1 - x (wiggle?)
// 2 - GND
// 3 - x (wiggle?)
// 4 - x (wiggle?)
// 5 - +5V
// 6 - GND
// 7 - LATCH
// 8 - CLOCK
// 9 - DATA

// Define pins
// TODO: Wiggle doesn't work for my glove :(

// Mapping of DB9 to ESP32 pins. I've used adjacent pins.
// DB9 Pin 7 -> P25 - Latch
#define LATCH GPIO_NUM_25
// DB9 Pin 8 -> P26 - Clock
#define CLOCK GPIO_NUM_26
// DB9 Pin 9 -> P27 - Data
#define DATA GPIO_NUM_27

// Define controller keys.
const int A_BUTTON      = 0;
const int B_BUTTON      = 1;
const int SELECT_BUTTON = 2;
const int START_BUTTON  = 3;
const int UP_BUTTON     = 4;
const int DOWN_BUTTON   = 5;
const int LEFT_BUTTON   = 6;
const int RIGHT_BUTTON  = 7;

// Makes it easier to print the values for debugging.
const char* buttonLabels[] = {"A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT"};

#define READ_DELAY 20  // ms
#define PULSE_DELAY 12 // microsecond


// Create a delay in miliseconds.
void delayMiliseconds(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS); 
}

// Create a delay in microseconds.
void delayMicroseconds(uint32_t us) {
    ets_delay_us(us);
}

// Clear the n-th bit of a byte using bitwise AND.
void bitClear(uint8_t* byte, uint8_t bit) {
    // Clear the bit using bitwise AND
    *byte &= ~(1 << bit);
}

// Set the n-th bit of a byte. Note to self: INDEX starts from 1.
void bitSet(uint8_t* byte, uint8_t bit) {
    *byte |= 1 << (bit - 1);
}

// Based on
// https://www.allaboutcircuits.com/projects/nes-controller-interface-with-an-arduino-uno/.
uint8_t readController() {
    // Preload a variable with all 1s. This means nothing is pressed.
    // Remember, 1 means not pressed (HIGH signal) and 0 means pressed (LOW signal).
    uint8_t tempData = 255;

    // TODO later: Use interrupts? As in, pulse the LATCH and then use
    // interrupts to see which pin was triggered? Does it work?
    // Read this: https://circuitdigest.com/microcontroller-projects/esp32-interrupt

    // Set everything to LOW to get ready to pulse the LATCH.
    gpio_set_level(LATCH, LOW);
    gpio_set_level(CLOCK, LOW);
    delayMicroseconds(PULSE_DELAY);
    delayMicroseconds(PULSE_DELAY);

    // Pulse the LATCH pin.
    gpio_set_level(LATCH, HIGH); // Set LATCH to HIGH.
    delayMicroseconds(PULSE_DELAY);

    // After latching, we can read the first bit (A_BUTTON).
    if (gpio_get_level(DATA) == LOW)
    {
        bitClear(&tempData, 0);
    }

    // Start reading the rest.
    gpio_set_level(LATCH, LOW);
    delayMicroseconds(PULSE_DELAY);
    // We have to read 7 more bits. So PULSE the clock and read the bits.
    for (int i = 1; i < 8; i ++) {
        gpio_set_level(CLOCK, HIGH);
        delayMicroseconds(PULSE_DELAY);
        // If the data is low, it means the bit is 0 and the button was pressed.
        if (gpio_get_level(DATA) == LOW) {
            bitClear(&tempData, i);
        }
        gpio_set_level(CLOCK, LOW);
        delayMicroseconds(PULSE_DELAY);
    }

    // Return the data.
    return tempData;
}

// Read the bitmask and print the values of the pressed buttons (bits that are 0).
void processButtons(uint8_t bitMask) {
  
    // Skip if input is all zeroes or all ones. We really do not press all keys together.
    if ((bitMask == 0x00) || (bitMask == 0xFF) ) {
        return;
    }

    //   printf("%d - ", bitMask);
    //   printBits(bitMask);
    for(int i=0; i<8; i++) {
        if (!(bitMask & (1 << i))) {
            printf("%s - ", buttonLabels[i]);
        }
    }
    printf("\n");
}

// PowerGlove startup
void powerglove_startup() {
    
    // Setup reading from the Power Glove.

    // Set LATCH to output.
    gpio_set_direction(LATCH, GPIO_MODE_OUTPUT);
    // Set CLOCK to output.
    gpio_set_direction(CLOCK, GPIO_MODE_OUTPUT);
    // TODO: Set flex to input.

    // Set DATA to input.
    gpio_set_direction(DATA, GPIO_MODE_INPUT);

    // Wait for 60 miliseconds.
    delayMiliseconds(60);

    // Clear the pins.
    gpio_set_level(LATCH, HIGH);
    gpio_set_level(CLOCK, HIGH);
}

// Convert powerglove keys to mouse.
void power_glove_to_mouse_task()
{
    const char *TAG = "power_glove_to_mouse_task";
    ESP_LOGI(TAG, "starting power_glove_to_mouse_task");
    for (;;) {

        uint8_t incomingData = 0;
        incomingData = readController();
        processButtons(incomingData);

        // Move the mouse depending on the incoming data.

        // Skip if input is all zeroes or all ones. We really do not press all keys together.
        if ((incomingData == 0x00) || (incomingData == 0xFF) ) {
            delayMiliseconds(DELAY);
            continue;
        }

        // Now we have to go through all bits and do specific things.
        xSemaphoreTake(s_local_param.mouse_mutex, portMAX_DELAY);

        // Bits are 0 if that button is pressed on the Power Glove.
        // const char* buttonLabels[] = {"A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT"};

        // Sample mapping from Power Glove to the mouse.
        // A: Left click
        // B: Right click
        // Directions: Same as the mouse.

        uint8_t buttons = 0;
        s_local_param.x_dir = 0;
        s_local_param.y_dir = 0;

        // A was pressed. Left-click or mouse button 1.
        if (!(incomingData & 0x01)) bitSet(&buttons, 2);

        // B was pressed. Right-click or mouse button 3.
        if (!(incomingData & 0x02)) bitSet(&buttons, 0);


        // if (!(byte & 0x04)) actionC(); // Select
        // if (!(byte & 0x08)) actionD(); // Start

        // Up
        if (!(incomingData & 0x10)) s_local_param.y_dir = -1;
        // Down
        if (!(incomingData & 0x20)) s_local_param.y_dir = 1;
        // Left
        if (!(incomingData & 0x40)) s_local_param.x_dir = -1;
        // Right
        if (!(incomingData & 0x80)) s_local_param.x_dir = 1;
        
        xSemaphoreGive(s_local_param.mouse_mutex);

        // Send the report.
        // void send_mouse_report(uint8_t buttons, char dx, char dy, char wheel)
        send_mouse_report(buttons, s_local_param.x_dir * STEP, s_local_param.y_dir * STEP, 0);

        Problem with button click is that we need to send the button twice
        // to simulate button down and up (e.g., press and release).

        send_mouse_report(buttons, 0, 0, 0);

        // Wait for 50 milliseconds
        delayMiliseconds(50);
    }
}

Mouse functions - start

Reads Power Glove controller input into mouse events.
void power_glove_to_mouse(void *pvParameters)
{
    const char *TAG = "power_glove_to_mouse";
    ESP_LOGI(TAG, "starting power_glove_to_mouse task");
    
    for (;;) {
        uint8_t incomingData = 0;
        // Read input.
        incomingData = readController();
        
        // Skip if input is all zeroes or all ones. We really do not press all keys together.
        if ((incomingData == 0x00) || (incomingData == 0xFF) ) {
            delayMiliseconds(DELAY);
            continue;
        }

        // Move the mouse depending on the incoming data.
        
        // Bits are 0 if that button is pressed on the Power Glove.
        // const char* buttonLabels[] = {"A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT"};

        // Sample mapping from Power Glove to the mouse.
        // A: Left click
        // B: Right click
        // Directions: Same as the mouse.

        uint8_t buttons = 0;
        char dx = 0;
        char dy = 0;

        // A was pressed. Left-click, set the first bit to 1.
        if (!(incomingData & BUTTON_A)) bitSet(&buttons, MOUSE_LEFT);

        // B was pressed. Right-click, set the 2nd bit to 1.
        if (!(incomingData & BUTTON_B)) bitSet(&buttons, MOUSE_RIGHT);

        // Optional: Do stuff with `Start` and `Select`.
        // if (!(incomingData & BUTTON_SELECT)) actionC(); // Select
        // if (!(incomingData & BUTTON_START)) actionD(); // Start

        // Up
        if (!(incomingData & BUTTON_UP)) dy = -1;
        // Down
        if (!(incomingData & BUTTON_DOWN)) dy = 1;
        // Left
        if (!(incomingData & BUTTON_LEFT)) dx = -1;
        // Right
        if (!(incomingData & BUTTON_RIGHT)) dx = 1;
        
        // 4th item is wheel which is zero here.
        send_mouse(buttons, dx*STEP, dy*STEP, 0);

        // Wait for 20 milliseconds
        delayMiliseconds(20);
    }
}

// Mouse functions - end