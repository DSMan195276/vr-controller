#include <stdio.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_serial.h"

#include "vr_con.h"

#define AT_PROMPT "$ "

static int in_at_mode = 0;

static void hc05_serial_send_cmd(const char *cmd)
{
    hc05_serial.write(cmd);
    hc05_serial.write('\r');
    hc05_serial.write('\n');
    hc05_serial.flush();
}

/* All responses are terminated with '\r\n' */
static void hc05_serial_display_response(void)
{
    size_t buf_len = 0;
    char input_buf[128];

    while (hc05_serial.available()) {
        while (1) {
            while (hc05_serial.available() == 0)
                ;

            if (hc05_serial.peek() == '\r')
                break;

            input_buf[buf_len] = hc05_serial.read();
            serial_printf("Byte: 0x%02x\n", input_buf[buf_len]);
            if (buf_len < sizeof(input_buf) - 1)
                buf_len++;
        }

        while (hc05_serial.available() == 0)
            ;
        hc05_serial.read(); /* '\r' */

        while (hc05_serial.available() == 0)
            ;
        hc05_serial.read(); /* '\n' */

        input_buf[buf_len] = '\0';

        serial_printf("HC05: %s\n", input_buf);
    }

}

int hc05_is_connected(void)
{
    return digitalRead(HC05_CONNECTED_PIN);
}

void hc05_reset(void)
{
    digitalWrite(HC05_RESET_PIN, 1);
    delay(100);
    digitalWrite(HC05_RESET_PIN, 0);
}

static void hc05_enter_at_mode(void)
{
    in_at_mode = 1;
    //hc05_serial.begin(115200);
    hc05_serial.begin(115200);
    //hc05_serial.begin(38400);
    digitalWrite(HC05_KEY_PIN, in_at_mode);
}

static void hc05_exit_at_mode(void)
{
    in_at_mode = 0;
    digitalWrite(HC05_KEY_PIN, in_at_mode);
}

static void hc05_cmd_handle(char *cmd)
{
    if (strcmp(cmd, "exit") == 0) {
        Serial.write("Exiting AT mode...\n");
        hc05_exit_at_mode();
        return ;
    }

    serial_printf("HC-05: AT CMD: %s\n", cmd);
    hc05_serial_send_cmd(cmd);
}

void hc05_start_at_mode(void)
{
    void (*orig_callback) (char *cmd) = cmd_handle_callback;
    const char *orig_prompt = cmd_prompt;

    hc05_enter_at_mode();
    cmd_handle_callback = hc05_cmd_handle;
    cmd_prompt = AT_PROMPT;

    Serial.write(cmd_prompt);

    while (in_at_mode) {
        cmd_check_serial();
        hc05_serial_display_response();
        flash_led_handle();
        delay(10);
    }

    cmd_handle_callback = orig_callback;
    cmd_prompt = orig_prompt;
}

