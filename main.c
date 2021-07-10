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

#include "main.h"

static int DEBUG_LEVEL;

static inline uint32_t hash_index(uint32_t bits, uint32_t mask, uint32_t value)
{
    return ((value * UINT32_C(0x61c88647)) >> (32 - bits)) % mask;
}

static inline bool vars_init(vars_t *table, uint32_t lookup_size)
{
    DEBUG_PRINT(DEBUG_LEVEL_3, "init vars");
    if(!table) return false;
    // allocate memory
    table->node = malloc(sizeof(vars_node_t) * lookup_size);
    if(!table->node) return false;
    // initialize with 0
    for(uint32_t i = 0; i < lookup_size; i++)
    {
        table->node[i].vars = 0;
        table->node[i].bank = 0;
        table->node[i].used = 0;
    }
    // initialize table
    table->lookup_size = lookup_size;
    table->mask = lookup_size;
    table->bits = HASH_BITS;
    DEBUG_PRINT(DEBUG_LEVEL_3, "init vars successful");
    return true;
}

static bool vars_free(vars_t *table)
{
    if(!table) return false;
    // for each node
    for(uint32_t i = 0; i < table->lookup_size; i++)
    {
        // for each used node
        for(uint32_t j = 0; j < table->node[i].used; j++)
        {
            free(table->node[i].vars[j]);
        }
        free(table->node[i].bank);
        free(table->node[i].vars);
    }
    free(table->node);
    return true;
}

static inline bool lex_init(lex_t *lex, uint32_t size)
{
    DEBUG_PRINT(DEBUG_LEVEL_3, "init lex");
    if(!lex) return false;
    lex->token = 0;
    lex->value = 0;
    lex->used = 0;
    lex->batches = size;
    lex->allocated = 0;
    DEBUG_PRINT(DEBUG_LEVEL_3, "init lex successful");
    return true;
}

static inline bool lex_add(lex_t *lex, token_t token, uint32_t value)
{
    DEBUG_PRINT(DEBUG_LEVEL_3, "add token %s", token_s[token]);
    if(!lex) return false;
    uint32_t required = (lex->used / lex->batches + 1) * lex->batches;
    if(required != lex->allocated)
    {
        lex->value = realloc(lex->value, sizeof(intptr_t) * required);
        if(!lex->value)
        {
            return false;
        }
        lex->token = realloc(lex->token, sizeof(token_t) * required);
        if(!lex->token)
        {
            return false;
        }
    }
    // copy the value
    lex->token[lex->used] = token;
    switch(token)
    {
        case TOKEN_ADD:
        {
            lex->value[lex->used] = (intptr_t)malloc(sizeof(int32_t) * 2);
            if(!(lex->value[lex->used])) return false;
            ((uint32_t *)(lex->value[lex->used]))[0] = ((uint32_t *)value)[0];
            ((uint32_t *)(lex->value[lex->used]))[1] = ((uint32_t *)value)[1];
        } break;
        case TOKEN_INPUT_NB:
        case TOKEN_INPUT_CH:
        case TOKEN_OUTPUT_CH:
        case TOKEN_OUTPUT_NB:
        case TOKEN_MEM_SET:
        case TOKEN_MEM_SWAP:
        case TOKEN_MEM_BOTH:
        case TOKEN_IF_NOT:
        case TOKEN_ELSE:
        case TOKEN_ELSE_END:
        case TOKEN_FOR:
        case TOKEN_FOR_END:
        case TOKEN_INVERT:
        {
            lex->value[lex->used] = value;
        } break;
        case TOKEN_END:
        {} break;
        default: return false;
    }
    lex->used++;
    DEBUG_PRINT(DEBUG_LEVEL_3, "add token successful");
    return true;
}

static inline bool lex_free(lex_t *lex)
{
    DEBUG_PRINT(DEBUG_LEVEL_3, "free tokens");
    if(!lex) return false;
    for(uint32_t i = 0; i < lex->used; i++)
    {
        if(lex->token[i] == TOKEN_ADD) free((int32_t *)lex->value[i]);
    }
    free(lex->value);
    free(lex->token);
    lex->used = 0;
    lex->allocated = 0;
    lex->batches = 0;
    return true;
}

