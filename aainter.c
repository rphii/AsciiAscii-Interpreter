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

// to-do 

#ifndef MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
//#include <conio.h>  // WINDOWS ONLY (getch())

#define HASH_BITS       10
#define NVARS           256   /* actually it is one less but this makes it easier */
#define TABLE_SIZE      1024
#define TOKEN_LIST_BATCH	16

#define STACK_BATCH     256
#define LAYER_BATCH     256
#define STR_BATCH       64

typedef enum
{
    TOKEN_NONE, // keep this the first one
    TOKEN_ADD,			// add two numbers and put the result in the first one
    TOKEN_MEM_SET,		// set memory bank
    TOKEN_MEM_SWAP,		// swap memory banks
    TOKEN_FOR,			// loop begin and eventually invert
    TOKEN_FOR_END,		// loop end (jump to begin)
	TOKEN_IF_ELSE,		// to optimize empty loops away
    TOKEN_ELSE,			// else begin
    TOKEN_ELSE_END,		// else end
	TOKEN_INVERT,		// only invert (eg. AAAAAA)
    TOKEN_INPUT_CH,
    TOKEN_INPUT_NB,
    TOKEN_OUTPUT_CH,
    TOKEN_OUTPUT_NB,
}
token_t;

char *token_s[] = 
{
	"TOKEN_NONE",
    "TOKEN_ADD",
    "TOKEN_MEM_SET",
    "TOKEN_MEM_SWAP",
    "TOKEN_FOR",
    "TOKEN_FOR_END",
    "TOKEN_IF_ELSE",
    "TOKEN_ELSE",
    "TOKEN_ELSE_END",
	"TOKEN_INVERT",
    "TOKEN_INPUT_CH",
    "TOKEN_INPUT_NB",
    "TOKEN_OUTPUT_CH",
    "TOKEN_OUTPUT_NB",
};

typedef struct
{
    int32_t **value;
    uint32_t *key;
    uint32_t used;               // how many times the node gets used
}
vars_node_t;

typedef struct
{
    vars_node_t *node;           // nodes
    uint32_t lookup_size;   // size of the table
    uint32_t mask;
    uint32_t bits;
}
vars_t;

typedef struct
{
	token_t token;
	intptr_t value;
}
node_t;

typedef struct
{
	uint32_t allocated;
	uint32_t batches;
	uint32_t used;
	node_t *nodes;
}
token_list_t;

/*
typedef struct
{
    int32_t *value;
    uint32_t *key;
    uint32_t used;               // how many times the node gets used
}
layers_node_t;

typedef struct
{
    layers_node_t *node;           // nodes
    uint32_t lookup_size;   // size of the table
    uint32_t mask;
    uint32_t bits;
}
layers_t;
*/
typedef struct
{
	int32_t bank[NVARS];
	bool used[NVARS];
	int32_t initial[NVARS];
}
layers_t;

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
	layers_t layers;
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
static uint32_t hash_index(uint32_t bits, uint32_t mask, uint32_t value);
static bool vars_init(vars_t *table, uint32_t lookup_size);
static bool vars_add(vars_t *table, int32_t key);
static bool vars_get(vars_t *table, int32_t ***value, int32_t key);
//static bool layers_add(layers_t *layers, uint32_t var, uint32_t key);
//static bool layers_remove(layers_t *layers, uint32_t key);
//static bool layers_get(layers_t *layers, int32_t *value, uint32_t key);	// new
static int lex_do(lex_t **lex, char *str);
static bool exe_init(vars_t *table, exe_t *exe);
static int execute(lex_t **lex);
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

#define __s(x) ((char *)(x)->s)

bool tokens_init(token_list_t *list, uint32_t size)
{
	if(!list) return false;
	list->nodes = 0;
	list->used = 0;
	list->batches = size;
	list->allocated = 0;
	return true;
}

bool tokens_add(token_list_t *list, token_t token, uint32_t value)
{
	if(!list) return false;
	uint32_t required = (list->used / list->batches + 1) * list->batches;
	if(required != list->allocated)
	{
		list->nodes = realloc(list->nodes, sizeof(node_t) * required);
		if(!list->nodes)
		{
			return false;
		}
	}
	list->nodes[list->used].token = token;
	list->nodes[list->used].value = value;
	list->used++;
	return true;
}

