#define __PROGRAMM__ "askue-port"

#ifndef _POSIX_SOURCE
    #define _POSIX_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h> /* Стандартные функции Unix */
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <ctype.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <libaskue.h>





#define apFlagEof 0
#define apFlagError 1
#define apFlagEnter 2

#define apOperationReceive 0
#define apOperationSet 1
#define apOperationUnset 2
#define apOperationVerify 3
#define apOperationApply 4
#define apOperationTransmit 5
#define apOperationPrint 6
#define apOperationDefault 7
#define apOperationConfig 8

#define apTypeHex 1
#define apTypeChar 2

#define apError -1
#define apWarning -2
#define apSuccess 0

/* Функции отладки */

void  view_content ( uint8_array_t *u8a, int type )
{
    printf ( "Отладка: буфер:" );
    
    if ( type == apTypeHex )
    {
        for ( size_t i = 0; i < u8a->Size; i++ )
        {
            printf ( "%.2x ", u8a->Item[ i ] );
        }
    }
    else if ( type == apTypeChar )
    {
        for ( size_t i = 0; i < u8a->Size; i++ )
        {
            if ( isalnum ( u8a->Item[ i ] ) || ispunct ( u8a->Item[ i ] ) )
            {
                printf ( "%c", u8a->Item[ i ] );
            }
            else if ( u8a->Item[ i ] == '\r' )
            {
                printf ( "\\r" );
            }
            else if ( u8a->Item[ i ] == '\n' )
            {
                printf ( "\\n" );
            }
        }
    }
    else
    {
        printf ( "неизвестный тип данных буфера." );
    }
    
    puts ( "" );
    
    fflush ( stdout );
}

void view_name ( const char *Name )
{
    printf ( "Отладка: имя: %s\n", Name );
}

/* --------------- */ 

/* Метки и функции работы с ними */
typedef struct _ap_label_t
{
    char *Name;
    uint8_array_t *Content;
    int Type;
} ap_label_t;

typedef struct _ap_label_list_t
{
    ap_label_t *Label;
    void *Next;
} ap_label_list_t;
 
// создать новую метку
ap_label_t* new_ap_label ( char *_Name, uint8_array_t *_Content, int _Type )
{
    ap_label_t *L = malloc ( sizeof ( ap_label_t ) );
    
    if ( L == NULL )
    {
        perror ( "Ошибка malloc()." );
        exit ( EXIT_FAILURE );
    }
    
    L->Name = _Name;
    L->Content = _Content;
    L->Type = _Type;
    
    return L;
}

// удалить метку
void delete_ap_label ( ap_label_t *_L )
{
    if ( _L->Name != NULL ) free ( _L->Name );
    if ( _L->Content != NULL ) 
    {
        uint8_array_destroy ( _L->Content );
        free ( _L->Content );
    }
    free ( _L );
}

// новый элемент списка меток
ap_label_list_t* new_ap_label_list ( ap_label_t *Label )
{
    ap_label_list_t *LL = malloc ( sizeof ( ap_label_list_t ) );
    
    if ( LL == NULL )
    {
        perror ( "Ошибка malloc()." );
        exit ( EXIT_FAILURE );
    }
    
    LL->Label = Label;
    LL->Next = NULL;
    
    return LL;
}

// удалить элемент списка меток
ap_label_list_t* delete_ap_label_list ( ap_label_list_t *LabelList )
{
    ap_label_list_t *RetVal = (ap_label_list_t*)LabelList->Next;
    delete_ap_label ( LabelList->Label );
    free ( LabelList );
    return RetVal;
}

// Добавить элемент списка в голову
ap_label_list_t* push_ap_label_list ( ap_label_list_t **Head, ap_label_list_t *Src )
{
    Src->Next = *Head;
    *Head = Src;
    return Src;
}

// Убрать элемент списка из головы
ap_label_list_t* pop_ap_label_list ( ap_label_list_t **Head )
{
    ap_label_list_t *RetVal = *Head;
    *Head = RetVal->Next;
    return RetVal;
}

