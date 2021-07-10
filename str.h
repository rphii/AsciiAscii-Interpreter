/*

MIT License

Copyright (c) 2021 rphii

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef ASCIIASCII_STR_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define STR_BATCH   64

typedef struct
{
    uint64_t l; // length of string
    uint64_t n; // bytes of memory
    uintptr_t s;    // the string
}
str_t;

bool str_append_va(str_t *str, char *format, va_list argp);
bool str_append(str_t *str, char *format, ...);
bool str_realloc(str_t *str, size_t size);
bool str_free(str_t *str);
bool str_set(str_t *str, char *format, ...);
bool str_to_int32(str_t *str, int32_t *num);
bool str_cmp(str_t *a, str_t *b);

#define ASCIIASCII_STR_H
#endif
