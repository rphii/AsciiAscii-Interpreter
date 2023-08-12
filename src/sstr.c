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

#include "sstr.h"

#define prvt_s(x) ((char *)(x)->s)

bool sstr_append_va(sstr_s *str, char *format, va_list argp)
{
    if(!str) return false;

    /* caller has to va_start and va_end before / after calling this function! */
    bool result = false;
    va_list argl = {0};
    va_copy(argl, argp); /* TODO check for error */
    int64_t len_app = vsnprintf(0, 0, format, argl);
    va_end(argl);
    if(len_app < 0) return false;
    int len_new = (SSTR_BATCH) * (uint64_t)((str->l + len_app) / (SSTR_BATCH) + 1);
    if(len_new != str->n)
    {
        str->s = (uintptr_t)realloc(prvt_s(str), sizeof(char) * len_new);
    }
    if(str->s)
    {
        str->n = len_new;
        len_app = vsnprintf(&prvt_s(str)[str->l], len_app + 1, format, argp);
        if(len_app >= 0)
        {
            str->l += len_app;
            result = true;
        }
    }
    if(!result)
    {
        sstr_free(str);
    }

    return result;
}

bool sstr_append(sstr_s *str, char *format, ...)
{
    if(!str) return false;
    /* va arg stuff */
    va_list argp;
    va_start(argp, format);
    bool result = sstr_append_va(str, format, argp);
    va_end(argp); /* end argument list */

    return result;
}

bool sstr_free(sstr_s *str)
{
    if(!str) return false;
    free(prvt_s(str));
    str->s = 0;
    str->n = 0;
    str->l = 0;
    return true;
}

bool sstr_set(sstr_s *str, char *format, ...)
{
    if(!str) return false;

    bool result = true;
    result &= sstr_free(str);

    va_list argp;
    va_start(argp, format);
    result &= sstr_append_va(str, format, argp);
    va_end(argp);

    return result;
}

char *sstr_get(sstr_s *str)
{
    if(!str) return 0;

    return prvt_s(str);
}

bool sstr_init(sstr_s *str)
{
    if(!str) return false;
    str->l = 0;
    str->n = 0;
    str->s = 0;
    return true;
}

bool sstr_init_p(sstr_s **str)
{
    if(!str) return false;
    *str = malloc(sizeof(str));
    if(*str)
    {
        (*str)->l = 0;
        (*str)->n = 0;
        (*str)->s = 0;
        return true;
    }
    return false;
}

/**
 * @brief reallocate sstr_s string
 * @param str
 * @param size
 * @return true if success
 */
bool sstr_realloc(sstr_s *str, size_t size)
{
    if(!str) return false;

    bool result = true;
    result &= sstr_free(str);

    if(!(str->s = (uintptr_t)realloc(prvt_s(str), size)))
    {
        result = false;
    }
    else if(size > 0)
    {
        str->n = size - 1;
    }
    else
    {
        str->n = 0;
    }
    str->l = 0;

    return result;
}

/**
 * @brief swap two strings
 * @param a
 * @param b
 * @return false on error
 * @note using XOR-swap
 */
bool sstr_swap(sstr_s *a, sstr_s *b)
{
    if(!a || !b) return false;
    /* string */
    a->s ^= b->s;
    b->s ^= a->s;
    a->s ^= b->s;
    /* number of bytes */
    a->n ^= b->n;
    b->n ^= a->n;
    a->n ^= b->n;
    /* length */
    a->l ^= b->l;
    b->l ^= a->l;
    a->l ^= b->l;
    return true;
}

/**
 * @brief cut off leading and trailing whitespace
 * @param str
 * @return true on success (also when removing nothing), false on error
 */
bool sstr_cut_ws_both(sstr_s *str)
{
    bool result = true;
    result &= sstr_cut_ws_lead(str);
    result &= sstr_cut_ws_trail(str);
    return result;
}

/**
 * @brief cut off leading whitespace
 * @param str
 * @return true on success (also when removing nothing), false on error
 */
bool sstr_cut_ws_lead(sstr_s *str)
{
    if(!str || !str->l) return false;
    bool result = true;
    uint64_t i = 0;
    for(; i < str->l; i++)
    {
        if(!isspace(prvt_s(str)[i])) break;
    }
    if(i)
    {
        /* NOTE maybe use memmov? */
        sstr_s cut = {0};
        result &= sstr_set(&cut, "%s", &prvt_s(str)[i]);
        sstr_swap(str, &cut);
        sstr_free(&cut);
    }
    return result;
}

