/*
 * Copyright 2019 Frank Hunleth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTIL_H
#define UTIL_H

#include "printf.h"

#define PROGRAM_NAME "picoboot"

#ifndef PROGRAM_VERSION
#define PROGRAM_VERSION unknown
#endif

#define xstr(s) str(s)
#define str(s) #s
#define PROGRAM_VERSION_STR xstr(PROGRAM_VERSION)

//#define DEBUG 1

// Logging functions
void info(const char *fmt, ...);
void fatal(const char *fmt, ...);

#define ERR_CLEANUP() do { rc = -1; goto cleanup; } while (0)
#define ERR_CLEANUP_MSG(MSG, ...) do { info(MSG, ## __VA_ARGS__); rc = -1; goto cleanup; } while (0)

#define OK_OR_CLEANUP(WORK) do { if ((WORK) < 0) ERR_CLEANUP(); } while (0)
#define OK_OR_CLEANUP_MSG(WORK, MSG, ...) do { if ((WORK) < 0) ERR_CLEANUP_MSG(MSG, ## __VA_ARGS__); } while (0)

#define ERR_RETURN(MSG, ...) do { info(MSG, ## __VA_ARGS__); return -1; } while (0)
#define OK_OR_RETURN(WORK) do { if ((WORK) < 0) return -1; } while (0)
#define OK_OR_RETURN_MSG(WORK, MSG, ...) do { if ((WORK) < 0) ERR_RETURN(MSG, ## __VA_ARGS__); } while (0)

#define OK_OR_FATAL(WORK, MSG, ...) do { if ((WORK) < 0) fatal(MSG, ## __VA_ARGS__); } while (0)
#define OK_OR_WARN(WORK, MSG, ...) do { if ((WORK) < 0) info(MSG, ## __VA_ARGS__); } while (0)

#ifdef DEBUG
#define debug info
#define OK_OR_DEBUG(WORK, MSG, ...) do { if ((WORK) < 0) info(MSG, ## __VA_ARGS__); } while (0)
#define assert(CONDITION) do { if (!(CONDITION)) fatal("assert failed at %s:%d", __FILE__, __LINE__); } while (0)
#else
#define debug(MSG, ...)
#define OK_OR_DEBUG(WORK, MSG, ...) do { (void) (WORK); } while (0)
#define assert(CONDITION)
#endif

// Minimal C library
void *memcpy_(void *dst, const void *src, size_t n);
void *memset_(void *b, int c, size_t len);
void qsort_(void *base, size_t nel, size_t width, int (*compar)(const void *, const void *));
int strcmp_(const char *s1, const char *s2);
char *strdup_(const char *s);
char *strndup_(const char *s, size_t n);
size_t strnlen_s_(const char *s, size_t n);
size_t strlen_(const char *s);
void putchar_(char c);
void *malloc_(size_t size);
void free_(void *ptr);

#endif // UTIL_H
