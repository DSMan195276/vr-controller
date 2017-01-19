#include <stdio.h>
#include <ctype.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include "usb_serial.h"

#include "vr_con.h"
#include "mpu6050_2.h"

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

void vr_con_run(void)
{
    char buf[200];
    struct MPU6050_accelgyro accelgyro;

    MPU6050_init();

    serial_printf("MPU6050 started\n");

    while (1) {
        MPU6050_read_accelgyro(&accelgyro);
        snprintf(buf, sizeof(buf), "%06d %06d %06d %06d %06d %06d %d\r\n",
                accelgyro.x_accel, accelgyro.y_accel, accelgyro.z_accel,
                accelgyro.x_gyro, accelgyro.y_gyro, accelgyro.z_gyro, millis());
        hc05_serial.write(buf);
        serial_printf("%s", buf);
        delay(1);
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
                     "vr-con-start: Start VR Controller\n"
                );
    } else if (strncmp(cmd, "vr-con-start", 12) == 0) {
        serial_printf("Starting VR Controller\n");
        vr_run();
    }
}

void setup(void)
{
	pinMode(LEDPIN, OUTPUT);

    pinMode(MPU6050_INT_PIN, INPUT_PULLDOWN);

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

    hc05_serial.begin(115200);

    vr_run();
}

void loop(void)
{
    cmd_check_serial();

    flash_led_handle();

    delay(10);
}