/**
 * @brief cut off trailing whitespace
 * @param str
 * @return true on success (also when removing nothing), false on error
 */
bool sstr_cut_ws_trail(sstr_s *str)
{
    if(!str || !str->l) return false;
    bool result = true;
    uint64_t i = str->l - 1;
    for(; i + 1 > 0; i--)
    {
        if(!isspace(prvt_s(str)[i])) break;
    }
    if(i + 1 < str->l)
    {
        /* NOTE maybe use memmov? */
        sstr_s cut = {0};
        prvt_s(str)[i + 1] = 0;
        result &= sstr_set(&cut, "%s", prvt_s(str));
        sstr_swap(str, &cut);
        sstr_free(&cut);
    }
    return result;
}

bool sstr_cut_set_both(sstr_s *str, char *set) /* cut off leading and trailing characters of a set */
{
    if(!str || !set || !str->l) return false;
    bool result = true;
    result &= sstr_cut_set_lead(str, set);
    result &= sstr_cut_set_trail(str, set);
    return result;
}

bool sstr_cut_set_lead(sstr_s *str, char *set) /* cut off leading characters of a set */
{
    if(!str || !set || !str->l) return false;
    bool result = true;
    uint64_t i = 0;
    int set_len = strlen(set);
    for(; i < str->l; i++)
    {
        bool found = false;
        for(int j = 0; j < set_len; j++)
        {
            if(prvt_s(str)[i] == set[j]) found = true;
        }
        if(!found) break;
    }
    if(i)
    {
        /* NOTE maybe use memmov? */
        sstr_s cut = {0};
        result &= sstr_set(&cut, "%s", &prvt_s(str)[i]);
        sstr_swap(str, &cut);
        sstr_free(&cut);
    }
    return result;
}

bool sstr_cut_set_trail(sstr_s *str, char *set) /* cut off trailing characters of a set */
{
    if(!str || !set || !str->l) return false;
    bool result = true;
    uint64_t i = str->l - 1;
    int set_len = strlen(set);
    for(; i + 1 > 0; i--)
    {
        bool found = false;
        for(int j = 0; j < set_len; j++)
        {
            if(prvt_s(str)[i] == set[j]) found = true;
        }
        if(!found) break;
    }
    if(i + 1 < str->l)
    {
        /* NOTE maybe use memmov? */
        sstr_s cut = {0};
        prvt_s(str)[i + 1] = 0;
        result &= sstr_set(&cut, "%s", prvt_s(str));
        sstr_swap(str, &cut);
        sstr_free(&cut);
    }
    return result;
}

/**
 * @brief cut from an index to an index
 * @param str => string to be cut
 * @param from => starting position (inclusive)
 * @param to => end position (exclusive). if 0 or less than from, cut to end
 * @return true => success
 *         false => internal error
 */
bool sstr_cut_from_to(sstr_s *str, uint64_t from, uint64_t to)     /* cut string from index to index */
{
    if(!str || str->l < from) return false;
    if(to <= from) to = str->l;
    if(to > str->l) to = str->l;
    prvt_s(str)[from] = 0;
    sstr_s mod = {0};
    if(!sstr_set(&mod, "%s%s", &prvt_s(str)[0], &prvt_s(str)[to])) return false;
    sstr_swap(&mod, str);
    sstr_free(&mod);
    return true;
}

/**
 * @brief convert string to int32_t number
 * @param str
 * @param num
 * @return false if string is not a number in the first place
 */
bool sstr_to_int32(sstr_s *str, int32_t *num)
{
    if(!str || !num || !str->l) return false;
    *num = 0; /* omit this to constantly add numbers */
    /* check for optional negative */
    bool negative = false;
    if(prvt_s(str)[0] == '-') negative = true;
    /* check if the user has entered a number in the first place */
    /* and simultaneously convert to integer */
    char *c = &prvt_s(str)[str->l - 1];
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
    /* if negative, invert */
    if(negative)
    {
        *num *= -1;
    }
    return true;
}

/**
 * @brief convert string to int32_t number
 * @param str
 * @param num
 * @return false if string is not a number in the first place
 */
