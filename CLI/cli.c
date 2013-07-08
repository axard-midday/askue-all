#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>


typedef int ( *cli_handler_f )( void *ptr, int *flag, const char *arg );

typedef enum 
{
    CLI_SHORT_OPT = 1,
    CLI_LONG_OPT = 2,
    CLI_NO_OPT = 3
} cli_option_type_t;

typedef enum 
{
    CLI_REQUIRED_ARG = 1,
    CLI_OPTIONAL_ARG = 2,
    CLI_NO_ARG = 3
} cli_argument_type_t;

typedef struct _cli_option_t
{
    const char *longname;
    char shortname;
    cli_argument_type_t has_arg;
    cli_handler_f handler;
    void *outvar;
    int *flag;
} cli_option_t;

#define CLI_LAST_OPTION ((cli_option_t){ NULL, 0, 0, NULL, NULL, NULL })

typedef enum
{
    CLI_SUCCESS = 0,
    CLI_ERROR_NOARG = 1, // нет требуемого за опцией аргумента
    CLI_ERROR_ARG = 2, // неправильный аргумент
    CLI_ERROR_NF_OPT = 3, // в списке cli_option_t* не найдено опций
    CLI_ERROR_HANDLER = 4, // ошибка обработчика
    CLI_ERROR_OPTYPE = 5 // неправильно указана опция
} cli_result_t;

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
        printf ( "Есть аргумент для короткой опции: %s\n", argv[ _i + 1 ] );
        *index += 2;
        
        return argv[ _i + 1 ];
    }
    else
    {
        return NULL;
    }
}

// получить аргумент для длинной опции
static
const char* get_long_opt_arg ( const char *argv )
{
    const char *argument = strchr ( argv, '=' );
    
    if ( argument != NULL )
        return argument + 1;
    else
        return argument;
}

// проверка что опция последняя
static
int is_last_option ( cli_option_t Option )
{
    return Option.longname == NULL &&
            Option.shortname == 0 &&
            Option.has_arg == 0 &&
            Option.handler == NULL &&
            Option.outvar == NULL &&
            Option.flag == NULL;
}

// проверка опции
static
int is_equal_option ( cli_option_t Opt, cli_option_type_t OType, const char *option )
{
    if ( OType == CLI_SHORT_OPT )
    {
        return Opt.shortname == option[ 1 ];
    }
    else
    {
        size_t len = cli_get_long_option_len ( option );
        return !!( strncmp ( Opt.longname, option + 2, len ) );
    }
}

// поиск опции
static
cli_option_t* find_option ( cli_option_t *Opts, cli_option_type_t OType, const char *option )
{
    int i = 0;
    cli_option_t *Result = NULL;
    
    while ( !is_last_option ( Opts[ i ] ) && Result == NULL )
    {
        if ( is_equal_option ( Opts[ i ], OType, option ) )
            Result = Opts + i;
        
        i++;
    }
    
    return Result;
}

static 
cli_result_t eval_option ( cli_option_t *Opts, cli_option_type_t OType, const char *option, const char *argument )
{
    cli_option_t *Opt = find_option ( Opts, OType, option );
    
    if ( Opt == NULL )
    {
        return CLI_ERROR_NF_OPT;
    }
    
    return ( !Opt->handler ( Opt->outvar, Opt->flag, argument ) ) ? CLI_SUCCESS 
                                                                   : CLI_ERROR_HANDLER;
}

// обработка опции CLI с обязательным аргументом
static
cli_result_t eval_option_rarg ( cli_option_t *Opts, cli_option_type_t OpType, int *i, int argc, char **argv )
{
    int _i = *i;
    
    const char *argument = NULL;
    const char *option = argv[ _i ];
    
    if ( OpType == CLI_SHORT_OPT )
    {
        printf ( "Короткая опция\n" );
        // разбор короткой опции
        argument = get_short_opt_arg ( i, argc, argv );
    }
    else
    {
        printf ( "Длинная опция\n" );
        // разбор длинной опции
        argument = get_long_opt_arg ( argv[ _i ] );
        ( *i )++;
    }  
     
    return ( argument != NULL ) ? eval_option ( Opts, OpType, option, argument ) 
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
        printf ( "Короткая опция\n" );
        // разбор короткой опции
        argument = get_short_opt_arg ( i, argc, argv ); 
    }
    else
    {
        printf ( "Длинная опция\n" );
        // разбор длинной опции
        argument = get_long_opt_arg ( argv[ _i ] );
        ( *i )++;
    }
    
    Result = eval_option ( Option, OpType, option, argument );
    
    return Result;
}

