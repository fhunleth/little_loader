/*
 * SPDX-FileCopyrightText: 2025 Frank Hunleth
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

ENTRY(_start)

PHDRS {
  text PT_LOAD FLAGS(5);   /* R+X */
  data PT_LOAD FLAGS(6);   /* RW */
}

SECTIONS {
  . = 0x40000000;
  .text : { *(.text*) } :text
  .rodata : { *(.rodata*) } :text

  . = ALIGN(16);
  .data : { *(.data*) } :data
  .bss : { *(.bss*) *(COMMON) } :data

  . = ALIGN(16);
  _stack_top = . + 0x1000;
}