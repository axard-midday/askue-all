#include "my.h"
#include "script_argument.h"

void script_arg_init ( script_arg_t *SArg )
{
    SArg->Log = mymalloc ( sizeof ( log_arg_t ) );
    SArg->Port = mymalloc ( sizeof ( port_arg_t ) );
    SArg->Device = mymalloc ( sizeof ( device_arg_t ) );
    SArg->Journal = mymalloc ( sizeof ( journal_arg_t ) );
    SArg->Parametr = mymalloc ( sizeof ( parametr_arg_t ) );
}   

int script_arg_set ( script_arg_t *SArg, int argc, char **argv )
{
    
}

void script_arg_destroy ( script_arg_t *SArg )
{
    myfree ( SArg->Log );
    myfree ( SArg->Port );
    myfree ( SArg->Device );
    myfree ( SArg->Journal );
    myfree ( SArg->Parametr );
}
