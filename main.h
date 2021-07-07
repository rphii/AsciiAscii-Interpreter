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

#ifndef MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <conio.h>  // WINDOWS ONLY (getch())

#define HASH_BITS       10
#define NVARS           256   /* actually it is one less but this makes it easier */
#define TABLE_SIZE      1024

#define STACK_BATCH     256
#define LAYER_BATCH     256
#define STR_BATCH       64

typedef enum
{
    TOKEN_NONE, // keep this the first one
    TOKEN_ADD,
    TOKEN_MEM_SET,
    TOKEN_MEM_SWAP,
    TOKEN_FOR,
    TOKEN_END_FOR,
    TOKEN_ELSE,
    TOKEN_END_ELSE,
    TOKEN_INPUT_CH,
    TOKEN_INPUT_NB,
    TOKEN_OUTPUT_CH,
    TOKEN_OUTPUT_NB,
}
token_t;

typedef struct
{
    int32_t **value;
    uint32_t *key;
    uint32_t used;               // how many times the node gets used
}
hash_node_t;

typedef struct
{
    hash_node_t *node;           // nodes
    uint32_t lookup_size;   // size of the table
    uint32_t mask;
    uint32_t bits;
}
hash_t;

typedef struct lex_s
{
    struct lex_s *next;
    token_t token;
    intptr_t value;
}
lex_t;

typedef struct
{
    uint32_t length;
    uint32_t batches;
    intptr_t *bottom;
}
stack_t;

typedef struct
{
    stack_t var_if;
    stack_t bank_if;
    stack_t value_if;
    uint32_t layer;
    uint32_t allocated;
}
exe_layer_t;

typedef struct exe_s
{
    int32_t **vars_source;
    int32_t **vars_other;
//    stack_t var_if;
    exe_layer_t layer;
    uint32_t bank;
}
exe_t;

typedef struct
{
    uint64_t l; // length of string
    uint64_t n; // bytes of memory
    uintptr_t s;    // the string
}
str_t;

// functions
static bool hash_init(hash_t *table, uint32_t lookup_size);
static bool hash_add(hash_t *table, int32_t key);
static bool hash_get(hash_t *table, int32_t ***value, uint32_t key);
static uint32_t hash_index(uint32_t bits, uint32_t mask, uint32_t value);
static bool lex_do(lex_t **lex, char *str);
static bool exe_init(hash_t *table, exe_t *exe);
static bool execute(lex_t **lex);
static bool stack_init(stack_t *stack, uint32_t batches);
static bool stack_push(stack_t *stack, intptr_t value);
static bool stack_peek(stack_t *stack, intptr_t *value);
static bool stack_pop(stack_t *stack, intptr_t *value);
static bool stack_find(stack_t *stack, intptr_t value, uint32_t *index);
static bool str_append(str_t *str, char *format, ...);                 // append a string
static bool str_append_va(str_t *str, char *format, va_list argp);     // append a string and a corresponding va_list to a string (normally don't call outside)
static bool str_free(str_t *str);                                      // free a string and or clean up
static bool str_set(str_t *str, char *format, ...);                    // set a string to another string

#define MAIN_H
#endif