bool sstr_to_uint32(sstr_s *str, uint32_t *num)
{
    if(!str || !num || !str->l) return false;
    *num = 0; /* omit this to constantly add numbers */
    /* check if the user has entered a number in the first place */
    /* and simultaneously convert to integer */
    char *c = &prvt_s(str)[str->l - 1];
    uint32_t factor = 1;
    for(int i = 0; i < str->l; i++)
    {
        if(*c < '0' || *c > '9')
        {
            return false;
        }
        *num += (factor * (*c-- - '0'));
        factor *= 10;
    }
    return true;
}

/**
 * @brief convert string to int64_t number
 * @param str
 * @param num
 * @return false if string is not a number in the first place
 */
bool sstr_to_int64(sstr_s *str, int64_t *num)
{
    if(!str || !num || !str->l) return false;
    *num = 0; /* omit this to constantly add numbers */
    /* check for optional negative */
    bool negative = false;
    if(prvt_s(str)[0] == '-') negative = true;
    /* check if the user has entered a number in the first place */
    /* and simultaneously convert to integer */
    char *c = &prvt_s(str)[str->l - 1];
    uint64_t factor = 1;
    for(int i = 1 * negative; i < str->l; i++)
    {
        if(*c < '0' || *c > '9')
        {
            return false;
        }
        *num += (factor * (*c-- - '0'));
        factor *= 10;
    }
    /* if negative, invert */
    if(negative)
    {
        *num *= -1;
    }
    return true;
}

/**
 * @brief convert string to int64_t number
 * @param str
 * @param num
 * @return false if string is not a number in the first place
 */
bool sstr_to_uint64(sstr_s *str, uint64_t *num)
{
    if(!str || !num || !str->l) return false;
    *num = 0; /* omit this to constantly add numbers */
    /* check if the user has entered a number in the first place */
    /* and simultaneously convert to integer */
    char *c = &prvt_s(str)[str->l - 1];
    uint64_t factor = 1;
    for(int i = 0; i < str->l; i++)
    {
        if(*c < '0' || *c > '9')
        {
            return false;
        }
        *num += (factor * (*c-- - '0'));
        factor *= 10;
    }
    return true;
}

/**
 * @brief compare two string for equality
 * @param a
 * @param b
 * @return true if equal, false if not (or error)
 */
bool sstr_cmp(sstr_s *a, sstr_s *b)
{
    if(!a || !b) return false; /* error case */
    /* compare the length of the strings */
    if(a->l != b->l) return false;
    /* compare each character */
    for(uint64_t i = 0; i < a->l; i++)
    {
        if(prvt_s(a)[i] != prvt_s(b)[i]) return false;
    }
    return true;
}

/**
 * @brief compare two string for equality
 * @param a
 * @param b
 * @return true if equal, false if not (or error)
 */
bool sstr_cmp_ci(sstr_s *a, sstr_s *b)
{
    if(!a || !b) return false; /* error case */
    /* compare the length of the strings */
    if(a->l != b->l) return false;
    /* compare each character */
    for(uint64_t i = 0; i < a->l; i++)
    {
        if(tolower(prvt_s(a)[i]) != tolower(prvt_s(b)[i])) return false;
    }
    return true;
}

bool sstr_cmp_ignr(sstr_s *a, sstr_s *b, char *set) /* compare two strings for equlity, ignoring any character inside a given set */
{
    if(!a || !b) return false; /* error case */
    if(!a->s && !b->s) return true;
    if(!a->s || !b->s) return false;
    /* compare each character */
    int set_len = 0;
    if(set) set_len = strlen(set);
    else if(a->l != b->l) return false; /* compare length (if set is empty) */
    /* compare each character */
    bool founda = false;
    bool foundb = false;
    for(uint64_t i = 0, j = 0; i <= a->l && j <= b->l; i++, j++)
    {
        founda = true;
        foundb = true;
        while((founda || foundb) && i < a->l && j < b->l)
        {
            founda = false;
            foundb = false;
            for(int k = 0; k < set_len; k++)
            {
                if(prvt_s(a)[i] == set[k]) founda = true; /* check if there is a matching character within the set and str a */
                if(prvt_s(b)[j] == set[k]) foundb = true; /* check if there is a matching character within the set and str a */
            }
            if(founda) i++;
            if(foundb) j++;
        }
        if(i < a->l && j < b->l)
        {
            if(prvt_s(a)[i] != prvt_s(b)[j]) return false;
        }
    }
    return true;
}

