#include <flexpret_stdio.h>
#include <flexpret_lock.h>

#include <ip_uart.h>

#define STDIO_UART_PIN 0
#define STDIO_UART_BAUD 115200
#define STDIO_MAX_DIGITS 32

ip_uart_config_t uart = {.initialized = false};

void print_run() {
    uart.pin = STDIO_UART_PIN;
    uart.baud = STDIO_UART_BAUD;
    uart.buf_size = 8;
    ip_uart_tx_run(&uart);
}

void print_int(int val) {
    while (!uart.initialized) {}

    char buf[32];
    int n_digits=0;
    while (val && n_digits<STDIO_MAX_DIGITS) {
        char digit = '0' + (val % 10);
        buf[n_digits++] = digit;
        val = val/10;
    }

    for (int i=n_digits-1; i>=0; i--) {
        ip_uart_tx_send(&uart, buf[i]);
    }
}


void print_str(const char *str) {
    while (!uart.initialized) {}
    while (*str != '\0') {
        ip_uart_tx_send(&uart, *str);
        str++;
    }
}