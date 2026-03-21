#include "serial.h"
#include "io.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define WIFI_BAUD           115200
#define WIFI_RX_TIMEOUT_MS  3000
#define WIFI_CMD_DELAY_MS   200
#define WIFI_LONG_DELAY_MS  8000

typedef enum {
    WIFI_OK = 0,
    WIFI_ERROR,
    WIFI_TIMEOUT,
    WIFI_BUSY,
    WIFI_NO_RESPONSE
} wifi_result_t;

extern uint32_t millis(void);

static wifi_result_t wifi_wait_response(const char* expected, uint32_t timeout_ms);
static int wifi_read_line(char* buf, int max_len, uint32_t timeout_ms);

static void wifi_write_str(const char* s) {
    while (*s) {
        serial_write(*s++);
    }
}

static wifi_result_t wifi_cmd(const char* cmd) {
    wifi_write_str(cmd);
    wifi_write_str("\r\n");

    delay_ms(WIFI_CMD_DELAY_MS);

    return wifi_wait_response("OK", 1500);
}

static wifi_result_t wifi_wait_response(const char* expected, uint32_t timeout_ms) {
    char line[128];
    uint32_t start = millis();

    while ((millis() - start) < timeout_ms) {

        int len = wifi_read_line(line, sizeof(line), 200);

        if (len <= 0) continue;

        if (strstr(line, expected)) return WIFI_OK;
        if (strstr(line, "ERROR") || strstr(line, "FAIL")) return WIFI_ERROR;
        if (strstr(line, "busy")) return WIFI_BUSY;
    }

    return WIFI_TIMEOUT;
}

static int wifi_read_line(char* buf, int max_len, uint32_t timeout_ms) {
    int pos = 0;
    uint32_t start = millis();

    while ((millis() - start) < timeout_ms) {

        if (!serial_available()) {
            delay_ms(1);
            continue;
        }

        char c = serial_read_char();

        if (c == '\n') {
            if (pos > 0 && buf[pos - 1] == '\r') pos--;
            buf[pos] = '\0';
            return pos;
        }

        if (pos < max_len - 1) {
            buf[pos++] = c;
        }

        start = millis();
    }

    return -1;
}

void wifi_init(void) {
    wifi_cmd("AT");
    wifi_cmd("AT+RST");
    delay_ms(2000);

    wifi_cmd("AT+CWMODE=1");
    wifi_cmd("AT+CIPMUX=0");
}

int wifi_connect(const char* ssid, const char* password) {
    char cmd[128];

    snprintf(cmd, sizeof(cmd),
        "AT+CWJAP=\"%s\",\"%s\"", ssid, password);

    wifi_write_str(cmd);
    wifi_write_str("\r\n");

    wifi_result_t res =
        wifi_wait_response("WIFI GOT IP", WIFI_LONG_DELAY_MS);

    return (res == WIFI_OK) ? 0 : -1;
}

int wifi_tcp_connect(const char* host, const char* port_str) {
    char cmd[128];

    snprintf(cmd, sizeof(cmd),
        "AT+CIPSTART=\"TCP\",\"%s\",%s", host, port_str);

    wifi_write_str(cmd);
    wifi_write_str("\r\n");

    wifi_result_t res =
        wifi_wait_response("CONNECT", WIFI_LONG_DELAY_MS);

    return (res == WIFI_OK) ? 0 : -1;
}

int wifi_send_data(const char* data, int len) {
    if (!data || len <= 0) return -1;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", len);

    wifi_write_str(cmd);

    if (wifi_wait_response(">", 2000) != WIFI_OK)
        return -1;

    for (int i = 0; i < len; i++) {
        serial_write(data[i]);
    }

    return (wifi_wait_response("SEND OK", 5000) == WIFI_OK) ? 0 : -1;
}

void wifi_close(void) {
    wifi_cmd("AT+CIPCLOSE");
}

void wifi_get_ip(void) {
    wifi_cmd("AT+CIFSR");
}
