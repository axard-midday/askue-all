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
cli_arg_type_t get_argtype ( const char *arg )
{
    if ( arg[ 0 ] == '-' )
    {
        if ( isalpha ( arg[ 1 ] ) &&
              ( arg[ 2 ] == '\0' ) )
        {
            return CLI_SHORT_ARG;
        }
        else if ( ( arg[ 1 ] == '-' ) && 
                   ( arg[ 2 ] != '\0' ) )
        {
            return CLI_LONG_ARG;
        }
        else
        {
            return CLI_NO_ARG;
        }
    }
    else
    {
        return CLI_NO_ARG;
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
int cmp_shortname ( cli_arg_t *Arg, int Name )
{
    return Arg->ShortName == Name;
}

int cmp_longname ( cli_arg_t *Arg, const char *Name )
{
    int R = 1;
    const char *ArgName = Arg->LongName;
    while ( *ArgName != '\0' && *ArgName != '=' && *Name != '\0' && R )
    {
        R = *ArgName == *Name;
        ArgName ++;
        Name ++;
    }
    return R;
}



static
cli_arg_t* find_arg_by_longname ( cli_arg_t *Args, const char *LongName )
{
    int i = 0;
    cli_arg_t *Result = NULL;
    
    while ( !is_last_arg ( Args + i ) && Result == NULL )
    {
        if ( cmp_longname ( Args + i, LongName ) )
            Result = Args + i;
        
        i++;
    }
    
    return Result;
}

static
cli_arg_t* find_arg_by_shortname ( cli_arg_t *Args, int ShortName )
{
    int i = 0;
    cli_arg_t *Result = NULL;
    
    while ( !is_last_arg ( Args + i ) && Result == NULL )
    {
        if ( cmp_shortname ( Args + i, ShortName ) )
            Result = Args + i;
        
        i++;
    }
    
    return Result;
}

// поиск аргумента
static
cli_arg_t* find_arg ( cli_arg_t *Args, const char *ArgName )
{
    cli_arg_t *Arg;
    // определение длинны аргумента
    switch ( get_argtype ( argv [ i ] )
    {
        case CLI_LONG_ARG:
            Arg = find_arg_by_longname ( Args, ArgName + 2 );
            break;
        case CLI_SHORT_ARG:
            Arg = find_arg_by_shortname ( Args, ArgName[ 1 ] );
            break;
        default:
            Arg = NULL;
            break;
    }
    
    return Arg;
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

// обработка аргумента CLI с обязательным значением
static
cli_result_t parse_req_val ( cli_arg_t *Arg, cli_arg_type_t ArgType, int *i, int argc, char **argv )
{
    int _i = *i;
    
    const char *StrArgVal = NULL;
    const char *StrArgName = argv[ _i ];
    
    if ( ArgType == CLI_SHORT_ARG )
    {
        // разбор короткой опции
        StrArgVal = get_short_opt_arg ( i, argc, argv );
    }
    else
    {
        // разбор длинной опции
        StrArgVal = get_long_arg_val ( StrArgName );
        ( *i )++;
    }  
     
    return ( StrArgVal != NULL ) ? eval_arg_handler ( Arg, ArgType, StrArgVal ) 
                                  : CLI_ERROR_NOVAL;
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

// кол-во необходимых для ввода аргументов
static
size_t get_need_arg_amount ( cli_arg_t *Args )
{
    size_t Amount = 0;
    
    while ( *Args != CLI_LAST_ARG )
    {
        if ( (*Args).ArgNeed == CLI_REQUIRED_ARG )
            Amount ++;
            
        Args ++;
    }
    
    return Amount;
}

// найти

cli_result_t cli_parse ( cli_arg_t *Args, int argc, char** argv )
{
    cli_result_t Result = CLI_SUCCESS;
    // номер аргумента из полученного списка
    int i = 1;
    // кол-во необходимых аргументов
    size_t NeedArgAmount = get_need_arg_amount ( Args );

    while ( ( i < argc ) && ( Result == CLI_SUCCESS ) )
    {
        cli_arg_t *Arg = find_arg ( Args, argv[ i ] );
        if ( Arg == NULL )
        {
            Result = CLI_ERROR_ARGTYPE
        }
        else
        {
            if ( Arg->ArgNeed == CLI_REQUIRED_ARG )
                NeedArgAmount--;
            
            const char *Val;
            if ( Arg->ValNeed != CLI_NO_VAL )
            {
                Val = find_val ( argv[ i ] );
                Result = ( Arg->Handler ( Arg->Var, Val ) == 0 ) ? CLI_SUCCESS : CLI_ERROR_HANDLER;
            }
            else
            {
                Val = NULL;
                Result = ( Arg->Handler ( Arg->Var, Val ) == 0 ) ? CLI_SUCCESS : CLI_ERROR_HANDLER;
            }
        }
        
        i++;
    }
    
    return ( !NeedArgAmount ) ? Result : CLI_ERROR_NOARG;
}


cli_arg_t *Arg;
            if ( is_long_arg ( 
            find_arg_by_longname ( Args, ArgType, argv[ i ] );
            // если найден необходимый аргумент, то уменьшить их кол-во
            if ( Arg->ArgNeed == CLI_REQUIRED_ARG )
                NeedArgAmount--;
            // обработка аргумента
            switch ( Arg->ValNeed )
            {
                case CLI_REQUIRED_VAL: 
                    Result = get_req_val ( Arg, OpType, &i, argc, argv ); 
                    break;
                case CLI_OPTIONAL_VAL: 
                    Result = eval_option_oarg ( Opt, OpType, &i, argc, argv ); 
                    break;
                case CLI_NO_VAL: 
                    Result = eval_option_noarg ( Opt, OpType, argv[ i ] );
                    i++;
                    break;
                default: 
                    Result = CLI_ERROR_ARG;
                    break;
            }