static inline bool lex_optimize(lex_t *lex)
{
    if(!lex) return false;
    lex_t optimized = {0};
    if(!lex_init(&optimized, LEX_BATCH)) return false;
    for(int32_t i = 0; i < lex->used; i++)
    {
        bool handled = false;
        token_t *token = &lex->token[i];
        intptr_t *value = &lex->value[i];
        // look ahead four values
        if(i < lex->used - 3)
        {
            // we actually just wants to invert the number?
            if(token[0] == TOKEN_FOR && token[1] == TOKEN_FOR_END && \
               token[2] == TOKEN_ELSE && token[3] == TOKEN_ELSE_END)
            {
               if(value[0] == value[1] && value[1] == value[2] && value[2] == value[3])
               {
                   DEBUG_PRINT(DEBUG_LEVEL_3, "we can shrink empty leai to invert");
                   if(!lex_add(&optimized, TOKEN_INVERT, *value)) return false;
                   handled = true;
                   i += 3;
                   continue;
               }
            }
        }
        // look ahead three values
        if(i < lex->used - 2)
        {
            // are we trying to set both banks?
            if(token[0] == TOKEN_MEM_SET && token[1] == TOKEN_MEM_SWAP && token[2] == TOKEN_MEM_SET)
            {
                if(value[0] == value[1] && value[1] == value[2])
                {
                    DEBUG_PRINT(DEBUG_LEVEL_3, "we set both banks immediately");
                    if(!lex_add(&optimized, TOKEN_MEM_BOTH, *value)) return false;
                    handled = true;
                    i+= 2;
                    continue;
                }
            }
            // the loop is empty?
            if(token[0] == TOKEN_FOR && token[1] == TOKEN_FOR_END && token[2] == TOKEN_ELSE)
            {
                if(value[0] == value[1] && value[1] == value[2])
                {
                    DEBUG_PRINT(DEBUG_LEVEL_3, "we can skip the loop");
                    if(!lex_add(&optimized, TOKEN_IF_NOT, *value)) return false;
                    handled = true;
                    i += 2;
                    continue;
                }
            }
        }
        // look ahead two values
        if(i < lex->used - 1)
        {
        }
        // unhandled case
        if(!handled)
        {
            if(!lex_add(&optimized, token[0], *value)) return false;
        }
    }
    if(!lex_swap(lex, &optimized)) return false;
    if(!lex_free(&optimized)) return false;
    return true;
}

static inline bool lex_swap(lex_t *a, lex_t *b)
{
    if(!a || !b) return false;
    DEBUG_PRINT(DEBUG_LEVEL_3, "swap tokens");
    // XOR swap (values)
    a->allocated ^= b->allocated;
    b->allocated ^= a->allocated;
    a->allocated ^= b->allocated;
    a->batches ^= b->batches;
    b->batches ^= a->batches;
    a->batches ^= b->batches;
    a->used ^= b->used;
    b->used ^= a->used;
    a->used ^= b->used;
    // swap with temporary values (pointers)
    token_t *temp_token = a->token;
    a->token = b->token;
    b->token = temp_token;
    intptr_t *temp_value = a->value;
    a->value = b->value;
    b->value = temp_value;
    DEBUG_PRINT(DEBUG_LEVEL_3, "swap tokens successful");
    return true;
}

