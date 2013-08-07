#include <libaskue.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "text_buffer.h"

void text_buffer_init ( text_buffer_t *Buffer, size_t size )
{
    Buffer->Size = size;
    Buffer->Text = mymalloc ( sizeof ( char ) * size );
    memset ( Buffer->Text, '\0', Buffer->Size );
}

int text_buffer_write ( text_buffer_t *Buffer, const char *fmt, ... ) 
{
    va_list argv;
    va_start ( argv, fmt );
    
    int result = vsnprintf ( Buffer->Text, Buffer->Size, fmt, argv );
    
    va_end ( argv );
    
    return result;
}

int text_buffer_append ( text_buffer_t *Buffer, const char *fmt, ... ) 
{
    size_t length = strlen ( Buffer->Text );
    
    va_list argv;
    va_start ( argv, fmt );
    
    int result = vsnprintf ( Buffer->Text + length, Buffer->Size - length, fmt, argv );
    va_end ( argv );
    
    return result;
}
