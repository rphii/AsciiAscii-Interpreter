#include "sio.h"

/**
 * @brief read a file
 * @param buf
 * @param filename
 * @return
 */
uint32_t sio_file_read(sstr_s *str, char *filename)
{
    /* open file */
    FILE *file = fopen(filename, "rb");
    if(!file) return 0;

    /* get file length */
    fseek(file, 0, SEEK_END);
    size_t bytes_file = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* allocate memory */
    if(!sstr_realloc(str, bytes_file + 1)) return 0;
    size_t read = bytes_file; /* sizeof(*str->s); */

    /* read file */
    uint32_t bytes_read = fread((char *)str->s, sizeof(char), read, file);
    str->l = bytes_read;
    ((char *)str->s)[bytes_read] = 0;

    /* close file */
    fclose(file);

    return bytes_read;
}

/**
 * @brief
 * @param buf
 * @param filename
 * @return
 */
uint32_t sio_file_write(sstr_s *str, char *filename)
{
    /* open file */
    FILE *file = fopen(filename, "wb");
    if(!file) return 0;

    /* write file */
    uint32_t result = fwrite((char *)str->s, sizeof(char), str->n, file);

    /* close file */
    fclose(file);

    return result;
}

/**
 * @brief request a string from the user
 * @param str
 * @return true if success, false if failed or if nothing was entered
 */
bool sio_user_get_str(sstr_s *str)
{
    char c = 0;
    bool result = true;
    sstr_set(str, "");
    while((c = getchar()) != '\n' && c != EOF) { result &= sstr_append(str, "%c", c); }
    if(!str->l && (!c || c == EOF || c == '\n')) { return false; }
    return result;
}

/**
 * @brief request an int32 number from the user
 * @param num => pointer to number
 * @param require => true to force the user to enter a number
 * @return true if user entered a number. false if user left it blank or there was no number or if an error occured.
 */
bool sio_user_get_int32(int32_t *num, bool require)
{
    sstr_s str = { 0 };
    bool result = true;
    do
    {
        if(sio_user_get_str(&str))
        {
            result = sstr_to_int32(&str, num);
            require = !result;
        }
        else
        {
            result = false;
        }
    } while(require);
    sstr_free(&str);
    return result;
}

/**
 * @brief request a uint32 number from the user
 * @param num => pointer to number
 * @param require => true to force the user to enter a number
 * @return true if user entered a number. false if user left it blank or there was no number or if an error occured.
 */
bool sio_user_get_uint32(uint32_t *num, bool require)
{
    sstr_s str = { 0 };
    bool result = true;
    do
    {
        if(sio_user_get_str(&str))
        {
            if((result = sstr_to_uint32(&str, num))) { require = !result; }
        }
        else
        {
            result = false;
        }
    } while(require);
    sstr_free(&str);
    return result;
}
