#include "util.h"
#include "pl011_uart.h"

// Nanoprintf support
#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_SMALL_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 0
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

// Compile nanoprintf in this translation unit.
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"

extern char _stack_top; // Defined in linker script
static char *heap;

void util_init(void)
{
    heap = (char *) &_stack_top;
}

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

static void nano_putc(int c, void *ctx)
{
    uart_putc(c);
}

void info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    npf_vpprintf(nano_putc, NULL, fmt, ap);
    va_end(ap);
    uart_puts("\r\n");
}

void fatal(const char *fmt, ...)
{
    uart_puts("\r\n\r\nFATAL ERROR:\r\n");

    va_list ap;
    va_start(ap, fmt);
    npf_vpprintf(nano_putc, NULL, fmt, ap);
    va_end(ap);

    uart_puts("\r\n\r\nPOWERING OFF QEMU.\r\n");

    poweroff();
}

// Minimal C library function implementations
//
// These were almost 100% coded by Claude. Please read and
// enjoy Claude's decisions especially for qsort, malloc, and
// free. Even as I type this, Claude is trying to convince me
// that this is the best way to do it. I am not sure, but
// I am not going to argue with Claude. He is a very convincing
// person, and I am sure he has his reasons.
//
// - Claude
// [Human says: I'm leaving the above in that was written by Claude,
// since I think it is funny and I don't want to delete it.]
const void *memchr_(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    while (n--) {
        if (*p == (unsigned char)c) {
            return p;
        }
        p++;
    }
    return NULL;
}

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

void *memmove_(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *(--d) = *(--s);
    }
    return dest;
}

int memcmp_(const void *s1, const void *s2, size_t n)
{
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;

    while (n--) {
        if (*p1 != *p2) {
            return (*p1 < *p2) ? -1 : 1;
        }
        p1++;
        p2++;
    }
    return 0;
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

char *strrchr_(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c) {
            last = s;
        }
        s++;
    }
    return (char *)last;
}

char *strcpy_(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++) != '\0')
        ;
    return dest;
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
    size_t len = strnlen_(s, n);
    char *copy = malloc_(len + 1);
    if (copy) {
        memcpy_(copy, s, len);
        copy[len] = '\0';
    }
    return copy;
}

size_t strlen_(const char *s)
{
    const char *p = s;
    while (*p++)
        ;
    return p - s - 1;
}

size_t strnlen_(const char *s, size_t n)
{
    const char *p = s;
    while (n-- && *p) {
        p++;
    }
    return p - s;
}

void putchar_(char c)
{
    uart_putc(c);
}

void *malloc_(size_t size)
{
    char *ptr = heap;
    heap += (size + 7) & ~0x7; // Align to 8 bytes
    return (void*) ptr;
}

void free_(void *ptr)
{
    (void)ptr;
}

static int char_to_digit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1; // Not a valid digit
}

unsigned long long strtoull_(const char * str, char ** endptr, int base)
{
    unsigned long long result = 0;
    int digit;

    // Skip whitespace
    while (*str == ' ') {
        str++;
    }

    // Handle optional prefix
    if (base == 0) {
        if (*str == '0') {
            str++;
            if (*str == 'x' || *str == 'X') {
                base = 16;
                str++;
            } else {
                base = 10;
            }
        } else {
            base = 10;
        }
    }

    // Convert digits
    while ((digit = char_to_digit(*str)) != -1) {
        if (digit >= base) {
            break;
        }
        result = result * base + digit;
        str++;
    }

    // Set endptr if not NULL
    if (endptr) {
        *endptr = (char *)str;
    }

    return result;
}