bool tokens_free(token_list_t *list)
{
	if(!list) return false;
	free(list->nodes);
	list->used = 0;
	list->allocated = 0;
	list->batches = 0;
	return true;
}

bool tokens_optimize(token_list_t *list)
{
	if(!list) return false;
	for(int32_t i = 0; i < list->used; i++)
	{
		token_t *token = &list->nodes[i].token;
		intptr_t *value = &list->nodes[i].value;
		// look ahead three values
		if(i < list->used - 3)
		{
			// we actually just wants to invert the number?
			if(token[0] == TOKEN_FOR && token[1] == TOKEN_FOR_END && \
			   token[2] == TOKEN_ELSE && token[3] == TOKEN_ELSE_END)
			{
			   if(value[0] == value[1]&& value[1] == value[2] && value[2] == value[3])
			   {
				   token[0] = TOKEN_INVERT;
				   token[1] = TOKEN_NONE;
				   token[2] = TOKEN_NONE;
				   token[3] = TOKEN_NONE;
			   }
			}
		}
		// look ahead two values
		if(i < list->used - 2)
		{
			// the loop is empty?
			if(token[0] == TOKEN_FOR && token[1] == TOKEN_FOR_END && token[2] == TOKEN_ELSE)
			{
				if(value[0] == value[1] && value[1] == value[2])
				{
					token[0] = TOKEN_IF_ELSE;
					token[1] = TOKEN_NONE;
					token[2] = TOKEN_NONE;
				}
			}
			// the else is empty?)
			if(token[0] == TOKEN_ELSE && token[1] == TOKEN_ELSE_END)
			{
				if(value[0] == value[1] && value[1] == value[2])
				{
					token[1] = TOKEN_NONE;
					token[2] = TOKEN_NONE;
				}
			}
		}
	}
}

bool vars_init(vars_t *table, uint32_t lookup_size)
{
    if(!table) return false;
    // allocate memory
    table->node = malloc(sizeof(vars_node_t) * lookup_size);
    if(!table->node) return false;
    // initialize with 0
    for(uint32_t i = 0; i < lookup_size; i++)
    {
        table->node[i].value = 0;
        table->node[i].key = 0;
        table->node[i].used = 0;
    }
    // initialize table
    table->lookup_size = lookup_size;
    table->mask = lookup_size;
    table->bits = HASH_BITS;
    return true;
}

static inline uint32_t hash_index(uint32_t bits, uint32_t mask, uint32_t value)
{
    return ((value * UINT32_C(0x61c88647)) >> (32 - bits)) % mask;
}

static inline void init_vars(int32_t *vars, uint32_t n)
{
    for(uint32_t i = 0; i < n; i++)
    {
        if((char)i <= '9' && (char)i >= '0') vars[i] = i - '0';
        else vars[i] = i;
    }
}

static inline bool vars_add(vars_t *table, int32_t key)
{
    if(!table) return false;
    // get a hash index
    uint32_t index = hash_index(table->bits, table->mask, key);
    // check if the key already exists
    vars_node_t *node = &table->node[index];
    uint32_t used = node->used;
    bool add = true;
    for(uint32_t i = 0; i < used; i++)
    {
        if(node->key[i] == key)
        {
              add = false;
              break;
        }
    }
    if(!add) return true;
    // allocate new memory
	printf("(add new vars hash table entry)");
    node->key = realloc(node->key, sizeof(uint32_t) * (used + 1));
    if(!node->key) return false;
    node->value = realloc(node->value, sizeof(int32_t *) * (used + 1));
    //printf("node->value : %x\n", node->value);
    if(!node->value) return false;
    node->value[used] = malloc(sizeof(int32_t) * NVARS);
    if(!node->value[used]) return false;
    // allocation successful
    init_vars(node->value[used], NVARS);
    node->key[used] = key;
    node->used++;     // successfully allocated memory
    return true;
}

