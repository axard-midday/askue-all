#include "my.h"
#include "cli.h"
#include "macro.h"
#include "script_argument.h"

void script_arg_new ( script_arg_t **Arg )
{
    (*Arg) = mymalloc ( sizeof ( script_arg_t ) );
    (*Arg)->Port = mymalloc ( sizeof ( port_arg_t ) );
    (*Arg)->Journal = mymalloc ( sizeof ( journal_arg_t ) );
    (*Arg)->Device = mymalloc ( sizeof ( device_arg_t ) );
    (*Arg)->Log = mymalloc ( sizeof ( log_arg_t ) );
}

void script_arg_delete ( script_arg_t *Arg )
{
    myfree ( Arg->Log );
    myfree ( Arg->Device );
    myfree ( Arg->Journal );
    myfree ( Arg->Port );
    myfree ( Arg );
}

/*  обработчики опций   */
// копирование указателя на аргумент
static
int __hndl_1 ( void *ptr, int *flag, const char *arg )
{
    *( const char** )ptr = arg;
    return 0;
}

// установка флага ведения протокола
static
int __hndl_protocol ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_PROTOCOL );
    return 0;
}

// установка флага "многословности"
static
int __hndl_verbose ( void *ptr, int *flag, const char *arg )
{
    SETBIT ( ( *( uint32_t* ) ptr ), ASKUE_FLAG_VERBOSE );
    return 0;
}


// инициализировать набор аргументов
int script_arg_init ( script_arg_t *Arg, int argc, char **argv )
{
    cli_option_t CliOpt[] =
    {
        { "port_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port->File), NULL },
        { "port_dbits", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port->DBits), NULL },
        { "port_sbits", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port->SBits), NULL },
        { "port_parity", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port->Parity), NULL },
        { "port_speed", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Port->Speed), NULL },
        { "device", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device->Name), NULL },
        { "timeout", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device->Timeout), NULL },
        { "parametr", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Device->Parametr), NULL },
        { "log_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Log->File), NULL },
        { "journal_file", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Journal->File), NULL },
        { "journal_flashback", 0, CLI_REQUIRED_ARG, __hndl_1, &(Arg->Journal->Flashback), NULL },
        { "protocol", 0, CLI_REQUIRED_ARG, __hndl_protocol, &(Arg->Flag), NULL },
        { "verbose", 0, CLI_REQUIRED_ARG, __hndl_verbose, &(Arg->Flag), NULL },
        CLI_LAST_OPTION
    };
    
    if ( cli_parse ( CliOpt, argc, argv ) != CLI_SUCCESS )
        return -1;
    else
        return 0;
}
