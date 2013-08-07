#include "my.h"
#include "script_stream.h"

void script_stream_new ( script_stream_t **ScriptStream )
{
    (*ScriptStream) = mymalloc ( sizeof ( script_stream_t ) );
}

void script_stream_delete ( script_stream_t *ScriptStream )
{
    myfree ( ScriptStream );
    // тест
}