static inline void lex_print(lex_t *tokens)
{
    if(!tokens) return;
    uint32_t layer = 0;
    int max_digits = 0;
    uint32_t max_digit = tokens->used;
    if(max_digit) max_digit--;
    while(max_digit)
    {
        max_digit /= 10;
        max_digits++;
    }
    for(uint32_t i = 0; i < tokens->used; i++)
    {
        DEBUG_CODE(DEBUG_LEVEL_1, printf("%*d: ", max_digits, i));
        for(uint32_t j = 0; j < layer; j++)
        {
            DEBUG_CODE(DEBUG_LEVEL_1, printf("  "));
        }
        DEBUG_CODE(DEBUG_LEVEL_1, printf("%s", token_s[tokens->token[i]]));
        switch(tokens->token[i])
        {
            case TOKEN_FOR:
            case TOKEN_IF_NOT:
            case TOKEN_ELSE:
                layer++;
                DEBUG_CODE(DEBUG_LEVEL_1, printf(" '%c'", tokens->value[i]));
                break;
            case TOKEN_FOR_END:
            case TOKEN_ELSE_END:
                DEBUG_CODE(DEBUG_LEVEL_1, printf(" '%c'", tokens->value[i]));
                layer--;
                break;
            case TOKEN_INVERT:
            case TOKEN_INPUT_NB:
            case TOKEN_INPUT_CH:
            case TOKEN_OUTPUT_CH:
            case TOKEN_OUTPUT_NB:
            case TOKEN_MEM_SET:
            case TOKEN_MEM_SWAP:
                DEBUG_CODE(DEBUG_LEVEL_1, printf(" '%c'", tokens->value[i]));
                break;
            case TOKEN_ADD:
                DEBUG_CODE(DEBUG_LEVEL_1, printf(" '%c'+'%c'", ((int32_t *)tokens->value[i])[0], ((int32_t *)tokens->value[i])[1]));
            default:
                break;
        }
        DEBUG_CODE(DEBUG_LEVEL_1, printf("\n"));
    }
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
        if(node->bank[i] == key)
        {
              add = false;
              break;
        }
    }
    if(!add) return true;
    // allocate new memory
    DEBUG_PRINT(DEBUG_LEVEL_3, "add vars");
    node->bank = realloc(node->bank, sizeof(uint32_t) * (used + 1));
    if(!node->bank) return false;
    node->vars = realloc(node->vars, sizeof(int32_t *) * (used + 1));
    if(!node->vars) return false;
    node->vars[used] = malloc(sizeof(int32_t) * NVARS);
    if(!node->vars[used]) return false;
    // allocation successful
    init_vars(node->vars[used], NVARS);
    node->bank[used] = key;
    node->used++;     // successfully allocated memory
    return true;
}

static inline bool vars_get(vars_t *table, int32_t **value, int32_t bank)
{
    if(!table || !value) return false;
    DEBUG_PRINT(DEBUG_LEVEL_3, "get vars for key %d", bank);
    // get a hash index
    uint32_t index = hash_index(table->bits, table->mask, bank);
    // check if the key exists
    vars_node_t *node = &table->node[index];
    uint32_t used = node->used;
    uint32_t pos = 0;
    bool exists = false;
    for(uint32_t i = 0; i < used; i++)
    {
        if(node->bank[i] == bank)
        {
            pos = i;
            exists = true;
            break;
        }
    }
    // add the key if it doesn't exist
    if(!exists)
    {
        DEBUG_PRINT(DEBUG_LEVEL_3, "add vars for key %d", bank);
        if(!vars_add(table, bank)) return false;
        pos = node->used - 1;
    }
    *value = node->vars[pos];
    DEBUG_PRINT(DEBUG_LEVEL_3, "vars for key %d found", bank);
    return true;
}

