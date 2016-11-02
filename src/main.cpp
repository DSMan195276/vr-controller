#include <stdio.h>
#include <ctype.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include "usb_serial.h"

#include "vr_con.h"

static int led_count;

#define CMD_PROMPT "> "

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))

void flash_led(void)
{
    digitalWrite(LEDPIN, 1);
    led_count = 0;
}

void flash_led_handle(void)
{
    if (led_count >= 0)
        led_count++;

    if (led_count == 10) {
        digitalWrite(LEDPIN, 0);
        led_count = -1;
    }
}

void main_cmd_handle(char *cmd)
{
    if (strncmp(cmd, "hc-", 3) == 0) {
        if (strncmp(cmd + 3, "at", 2) == 0) {

            Serial.write("HC-05: Starting AT mode\n");
            hc05_start_at_mode();
        } else if (strncmp(cmd + 3, "connected", 9) == 0) {
            serial_printf("HC-05: %s\n", (hc05_is_connected())? "Connected": "Not connected");
        }
    } else if (strncmp(cmd, "help", 4) == 0) {
        Serial.write("VR Controller - Teensy LC\n"
                     "Command list:\n"
                     "\n"
                     "hc-at: Enter HC-05 AT mode\n"
                     "hc-connected: Report if the HC-05 is connected to a device\n"
                );
    }
}

void setup(void)
{
	pinMode(LEDPIN, OUTPUT);

    pinMode(MPU_6050_INT_PIN, INPUT_PULLDOWN);

    pinMode(HC05_RESET_PIN, OUTPUT);
    pinMode(HC05_KEY_PIN, OUTPUT);
    pinMode(HC05_CONNECTED_PIN, INPUT_PULLDOWN);

    flash_led();

    delay(10);

    long wait_for_serial = millis();

    /* Wait up to 200ms for USB serial to become availiable
     *
     * If it never comes we just continue on, we don't need it for normal
     * functionality */
    while (!Serial && (millis() - wait_for_serial) <= 1000)
        ;

    Serial.print("Teensy starting...\n");

    cmd_prompt = CMD_PROMPT;
    Serial.print(cmd_prompt);
    cmd_handle_callback = main_cmd_handle;
}

void loop(void)
{
    cmd_check_serial();

    flash_led_handle();

    delay(10);
}

