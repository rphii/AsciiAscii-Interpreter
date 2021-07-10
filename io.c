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

#include "io.h"

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

uint32_t io_file_read(str_t *str, char *filename)
{
    // open file
    FILE *file = fopen(filename, "rb");
    if(!file) return 0;
    
    // get file length
    fseek(file, 0, SEEK_END);
    size_t bytes_file = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // allocate memory
    if(!str_realloc(str, bytes_file + 1)) return 0;
    size_t read = bytes_file;// / sizeof(*str->s);
    
    // read file
    uint32_t bytes_read = fread((char *)str->s, sizeof(char), read, file);
    str->l = bytes_read;
    ((char *)str->s)[bytes_read] = 0;
    
    // close file
    fclose(file);
    
    return bytes_read;
}