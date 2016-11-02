#ifndef VC_CON_H
#define VC_CON_H

#define serial_printf(fmt, ...) \
    do { \
        char __buffer[160]; \
        size_t __len; \
        __len = snprintf(__buffer, sizeof(__buffer), fmt, ## __VA_ARGS__); \
        Serial.write(__buffer, __len); \
    } while (0)

#define LEDPIN 13

#define HC05_RESET_PIN     2
#define HC05_KEY_PIN       3
#define HC05_CONNECTED_PIN 4

#define MPU_6050_INT_PIN   10

#define hc05_serial Serial1

extern const char *cmd_prompt;
extern void (*cmd_handle_callback) (char *cmd);

void hc05_start_at_mode(void);
int hc05_is_connected(void);
void hc05_reset(void);

/* Process any commands from the USB-Serial */
void cmd_check_serial(void);
char *cmd_get_next(char *cmd);

void flash_led(void);
void flash_led_handle(void); /* Should be called in any infinite loops */

#endif