ap_label_list_t* find_ap_label_list_by_name ( ap_label_list_t *Dest, const char *Name )
{
    ap_label_list_t *Item = Dest;
    while ( Item != NULL && 
             strncmp ( Item->Label->Name, Name, strlen ( Name ) ) )
    {
        Item = Item->Next;
    }
    return Item;
}


ap_label_list_t* find_ap_label_list_by_next ( ap_label_list_t *Dest, void *Next )
{
    ap_label_list_t *Item = Dest;
    while ( Item != NULL && 
             Item->Next != Next )
    {
        Item = Item->Next;
    }
    return Item;
}

// Добавить элемент списка в голову
int remove_ap_label_list ( ap_label_list_t **Head, const char *Name )
{
    ap_label_list_t *Item = find_ap_label_list_by_name ( *Head, Name );
    if ( Item == NULL )
        return apWarning;
    ap_label_list_t *Prev = find_ap_label_list_by_next ( *Head, Item );
    if ( Prev != NULL )
        Prev->Next = delete_ap_label_list ( Item );
    else
        *Head = delete_ap_label_list ( Item );
        
    return apSuccess;
}
 
/* ------------------------------ */

typedef struct _ap_buffer_t
{
    uint8_array_t *Content;
    int Type;
} ap_buffer_t;

/*  обработчики состояний машины  */
typedef struct _ap_config_t
{
    askue_port_t *Port;
    char *Str;
    int Operation;
    ap_buffer_t *Buffer;
    long int Timeout;
    ap_label_list_t *List;
} ap_config_t;

uint32_t gFlags;


// проверить тип данных
int __check_type ( const char *str )
{
    while ( *str == ' ' ) str++;
    
    if ( isxdigit ( str[ 0 ] ) &&
         isxdigit ( str[ 1 ] ) &&
         str[ 2 ] == ' ' )
    {
        return apTypeHex;
    }
    else if ( ( isalnum ( str[ 0 ] ) || ispunct ( str[ 0 ] ) ) &&
               ( isalnum ( str[ 1 ] ) || ispunct ( str[ 1 ] ) ) &&
               ( isalnum ( str[ 2 ] ) || ispunct ( str[ 2 ] ) ) )
    {
        return apTypeChar;
    }
    else
    {
        return apWarning;
    }
}

void set_content ( uint8_array_t *u8a, const char *str, int type )
{
    uint8_array_destroy ( u8a );
    
    if ( type == apTypeHex )
    {
        char *Next = ( char* ) str;
        char *End = ( char* ) str + strlen ( str );

        do {
            long int li = strtol ( Next, &Next, 16 );
            uint8_t u8 = ( uint8_t )li;
            uint8_array_append ( u8a, ( uint8_t[] ) { u8 }, 1 );
        } while ( Next != End );
    }
    else if ( type == apTypeChar )
    {
        size_t len = strlen ( str ), i = 0;
        while ( i < len )
        {
            if ( str[ i ] == '\\' && str[ i + 1 ] == 'r' )
            {
                uint8_array_append ( u8a, ( uint8_t[] ) { '\r' }, 1 );
                i += 2; 
            }
            else if ( str[ i ] == '\\' && str[ i + 1 ] == 'n' )
            {
                uint8_array_append ( u8a, ( uint8_t[] ) { '\n' }, 1 );
                i += 2;
            }
            else
            {
                uint8_array_append ( u8a, ( uint8_t[] ) { str[ i ] }, 1 );
                i += 1;
            }
        }
    }
}

// выход из программы
void signal_handler ( int s )
{
    SETBIT ( gFlags, apFlagEof );
    puts ( "\nЗавершение работы!" );
    exit ( EXIT_SUCCESS );
}