static inline bool vars_get(vars_t *table, int32_t ***value, int32_t key)
{
    if(!table || !value) return false;
	printf("(get values for key %d)", key);
    // get a hash index
    uint32_t index = hash_index(table->bits, table->mask, key);
    // check if the key exists
    vars_node_t *node = &table->node[index];
    uint32_t used = node->used;
    //printf("%d used\n", used);
    uint32_t pos = 0;
    bool exists = false;
    for(uint32_t i = 0; i < used; i++)
    {
        if(node->key[i] == key)
        {
            pos = i;
            exists = true;
            break;
        }
    }
    // add the key if it doesn't exist
    if(!exists)
    {
        //printf("add...\n");
		printf("(add vars for key %d to table)", key);
        if(!vars_add(table, key)) return false;
        pos = node->used - 1;
        //printf("pos: %d\n", pos);
    }
    *value = &node->value[pos];
	printf("(vars for key %d found)", key);
    //printf("%d: %x\n", key, value);
    return true;
}
static inline bool lex_add(lex_t **lex, token_t token, intptr_t value)
{
    if(!lex) return false;
    // go to end
    while(*lex && (*lex)->next) *lex = (*lex)->next;
    if(!*lex)
    {
        // first item
        *lex = malloc(sizeof(lex_t));
    }
    else
    {
        // next item
        (*lex)->next = malloc(sizeof(lex_t));
        *lex = (*lex)->next;
    }
    if(!*lex) return false;
    // allocation successful
    (*lex)->token = TOKEN_NONE;
    (*lex)->next = 0;
    (*lex)->value = 0;
    // add the token (and possibly value)
    switch(token)
    {
        case TOKEN_ADD:
        {
            (*lex)->value = (intptr_t)malloc(sizeof(int32_t) * 2);
            if(!(*lex)->value) return false;
            ((int32_t *)(*lex)->value)[0] = ((int32_t *)value)[0];
            ((int32_t *)(*lex)->value)[1] = ((int32_t *)value)[1];
        } break;
        case TOKEN_INPUT_NB:
        case TOKEN_INPUT_CH:
        case TOKEN_OUTPUT_CH:
        case TOKEN_OUTPUT_NB:
        case TOKEN_MEM_SET:
        case TOKEN_MEM_SWAP:
        case TOKEN_ELSE:
        case TOKEN_FOR_END:
        case TOKEN_ELSE_END:
        case TOKEN_FOR:
		case TOKEN_INVERT:
		case TOKEN_IF_ELSE:
        {
            (*lex)->value = value;
        } break;
        default: return false;
    }
    (*lex)->token = token;
    return true;
}

