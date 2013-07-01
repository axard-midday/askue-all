#include <stdlib.h>
#include "log_error.h"


/*
 * Обёртка для malloc()
 */
void* askue_malloc ( size_t size )
{
        void *ptr = malloc ( size );
        
        if ( ptr == NULL )
        {
                exit ( EE_MALLOC );
        } 
        
        return ptr;
}

/*
 * Обёртка realloc()
 */
void* askue_realloc ( void *ptr, size_t size)
{
        void *nptr = realloc ( ptr, size );
        
        if ( nptr == NULL )
        {
                exit ( EE_REALLOC );
        }
        
        return nptr;
}
 
/*
 * Обёртка free()
 */
void askue_free ( void *ptr )
{
        if ( ptr ) free ( ptr );
}