int is_stdin_ready ( void )
{
    struct timeval Timeout = { 0, 10000 };
        
    fd_set ReadSet;
    FD_ZERO ( &ReadSet );
    FD_SET ( fileno ( stdin ), &ReadSet );
    
    int RetVal = select ( fileno ( stdin ) + 1, &ReadSet, NULL, NULL, &Timeout );
    
    if ( RetVal == 0 )
    {
        return 0;
    }
    else if ( RetVal == -1 )
    {
        return apError;
    }
    else
    {
        assert ( FD_ISSET ( fileno ( stdin ), &ReadSet ) );
        return 1;
    }
}

void mystrcat ( char **dest, const char *src, size_t Amount )
{
    size_t S = ( *dest != NULL ) ? strlen ( *dest ) : 0;
    char *buf = realloc ( *dest, S + Amount + 1 );
    
    if ( buf == NULL )
    {
        perror ( "Ошибка realloc()." );
        free ( dest );
        exit ( EXIT_FAILURE );
    }
    
    while ( Amount > 0 )
    {
        buf[ S ] = *src;
        S++;
        src++;
        Amount--;
    }
    
    buf[ S ] = '\0';
    *dest = buf;
}

// прочитать и записать в конец str
int __read_substring ( char **str, size_t Amount )
{
    char Buffer[ Amount ];
    memset ( Buffer, '\0', Amount );
    int i = 0, c;
    while ( ( ( c = fgetc ( stdin ) ) != EOF ) &&
             ( c != '\n' ) &&   
             ( i < Amount ) )
    {
        Buffer[ i ] = ( char ) c;
        i++;
    }
    
    if ( c != EOF )
    {
        mystrcat ( str, Buffer, Amount );
        return 0;
    }
    else if ( ferror ( stdin ) )
    {
        return -1;
    } 
    else
    {
        return 1;
    }
}

// прочитать все байты которые есть на stdin
int __read_string ( char **str )
{
    int Amount = 0;
    ioctl ( fileno ( stdin ), FIONREAD, &Amount );
    if( Amount > 0 ) //есть доступные символы
    {
        return __read_substring ( str, Amount );
    }
    else
    {
        return fgetc ( stdin ) == EOF;
    }
}
/*
int is_new_line ( const char *str )
{
    int Result;
    while ( ( Result = *str == '\n' ) == 0 )
        str++;
    return Result;
}
*/
// чтение строки
// str надо освободить с помощью free
int read_string ( ap_config_t *apCfg )
{
    UNSETBIT ( apCfg->Flag, apFlagEnter );
    UNSETBIT ( apCfg->Flag, apFlagEof );
    UNSETBIT ( apCfg->Flag, apFlagError );
    
    //myfree ( apCfg->Str );
    if ( apCfg->Str != NULL )
    {
        free ( apCfg->Str );
        apCfg->Str = NULL;
    }
    apCfg->Str = readline ( "<<<:" );
    add_history ( apCfg->Str );
    stifle_history ( 5 );
    
    if ( apCfg->Str == NULL )
        SETBIT( apCfg->Flag, apFlagEof );
    
    return ( TESTBIT (  apCfg->Flag, apFlagEof ) ) ? apWarning : apSuccess;
    
    /*
    UNSETBIT ( apCfg->Flag, apFlagEnter );
    UNSETBIT ( apCfg->Flag, apFlagEof );
    UNSETBIT ( apCfg->Flag, apFlagError );
    
    apCfg->Str = NULL;
    
    while ( !TESTBIT( apCfg->Flag, apFlagEnter ) &&
             !TESTBIT( apCfg->Flag, apFlagError ) &&
             !TESTBIT( apCfg->Flag, apFlagEof ) )
    {
        int Ready = is_stdin_ready ();
        if ( Ready == -1 )
        {
            perror ( "Ошибка проверки stdin." );
            SETBIT( apCfg->Flag, apFlagError );
        }
        else if ( Ready == 1 )
        {
            int Read = __read_string ( &( apCfg->Str ) );
            if ( Read == 0 )
            {
                SETBIT ( apCfg->Flag, apFlagEnter );
            }
            else if ( Read == -1 )
            {
                SETBIT( apCfg->Flag, apFlagError );
            }
            else
            {
                SETBIT( apCfg->Flag, apFlagEof );
            }
        }
    }
    
    return ( TESTBIT ( apCfg->Flag, apFlagError ) ) ? apError :
            ( TESTBIT (  apCfg->Flag, apFlagEof ) ) ? apWarning : apSuccess;
    */
}

