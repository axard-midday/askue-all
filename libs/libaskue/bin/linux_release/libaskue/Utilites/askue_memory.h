#ifndef ASKUE_MEMORY_H_
#define ASKUE_MEMORY_H_ 1

/*
 * Обёртка для malloc()
 */
void* askue_malloc ( size_t size );

/*
 * Обёртка realloc()
 */
void* askue_realloc ( void *ptr, size_t size);

/*
 * Обёртка free()
 */
void askue_free ( void *ptr );

#endif
