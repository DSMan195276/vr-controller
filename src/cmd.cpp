/*
 * Buffered command input from the Usb-Serial connection
 */
#include <stdio.h>
#include <ctype.h>
#include "avr_emulation.h"
#include "WProgram.h"
#include "usb_desc.h"
#include "usb_dev.h"
#include "usb_serial.h"

#include "vr_con.h"

static size_t cmd_len;
static char cmd_entry[128];

const char *cmd_prompt;
void (*cmd_handle_callback) (char *);


char *cmd_get_next(char *cmd)
{
    while (*cmd && *cmd == ' ')
        cmd++;

    return cmd;
}

void cmd_add_buf(const char *buf, size_t len)
{
    const char *b = buf, *end = buf + len;

    while (b != end) {
        while (cmd_len < sizeof(cmd_entry) - 1 && b != end && *b != '\n' && *b != '\r') {
            Serial.write(*b);

            switch (*b) {
            case '\b':
                cmd_entry[--cmd_len] = '\0';
                break;
            default:
                cmd_entry[cmd_len++] = *b;
                break;
            }
            b++;
        }

        cmd_entry[cmd_len] = '\0';

        if (cmd_len == sizeof(cmd_entry) - 1 || *b == '\n' || *b == '\r') {
            Serial.write('\n');
            flash_led();
            cmd_len = 0;

            (cmd_handle_callback) (cmd_entry);
            Serial.print(cmd_prompt);
        }

        if (b != end && (*b == '\n' || *b == '\r'))
            b++;
    }
}

void cmd_check_serial(void)
{
    int bytes_avail;

    if ((bytes_avail = Serial.available())) {
        char buf[bytes_avail + 1];
        size_t r;

        r = usb_serial_read(buf, bytes_avail);

        cmd_add_buf(buf, r);
    }
}