void stdin_invitation ( void )
{
    putc ( '<', stdout );
    putc ( '<', stdout );
    putc ( '<', stdout );
    putc ( ':', stdout );
    fflush ( stdout );
}

void stdout_invitation ( void )
{
    putc ( '>', stdout );
    putc ( '>', stdout );
    putc ( '>', stdout );
    putc ( ':', stdout );
    fflush ( stdout );
}

/*
 * Приём команд пользователя
 */
int __func_OperationReceive ( ap_config_t *apCfg )
{
    return read_string ( apCfg );
}

// печать символа конца строки и сброс буфера потока вывода
int __print_endl ( void )
{
    if ( putc ( '\n', stdout ) == '\n' &&
         fflush ( stdout ) == 0 )
        return apSuccess;
    else
        return apError;
}

// печать двоичных байт в hex-формате
int __func_OperationPrint_Hex ( ap_config_t *apCfg )
{
    uint8_array_t *Buf = &( apCfg->Content );
    int R = 0;
    
    for ( size_t i = 0; i < Buf->Size && !R; i++ )
    {
        R = printf ( "%.2x ", Buf->Item[ i ] ) != 3;
    }
    
    if ( R ) 
        return apError;
    else
        return __print_endl();
    
}

// печать символов
int __func_OperationPrint_Char ( ap_config_t *apCfg )
{
    uint8_array_t *Buf = &( apCfg->Content );
    int R = 0;
    
    for ( size_t i = 0; i < Buf->Size && !R; i++ )
    {
        if ( isalnum ( Buf->Item[ i ] ) || ispunct ( Buf->Item[ i ] ) )
        {
            R = printf ( "%c", Buf->Item[ i ] ) != 1;
        }
        else if ( Buf->Item[ i ] == '\r' )
        {
            R = printf ( "\\r" ) != 2;
        }
        else if ( Buf->Item[ i ] == '\n' )
        {
            R = printf ( "\\n" ) != 2;
        }
    }
    
    if ( R ) 
        return apError;
    else
        return __print_endl();
}

/*
 * Печать вывода
 */
int __func_OperationPrint ( ap_config_t *apCfg )
{   
    if ( apCfg->Type == apTypeHex )
    {
        stdout_invitation ();
        return __func_OperationPrint_Hex ( apCfg );
    }
    else if ( apCfg->Type == apTypeChar )
    {
        stdout_invitation ();
        return __func_OperationPrint_Char ( apCfg );
    }
    
    return apError;
}


/*
 * передача в порт
 */
int __func_OperationTransmit ( ap_config_t *apCfg )
{
    // передача фрейма
    if ( port_write ( &( apCfg->Port ), &( apCfg->Content ) ) == -1 )
    {
        perror ( "Ошибка передачи данных." );
        return apError;
    }
    // очистка буфера
    // приём фрейма
    if ( port_read ( &( apCfg->Port ), &( apCfg->Content ), apCfg->Timeout / 2 ) == -1 )
    {
        perror ( "Ошибка приёма данных." );
        return apError;
    }
    uint8_array_t u8a;
    uint8_array_init ( &u8a, 0 );
    if ( port_read ( &( apCfg->Port ), &u8a, apCfg->Timeout / 2 ) == -1 )
    {
        perror ( "Ошибка приёма данных." );
        return apError;
    }
    uint8_array_append ( &( apCfg->Content ), u8a.Item, u8a.Size );
    uint8_array_destroy ( &u8a );
    
    return apSuccess;
}
 
