#include <stdlib.h>
#include <string.h>

/*                      Обёртка malloc                                */
void* mymalloc ( size_t size )
{
    void *ptr = malloc ( size );
    if ( ptr == NULL )
        exit ( EXIT_FAILURE );
    return ptr;
}

/*                      Обёртка free                                  */
void myfree ( void *ptr )
{
    if ( ptr != NULL ) 
    {
        free ( ptr );
    }
}

/*                      Обёртка strdup                                */
char* mystrdup ( const char *src )
{
    char *dest = strdup ( src );
    if ( dest == NULL )
        exit ( EXIT_FAILURE );
    return dest;
}

/*                      Обёртка strndup                               */
char* mystrndup ( const char *src, size_t n )
{
    char *dest = strndup ( src, n );
    if ( dest == NULL )
        exit ( EXIT_FAILURE );
    return dest;
}
