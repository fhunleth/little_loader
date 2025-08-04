/*
 * SPDX-FileCopyrightText: 2025 Frank Hunleth
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PL011_UART_H
#define PL011_UART_H

void uart_putc(char c);
void uart_puts(const char *s);
void uart_init(void);

#endif // PL011_UART_H
