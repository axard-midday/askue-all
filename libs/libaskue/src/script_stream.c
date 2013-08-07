#include "my.h"
#include "script_stream.h"

script_stream_new ( script_stream_t **ScriptStream )
{
    (*ScriptStream) = mymalloc ( sizeof ( script_stream_t ) );
}

script_stream_new ( script_stream_t *ScriptStream )
{
    myfree ( ScriptStream );
    // тест
}
