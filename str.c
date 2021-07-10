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

#include "str.h"

#define __s(x)  ((char *)(x)->s)

bool str_append_va(str_t *str, char *format, va_list argp)
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

bool str_realloc(str_t *str, size_t size)
{
    if(!str) return false;
    
    bool result = true;
    result &= str_free(str);
    
    if(!(str->s = (uintptr_t)realloc(__s(str), size)))
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

bool str_cmp(str_t *a, str_t *b)
{
    if(!a || !b) return false;  // error case
    // compare the length of the strings
    if(a->l != b->l) return false;
    // compare each character
    for(uint64_t i = 0; i < a->l; i++)
    {
        if(__s(a)[i] != __s(b)[i]) return false;
    }
    return true;
}


#undef __s//(x) ((char *)(x)->s)