// обработка опции CLI с отсутствующим аргументом
static
cli_result_t eval_option_noarg ( cli_option_t *Opts, cli_option_type_t OpType, const char *option )
{
    return eval_option ( Opts, OpType, option, NULL );
}

cli_result_t cli_parse ( cli_option_t *Option, int argc, char** argv )
{
    cli_result_t Result = CLI_SUCCESS;
    int i = 1;
    fputs ( "Старт разбора аргументов\n", stdout );
    
    while ( ( i < argc ) && ( Result == CLI_SUCCESS ) )
    {
        printf ( "Разбор: %d = %s\n", i, argv[ i ] );
        cli_option_type_t OpType = get_optype ( argv [ i ] );
        
        if ( OpType == CLI_NO_OPT )
        {
            Result = CLI_ERROR_OPTYPE;
        }
        else switch ( Option[ i ].has_arg )
        {
            case CLI_REQUIRED_ARG: 
                printf ( "Требуется аргумент\n" );
                Result = eval_option_rarg ( Option, OpType, &i, argc, argv ); 
                break;
            case CLI_OPTIONAL_ARG: 
                printf ( "Возможен аргумент\n" );
                Result = eval_option_oarg ( Option, OpType, &i, argc, argv ); 
                break;
            case CLI_NO_ARG: 
                printf ( "Не требуется аргумент\n" );
                Result = eval_option_noarg ( Option, OpType, argv[ i ] );
                i++;
                break;
            default: 
                printf ( "Ошибка\n" );
                Result = CLI_ERROR_ARG;
                break;
        }
        
    }
    
    return Result;
}

/*
typedef struct _cli_option_t
{
    const char *longname;
    char shortname;
    cli_argument_type_t has_arg;
    cli_handler_f handler;
    void *outvar;
    int *flag;
} cli_option_t;
* 
* 
*  
typedef int ( *cli_handler_f )( void *ptr, int *flag, const char *arg );
*/

typedef struct
{
    int age;
    const char *name;
    int sex;
    const char *country;
} person_t;

int person_set_age ( void *ptr, int *flag, const char *arg )
{
    *( int* ) ptr = ( int ) strtol ( arg, NULL, 10 );
    
    return 0;
}

int person_set_name ( void *ptr, int *flag, const char *arg )
{
    ( *( const char** ) ptr ) = arg;
    
    return 0;
}

int person_set_malesex ( void *ptr, int *flag, const char *arg )
{
    *( int* ) ptr = 1;
    
    return 0;
}

int person_set_femalesex ( void *ptr, int *flag, const char *arg )
{
    *( int* ) ptr = 0;
    
    return 0;
}

int person_set_country ( void *ptr, int *flag, const char *arg )
{
    if ( arg != NULL )
    {
        ( *( const char** ) ptr ) = arg;
    }
    else
    {
        ( *( const char** ) ptr ) = "RF";
    }
    
    return 0;
}

int main(int argc, char **argv)
{
    printf ( "%d\n", argc );
    
    person_t SomeBody;
    
	cli_option_t CliOpts[] = 
    {
        { "age", 'a', CLI_REQUIRED_ARG, person_set_age, &( SomeBody.age ), NULL },
        { "name", 'n', CLI_REQUIRED_ARG, person_set_name, &( SomeBody.name ), NULL },
        { "male", 'm', CLI_NO_ARG, person_set_malesex, &( SomeBody.sex ), NULL },
        { "female", 'f', CLI_NO_ARG, person_set_femalesex, &( SomeBody.sex ), NULL },
        { "country", 'c', CLI_OPTIONAL_ARG, person_set_country, &( SomeBody.country ), NULL },
        CLI_LAST_OPTION
    };
    
    int R = cli_parse ( CliOpts, argc, argv );
    
    if ( R != CLI_SUCCESS )
    {
        printf ( "error: %d\n", R );
        return 1;
    }
    
    printf ( "Person history:\n" );
    printf ( "Name: %s", SomeBody.name );
    printf ( "Age: %d", SomeBody.age );
    printf ( "Sex: %s", ( SomeBody.sex ) ? "male" : "female" );
    printf ( "Country: %s", SomeBody.country );
    
	return 0;
}