static inline int lex_do(lex_t **lex, char *str)
{
    if(!lex || !str) return __LINE__;
    size_t len = strlen(str);    // length of string
    if(!len) return __LINE__;
    if(len % 2)
    {
        printf("[Note] String is not ending on a pair of characters, ignoring the last one.\n");
    }
    uint32_t last_var = 0;
    //intptr_t this_var = 0;
	intptr_t vars[NVARS * 4] = {0};		// [0] = newest, [n] = oldest
	token_t tokens[NVARS * 4] = {0};	// [0] = newest, [n] = oldest
	int tokens_queued = 0;
	// token_t last_token = TOKEN_NONE;	// have a seperat variable for last token
    int32_t *tuple = malloc(sizeof(int32_t) * 2);
    if(!tuple) return __LINE__;
    lex_t *base = 0;
    uint8_t leai[NVARS] = {0}; // loop, else and invert
    for(uint32_t i = 0; i < len - 1; i += 2)
    {
        uint32_t j = i + 1;
		// move tokens
		for(int q = tokens_queued, r = tokens_queued - 1; q > 0; q--, r-- )
		{
			vars[q] = vars[r];
			tokens[q] = tokens[r];
			//printf("%d:%s,", q, token_s[tokens[q]]);
		}
		bool skip = false;
        // ==== MEMORY ====
        // printf("%c == %c\n", str[i], str[j]);
        if(str[i] == '`' && str[j] == '`')
        {
            if(tokens[tokens_queued] == TOKEN_NONE) skip = true;
            else if(tokens[tokens_queued] == TOKEN_MEM_SET || tokens[tokens_queued] == TOKEN_MEM_SWAP)
            {
                tokens[0] = TOKEN_MEM_SWAP;
				tokens_queued++;
                printf("mem_swap\n");
            }
            else
            {
                tokens[0] = TOKEN_MEM_SET;
				tokens_queued++;
                printf("mem_set\n");
            }
            //printf("last var: %d\n", last_var);
            vars[0] = last_var;
        }
        // ==== USER INPUT ====
        else if(str[i] == '`')
        {
            vars[0] = str[j];
            if(str[j] >= '0' && str[j] <= '9') tokens[0] = TOKEN_INPUT_NB;
            else tokens[0] = TOKEN_INPUT_CH;
			tokens_queued++;
            last_var = str[j];
            //printf("input\n");
        }
        // ==== OUTPUT ====
        else if(str[j] == '`')
        {
            vars[0] = str[i];
            if(str[i] >= '0' && str[i] <= '9') tokens[0] = TOKEN_OUTPUT_NB;
            else tokens[0] = TOKEN_OUTPUT_CH;
			tokens_queued++;
            last_var = str[i];
            //printf("output\n");
        }
        // === LOOP, ELSE AND INVERT (LEAI) ===
        else if(str[i] == str[j])
        {
            vars[0] = str[i];
            leai[(uint32_t)str[i]]++;
            switch(leai[(uint32_t)str[i]])
            {
                case 1: tokens[0] = TOKEN_FOR;
					skip = true;
                    // printf("for\n");
                    break;
                case 2: tokens[0] = TOKEN_FOR_END;
					/*if(tokens[1] == TOKEN_FOR && vars[1] == vars[0]) */skip = true;
					i -= 2;	// add TOKEN_ELSE afterwards
                    // printf("for_end\n");
                    break;
				case 3: tokens[0] = TOKEN_ELSE;
					/*if(tokens[1] == TOKEN_FOR_END && vars[1] == vars[0]) */skip = true;
                    // printf("else\n");
					break;
                case 4: tokens[0] = TOKEN_ELSE_END;
                    leai[(uint32_t)str[i]] = 0;
                    // printf("else_end\n");
                    break;
                default:
                    return __LINE__;     // this line will technically never be reached
            }
			tokens_queued++;
            last_var = str[i];
        }
        // ==== ADD ====
        else
        {
            tokens[0] = TOKEN_ADD;
			tokens_queued++;
            tuple[0] = str[i];
            tuple[1] = str[j];
            vars[0] = (intptr_t)tuple;
            last_var = str[i];
            //printf("add\n");
        }
		//printf("%d:%s\n", 0, token_s[tokens[0]]);
        // add to the lexer
        if(!skip)
        {
			printf(".");
			// code wants to invert
			if(tokens[0] == TOKEN_ELSE_END && tokens[1] == TOKEN_ELSE && \
			   tokens[2] == TOKEN_FOR_END && tokens[3] == TOKEN_FOR)
			{
				if(vars[0] == vars[1] && vars[1] == vars[2] && vars[2] == vars[3])
				{
					// invert instead of looping without reason
					tokens[0] = TOKEN_INVERT;
					tokens[1] = TOKEN_NONE;
					tokens[2] = TOKEN_NONE;
					tokens[3] = TOKEN_NONE;
				}
			}
			// remove empty else
			if(tokens[0] == TOKEN_ELSE_END && tokens[1] == TOKEN_ELSE)
			{
				if(vars[0] == vars[1])
				{
					tokens[0] = TOKEN_NONE;
					tokens[1] = TOKEN_NONE;
				}
			}
			// remove empty loop
			if(tokens[0] == TOKEN_FOR && tokens[1] == TOKEN_FOR_END && tokens[2] == TOKEN_ELSE)
			{
				if(vars[0] == vars[1])
				{
					tokens[0] = TOKEN_NONE;
					tokens[1] = TOKEN_NONE;
					tokens[2] = TOKEN_IF_ELSE;
				}
			}
			// add all queued tokens
			if(!tokens_queued) return __LINE__;
			for(int q = tokens_queued - 1; q + 1 > 0; q--)
			{
				// add tokens
				if(tokens[q] != TOKEN_NONE)
				{
					if(!lex_add(lex, tokens[q], vars[q])) return __LINE__;
					printf("add %s\n", token_s[tokens[q]]);
					tokens[q] = TOKEN_NONE;
				}
				// maybe get the base of the lexer
				if(!base)
				{
					base = *lex;
				}
			}
			tokens_queued = 0;
        }
    }
    free(tuple);
    // error: check if leai[] is not all 0's
    bool missing_leai = false;
    for(int i = 0; i < NVARS; i++)
    {
        if(leai[i])
        {
            missing_leai = true;
            break;
        }
    }
    // any errors?
    if(missing_leai)
    {
        printf("[Error] There is a missing loop, else or end.\n");
        return __LINE__;
    }
    // success
    *lex = base;
    return 0;
}

