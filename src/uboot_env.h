/*
 * SPDX-FileCopyrightText: 2017 Frank Hunleth
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UBOOT_ENV_H
#define UBOOT_ENV_H

#include <stdint.h>
#include <stddef.h>

struct uboot_name_value {
    char *name;
    char *value;
    struct uboot_name_value *next;
};

struct uboot_env {
    size_t env_size;

    struct uboot_name_value *vars;
};

void uboot_env_init(struct uboot_env *env, size_t len);
int uboot_env_read(struct uboot_env *env, const char *buffer);
int uboot_env_setenv(struct uboot_env *env, const char *name, const char *value);
int uboot_env_unsetenv(struct uboot_env *env, const char *name);
int uboot_env_getenv(struct uboot_env *env, const char *name, char **value);
int uboot_env_write(struct uboot_env *env, char *buffer);
void uboot_env_free(struct uboot_env *env);

#endif // UBOOT_ENV_H
