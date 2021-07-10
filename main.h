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

#ifndef ASCIIASCII_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "str.h"
#include "io.h"

#define HASH_BITS           10
#define NVARS               256   // actually it is one less but this makes it easier
#define TABLE_SIZE          256
#define LEX_BATCH           16

#define STACK_BATCH         256
#define LAYER_BATCH         256

/** DEBUG_LEVEL
 * 0 : shows no debug messages (DEBUG_LEVEL_0)
 * 1 : shows generated code: original & optimized (DEBUG_LEVEL_2 + DEBUG_LEVEL_1)
 * 2 : shows status messages and warnings (DEBUG_LEVEL_1)
 * 3 : shows as many debug messages as possible (DEBUG_LEVEL_3 + DEBUG_LEVEL_2)
 */
//#define DEBUG_LEVEL     DEBUG_LEVEL_1
#define DEBUG_LEVEL_0   0
#define DEBUG_LEVEL_1   1
#define DEBUG_LEVEL_2   2
#define DEBUG_LEVEL_3   3

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(l, fmt, ...) do { if (DEBUG_LEVEL >= l) printf(("("fmt")"), ##__VA_ARGS__); } while(0)
#else
#error "DEBUG_PRINT already defined"
#endif
#ifndef DEBUG_CODE
#define DEBUG_CODE(l, code) do { if (DEBUG_LEVEL >= l) code; } while(0)
#else
#error "DEBUG_CODE already defined"
#endif

typedef enum
{
    TOKEN_NONE, // keep this the first one
    TOKEN_ADD,
    TOKEN_MEM_SET,
    TOKEN_MEM_SWAP,
    TOKEN_MEM_BOTH,
    TOKEN_INVERT,
    TOKEN_FOR,
    TOKEN_FOR_END,
    TOKEN_IF_ELSE,
    TOKEN_ELSE,
    TOKEN_ELSE_END,
    TOKEN_INPUT_CH,
    TOKEN_INPUT_NB,
    TOKEN_OUTPUT_CH,
    TOKEN_OUTPUT_NB,
    TOKEN_END,
}
token_t;

char *token_s[] = 
{
    "TOKEN_NONE",
    "TOKEN_ADD",
    "TOKEN_MEM_SET",
    "TOKEN_MEM_SWAP",
    "TOKEN_MEM_BOTH",
    "TOKEN_INVERT",
    "TOKEN_FOR",
    "TOKEN_FOR_END",
    "TOKEN_IF_ELSE",
    "TOKEN_ELSE",
    "TOKEN_ELSE_END",
    "TOKEN_INPUT_CH",
    "TOKEN_INPUT_NB",
    "TOKEN_OUTPUT_CH",
    "TOKEN_OUTPUT_NB",
    "TOKEN_END",
};

typedef struct
{
    uint32_t allocated;
    uint32_t batches;
    uint32_t used;
    token_t *token;
    intptr_t *value;
}
lex_t;

typedef struct
{
    int32_t **vars;
    uint32_t *bank;
    uint32_t used;      // how many times the node gets used
}
vars_node_t;

typedef struct
{
    vars_node_t *node;      // nodes
    uint32_t lookup_size;   // size of the table
    uint32_t mask;
    uint32_t bits;
}
vars_t;

typedef struct
{
    int32_t *value[NVARS];
    int32_t initial[NVARS];
    bool used[NVARS];
}
layers_t;

typedef struct
{
    uint32_t length;
    uint32_t batches;
    intptr_t *bottom;
}
stack_t;

typedef struct exe_s
{
    uint32_t for_positions[NVARS];
    int32_t *vars_source;
    int32_t *vars_other;
    vars_t bank_both;
    layers_t layers;
    uint32_t bank;
}
exe_t;

// functions
static uint32_t hash_index(uint32_t bits, uint32_t mask, uint32_t value);
static bool vars_init(vars_t *table, uint32_t lookup_size);
static bool lex_init(lex_t *list, uint32_t size);
static bool lex_add(lex_t *list, token_t token, uint32_t value);
static bool lex_free(lex_t *list);
static bool lex_optimize(lex_t *list);
static bool lex_swap(lex_t *a, lex_t *b);
static bool lex_clean(lex_t *list);
static void lex_print(lex_t *lex);
static void init_vars(int32_t *vars, uint32_t n);
static bool vars_add(vars_t *table, int32_t key);
static bool vars_get(vars_t *table, int32_t **value, int32_t bank);
static int lex_do(lex_t *lex, char *str);
static bool var_get(exe_t *exe, int32_t *vars, uint32_t var, int32_t **value);
static bool exe_init(exe_t *exe);
static int execute(lex_t *lex);
static void list_flags();

#define ASCIIASCII_MAIN_H
#endif
