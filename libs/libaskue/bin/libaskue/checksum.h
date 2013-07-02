/*
 * checksum.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_CHECKSUM_H_
#define ASKUE_CHECKSUM_H_


#include <stdint.h>
#include <string.h>

#include "uint8_array.h"
#include "macro.h"

/*
 * Проверка контрольной суммы
 */
bool_t valid_checksum ( const uint8_t *data, size_t len, const uint8_array_t *checksum,  
                        uint8_array_t* ( *cs ) ( const uint8_t*, size_t size ),
                        uint8_array_t* ( *cs_order ) ( uint8_array_t* ) );

/*
 * Дописывание контрольной суммы в массив байт
 */
uint8_array_t* append_checksum ( uint8_array_t *ptr, 
                                 uint8_array_t* ( *cs ) ( const uint8_t*, size_t size ),
                                 uint8_array_t* ( *cs_order ) ( uint8_array_t* ) );

#endif /* ASKUE_CHECKSUM_H_ */
