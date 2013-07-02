/*
 * checksum.c
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */
#include "checksum.h"

/*
 * Функции проверки
 */
bool_t valid_checksum ( const uint8_t *data, size_t len, const uint8_array_t *checksum,  
                        uint8_array_t* ( *cs ) ( const uint8_t*, size_t size ),
                        uint8_array_t* ( *cs_order ) ( uint8_array_t* ) )
{
	//вычислить контрольную сумму
	uint8_array_t *tmp_checksum;
	
	if ( cs_order != NULL )
	{
		tmp_checksum = cs_order ( cs ( data, len ) );
	}
	else
	{
		if ( cs != NULL )
		{
			tmp_checksum = cs ( data, len );
		}
		else
		{
			return FALSE;
		}
	}
	
	//сравнить
	bool_t result = ( !memcmp ( checksum -> data, tmp_checksum -> data, checksum -> len ) ) ? TRUE : FALSE ;

	uint8_array_delete ( tmp_checksum );

	return result;
}

/*
 * Функция дописывания контрольных сумм
 */

uint8_array_t* append_checksum ( uint8_array_t *ptr, 
                                 uint8_array_t* ( *cs ) ( const uint8_t*, size_t size ),
                                 uint8_array_t* ( *cs_order ) ( uint8_array_t* ) )
{
	/*
	 * Просчитать контрольную сумму и выставить байты в порядке передачи
	 */
	uint8_array_t* checksum = NULL;
	
	if ( cs_order != NULL)
    		checksum = ( cs != NULL ) ? cs_order ( cs ( ptr -> data, ptr -> len ) ) : NULL;
	else
		checksum = ( cs != NULL ) ? cs ( ptr -> data, ptr -> len ) : NULL;

	uint8_array_t* result = uint8_array_append_u8a ( ptr, checksum );

	uint8_array_delete ( checksum );

	return result;
}













