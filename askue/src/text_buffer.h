#ifndef ASKUE_TEXT_BUFFER_H_
#define ASKUE_TEXT_BUFFER_H_

#include <stdint.h>

#include "my.h"

typedef struct
{
    char *Text;
    size_t Size;
} text_buffer_t;

// инициализация текстового буфера
void text_buffer_init ( text_buffer_t *Buffer, size_t size );

// удаление выделенной на него памяти
#define text_buffer_destroy( _pBuffer_ ) myfree ( (_pBuffer_)->Text )

// запись в буфер
int text_buffer_write ( text_buffer_t *Buffer, const char *fmt, ... );

// добавление в конец буфера
int text_buffer_append ( text_buffer_t *Buffer, const char *fmt, ... );


#endif /* ASKUE_TEXT_BUFFER_H_ */
