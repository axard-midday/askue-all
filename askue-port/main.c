#ifndef _POSIX_SOURCE
    #define _POSIX_C_SOURCE 201308L
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

#ifndef _AP_TBUFFER_LENGTH
    #define _AP_TBUFFER_LENGTH 1024
#endif
#ifndef _AP_HISTORY_LENGTH
    #define _AP_HISTORY_LENGTH 5
#endif

#define ap_g_PROMPT "<<<:"

typedef enum _ap_state_t
{
    ap_StateRead,           // Читать команду от пользователя
    ap_StatePrint,          // Печать полученных данных
    ap_StateCheckOp,        // Определение типа операции
    ap_StateExecOp,         // Выполнить операцию
    ap_StateExit,           // Завершить работу
    ap_StatePortIO          // Приёмо-передача фрейма по RS232
} ap_state_t;

typedef enum _ap_exit_t
{
    ap_Exit,
    ap_Continue
} ap_exit_t;

typedef enum _ap_error_t
{
    ap_Error,
    ap_Success
} ap_error_t;

typedef enum _ap_operation_t
{
    ap_OpNo,            // Нет операции
    ap_OpTransmit,       // передать данные
    ap_OpSetLabel,      // "+"
    ap_OpUnsetLabel,    // "-"
    ap_OpApplyLabel,    // "!"
    ap_OpHelp           // "?"
} ap_operation_t;

typedef enum _ap_content_type_t
{
    ap_ContentChar,
    ap_ContentHex,
    ap_ContentNo
} ap_content_type_t;

/* Метки */
typedef struct _ap_label_t
{
    char             *Name;
    uint8_array_t     *Content;
    int               Type;
} ap_label_t;

typedef struct _ap_label_list_t
{
    ap_label_t  *Label;
    void         *Next;
} ap_label_list_t;

typedef struct _ap_hex_buffer_t
{
    uint8_array_t           *Content;
    ap_content_type_t     Type;
} ap_hex_buffer_t;

typedef struct _ap_text_buffer_t
{
    char Text[ _AP_TBUFFER_LENGTH ];
} ap_text_buffer_t;

/*  обработчики состояний машины  */
typedef struct _ap_config_t
{
    askue_port_t          *Port;
    ap_hex_buffer_t     *hBuffer;
    ap_text_buffer_t    *tBuffer;
    ap_label_list_t     *List;
    ap_state_t           State;
    ap_operation_t      Operation;
    ap_exit_t             Exit;
    ap_error_t            Error;  
    long int             Timeout;
} ap_config_t;

// создать новую метку
static
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
static
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
static
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
static
ap_label_list_t* delete_ap_label_list ( ap_label_list_t *LabelList )
{
    ap_label_list_t *RetVal = (ap_label_list_t*)LabelList->Next;
    delete_ap_label ( LabelList->Label );
    free ( LabelList );
    return RetVal;
}

// Добавить элемент списка в голову
static
ap_label_list_t* push_ap_label_list ( ap_label_list_t **Head, ap_label_list_t *Src )
{
    Src->Next = *Head;
    *Head = Src;
    return Src;
}

// Убрать элемент списка из головы
/*
static
ap_label_list_t* pop_ap_label_list ( ap_label_list_t **Head )
{
    ap_label_list_t *RetVal = *Head;
    *Head = RetVal->Next;
    return RetVal;
}
*/

static
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

static
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
static
int remove_ap_label_list ( ap_label_list_t **Head, const char *Name )
{
    ap_label_list_t *Item = find_ap_label_list_by_name ( *Head, Name );
    if ( Item == NULL )
        return -1;
    ap_label_list_t *Prev = find_ap_label_list_by_next ( *Head, Item );
    if ( Prev != NULL )
        Prev->Next = delete_ap_label_list ( Item );
    else
        *Head = delete_ap_label_list ( Item );
        
    return 0;
}
 
/* ------------------------------ */