static inline int lex_do(lex_t *tokens, char *str)
{
    if(!tokens || !str) return __LINE__;
    size_t len = strlen(str);    // length of string
    lex_init(tokens, LEX_BATCH);
    if(len % 2)
    {
        DEBUG_CODE(DEBUG_LEVEL_2, fprintf(stderr, "[Warning] String is not ending on a pair of characters, ignoring the last one.\n"));
    }
    if(len)
    {
        intptr_t this_var = 0;
        intptr_t last_var = 0;
        int lex_queued = 0;
        token_t this_token = TOKEN_NONE;
        token_t last_token = TOKEN_NONE;
        int32_t *tuple = malloc(sizeof(int32_t) * 2);
        if(!tuple) return __LINE__;
        uint8_t leai[NVARS] = {0}; // loop, else and invert
        for(uint32_t i = 0; i < len - 1; i += 2)
        {
            uint32_t j = i + 1;
            // ==== MEMORY ====
            if(str[i] == '`' && str[j] == '`')
            {
                if(last_token == TOKEN_MEM_SET && last_token != TOKEN_MEM_SWAP)
                {
                    this_token = TOKEN_MEM_SWAP;
                    lex_queued++;
                }
                else
                {
                    this_token = TOKEN_MEM_SET;
                    lex_queued++;
                }
                this_var = last_var;
            }
            // ==== USER INPUT ====
            else if(str[i] == '`')
            {
                this_var = str[j];
                if(str[j] >= '0' && str[j] <= '9') this_token = TOKEN_INPUT_NB;
                else this_token = TOKEN_INPUT_CH;
                lex_queued++;
                last_var = str[j];
            }
            // ==== OUTPUT ====
            else if(str[j] == '`')
            {
                this_var = str[i];
                if(str[i] >= '0' && str[i] <= '9') this_token = TOKEN_OUTPUT_NB;
                else this_token = TOKEN_OUTPUT_CH;
                lex_queued++;
                last_var = str[i];
            }
            // === LOOP, ELSE AND INVERT (LEAI) ===
            else if(str[i] == str[j])
            {
                this_var = str[i];
                leai[(uint32_t)str[i]]++;
                switch(leai[(uint32_t)str[i]])
                {
                    case 1: this_token = TOKEN_FOR;
                        break;
                    case 2: this_token = TOKEN_FOR_END;
                        i -= 2;
                        break;
                    case 3: this_token = TOKEN_ELSE;
                        break;
                    case 4: this_token = TOKEN_ELSE_END;
                        leai[(uint32_t)str[i]] = 0;
                        break;
                    default:
                        return __LINE__;     // this line will technically never be reached
                }
                lex_queued++;
                last_var = str[i];
            }
            // ==== ADD ====
            else
            {
                this_token = TOKEN_ADD;
                lex_queued++;
                tuple[0] = str[i];
                tuple[1] = str[j];
                this_var = (intptr_t)tuple;
                last_var = str[i];
            }
            // add to the lexers
            if(this_token)
            {
                if(!lex_add(tokens, this_token, this_var)) return __LINE__;
                last_token = this_token;
            }
        }
        free(tuple);
        // error: check if leai[] is not all 0's
        bool missing_leai = false;
        int missing_pos = 0;
        for(int i = 0; i < NVARS; i++)
        {
            if(leai[i])
            {
                missing_pos = i;
                missing_leai = true;
                break;
            }
        }
        // any errors?
        if(missing_leai)
        {
            fprintf(stderr, "There is a missing loop, else or end. (maybe it is %c)\n", missing_pos);
            return __LINE__;
        }
    }
    if(!lex_add(tokens, TOKEN_END, 0)) return __LINE__;
    // success
    DEBUG_CODE(DEBUG_LEVEL_1, printf("==== ORIGINAL LIST ====\n"));
    DEBUG_CODE(DEBUG_LEVEL_1, lex_print(tokens));
    if(!lex_optimize(tokens)) return __LINE__;
    DEBUG_CODE(DEBUG_LEVEL_1, printf("==== OPTIMIZED LIST ====\n"));
    DEBUG_CODE(DEBUG_LEVEL_1, lex_print(tokens));
    return 0;
}

static inline bool var_get(exe_t *exe, int32_t *vars, uint32_t var, int32_t **value)
{
    if(!exe || !vars) return false;
    DEBUG_PRINT(DEBUG_LEVEL_3, "get var '%c'", var);
    if(exe->layers.used[var])
    {
        DEBUG_PRINT(DEBUG_LEVEL_3, "get from loop respected table");
        *value = exe->layers.value[var];
    }
    else
    {
        DEBUG_PRINT(DEBUG_LEVEL_3, "get from passed table");
        *value = &vars[var];
    }
    DEBUG_PRINT(DEBUG_LEVEL_3, "got %d for var '%c')", **value, var);
    return true;
}

static inline bool exe_init(exe_t *exe)
{
    if(!exe) return false;
    bool result = true;
    result &= vars_init(&exe->bank_both, TABLE_SIZE);
    result &= vars_get(&exe->bank_both, &exe->vars_source, exe->bank);
    result &= vars_get(&exe->bank_both, &exe->vars_other, exe->bank);
    return result;
}