/*
 * добавить метку в список
 */
int __func_OperationSet ( ap_config_t *apCfg )
{
    char *NamePtr = apCfg->Str + 1;
    while ( *NamePtr == ' ' ) NamePtr++;
    
    // поиск начала фрейма
    char *ContentPtr = strchr ( NamePtr, ':' );
    if ( ContentPtr == NULL )
    {
        puts ( "Предупреждение. Запрос на добавление пустой метки." );
        return apWarning;
    }
    
    // копирование имени
    char *Name = mymalloc ( sizeof ( char ) * ( ContentPtr - NamePtr + 1 ) );
    memset ( Name, '\0', ( size_t ) ( ContentPtr - NamePtr + 1 ) );
    strncpy ( Name, NamePtr, ContentPtr - NamePtr );
    
    // поиск действительного начала фрейма
    ContentPtr++;
    while ( *ContentPtr == ' ' ) ContentPtr++;

    // определение типа
    int Type = __check_type ( ContentPtr );
    if ( Type == apWarning ) 
    {
        free ( Name );
        puts ( "Предупреждение. Неизвестный тип данных." );
        return apWarning;
    }

    // копирование содержимого
    uint8_array_t *Content = mymalloc ( sizeof ( uint8_array_t* ) );
    uint8_array_init ( Content, 0 );
    set_content ( Content, ContentPtr, Type );

    // поиск метки
    ap_label_list_t *Old = find_ap_label_list_by_name ( apCfg->List, Name );
    if ( Old == NULL )
    {
        // добавить метку в список
        ap_label_t *L = new_ap_label ( Name, Content, Type );
        ap_label_list_t *LL = new_ap_label_list ( L );
        push_ap_label_list ( &(apCfg->List), LL );
    }
    else
    {
        // изменить метку
        delete_ap_label ( Old->Label );
        Old->Label = new_ap_label ( Name, Content, Type );
    }
    
    return apSuccess;
}

// удалить метку из списка
int __func_OperationUnset ( ap_config_t *apCfg )
{
    // найти начало имени метки
    char *NamePtr = apCfg->Str + 1;
    while ( *NamePtr == ' ' ) NamePtr++;
    if ( remove_ap_label_list ( &( apCfg->List ), NamePtr ) == apWarning )
        printf ( "Предупреждение. Метка с именем <%s> - не найдена.\n", NamePtr );
    else
        printf ( "Успех. Метка с именем <%s> - удалена.\n", NamePtr );
        
    return apSuccess;
}


/*
 * Распознание символов для передачи в порт
 */
int __func_OperationDefault ( ap_config_t *apCfg )
{
    int Type = __check_type ( apCfg->Str );
    
    if ( Type == -2 ) return apWarning;
    apCfg->Type = Type;

    char *str = apCfg->Str;
    while ( *str == ' ' ) str++;
    
    set_content ( &( apCfg->Content ), str, apCfg->Type );
    
    return apSuccess;
}

/*
 * Вызвать метку
 */
int __func_OperationApply ( ap_config_t *apCfg )
{
    char *Name = apCfg->Str + 1;
    while ( *Name == ' ' ) Name++;
    
    ap_label_list_t *LL = find_ap_label_list_by_name ( apCfg->List, Name );
    
    if ( LL == NULL )
    {
        puts ( "Предупреждение. Метка не найдена." );
        return apWarning;
    }
    
    uint8_array_update ( &( apCfg->Content ), LL->Label->Content->Item, LL->Label->Content->Size );
    
    apCfg->Type = LL->Label->Type;
    // view_content ( &( apCfg->Content ), apCfg->Type );
    // view_name ( LL->Label->Name );
    
    return apSuccess;
}

/* ------------------------------ */

/* вывод команды в hex-виде */
int __func_OperationVerify ( ap_config_t *apCfg )
{
    stdout_invitation();
    return __func_OperationPrint_Hex ( apCfg );
}


