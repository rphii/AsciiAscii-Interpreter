#ifndef SIO_H

/**********************************************************
 * @category INCLUDES                                     *
 **********************************************************/

#include <stdint.h>


/**********************************************************
 * @category ADDITIONAL DEPENDENCIES                      *
 **********************************************************/

#include "sstr.h"


/**********************************************************
 * @category PUBLIC FUNCTION PROTOTYPES                   *
 **********************************************************/

uint32_t sio_file_read(sstr_s *str, char *filename);
uint32_t sio_file_write(sstr_s *str, char *filename);
bool sio_user_get_str(sstr_s *str);
bool sio_user_get_int32(int32_t *num, bool repeat);
bool sio_user_get_uint32(uint32_t *num, bool repeat);

#define SIO_H
#endif