bool var_set(vars_t *table, exe_t *exe, int32_t **vars, uint32_t var, int32_t value)
{
    if(!table || !exe || !vars) return false;
    bool stack = true;
    uint32_t var_stack = 0;
    int32_t **vars_stack = 0;
    uint32_t bank_stack = 0;
    printf("(set var '%c' to %d)", var, value);
    uint32_t stack_index = 0;
    if(stack_find(&exe->layer.var_if, var, &stack_index))
    {
        var_stack = var;
        bank_stack = exe->layer.bank_if.bottom[stack_index];
    }
    else
    {
        stack = false;
    }
    /*old
    stack &= stack_peek(&exe->layer.var_if, (intptr_t *)&var_stack);
    stack &= stack_peek(&exe->layer.bank_if, (intptr_t *)&bank_stack);*/
    // is the variable in the var_stack?
    if(stack && var_stack == var)
    {
        // set variable from different table
		printf("(setting in loop respected table)");
        if(!vars_get(table, &vars_stack, var_stack)) return false;
        (*vars_stack)[var] = value;
    }
    else
    {
		printf("(setting in passed table)");
        (*vars)[var] = value;
    }
    return true;
}

bool var_get(vars_t *table, exe_t *exe, int32_t **vars, uint32_t var, int32_t *value)
{
    if(!exe || !vars) return false;
	printf("(get var '%c')", var);
    bool stack = true;
    uint32_t var_stack = 0;
    int32_t **vars_stack = 0;
    uint32_t bank_stack = 0;
    uint32_t stack_index = 0;
    if(stack_find(&exe->layer.var_if, var, &stack_index))
    {
        var_stack = var;
        bank_stack = exe->layer.bank_if.bottom[stack_index];
    }
    else
    {
        stack = false;
    }
    /* old
    stack &= stack_peek(&exe->layer.var_if, (intptr_t *)&var_stack);
    stack &= stack_peek(&exe->layer.bank_if, (intptr_t *)&bank_stack);
    */
    // is the variable in the var_stack?
    if(stack && var_stack == var)
    {
        // get variable from different table
		printf("(get from loop respected table)");
        if(!vars_get(table, &vars_stack, var_stack)) return false;
        *value = (*vars_stack)[var];
    }
    else
    {
		printf("(get from passed table)");
        *value = (*vars)[var];
    }
	printf("(got %d for var '%c')", *value, var);
    return true;
}

static inline bool exe_init(vars_t *table, exe_t *exe)
{
    if(!table || !exe) return false;
    bool result = true;
    result &= vars_get(table, &exe->vars_source, exe->bank);
    result &= vars_get(table, &exe->vars_other, exe->bank);
    result &= stack_init(&exe->layer.var_if, STACK_BATCH);
    result &= stack_init(&exe->layer.bank_if, STACK_BATCH);
    result &= stack_init(&exe->layer.value_if, STACK_BATCH);
    return result;
}

bool io_user_get_str(str_t *str)
{
    char c = 0;
    bool result = true;
    str_set(str, "");
    while((c = getchar()) != '\n' && c != EOF)
    {
        result &= str_append(str, "%c", c);
    }
    if(!str->l && (!c || c == EOF || c == '\n'))
    {
        return false;
    }
    return result;
}
inline bool str_append_va(str_t *str, char *format, va_list argp)
{
    if(!str) return false;
    
    // caller has to va_start and va_end before / after calling this function!
    bool result = false;
    int64_t len_app = vsnprintf(0, 0, format, argp);
    if(len_app < 0) return false;
    int len_new = (STR_BATCH) * (uint64_t)((str->l + len_app) / (STR_BATCH) + 1);
    if(len_new != str->n)
    {
        str->s = (uintptr_t)realloc(__s(str), sizeof(char) * len_new);
    }
    if(str->s)
    {
        str->n = len_new;
        len_app = vsnprintf(&__s(str)[str->l], len_app + 1, format, argp);
        if(len_app >= 0)
        {
            str->l += len_app;
            result = true;
        }
    }
    if(!result)
    {
        str_free(str);
    }
    
    return result;
}

bool str_append(str_t *str, char *format, ...)
{
    if(!str) return false;
    // va arg stuff
    va_list argp;
    va_start(argp, format);
    bool result = str_append_va(str, format, argp);
    va_end(argp);     // end argument list
    
    return result;
}