// выделить память под конфиг
static
void _ap_init_memory_cfg ( ap_config_t **ap_cfg )
{
    // основа конфига
    ( *ap_cfg ) = ( ap_config_t* ) mymalloc ( sizeof ( ap_config_t ) );
    ( *ap_cfg )->Port = ( askue_port_t* ) mymalloc ( sizeof ( askue_port_t ) );
    ( *ap_cfg )->hBuffer = ( ap_hex_buffer_t* ) mymalloc ( sizeof ( ap_hex_buffer_t ) );
    ( *ap_cfg )->tBuffer = ( ap_text_buffer_t* ) mymalloc ( sizeof ( ap_text_buffer_t ) );
    ( *ap_cfg )->List = ( ap_label_list_t* ) mymalloc ( sizeof ( ap_label_list_t ) );
    // углубляясь в дебри конфига
    ( *ap_cfg )->hBuffer->Content = ( uint8_array_t* ) mymalloc ( sizeof ( uint8_array_t ) );
}

// начальные установки памяти
static
void _ap_init_cfg ( ap_config_t **ap_cfg )
{
    memset ( ( *ap_cfg )->tBuffer->Text, '\0', _AP_TBUFFER_LENGTH );
    uint8_array_init ( ( *ap_cfg )->hBuffer->Content, 0 );
    ( *ap_cfg )->hBuffer->Type = ap_ContentNo;
    ( *ap_cfg )->State = ap_StateRead;
    ( *ap_cfg )->Operation = ap_OpNo;
    ( *ap_cfg )->List = NULL;
    ( *ap_cfg )->Error = ap_Success;
    ( *ap_cfg )->Exit = ap_Continue;
}

// установить аргумент - файл порта
static
int __cli_handler_port ( void *ptr, const char *arg )
{
    *( const char** )ptr = arg;
    return 0;
}

static
int __cli_handler_timeout ( void *ptr, const char *arg )
{
    *( long int* )ptr = strtol ( arg, NULL, 10 );
    return 0;
}

static
int _ap_init_args ( ap_config_t *ap_cfg, int argc, char **argv )
{
    const char *port_args[ 5 ];
    #define PORT_FILE ( port_args[ 0 ] )
    #define PORT_SPEED ( port_args[ 1 ] )
    #define PORT_DBITS ( port_args[ 2 ] )
    #define PORT_SBITS ( port_args[ 3 ] )
    #define PORT_PARITY ( port_args[ 4 ] )
    
    cli_arg_t CliArg[] =
    {
        { "port-file", 'f', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_port, &PORT_FILE },
        { "port-speed", 's', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_port, &PORT_SPEED },
        { "port-dbits", 'd', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_port, &PORT_DBITS },
        { "port-sbits", 'b', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_port, &PORT_SBITS },
        { "port-parity", 'p', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_port, &PORT_PARITY },
        { "timeout", 't', CLI_REQUIRED_ARG, CLI_REQUIRED_VAL, __cli_handler_timeout, &( ap_cfg->Timeout ) },
        CLI_LAST_ARG
    };
    
    cli_result_t CliResult = cli_parse ( CliArg, argc, argv );
    
    
    
    if ( CliResult != CLI_SUCCESS )
    {
        switch ( CliResult )
        {
            case CLI_ERROR_NOVAL:
                puts ( "Ошибка. Нет требуемого за аргументом значения." );
                break;
            case CLI_ERROR_HANDLER:
                puts ( "Ошибка. Ошибка обработчика." );
                break;
            case CLI_ERROR_NEEDARG:
                puts ( "Ошибка. Указано недостаточное кол-во обязательных аргументов." );
                break;
            case CLI_ERROR_ARGTYPE:
                puts ( "Ошибка. Тип аргумента не определён." );
                break;
            default:
                puts ( "Ошибка." );
                break;
        }
        
        return -1;
    }
    else if ( port_init ( ap_cfg->Port, PORT_FILE, PORT_SPEED, PORT_DBITS, PORT_SBITS, PORT_PARITY ) == -1 )
    {
        perror ( "Ошибка открытия порта." );
        return -1;   
    }
    else
    {
        printf ( "port-file = %s\n", PORT_FILE );
        printf ( "port-speed = %s\n", PORT_SPEED );
        printf ( "port-dbits = %s\n", PORT_DBITS );
        printf ( "port-sbits = %s\n", PORT_SBITS );
        printf ( "port-parity = %s\n", PORT_PARITY );
        return 0;
    }
    #undef PORT_FILE
    #undef PORT_SPEED
    #undef PORT_DBITS
    #undef PORT_SBITS
    #undef PORT_PARITY
}

