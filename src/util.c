#include "util.h"
#include "pl011_uart.h"
#include "printf.h"

static void poweroff(void)
{
    // PSCI system_off function ID: 0x84000008
    asm volatile (
        "mov x0, %0\n"
        "smc #0\n"
        :
        : "r"(0x84000008UL)
        : "x0"
    );

    // Fallback if PSCI doesn't work
    for (;;) { __asm__ volatile ("wfe"); }
}

void info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf_(fmt, ap);
    va_end(ap);
    uart_puts("\r\n");
}

void fatal(const char *fmt, ...)
{
    uart_puts("\r\n\r\nFATAL ERROR:\r\n");

    va_list ap;
    va_start(ap, fmt);
    vprintf_(fmt, ap);
    va_end(ap);

    uart_puts("\r\n\r\nPOWERING OFF QEMU.\r\n");

    poweroff();
}

// Minimal C library function implementations
void *memcpy_(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

void *memset_(void *b, int c, size_t len)
{
    unsigned char *p = b;
    while (len--)
        *p++ = c;

    return b;
}

void qsort_(void *base, size_t nel, size_t width, int (*compar)(const void *, const void *))
{
    // Bubble sort
    char *arr = (char *)base;
    for (size_t i = 0; i < nel - 1; i++) {
        for (size_t j = 0; j < nel - i - 1; j++) {
            if (compar(arr + j * width, arr + (j + 1) * width) > 0) {
                // Swap elements
                char temp[width];
                memcpy_(temp, arr + j * width, width);
                memcpy_(arr + j * width, arr + (j + 1) * width, width);
                memcpy_(arr + (j + 1) * width, temp, width);
            }
        }
    }
}

int strcmp_(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strdup_(const char *s)
{
    size_t len = strlen_(s) + 1;
    char *copy = malloc_(len);
    if (copy) {
        memcpy_(copy, s, len);
    }
    return copy;
}

char *strndup_(const char *s, size_t n)
{
    size_t len = strnlen_s_(s, n);
    char *copy = malloc_(len + 1);
    if (copy) {
        memcpy_(copy, s, len);
        copy[len] = '\0';
    }
    return copy;
}

size_t strnlen_s_(const char *s, size_t n)
{
    const char *p = s;
    while (n-- && *p) {
        p++;
    }
    return p - s;
}

size_t strlen_(const char *s)
{
    const char *p = s;
    while (*p++)
        ;
    return p - s - 1;
}

void putchar_(char c)
{
    uart_putc(c);
}

extern char _stack_top; // Defined in linker script
void *malloc_(size_t size)
{
    static char *heap = NULL;
    if (heap == NULL) {
        heap = (char *) &_stack_top;
    }
    char *ptr = heap;
    heap += size;
    return (void*) ptr;
}

void free_(void *ptr)
{
    (void)ptr;
}