bool str_free(str_t *str)
{
    if(!str) return false;
    free(__s(str));
    str->s = 0;
    str->n = 0;
    str->l = 0;
    return true;
}

bool str_set(str_t *str, char *format, ...)
{
    if(!str) return false;
    
    bool result = true;
    result &= str_free(str);
    
    va_list argp;
    va_start(argp, format);
    result &= str_append_va(str, format, argp);
    va_end(argp);
    
    return result;
}

bool str_to_int32(str_t *str, int32_t *num)
{
    if(!str || !num || !str->l) return false;
    *num = 0;   // omit this to constantly add numbers
    // check for optional negative
    bool negative = false;
    if(__s(str)[0] == '-') negative = true;
    // check if the user has entered a number in the first place
    // and simultaneously convert to integer
    char *c = &__s(str)[str->l - 1];
    uint32_t factor = 1;
    for(int i = 1 * negative; i < str->l; i++)
    {
        if(*c < '0' || *c > '9')
        {
            return false;
        }
        *num += (factor * (*c-- - '0'));
        factor *= 10;
    }
    // if negative, invert
    if(negative)
    {
        *num *= -1;
    }
    return true;
}

bool io_user_get_int32(int32_t *num, bool require)
{
    str_t str = {0};
    bool result = true;
    do
    {
        if(io_user_get_str(&str))
        {
            result = str_to_int32(&str, num);
            require = !result;
        }
        else
        {
            result = false;
        }
    }
    while(require);
    str_free(&str);
    return result;
}