// инициализация программы
static
void ap_init ( ap_config_t **ap_cfg, int argc, char **argv )
{
    // выделить память
    _ap_init_memory_cfg ( ap_cfg );
    // инициализировать выделенную память
    _ap_init_cfg ( ap_cfg );
    // разобрать аргументы командной строки
    if ( _ap_init_args ( *ap_cfg, argc, argv ) )
    {
        ( *ap_cfg )->Error = ap_Error;
    }
    else
    {
        ( *ap_cfg )->Error = ap_Success;
    }
}

// функция чтения
static
void _ap_read ( ap_config_t *ap_cfg )
{
    char *Str = NULL;
    // чтение строки
    Str = readline ( ap_g_PROMPT );
    // пришёл конец файла
    if ( Str == NULL ) 
    {
        ap_cfg->State = ap_StateExit; // приготовиться на выход
        return; //досрочное завершение
    }
    printf ( " %s: %s\n", "Прочитана строка", Str );
    // добавить в историю
    add_history ( Str );
    // сжать историю до N 
    stifle_history ( _AP_HISTORY_LENGTH );
    // передача данных в буфер
    snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s", Str );
    // освободить память
    free ( Str );
    // следующее состояние
    ap_cfg->State = ap_StateCheckOp;
}

// функция определения типа операции
static
ap_operation_t _get_checkop ( const char *str )
{
    return ( str[ 0 ] == '+' ) ? ap_OpSetLabel :
            ( str[ 0 ] == '-' ) ? ap_OpUnsetLabel :
            ( str[ 0 ] == '?' ) ? ap_OpHelp :
            ( str[ 0 ] == '!' ) ? ap_OpApplyLabel : ap_OpTransmit;
}

// узнать тип операции
static
void _ap_checkop ( ap_config_t *ap_cfg )
{
    // тип операции
    ap_cfg->Operation = _get_checkop ( ap_cfg->tBuffer->Text );
    switch ( ap_cfg->Operation )
    {
        case ap_OpApplyLabel: 
            puts ( " Операция 'Применить метку'." );
            break;
        case ap_OpUnsetLabel: 
            puts ( " Операция 'Удалить метку'." );
            break;
        case ap_OpSetLabel: 
            puts ( " Операция 'Запомнить метку'." );
            break;
        case ap_OpHelp: 
            puts ( " Операция 'Посмотреть метку'." );
            break;
        default:
            puts ( " Операция 'Передать данные'." );
            break;
    }
    // следующее состояние
    ap_cfg->State = ap_StateExecOp;
}

// извлечь из текстового буфера имя
static
const char *_extract_name ( const char *tBuffer )
{
    const char *Ptr = tBuffer + 1;
    while ( *Ptr == ' ' ) Ptr++;
    return Ptr;
}

// извлечь из текстового буфера имя
static
const char *_extract_content ( const char *tBuffer )
{
    const char *Ptr = strchr ( tBuffer, ':' );
    if ( Ptr == NULL )
        return NULL;
    Ptr++;
    while ( *Ptr == ' ' ) Ptr++;
    return Ptr;
}

// получить длину имени
static
size_t _wordlen ( const char *str )
{
    size_t size = 0;
    while ( ( *str != '\0' ) && ( *str != ' ' ) && ( *str != ':' ) )
    {
        str++;
        size++;
    }
    
    return size;
}

// определить тип данных
ap_content_type_t _checkdtype ( const char *str )
{
    // Проверка что это hex-формат
    if ( isxdigit ( str[ 0 ] ) &&
         isxdigit ( str[ 1 ] ) &&
         str[ 2 ] == ' ' )
    {
        return ap_ContentHex;
    }
    else // в остальных случаяъ это символы
    {
        return ap_ContentChar;
    }
}