/* TODO fix comparing equal set characters? maybe...? */
bool sstr_cmp_ignr_ci(sstr_s *a, sstr_s *b, char *set) /* compare two strings for equlity, ignoring any character inside a given set */
{
    if(!a || !b) return false; /* error case */
    if(!a->s && !b->s) return true;
    if(!a->s || !b->s) return false;
    /* compare each character */
    int set_len = 0;
    if(set) set_len = strlen(set);
    else if(a->l != b->l) return false; /* compare length (if set is empty) */
    /* compare each character */
    bool founda = false;
    bool foundb = false;
    for(uint64_t i = 0, j = 0; i <= a->l && j <= b->l; i++, j++)
    {
        founda = true;
        foundb = true;
        while((founda || foundb) && i <= a->l && j <= b->l)
        {
            founda = false;
            foundb = false;
            for(int k = 0; k < set_len; k++)
            {
                if(tolower(prvt_s(a)[i]) == tolower(set[k])) founda = true; /* check if there is a matching character within the set and str a */
                if(tolower(prvt_s(b)[j]) == tolower(set[k])) foundb = true; /* check if there is a matching character within the set and str a */
            }
            if(founda) i++;
            if(foundb) j++;
        }
        if(i <= a->l && j <= b->l)
        {
            if(tolower(prvt_s(a)[i]) != tolower(prvt_s(b)[j])) return false;
        }
    }
    return true;
}

int sstr_count(sstr_s *str, char c) /* count nb. of characters that match c */
{
    if(!str) return 0;
    int result = 0;
    for(uint64_t i = 0; i <= str->l; i++)
        if(prvt_s(str)[i] == c) result++;
    return result;
}

/**
 * @brief find a matching character within a string
 * @param str => string to be searched within
 * @param c => char to be searched for
 * @param offs => searching offset
 * @param find => found index, pass NULL to ignore
 * @return true => found matching string
 *         false => found no matching string
 */
bool sstr_find_first_char(sstr_s *str, char c, uint64_t offs, uint64_t *find) /* find a matching character */
{
    if(!str) return 0;
    bool found = false;
    while(offs < str->l)
    {
        if(prvt_s(str)[offs] == c)
        {
            found = true;
            break;
        }
        offs++;
    }
    if(found && find) *find = offs;
    return found;
}

/**
 * @brief find a matching string within a string
 * @param str => string to be searched within
 * @param str2 => string to be searched for
 * @param offs => searching offset
 * @param find => found index, pass NULL to ignore
 * @return true => found matching string
 *         false => found no matching string
 */
bool sstr_find_first_sstr(sstr_s *str, sstr_s *str2, uint64_t offs, uint64_t *find)    /* find a matching string */
{
    if(!str || !str2) return false;
    bool found = false;
    uint64_t i = offs;
    uint64_t j = 0;
    while(i < str->l)
    {
        j = 0;
        while(j < str2->l)
        {
            if(prvt_s(str)[i + j] == prvt_s(str2)[j]) j++;
            else break;
        }
        if(j != str2->l) i++;
        else
        {
            found = true;
            break;
        }
    }
    if(found && find) *find = i;
    return found;
}

/**
 * @brief 
 * @param str
 * @param split
 * @param c
 * @return resulting amount of entries
 */
int sstr_split(sstr_s *str, sstr_s **split, char c) /* split strings on each character that equals c */
{
    if(!str || !split) return 0;
    /* figure out total count of matching characters to only make one allocation */
    bool result = true;
    int count = sstr_count(str, c) + 1;
    if(!count) return 0;
    *split = realloc(*split, sizeof(sstr_s) * count);
    if(!*split) return false;
    /* split */
    int k = 0;
    uint64_t i = 0; /* start */
    uint64_t j = 0; /* end */
    for(;;)
    {
        /* search matching character */
        i = j;
        if(sstr_find_first_char(str, c, i, &j))
        {
            if(j <= str->l && i < str->l)
            {
                /* copy portion of string */
                prvt_s(str)[j] = 0;
                /*            printf("copying: %s\n", &prvt_s(str)[i]); */
                result &= sstr_init(&(*split)[k]);
                result &= sstr_set(&(*split)[k], "%s", &prvt_s(str)[i]);
                prvt_s(str)[j++] = c;
                k++;
            }
        }
        else break;
    }
    /* TODO handle result */
    return k;
}

#undef prvt_s /*(x) ((char *)(x)->s) */
