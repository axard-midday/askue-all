/*
 * checksum.c
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */
#include "checksum.h"

/*
 * Функции расчёта контрольных сумм
 */
uint8_array_t* modbus_crc16 ( const uint8_t *data, size_t len )
{
    uint16_t crc_16 = 0xffff;

    for ( size_t i = 0; i < len; i++ )
    {
        crc_16 ^= data[i];
        for ( size_t j = 0; j < 8; j++ )
		{
            if( crc_16 & 0x0001 )
                crc_16 = ( ( crc_16 >> 1 ) & 0x7fff) ^ 0xa001;
            else
                crc_16 = ( crc_16 >> 1 ) & 0x7fff;
		}
    }

	uint8_array_t *result = uint8_array_new ( 2 );

	result -> data[ 0 ] = GET_BYTE ( crc_16, 1 );

	result -> data[ 1 ] = GET_BYTE ( crc_16, 0 );

    return result;
}

/*
 * Простая контрольная сумма
 */
uint8_array_t* simple_checksum ( const uint8_t *data, size_t len)
{
    	uint32_t crc = 0;

 	for ( size_t i = 0; i < len; i++ )
       		crc += data[i];

    	crc &= 0xff;
	
	uint8_array_t *result = uint8_array_new ( 1 );

	result -> data[ 0 ] = ( uint8_t ) crc;

    	return result;
}

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

	uint8_array_t* result = uint8_array_append_ba ( ptr, checksum );

	uint8_array_delete ( checksum );

	return result;
}













