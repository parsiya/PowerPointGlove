# Note on Shield
The shield is OK. It has some issues.

1. You need to press the button to turn it on.
2. It will shut down if you draw less than 90mA of current.
3. This can be fixed by setting the `Switch` to `Hold`, but that just draws 90mA from the battery to keep the circuit alive. This is an issue if you want to use the battery to run a device for long, but not an issue for me as I can turn it on and off at will.
4. If the system is on and you pull a battery, the system cannot be turned on until you plug in something to start charging the battery for a second or two (at least). This is perhaps the most annoying issue with the shield for me.

I got it from AliExpress, it's the same as this one so the reviews here and the notes work:
https://www.amazon.com/Diymore-Lithium-Battery-Charging-Arduino/dp/B07SZKNST4/

# Note on the code
Seems like some of the incoming bits were different here than on the
benjamin.computer code. Might be due to endianness?

Increased the delay for reading the buttons to 100 miliseconds.

The code in the original does not count for multiple keys pressed at a time.
In the blog post create this table.

The final table, bit numbers starting from zero.

| Button | Binary Code | Reset Bit |
| ------ | ----------- | --------- |
| A      | 11111110    | 0         |
| B      | 11111101    | 1         |
| SELECT | 11111011    | 2         |
| START  | 11110111    | 3         |
| UP     | 11101111    | 4         |
| DOWN   | 11011111    | 5         |
| LEFT   | 10111111    | 6         |
| RIGHT  | 01111111    | 7         |


https://www.youtube.com/watch?v=KAUp1c3_8wg

Program 14 on the power glove makes it a normal controller. By default it
functions like a normal controller unless it is connected to NES with a power
glove game running.

----------

# Arduino Uno R4 WiFi

Mention how you tried to flash the ESP32 to no avail because you need a CP210x
(ot TTL whatever) to program the flash

https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Binary_Lists/esp_at_binaries.html
firmware. But none worked on the R4.

Finally realized you need to do this

https://www.espboards.dev/blog/how-to-program-arduino-uno-r4-mcus/ Search for
`Programming the ESP32-S3 directly`. You will lose some functionality but we
do not care here. we're using the board as a glorified esp32 with 5V pins.

Seems like we need a C2102 to program the ESP32 directly.