// распарсить содержимое
static
ap_content_type_t _parse_content ( uint8_array_t *u8a, const char *str )
{
    // определить тип данных
    ap_content_type_t Type = _checkdtype ( str );
    
    if ( Type == ap_ContentHex ) // заполнение hex-данных
    {
        puts ( "   Hex-тип." );
        // найти начало и конец строки
        char *Next = ( char* ) str;
        char *End = ( char* ) str + strlen ( str );
        // проход от начала до конца
        do {
            long int li = strtol ( Next, &Next, 16 );
            uint8_t u8 = ( uint8_t )li;
            uint8_array_append ( u8a, ( uint8_t[] ) { u8 }, 1 );
        } while ( Next != End );
    }
    else if ( Type == ap_ContentChar ) // заполнение символьными данными
    {
        puts ( "   Char-тип." );
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
    
    return Type;
}

// установить метку
static
void _ap_set_label ( ap_config_t *ap_cfg )
{
    // получить указатель на имя
    const char *Name = _extract_name ( ap_cfg->tBuffer->Text );
    if ( Name == NULL )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Добавление метки без имени." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // получить указатель на содержимое
    const char *Content = _extract_content ( ap_cfg->tBuffer->Text );
    if ( Name == NULL )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Добавление пустой метки." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // получить длину имени
    size_t Name_len = _wordlen ( Name );
    // скопировать имя
    char *_Name = strndup ( Name, Name_len );
    printf ( "  Добавляется метка с именем <%s>\n", _Name );
    // выделить память под содержимое метки
    uint8_array_t *u8a = mymalloc ( sizeof ( uint8_array_t ) );
    // инициализировать массив
    uint8_array_init ( u8a, 0 );
    // распарсить содержимое
    ap_content_type_t Type = _parse_content ( u8a, Content );
    // создать метку
    // поиск старой метки с таким же именем
    ap_label_list_t *Old = find_ap_label_list_by_name ( ap_cfg->List, _Name );
    if ( Old == NULL ) // такой метки нет
    {
        // добавить метку в список
        ap_label_t *L = new_ap_label ( _Name, u8a, Type );
        ap_label_list_t *LL = new_ap_label_list ( L );
        // вставить в конец списка
        push_ap_label_list ( &(ap_cfg->List), LL );
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Метка успешно добавлена." );
    }
    else // метка существует
    {
        // удалить старую метку
        delete_ap_label ( Old->Label );
        // создать новую
        Old->Label = new_ap_label ( _Name, u8a, Type );
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Метка успешно заменена." );
    }
    
    ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
}

// удалить метку
static
void _ap_unset_label ( ap_config_t *ap_cfg )
{
    // найти начало имени метки
    // получить указатель на имя
    const char *Name = _extract_name ( ap_cfg->tBuffer->Text );
    if ( Name == NULL ) // имя метки не ввели
    {
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Не указано имя метки для удаления." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // удаление метки
    if ( remove_ap_label_list ( &( ap_cfg->List ), Name ) )
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Метка не найдена." );
    else
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Метка успешно удалена." );
    // перейти к выводу сообщения на экран
    ap_cfg->State = ap_StatePrint; 
}

// применить метку
static
void _ap_apply_label ( ap_config_t *ap_cfg )
{
    // получить имя метки
    const char *Name = _extract_name ( ap_cfg->tBuffer->Text );
    if ( Name == NULL ) // имя метки не ввели
    {
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Не указано имя метки для поиска." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // поиск метки
    ap_label_list_t *LL = find_ap_label_list_by_name ( ap_cfg->List, Name );
    // если метка не найдена
    if ( LL == NULL )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Метка не найдена." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // Записать в hex-буфер содержимое метки
    uint8_array_update ( ap_cfg->hBuffer->Content, LL->Label->Content->Item, LL->Label->Content->Size );
    // Указать тип содержимого метки
    ap_cfg->hBuffer->Type = LL->Label->Type;
    ap_cfg->State = ap_StatePortIO;
}

// перевести hex-содержимове метки в строку
// память под строку должна быть освобождена
static
char *_hex_content2str ( uint8_array_t *u8a )
{
    // выделить память под строку
    char *str = mymalloc ( sizeof ( char ) * ( u8a->Size * 3 + 1 ) );
    // обнулить память под строку
    memset ( str, '\0', sizeof ( char ) * ( u8a->Size * 3 + 1 ) );
    // смещение по строке
    for ( size_t i = 0; i < u8a->Size; i++ )
    {
        char substr[ 3 ];
        snprintf ( substr, 3, "%.2x", u8a->Item[ i ] );
        strcat ( str, substr );
        strcat ( str, " " );
    }
        
    return str;
}

// посчитать кол-во спец. символов в буфере
size_t _count_spchar ( uint8_array_t *u8a )
{
    size_t sp_char = 0;
    // посчитать кол-во спец. символов
    for ( size_t i = 0; i < u8a->Size; i++ )
        if ( u8a->Item[ i ] == '\n' || u8a->Item[ i ] == '\r' ) sp_char++;
        
    return sp_char;
}

// перевести char-содержимове метки в строку
// память под строку должна быть освобождена
static
char *_char_content2str ( uint8_array_t *u8a )
{
    // посчитать кол-во спец. символов в буфере
    size_t sp_char = _count_spchar ( u8a );
    size_t slen = u8a->Size + 1 + sp_char;
    // выделить память под строку
    char *str = mymalloc ( sizeof ( char ) * slen );
    // обнулить память под строку
    memset ( str, '\0', sizeof ( char ) * slen );   
    // копирование символов
    for ( size_t i = 0, j = 0; i < u8a->Size && j < slen; i++ )
    {
        if ( u8a->Item[ i ] == '\n' )
        {
            str[ j ] = '\\';
            j++;
            str[ j ] = 'n';
            j++;
        }
        else if ( u8a->Item[ i ] == '\r' )
        {
            str[ j ] = '\\';
            j++;
            str[ j ] = 'r';
            j++;
        }
        else
        {
            str[ j ] = u8a->Item[ i ];
            j++;
        }
    }
    
    return str;
}

// перевести содержимове метки в строку
// память под строку должна быть освобождена
char *_content2str ( uint8_array_t *u8a, ap_content_type_t type )
{
    if ( type == ap_ContentHex ) // если метка содержит символы
    {
        return _hex_content2str ( u8a );
    }
    else if ( type == ap_ContentChar ) // если метка с hex-данными
    {
        return _char_content2str ( u8a );
    }
    else
        return NULL;
}

// получить справку по метке
static
void _ap_help_label ( ap_config_t *ap_cfg )
{
    // получить имя метки
    const char *Name = _extract_name ( ap_cfg->tBuffer->Text );
    if ( Name == NULL ) // имя метки не ввели
    {
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Не указано имя метки для поиска." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // поиск метки
    ap_label_list_t *LL = find_ap_label_list_by_name ( ap_cfg->List, Name );
    // если метка не найдена
    if ( LL == NULL )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Предупреждение. Метка не найдена." );
        ap_cfg->State = ap_StatePrint; // перейти к выводу сообщения на экран
        return; // досрочное завершение
    }
    // формирование ответа
    const char *Template = " %s: %s\n     %s: %s\n";
    // получить содержимое метки в виде строки
    char *ContentStr = _content2str ( LL->Label->Content, LL->Label->Type );
    // сообщение
    snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, Template, "Имя метки", LL->Label->Name, "Содержимое метки", ContentStr );
    // освободить память
    free ( ContentStr );
    // перейти к выводу сообщения на экран
    ap_cfg->State = ap_StatePrint; 
}

// подготовить сырые данные для передачи в порт
static
void _ap_transmit ( ap_config_t *ap_cfg )
{
    // распарсить данные введённые с консоли
    ap_cfg->hBuffer->Type = _parse_content ( ap_cfg->hBuffer->Content, ap_cfg->tBuffer->Text );
    // перейти к передаче через порт
    ap_cfg->State = ap_StatePortIO;
}

// выполнить операцию
static
void _ap_execop ( ap_config_t *ap_cfg )
{
    switch ( ap_cfg->Operation )
    {
        case ap_OpSetLabel:
            puts ( " Операция 'Запомнить метку'." );
            _ap_set_label ( ap_cfg );
            break;
        case ap_OpUnsetLabel:
            puts ( " Операция 'Удалить метку'." );
            _ap_unset_label ( ap_cfg );
            break;
        case ap_OpApplyLabel:
            puts ( " Операция 'Применить метку'." );
            _ap_apply_label ( ap_cfg );
            break;
        case ap_OpHelp:
            puts ( " Операция 'Посмотреть метку'." );
            _ap_help_label ( ap_cfg );
            break;
        case ap_OpTransmit:
            puts ( " Операция 'Передать данные'." );
            _ap_transmit ( ap_cfg );
            break;
        default:
            break;
    }
}

// приёмо-передача через порт
static
void _ap_port_io ( ap_config_t *ap_cfg )
{
    puts ( " Передача по RS232..." );
    // передача фрейма
    if ( port_write ( ap_cfg->Port, ap_cfg->hBuffer->Content ) == -1 )
    {
        // сообщение
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Ошибка передачи данных." );
        // наличие ошибки
        ap_cfg->Error = ap_Error;
        // на стадию печати
        ap_cfg->State = ap_StatePrint; 
        // досрочный выход
        return;
    }
    puts ( " Передача по RS232 - завершена." );
    // Содержимое отправляемое
    char *OutputContent = _content2str ( ap_cfg->hBuffer->Content, ap_cfg->hBuffer->Type );
    puts ( " Перевод отправляемого содержимого в строку..." );
    printf ( " Результат перевода: %s\n", OutputContent );
    // очистка буфера
    uint8_array_init ( ap_cfg->hBuffer->Content, 0 );
    // приём первой части фрейма
    puts ( " Приём по RS232..." );
    if ( port_read ( ap_cfg->Port, ap_cfg->hBuffer->Content, ap_cfg->Timeout / 2 ) == -1 )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Ошибка при приёме данных." );
        ap_cfg->Error = ap_Error; // наличие ошибки
        free ( OutputContent );
        ap_cfg->State = ap_StatePrint; // на стадию печати 
        return; // досрочный выход
    }
    puts ( " Приём по RS232 - завершён." );
    // дополнительный буфер
    uint8_array_t u8a;
    // инициализация дополнительного буфера
    uint8_array_init ( &u8a, 0 );
    // приём второй части фрейма
    puts ( " Приём по RS232..." );
    if ( port_read ( ap_cfg->Port, &u8a, ap_cfg->Timeout / 2 ) == -1 )
    {
        snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, "%s\n", "Ошибка при приёме данных." );
        ap_cfg->Error = ap_Error; // наличие ошибки
        free ( OutputContent );
        ap_cfg->State = ap_StatePrint;  // на стадию печати
        return; // досрочный выход
    }
    puts ( " Приём по RS232 - завершён." );
    // добавить в буфер то что прочитали во второй раз
    uint8_array_append ( ap_cfg->hBuffer->Content, u8a.Item, u8a.Size );
    uint8_array_destroy ( &u8a );
    // Содержимое принимаемое
    char *InputContent = _content2str ( ap_cfg->hBuffer->Content, ap_cfg->hBuffer->Type );
    puts ( " Перевод принятого содержимого в строку..." );
    printf ( " Результат перевода: %s\n", InputContent );
    // Шаблон для вывода
    const char *Template = " %s: %s\n     %s: %s\n";
    // сообщение
    snprintf ( ap_cfg->tBuffer->Text, _AP_TBUFFER_LENGTH, Template, "Отправлено", OutputContent, "Принято", InputContent );
    // на стадию печати
    ap_cfg->State = ap_StatePrint; 
    
    // освободить память
    free ( InputContent );
    free ( OutputContent  );
}