// распознавание операции
int __what_operation ( const char *str )
{
    return ( str[ 0 ] == '+' ) ? apOperationSet :
            ( str[ 0 ] == '-' ) ? apOperationUnset :
            ( str[ 0 ] == '?' ) ? apOperationVerify :
            ( str[ 0 ] == '!' ) ? apOperationApply : apOperationDefault;
}

/*      Имитация декораторов питона     */
static
void DECORATE__func_OperationReceive ( ap_config_t *apCfg )
{
    int R = __func_OperationReceive ( apCfg );
    if ( R == apError ) // обработчик
    {
        SETBIT ( apCfg->Flag, apFlagError );
    }
    else if ( R == apSuccess )
    {
        apCfg->Operation = __what_operation ( apCfg->Str );
    }
}

/* ------------------------------ */

/*     Цикл работы программы      */  

void REPL ( ap_config_t *apCfg )
{
    switch ( apCfg->Operation )
    {
        // приём данных пользователя
        case apOperationReceive:
            DECORATE__func_OperationReceive ( apCfg );
            break;
        // вывод данных на экран
        case apOperationPrint:
            if ( __func_OperationPrint ( apCfg ) == apError ) // обработчик
            {
                SETBIT ( apCfg->Flag, apFlagError );
            }
            apCfg->Operation = apOperationReceive;
            break;
        // передача данных в порт
        case apOperationTransmit:
            if ( __func_OperationTransmit ( apCfg ) == apError ) // обработчик
            {
                SETBIT ( apCfg->Flag, apFlagError );
            }
            apCfg->Operation = apOperationPrint; // вывести данные на экран
            break;
        // '+'
        case apOperationSet:
            if ( __func_OperationSet ( apCfg ) == apError )
            {
                SETBIT ( apCfg->Flag, apFlagError );
            }
            apCfg->Operation = apOperationReceive;
            break;
        // '-'
        case apOperationUnset:
            if ( __func_OperationUnset ( apCfg ) == apError )
            {
                SETBIT ( apCfg->Flag, apFlagError );
            }
            apCfg->Operation = apOperationReceive;
            break;
        // '?'
        case apOperationVerify:
            // puts ( "Редактирование пока недоступно." );
            __func_OperationVerify ( apCfg );
            apCfg->Operation = apOperationReceive;
            break;
        // '!'
        case apOperationApply:
            if ( __func_OperationApply ( apCfg ) == apSuccess )
                apCfg->Operation = apOperationTransmit;
            else
                apCfg->Operation = apOperationReceive;
            break;
        // '.'
        case apOperationConfig:
            puts ( "Конфигурирование пока недоступно." );
            // __func_OperationConfig ( apCfg );
            apCfg->Operation = apOperationReceive;
            break;
        // данные для передачи введены на прямую
        default:
            if ( __func_OperationDefault ( apCfg ) == apError )
            {
                SETBIT ( apCfg->Flag, apFlagError );
            }
            else
            {
                apCfg->Operation = apOperationTransmit;
            }
            break;
    }
    
}

/* ------------------------------ */

// установить аргумент - файл порта
int __cli_handler_port ( void *ptr, int *flag, const char *arg )
{
    *( const char** )ptr = arg;
    return 0;
}

// установить аргумент - скорость обмена через порт
int __cli_handler_port ( void *ptr, int *flag, const char *arg )
{
    *( const char** )ptr = arg;
    return 0;
}

int __cli_handler_timeout ( void *ptr, int *flag, const char *arg )
{
    *( long int* )ptr = strtol ( arg, NULL, 10 );
    return 0;
}