static inline int execute(lex_t **lex)
{
    if(!lex) return __LINE__;
    // init hash table
    exe_t exe = {0};
    vars_t table = {0};
    if(!vars_init(&table, TABLE_SIZE)) return __LINE__;
    if(!exe_init(&table, &exe)) return __LINE__;
    // execute
    bool skip_next = __LINE__;
    lex_t *base = *lex;
    stack_t for_stack = {0};
    if(!stack_init(&for_stack, STACK_BATCH)) return __LINE__;
    while(*lex)
    {
        switch((*lex)->token)
        {
            case TOKEN_ADD:
            {
				printf("(TOKEN_ADD)");
                int32_t a = 0;
                int32_t b = 0;
                if(!var_get(&table, &exe, exe.vars_source, ((uint32_t *)(*lex)->value)[0], &a)) return __LINE__;
                if(!var_get(&table, &exe, exe.vars_other, ((uint32_t *)(*lex)->value)[1], &b)) return __LINE__;
                if(b)
                {
                    if(!var_set(&table, &exe, exe.vars_source, ((int32_t *)(*lex)->value)[0], a + b)) return __LINE__;
                }
                else
                {
                    if(!var_set(&table, &exe, exe.vars_source, ((int32_t *)(*lex)->value)[0], 0)) return __LINE__;
                }
            } break;
            case TOKEN_MEM_SET:
            {
				printf("(TOKEN_MEM_SET)");
                int32_t bank = 0;
                if(!var_get(&table, &exe, exe.vars_source, (*lex)->value, &bank)) return __LINE__;
                exe.bank = bank;
                if(!vars_get(&table, &exe.vars_source, exe.bank)) return __LINE__;
            } break;
            case TOKEN_MEM_SWAP:
            {
				printf("(TOKEN_MEM_SWAP)");
                int32_t **temp = exe.vars_source;
                exe.vars_source = exe.vars_other;
                exe.vars_other = temp;
            } break;
            case TOKEN_FOR:
            {
				printf("(TOKEN_FOR)");
                exe.layer.layer++;
                // check if value is nonzero
                int32_t value = 0;
                uint32_t var = 0;
                bool stack = true;
                // get value of variable associated with the loop
                if(!var_get(&table, &exe, exe.vars_source, (*lex)->value, &value)) return __LINE__;
				printf("%d,", value);
                // get variale at top of if-stack
                stack &= stack_peek(&exe.layer.var_if, (intptr_t *)&var);
				printf("%d", value);
                // is the value associated with the loop not zero?
                if(value)
                {
					printf("(value is not zero)");
                    // are the vars associated with top of if-stack and loop not equal? or is the stack empty?
                    if(!stack || var != (*lex)->value)
                    {
                        // var isn't in the stack yet, add it
                        if(!stack_push(&for_stack, (intptr_t)*lex)) return __LINE__;       // push lexer position to jump back
                        if(!stack_push(&exe.layer.var_if, (*lex)->value)) return __LINE__; // push original var
                        if(!stack_push(&exe.layer.bank_if, exe.bank)) return __LINE__;     // push original bank
                        if(!stack_push(&exe.layer.value_if, value)) return __LINE__;       // push original value
                    }
                    // increment or decrement
                    if(value > 0)
                    {
						printf("(decrement value)");
                        // value is larger than 0, decrement
                        if(!var_set(&table, &exe, exe.vars_source, (*lex)->value, value - 1)) return __LINE__;
                    }
                    else
                    {
						printf("(increment value)");
                        // value is smaller than 0, increment
                        if(!var_set(&table, &exe, exe.vars_source, (*lex)->value, value + 1)) return __LINE__;
                    }
                }
                else
                {
					printf("(value is zero)");
                    // value associated with loop is zero, invert original value -> loop is over
                    // is the value in the stack? or in other words, was the loop touched?
					if(stack && var == (*lex)->value)
                    {
                        // stack operations
						printf("(remove loop value from stack)");
                        if(!stack_pop(&for_stack, (intptr_t *)0)) return __LINE__;                         // pop initial for address
                        if(!stack_pop(&exe.layer.value_if, (intptr_t *)&value)) return __LINE__;           // get initial value
                        if(!stack_pop(&exe.layer.var_if, 0)) return __LINE__;                              // pop initial var
                        if(!stack_pop(&exe.layer.bank_if, 0)) return __LINE__;                             // pop initial bank
                        printf("(invert original nb)");
						value *= -1;                                                                    // invert value
                        if(!var_set(&table, &exe, exe.vars_source, (*lex)->value, value)) return __LINE__; // set new value
                        // go to end end of else
						printf("(skip to end of else)");
                        var = (*lex)->value;                    // get var of this loop
                        for(;;)
                        {
                            if(*lex) *lex = (*lex)->next;       // is there still a exer struct?
                            else return __LINE__;
                            if((*lex)->token == TOKEN_ELSE_END) // is the token equal to the one we're searching for?
                            {
                                if((*lex)->value == var) break; // also, the var is equal to the var of this loop?
                            }
                        }
                        //while(*lex && (*lex)->token != TOKEN_ELSE_END) *lex = (*lex)->next;
                        //skip_next = true;
                    }
                    else
                    {
						printf("(skip to else)");
                        // loop was not touched, skip to else
                        var =(*lex)->value;                     // get var of this loop
                        for(;;)
                        {
                            if(*lex) *lex = (*lex)->next;       // is there still a exer struct?
                            else return __LINE__;
                            if((*lex)->token == TOKEN_ELSE)     // is the token equal to the one we're searching for?
                            {
                                if((*lex)->value == var) break; // also, the var is equal to the var of this loop?
                            }
                        }
//                        while(*lex && (*lex)->token != TOKEN_ELSE) *lex = (*lex)->next;
                        //skip_next = true;
                    }
                }
            } break;
			case TOKEN_INVERT:
			{
				
			} break;
            case TOKEN_ELSE:
            {
				printf("(TOKEN_ELSE)");
                // no code required
            } break;
            case TOKEN_FOR_END:
            {
				printf("(TOKEN_FOR_END)");
                if(!stack_peek(&for_stack, (intptr_t *)lex)) return __LINE__;
                skip_next = true;
                // no code required
            } break;
            case TOKEN_ELSE_END:
            {
				printf("(TOKEN_ELSE_END)");
                // no code required
            } break;
            case TOKEN_INPUT_CH:
            {
				printf("(TOKEN_INPUT_CH)");
                char c = 0;
                //c = getc(stdin);
                if(!var_set(&table, &exe, exe.vars_source, (*lex)->value, c)) return __LINE__;
            } break;
            case TOKEN_INPUT_NB:
            {
				printf("(TOKEN_INPUT_NB)");
                int32_t value = 0;
                io_user_get_int32(&value, true);
                if(!var_set(&table, &exe, exe.vars_source, (*lex)->value, value)) return __LINE__;
            } break;
            case TOKEN_OUTPUT_CH:
            {
				printf("(TOKEN_OUTPUT_CH)");
                int32_t value = 0;
                if(!var_get(&table, &exe, exe.vars_source, (*lex)->value, &value)) return __LINE__;
                printf("%c", (char)value);
            } break;
            case TOKEN_OUTPUT_NB:
            {
				printf("(TOKEN_OUTPUT_NB)");
                int32_t value = 0;
                if(!var_get(&table, &exe, exe.vars_source, (*lex)->value, &value)) return __LINE__;
                printf("%d", value);
            } break;
            default: return __LINE__;
        }
        // next instruction
        if(!skip_next) *lex = (*lex)->next;
        skip_next = false;
    }
    // success
    printf("\n\n[Note] Finished.\n");
    *lex = base;
    return 0;
}

