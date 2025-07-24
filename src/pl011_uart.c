#include "pl011_uart.h"
#include <stdint.h>

#define UART0_BASE 0x09000000

#define UART_DR         (*(volatile uint32_t *)(UART0_BASE + 0x00))
#define UART_FR         (*(volatile uint32_t *)(UART0_BASE + 0x18))
#define UART_IBRD       (*(volatile uint32_t *)(UART0_BASE + 0x24))
#define UART_FBRD       (*(volatile uint32_t *)(UART0_BASE + 0x28))
#define UART_LCRH       (*(volatile uint32_t *)(UART0_BASE + 0x2C))
#define UART_CR         (*(volatile uint32_t *)(UART0_BASE + 0x30))
#define UART_IMSC       (*(volatile uint32_t *)(UART0_BASE + 0x38))

void uart_init(void)
{
    UART_CR = 0x0;                        // Disable UART
    UART_IBRD = 1;                        // Integer baud rate
    UART_FBRD = 40;                       // Fractional baud rate
    UART_LCRH = (3 << 5);                // 8n1, FIFO disabled
    UART_IMSC = 0;                        // Mask interrupts
    UART_CR = (1 << 9) | (1 << 8) | 1;    // Enable TX, RX, UART
}

void uart_putc(char c)
{
    while (UART_FR & (1 << 5)) {} // wait until TXFF is clear
    UART_DR = c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