static inline int execute(lex_t *tokens)
{
    if(!tokens || !tokens->token || !tokens->value) return __LINE__;
    // init exe
    exe_t exe = {0};
    if(!exe_init(&exe)) return __LINE__;
    // execute
    uint32_t i = 0;
    bool running = true;
    while(running)
    {
        token_t lex_token = tokens->token[i];
        intptr_t lex_value = tokens->value[i];
        DEBUG_PRINT(DEBUG_LEVEL_3, "%s", token_s[lex_token]);
        switch(lex_token)
        {
            case TOKEN_ADD:
            {
                int32_t *a = 0;
                int32_t *b = 0;
                if(!var_get(&exe, exe.vars_source, ((uint32_t *)lex_value)[0], &a)) return __LINE__;
                if(!var_get(&exe, exe.vars_other, ((uint32_t *)lex_value)[1], &b)) return __LINE__;
                if(*b)
                {
                    *a += *b;
                }
                else
                {
                    *a = 0;
                }
            } break;
            case TOKEN_MEM_SET:
            {
                int32_t *bank = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &bank)) return __LINE__;
                exe.bank = *bank;
                if(!vars_get(&exe.bank_both, &exe.vars_source, exe.bank)) return __LINE__;
            } break;
            case TOKEN_MEM_SWAP:
            {
                int32_t *temp = exe.vars_source;
                exe.vars_source = exe.vars_other;
                exe.vars_other = temp;
            } break;
            case TOKEN_MEM_BOTH:
            {
                int32_t *bank = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &bank)) return __LINE__;
                exe.bank = *bank;
                if(!vars_get(&exe.bank_both, &exe.vars_source, exe.bank)) return __LINE__;
                if(!vars_get(&exe.bank_both, &exe.vars_other, exe.bank)) return __LINE__;
            } break;
            case TOKEN_FOR:
            {
                // check if value is nonzero
                int32_t *value = 0;
                uint32_t var = lex_value;
                // get value of variable associated with the loop
                if(!var_get(&exe, exe.vars_source, var, &value)) return __LINE__;
                bool inloop = ((exe.layers.used)[var] == true);
                // is the value associated with the loop not zero?
                if(*value)
                {
                    DEBUG_PRINT(DEBUG_LEVEL_3, "value is not zero");
                    // are we in a loop?
                    if(!inloop)
                    {
                        // var isn't stored yet, add it
                        exe.for_positions[var] = i - 1;
                        exe.layers.used[var] = true;
                        exe.layers.value[var] = &exe.vars_source[var];
                        exe.layers.initial[var] = exe.vars_source[var];
                    }
                    // increment or decrement
                    if(*value > 0)
                    {
                        DEBUG_PRINT(DEBUG_LEVEL_3, "decrement value");
                        // value is larger than 0, decrement
                        *value -= 1;
                    }
                    else
                    {
                        DEBUG_PRINT(DEBUG_LEVEL_3, "increment value");
                        // value is smaller than 0, increment
                        *value += 1;
                    }
                }
                else
                {
                    DEBUG_PRINT(DEBUG_LEVEL_3, "value is zero");
                    // value associated with loop is zero, invert original value -> loop is over
                    // was the loop touched?
                    if(inloop)
                    {
                        DEBUG_PRINT(DEBUG_LEVEL_3, "reset initial values");
                        exe.for_positions[var] = 0;
                        exe.layers.used[var] = 0;
                        exe.layers.value[var] = 0;
                        DEBUG_PRINT(DEBUG_LEVEL_3, "invert original number");
                        *value = -exe.layers.initial[var];                              // invert value
                        // go to end end of else
                        DEBUG_PRINT(DEBUG_LEVEL_3, "skip to end of else");
                        var = lex_value;                    // get var of this loop
                        for(;;)
                        {
                            if(!(i < tokens->used)) return __LINE__;
                            if(tokens->token[++i] == TOKEN_ELSE_END) // is the token equal to the one we're searching for?
                            {
                                if(tokens->value[i] == var) break; // also, the var is equal to the var of this loop?
                            }
                        }
                    }
                    else
                    {
                        DEBUG_PRINT(DEBUG_LEVEL_3, "skip to else");
                        // loop was not touched, skip to else
                        var = lex_value;                     // get var of this loop
                        for(;;)
                        {
                            if(!(i < tokens->used)) return __LINE__;
                            if(tokens->token[++i] == TOKEN_ELSE) // is the token equal to the one we're searching for?
                            {
                                if(tokens->value[i] == var) break; // also, the var is equal to the var of this loop?
                            }
                        }
                    }
                }
            } break;
            case TOKEN_IF_NOT:
            {
                int32_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                if(*value)
                {
                    // skip to end of else
                    for(;;)
                    {
                        if(!(i < tokens->used)) return __LINE__;
                        if(tokens->token[++i] == TOKEN_ELSE_END) // is the token equal to the one we're searching for?
                        {
                            if(tokens->value[i] == lex_value) break; // also, the var is equal to the var of this loop?
                        }
                    }
                }
                else
                {
                    *value *= -1;
                }
            } break;
            case TOKEN_INVERT:
            {
                intptr_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                *value *= -1;
            } break;
            case TOKEN_ELSE:
            {
                // no code required
            } break;
            case TOKEN_FOR_END:
            {
                i = exe.for_positions[lex_value];
            } break;
            case TOKEN_ELSE_END:
            {
                // no code required
            } break;
            case TOKEN_INPUT_CH:
            {
                int32_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                str_t str = {0};
                uint32_t bank = exe.bank;
                if(!io_user_get_str(&str)) return __LINE__;
                for(uint32_t k = 0; k <= str.l; k++)
                {
                    if(!vars_get(&exe.bank_both, &exe.vars_source, bank++)) return __LINE__;
                    if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                    *value = ((char *)str.s)[k];
                }
                // terminating character
                if(!vars_get(&exe.bank_both, &exe.vars_source, str.l)) return __LINE__;
                *value = 0;
                // return to initial bank
                if(!vars_get(&exe.bank_both, &exe.vars_source, exe.bank)) return __LINE__;
                str_free(&str);
            } break;
            case TOKEN_INPUT_NB:
            {
                int32_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                io_user_get_int32(value, true);
            } break;
            case TOKEN_OUTPUT_CH:
            {
                int32_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                printf("%c", (char)*value);
            } break;
            case TOKEN_OUTPUT_NB:
            {
                int32_t *value = 0;
                if(!var_get(&exe, exe.vars_source, lex_value, &value)) return __LINE__;
                printf("%d", *value);
            } break;
            case TOKEN_END:
            {
                running = false;
            } break;
            default: return __LINE__;
        }
        // next instruction
        i++;
    }
    if(!vars_free(&exe.bank_both)) return __LINE__;
    // success
    return 0;
}