// выход из программы
static
void _ap_exit ( ap_config_t *ap_cfg )
{
    ap_cfg->Exit = ap_Exit;
}

// печать 
static
void _ap_print ( ap_config_t *ap_cfg )
{
    size_t len = strlen ( ap_cfg->tBuffer->Text );
    const char *Template = ( ap_cfg->tBuffer->Text[ len - 1 ] == '\n' ) ? ">>>:%s" : ">>>:%s\n";
    printf ( Template, ap_cfg->tBuffer->Text );
    fflush ( stdout );
    ap_cfg->State = ap_StateRead; 
}

// проход по состояниям программы
static
void ap_logic ( ap_config_t *ap_cfg )
{
    while ( ap_cfg->Error == ap_Success && ap_cfg->Exit == ap_Continue )
    {
        switch ( ap_cfg->State )
        {
            case ap_StateRead:
                puts ( "Чтение..." );
                _ap_read ( ap_cfg );
                break;
            case ap_StateCheckOp:
                puts ( "Проверка типа операции..." );
                _ap_checkop ( ap_cfg );
                break;
            case ap_StateExecOp:
                puts ( "Выполненение операции..." );
                _ap_execop ( ap_cfg );
                break;
            case ap_StatePortIO:
                puts ( "Обмен данными через RS232..." );
                _ap_port_io ( ap_cfg );
                break;
            case ap_StatePrint:
                puts ( "Вывод сообщений на экран..." );
                _ap_print ( ap_cfg );
                break;
            case ap_StateExit:
                puts ( "Выход..." );
                _ap_exit ( ap_cfg );
                break;
            default:
                puts ( "Хуйня какая-то!" );
                ap_cfg->Error = ap_Error;
                break;
        }
    }
}