/*    Инициализация конфига     */
int ap_init ( ap_config_t *ap_cfg, int argc, char **argv )
{
    ap_cfg->Str = NULL;
    ap_cfg->Flag = 0;
    ap_cfg->List = NULL;
    ap_cfg->Buffer = mymalloc ( sizeof ( ap_buffer_t ) );
    ap_cfg->Buffer->Content = mymalloc ( sizeof ( uint8_array_t ) );
    uint8_array_init ( AP_Cfg->Content, 0 );
    ap_cfg->Buffer->Type = apTypeNo;
    ap_cfg->Operation = apOperationReceive;
    
    //askue_port_cfg_t Cfg;
    const char *port_args[];
    #define PORT_FILE 0
    #define PORT_SPEED 1
    #define PORT_DBITS 2
    #define PORT_SBITS 3
    #define PORT_PARITY 4
    
    cli_option_t CliOption[] =
    {
        { "port-file", 'f', CLI_REQUIRED_ARG, __cli_handler_port_file, &( port_args[ PORT_FILE ] ), NULL },
        { "port-speed", 's', CLI_REQUIRED_ARG, __cli_handler_port, &( port_args[ PORT_SPEED ] ), NULL },
        { "port-dbits", 'd', CLI_REQUIRED_ARG, __cli_handler_port, &( port_args[ PORT_DBITS ] ), NULL },
        { "port-sbits", 'b', CLI_REQUIRED_ARG, __cli_handler_port, &( port_args[ PORT_SBITS ] ), NULL },
        { "port-parity", 'p', CLI_REQUIRED_ARG, __cli_handler_port, &( port_args[ PORT_PARITY ] ), NULL },
        { "timeout", 't', CLI_REQUIRED_ARG, __cli_handler_timeout, &( AP_Cfg.Timeout ), NULL },
        CLI_LAST_OPTION
    };
    
    cli_result_t CliResult = cli_parse ( CliOption, argc, argv );
    if ( CliResult != CLI_SUCCESS )
    {
        puts ( "Ошибка разбора аргументов." );
        return -1;
    }
    else
    {
        port_init ( &(AP_Cfg.Port), port_args[ PORT_FILE ], 
                                    port_args[ PORT_SPEED ], 
                                    port_args[ PORT_DBITS ], 
                                    port_args[ PORT_SBITS ], 
                                    port_args[ PORT_PARITY ] );
        return 0;
    }
    #undef PORT_FILE
    #undef PORT_SPEED
    #undef PORT_DBITS
    #undef PORT_SBITS
    #undef PORT_PARITY
}
/*  --------------------------  */

/*    Интерфейс взаимодействия с пользователем      */
static
int ap_interface ( ap_config_t ap_cfg; )
{
    
}


/*    Точка входа в программу     */

int main(int argc, char **argv)
{
    // все флаги сняты
    gFlags = 0;
    // конфигурация проги
    ap_config_t ap_cfg;
    // разбор аргументов командной строки
    if ( ap_init ( &ap_cfg, argc, argv ) )
        return -1;
    // настройка сигналов на завершение
    signal ( SIGINT, signal_handler );
    signal ( SIGQUIT, signal_handler ); 
    // старт интерфейса
    while ( ( ap_interface ( &ap_cfg ) == apSuccess ) &&
             ( ap_logic ( &ap_cfg ) == apSuccess ) );
             
    ap_goodbuy ();
    ap_destroy ( &ap_cfg );
    /*
    while ( !TESTBIT ( AP_Cfg.Flag, apFlagError ) &&
             !TESTBIT ( AP_Cfg.Flag, apFlagEof ) )
    {
        REPL ( &AP_Cfg );
    }
    
    if ( TESTBIT ( AP_Cfg.Flag, apFlagEof ) )
        puts ( "\nЗавершение работы!" );
    
        
    ap_label_list_t *Now = AP_Cfg.List;        

    while ( Now != NULL )
    {
        ap_label_list_t *Next = Now->Next;
        delete_ap_label_list ( Now );
        Now = Next;
        if ( Now != NULL ) Next = Now->Next;
    }
    
    port_destroy ( &(AP_Cfg.Port ) );
    */
    
	return 0;
}

