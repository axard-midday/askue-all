#include "uint8_array.h"
#include "bool.h"
#include "askue_memory.h"
#include <stdint.h>
#include <string.h>

uint8_array_t* uint8_array_new ( size_t size )
{
	uint8_array_t* ptr = ( uint8_array_t* ) askue_malloc ( sizeof ( uint8_array_t ) + size * sizeof ( uint8_t ) );
	ptr -> len = size;
		
	if ( size )
	{
		memset ( ptr -> data, 0, size );
    }
	
	return ptr;
}

uint8_array_t* uint8_array_delete ( uint8_array_t* ptr )
{
	askue_free ( ptr );
	
	return NULL;
}

uint8_array_t* uint8_array_resize ( uint8_array_t* ptr, size_t size )
{
	uint8_array_t *tmp = ( uint8_array_t* ) askue_realloc ( ptr, sizeof ( uint8_array_t ) + size * sizeof( uint8_t ) );
	tmp -> len = size;
	
	return tmp;
}

uint8_array_t* uint8_array_append_data ( uint8_array_t* dest, const uint8_t *data, size_t data_size )
{
	size_t old_len = dest -> len;
	uint8_array_t* tmp_dest = uint8_array_resize ( dest, dest -> len + data_size );
        memcpy ( tmp_dest -> data + old_len, data, data_size );
	
	return tmp_dest;
}

uint8_array_t* uint8_array_append_u8a ( uint8_array_t* dest, const uint8_array_t* src )
{
	if ( src != NULL )
		return uint8_array_append_data ( dest, src -> data, src -> len );
	else
		return dest;
}

uint8_array_t* uint8_array_update_data ( uint8_array_t *dest, const uint8_t *data, size_t data_size )
{
	dest = uint8_array_resize ( dest, data_size );
	
	memcpy ( dest -> data, data, data_size );
	
	return dest;
}

uint8_array_t* uint8_array_update_u8a ( uint8_array_t *dest, const uint8_array_t *src )
{
	if ( src != NULL )
		return uint8_array_update_data ( dest, src -> data, src -> len );
	else
		return dest;
}

bool_t uint8_array_compare_eq ( const uint8_array_t *u8a_1 ,const uint8_array_t *u8a_2 )
{
    if ( u8a_1->len != u8a_2->len )
    {
        return FALSE;
    }
    else
    {
        return memcmp ( u8a_1->data, u8a_2->data, u8a_1->len ) == 0;
    }
}


