// выход из программы
static
void signal_handler ( int s )
{
    puts ( "\nЗавершение работы!" );
    exit ( EXIT_SUCCESS );
}

// установка сигнала
static
void ap_setsignal ( ap_config_t *ap_cfg )
{
    if ( ( signal ( SIGINT, signal_handler ) != SIG_ERR ) &&
         ( signal ( SIGQUIT, signal_handler ) != SIG_ERR ) &&
         ( signal ( SIGTERM, signal_handler ) != SIG_ERR ) ) 
    {
        ap_cfg->Error = ap_Success;
    }
    else
    {
        ap_cfg->Error = ap_Error;
    }
}

// печать помощи
static
void ap_print_help ( void )
{
    puts ( "/******************************************************/" );
    puts ( "                        Справка.                        " );
    puts ( "Ввод данных в hex-формате осуществляется записью, через " );
    puts ( "пробел hex-кодов байт в виду двух шестнадцатиричных цифр" );
    puts ( "Например: a3 b2 12 34 88 00 0d ff 0d");
    puts ( "Ввод данных в char-формате ( строка символов ) " );
    puts ( "осуществляется записью непрерыной строки символов." );
    puts ( "Символ возврата каретки и новой строки обозначаются как" );
    puts ( "\r и \n соответственно.");
    puts ( "Например: #USP1280F[B1274AF\r" );
    puts ( "Добавление метки:" );
    puts ( "+ <имя_метки>: <данные>" );
    puts ( "Удаление метки:" );
    puts ( "- <имя_метки>" );
    puts ( "Передача данных метки в порт:" );
    puts ( "! <имя_метки>" );
    puts ( "Просмотр содержимого метки" );
    puts ( "? <имя_метки>" );
    puts ( "Программа ведёт историю введённых команд. Навигация по" );
    puts ( "истории осуществляется с помощью кнопок <Стрелка Вверх>" );
    puts ( "и <Стрелка Вниз>." );
    puts ( "Приятной работы. Да пребудет с вами Сила..." );
    puts ( "/******************************************************/" );
}

/*    Точка входа в программу     */

int main(int argc, char **argv)
{
    // конфигурация проги
    ap_config_t *ap_cfg;
    // разбор аргументов командной строки
    ap_init ( &ap_cfg, argc, argv );
    if ( ap_cfg->Error == ap_Error )
        return -1;
    // настройка сигналов на завершение
    ap_setsignal ( ap_cfg );
    if ( ap_cfg->Error == ap_Error )
    {
        perror ( "Ошибка установки сигналов." );
        return -1;
    }
    ap_print_help ();
    
    ap_logic ( ap_cfg );
    
    if ( ap_cfg->Error == ap_Error &&
         ap_cfg->State == ap_StatePrint )
    _ap_print ( ap_cfg );
    
    if ( ap_cfg->Exit == ap_Exit )
        puts ( "\nЗавершение работы!" );
        
    ap_label_list_t *Now = ap_cfg->List;        

    while ( Now != NULL )
    {
        ap_label_list_t *Next = Now->Next;
        delete_ap_label_list ( Now );
        Now = Next;
        if ( Now != NULL ) Next = Now->Next;
    }
    
    port_destroy ( ap_cfg->Port );
    
	return 0;
}



