bool stack_init(stack_t *stack, uint32_t batches)
{
    bool result = false;
    bool init = true;
    if(stack->bottom)
    {
        init = false;
    }
    stack->bottom = realloc(stack->bottom, sizeof(intptr_t) * batches);
    if(stack->bottom)
    {
        result = true;
    }
    else
    {
        stack->bottom = 0;
    }
    if(init || !stack->bottom)
    {
        stack->length = 0;
    }
    stack->batches = result * batches;
    return result;
}

bool stack_push(stack_t *stack, intptr_t value)
{
    bool result = true;
	printf("(stack push)");
    if((stack->length && !(stack->length % STACK_BATCH)) || !stack->batches)
    {
        result &= stack_init(stack, stack->batches + STACK_BATCH + 1);
    }
    if(result && stack->batches)
    {
        stack->bottom[stack->length] = value;
        stack->length++;
    }
    return result;
}

bool stack_peek(stack_t *stack, intptr_t *value)
{
    bool result = false;
	printf("(stack peek)");
    if(stack->length)
    {
        *value = stack->bottom[stack->length - 1];
        result = true;
		printf("(peek successful)");
    }
    return result;
}

bool stack_pop(stack_t *stack, intptr_t *value)
{
    bool result = false;
	printf("(stack pop)");
    if(stack->length)
    {
        stack->length--;
        if(value)
        {
            *value = stack->bottom[stack->length];
        }
		printf("(pop successful)");
        result = true;
    }
    return result;
}

static bool stack_find(stack_t *stack, intptr_t value, uint32_t *index)
{
	printf("(stack find)");
    bool result = false;
    for(uint32_t i = stack->length; i + 1 > 0; i--)
    {
        if(value == stack->bottom[stack->length])
        {
            *index = i;
            result = true;
			printf("(found at index %d)", i);
			break;
        }
    }
    return result;
}

int main(void)
{
    // N0N4N9NNNNNNLL`II`L1S0SNSISSSSL0SSLLLL
    // ^ above makes execute() exit with -2 after hitting enter
//    char *code = {"n0n9n1101a11202a22b`2`n`2222a`1`n`11111`"};   // <-- code goes in here
//    char *code = {"`222211`220`22"};   // truth machine
//    char *code = {"H`e`l`l`o`,` `W`o`r`l`d`!`a0a9a1a`"};   // hello world

//    char *code = {"111111i1111111E0E4E9EEEEEELLi1```IL1R0RERIRRI`R0RRL0RRLLLL"};   // cat ???
//    char *code = {"E0E4E9EEEEEEL0L1LLL2`CR0RCRERR111111L1``111111L1C0RRn0nLL0RRLLLLL0LnLLm0mLmmmmmmi0imin``C`LLLL"};   // cat ???
//    char *code = {"N0N9N1E0E4E9EEEEEE909E9`LL`IL1R0RERI909RN`9`LLLL"};   // cat ???
    
//    char *code = {"1111111`"};
    
    //char *code = {"i0`2t0t2202121ttb0bii1tttt111111i0i1111111tti12`tttt"};
    //char *code = {"i027t0t2202121ttb0bii1tttt111111i0i1111111303t3`tti1a`tttt"};
    char *code = {"11112a11"};
    
//    char *code = {""};   // <-- code goes in here
    // start lexing
    lex_t *lex = 0;
	int result = 0;
    if((result = lex_do(&lex, code))) return result;
    // execute
    if((result = execute(&lex))) return result;
	// finished
    return 0;
}

#undef __s//(x) ((char *)(x)->s)