static void list_flags()
{
    fprintf(stderr, "(you can also leave the flags empty)\n");
    fprintf(stderr, "An argument with a leading - is a flag. The following flags are implemented (case sensitive):\n");
    fprintf(stderr, "F : List Flags (ignore file)\n");
    fprintf(stderr, "D0 : no debug (default)\n");
    fprintf(stderr, "D1 : show generated code\n");
    fprintf(stderr, "D2 : status messages and warnings + D1\n");
    fprintf(stderr, "D3 : alot of debug messages + D2\n");
}

int main(int argc, char **args)
{
    int result = 0;
    str_t arg = {0};
    str_t code = {0};
    str_t flag = {0};
    if(argc < 2)
    {
        fprintf(stderr, "==== AsciiAscii Interpreter by rphii ====\n");
        fprintf(stderr, "Incorrect usage... See below:\n");
        fprintf(stderr, "AsciiAscii-Interpreter -F Filename.ascii2\n");
        fprintf(stderr, "Or enter a file now: ");
        str_t file = {0};
        io_user_get_str(&file);
        if(!io_file_read(&code, (char *)file.s))
        {
            fprintf(stderr, "Could not find or open file.\n");
        }
    }
    for(int i = 1; i < argc; i++)
    {
        if(args[i][0] == '-')
        {
            str_set(&arg, "%s", &args[i][1]);
            str_set(&flag, "D0");
            if(str_cmp(&arg, &flag)) DEBUG_LEVEL = DEBUG_LEVEL_0;
            str_set(&flag, "D1");
            if(str_cmp(&arg, &flag)) DEBUG_LEVEL = DEBUG_LEVEL_1;
            str_set(&flag, "D2");
            if(str_cmp(&arg, &flag)) DEBUG_LEVEL = DEBUG_LEVEL_2;
            str_set(&flag, "D3");
            if(str_cmp(&arg, &flag)) DEBUG_LEVEL = DEBUG_LEVEL_3;
            str_set(&flag, "F");
            if(str_cmp(&arg, &flag))
            {
                list_flags();
                return 0;
            }
        }
        else
        {
            if(!io_file_read(&code, args[i]))
            {
                fprintf(stderr, "Could not find or open file.\n");
            }
        }
    }
    // if there is code
    if(code.s)
    {
        // start lexing
        lex_t lex = {0};
        
        DEBUG_CODE(DEBUG_LEVEL_2, printf("[Note] Lexing\n"));
        if(!result) result = lex_do(&lex, (char *)code.s);
        DEBUG_CODE(DEBUG_LEVEL_2, printf("\n[Note] Executing\n"));
        // execute
        if(!result) result = execute(&lex);
        DEBUG_CODE(DEBUG_LEVEL_2, printf("\n[Note] Finished\n"));
        DEBUG_CODE(DEBUG_LEVEL_0, printf("\n"));
        
        lex_free(&lex);
    }
    // finished
    str_free(&arg);
    str_free(&code);
    str_free(&flag);
    return result;
}

