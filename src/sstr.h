#ifndef SSTR_H

/**********************************************************
 * @category includes                                     *
 **********************************************************/

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

/**********************************************************
 * @category PREPROCESSOR                                 *
 **********************************************************/

#define SSTR_BATCH   64

#if SSTR_BATCH <= 0
#error "SSTR_BATCH must be greater than zero"
#endif

/**********************************************************
 * @category structures                                   *
 **********************************************************/

typedef struct
{
    uint64_t l; /* length of string */
    uint64_t n; /* bytes of memory */
    uintptr_t s;    /* the string */
}
sstr_s;

/**********************************************************
 * @category prototypes                                   *
 **********************************************************/

bool sstr_append(sstr_s *str, char *format, ...);                 /* append a string */
bool sstr_append_va(sstr_s *str, char *format, va_list argp);     /* append a string and a corresponding va_list to a string (normally don't call outside) */
bool sstr_free(sstr_s *str);                                      /* free a string and or clean up */
bool sstr_set(sstr_s *str, char *format, ...);                    /* set a string to another string */
char *sstr_get(sstr_s *str);                                      /* returns address of string (e.g. for printing) */
bool sstr_init(sstr_s *str);                                      /* initialize (every) string to all 0s */
bool sstr_init_p(sstr_s **str);                                   /* initialize string pointer */
bool sstr_realloc(sstr_s *str, size_t size);                      /* reallocate a string's memory (handled internally; normally don't call outside) */

bool sstr_swap(sstr_s *a, sstr_s *b);                              /* swap two strings */

bool sstr_cut_ws_both(sstr_s *str);                               /* cut off leading and trailing whitespace */
bool sstr_cut_ws_lead(sstr_s *str);                               /* cut off leading whitespace */
bool sstr_cut_ws_trail(sstr_s *str);                              /* cut off trailing whitespace */
bool sstr_cut_set_both(sstr_s *str, char *set);                   /* cut off any leading and trailing characters of a set */
bool sstr_cut_set_lead(sstr_s *str, char *set);                   /* cut off any leading characters of a set */
bool sstr_cut_set_trail(sstr_s *str, char *set);                  /* cut off any trailing characters of a set */
bool sstr_cut_from_to(sstr_s *str, uint64_t from, uint64_t to);   /* cut string from index to index */

/* replace */
/* also for match? */
/* also for set match?... */

bool sstr_to_int32(sstr_s *str, int32_t *num);                    /* convert string to 32 bit signed integer */
bool sstr_to_uint32(sstr_s *str, uint32_t *num);                  /* convert string to 32 bit unsigned integer */
bool sstr_to_int64(sstr_s *str, int64_t *num);                    /* convert string to 64 bit signed integer */
bool sstr_to_uint64(sstr_s *str, uint64_t *num);                  /* convert string to 64 bit unsigned integer */

bool sstr_cmp(sstr_s *a, sstr_s *b);                               /* compare two strings for exact equality */
bool sstr_cmp_ci(sstr_s *a, sstr_s *b);                            /* compare two strings for equality, case insensitive */
bool sstr_cmp_ignr(sstr_s *a, sstr_s *b, char *set);               /* compare two strings for equality, ignoring any character inside a given set */
bool sstr_cmp_ignr_ci(sstr_s *a, sstr_s *b, char *set);            /* compare two strings for equality, case insensitive, ignoring any character inside a given  */

int sstr_count(sstr_s *str, char c);                              /* count nb. of characters that match c */
bool sstr_find_first_char(sstr_s *str, char c, uint64_t offs, uint64_t *find); /* find a matching character */
bool sstr_find_first_sstr(sstr_s *str, sstr_s *str2, uint64_t offs, uint64_t *find);    /* find a matching string */

int sstr_split(sstr_s *str, sstr_s **split, char c);               /* split strings on each character that equals c */

#define SSTR_H
#endif
