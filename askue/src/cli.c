#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>

#include "cli.h"

// выделить длинную опцию
static
size_t cli_get_long_option_len ( const char *argv )
{
    size_t len = strlen ( argv ) - 1;
    while ( len > 0 && argv[ len ] != '=' )
    {
        len--;
    }
    len -= 2; // отсечь знак "--"
    
    return len;
}

// тип опции
static
cli_option_type_t get_optype ( const char *arg )
{
    if ( arg[ 0 ] == '-' )
    {
        if ( isalpha ( arg[ 1 ] ) &&
              ( arg[ 2 ] == '\0' ) )
        {
            return CLI_SHORT_OPT;
        }
        else if ( ( arg[ 1 ] == '-' ) && 
                   ( arg[ 2 ] != '\0' ) )
        {
            return CLI_LONG_OPT;
        }
        else
        {
            return CLI_NO_OPT;
        }
    }
    else
    {
        return CLI_NO_OPT;
    }
}

// получить аргумент для короткой опции
static
const char* get_short_opt_arg ( int *index, int argc, char **argv )
{
    int _i = *index;
    if ( ( _i + 1 ) < argc &&
         get_optype ( argv[ _i + 1 ] ) == CLI_NO_OPT )
    {
        *index += 2;
        
        return argv[ _i + 1 ];
    }
    else
    {
        *index += 1;
        return NULL;
    }
}

// получить аргумент для длинной опции
static
const char* get_long_opt_arg ( const char *argv )
{
    const char *argument = strchr ( argv, '=' );
    
    if ( argument != NULL )
    {
        return argument + 1;
    }
    else
    {
        return argument;
    }
}

// проверка что опция последняя
static
int is_last_option ( cli_option_t *Option )
{
    return Option->longname == NULL &&
            Option->shortname == 0 &&
            Option->has_arg == 0 &&
            Option->handler == NULL &&
            Option->outvar == NULL &&
            Option->flag == NULL;
}

// проверка опции
static
int is_equal_option ( cli_option_t *Opt, cli_option_type_t OType, const char *option )
{
    if ( OType == CLI_SHORT_OPT )
    {
        return Opt->shortname == option[ 1 ];
    }
    else
    {
        size_t len = cli_get_long_option_len ( option );
        return ( strncmp ( Opt->longname, option + 2, len ) ) ? 0 : 1;
    }
}

// поиск опции
static
cli_option_t* find_option ( cli_option_t *Opt, cli_option_type_t OType, const char *option )
{
    int i = 0;
    cli_option_t *Result = NULL;
    
    while ( !is_last_option ( &Opt[ i ] ) && Result == NULL )
    {
        if ( is_equal_option ( &Opt[ i ], OType, option ) )
            Result = Opt + i;
        
        i++;
    }
    
    return Result;
}

static 
cli_result_t eval_option ( cli_option_t *Opt, cli_option_type_t OType, const char *option, const char *argument )
{ 
    if ( Opt == NULL )
    {
        return CLI_ERROR_NF_OPT;
    }
    
    return ( !Opt->handler ( Opt->outvar, Opt->flag, argument ) ) ? CLI_SUCCESS 
                                                                   : CLI_ERROR_HANDLER;
}

// обработка опции CLI с обязательным аргументом
static
cli_result_t eval_option_rarg ( cli_option_t *Opt, cli_option_type_t OpType, int *i, int argc, char **argv )
{
    int _i = *i;
    
    const char *argument = NULL;
    const char *option = argv[ _i ];
    
    if ( OpType == CLI_SHORT_OPT )
    {
        // разбор короткой опции
        argument = get_short_opt_arg ( i, argc, argv );
    }
    else
    {
        // разбор длинной опции
        argument = get_long_opt_arg ( argv[ _i ] );
        ( *i )++;
    }  
     
    return ( argument != NULL ) ? eval_option ( Opt, OpType, option, argument ) 
                                 : CLI_ERROR_NOARG;
}

// обработка опции CLI с опциональным аргументом
static
cli_result_t eval_option_oarg ( cli_option_t *Option, cli_option_type_t OpType, int *i, int argc, char **argv )
{
    int _i = *i;
    
    const char *argument = NULL;
    char *option = argv[ _i ];
    
    cli_result_t Result;
    if ( OpType == CLI_SHORT_OPT )
    {
        // разбор короткой опции
        argument = get_short_opt_arg ( i, argc, argv ); 
    }
    else
    {
        // разбор длинной опции
        argument = get_long_opt_arg ( argv[ _i ] );
        ( *i )++;
    }
    
    Result = eval_option ( Option, OpType, option, argument );
    
    return Result;
}

// обработка опции CLI с отсутствующим аргументом
static
cli_result_t eval_option_noarg ( cli_option_t *Opt, cli_option_type_t OpType, const char *option )
{
    return eval_option ( Opt, OpType, option, NULL );
}

cli_result_t cli_parse ( cli_option_t *Option, int argc, char** argv )
{
    cli_result_t Result = CLI_SUCCESS;
    int i = 1;

    while ( ( i < argc ) && ( Result == CLI_SUCCESS ) )
    {
        cli_option_type_t OpType = get_optype ( argv [ i ] );
        
        if ( OpType == CLI_NO_OPT )
        {
            Result = CLI_ERROR_OPTYPE;
        }
        else 
        {
            cli_option_t *Opt = find_option ( Option, OpType, argv[ i ] );
            switch ( Opt->has_arg )
            {
                case CLI_REQUIRED_ARG: 
                    Result = eval_option_rarg ( Opt, OpType, &i, argc, argv ); 
                    break;
                case CLI_OPTIONAL_ARG: 
                    Result = eval_option_oarg ( Opt, OpType, &i, argc, argv ); 
                    break;
                case CLI_NO_ARG: 
                    Result = eval_option_noarg ( Opt, OpType, argv[ i ] );
                    i++;
                    break;
                default: 
                    Result = CLI_ERROR_ARG;
                    break;
            }
        }
    }
    
    return Result;
}
