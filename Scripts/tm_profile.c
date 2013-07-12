#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <libaskue.h>
#include <sqlite3.h>
#include <libconfig.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <byteswap.h>
#include <time.h>

#include "cli.h"

#define SCRIPT_NAME "cnt_tm_profile"

#define SQL_FindLostRecord \
"SELECT time_tbl.time, date_tbl.date FROM time_tbl, date_tbl \
        WHERE\
                ( ( NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p-' )\
		  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q-' ) )\
	          AND time_tbl.time <= '23:30:00' \
	          AND date_tbl.date < ( SELECT DATE ( 'now' ) )\
	          AND date_tbl.date > ( SELECT DATE ( 'now', '-31 day' ) ) )\
	       OR\
	       ( ( NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time \
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'p-' )\
		  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q+' )\
                  OR NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl \
                        WHERE time_tbl.time = reg_tbl.time\
                                AND reg_tbl.date = date_tbl.date\
                                AND reg_tbl.cnt = %d\
                                AND reg_tbl.type = 'q-' ) )\
	         AND time_tbl.time < ( SELECT TIME ( 'now', '-1 hour' ) ) \
	         AND date_tbl.date = ( SELECT DATE ( 'now' ) ) )\
	ORDER BY date_tbl.date DESC, time_tbl.time DESC;"

// 0 - не найден
// 1 в поиске
// 2 найден
// -1 ошибка
typedef enum _search_status_t
{
	SEARCH_STATUS_NO = 0,
	SEARCH_STATUS_IN_PROGRESS = 1,
	SEARCH_STATUS_YES = 2,
	SEARCH_STATUS_ERROR = -1
} search_status_t;

typedef struct 
{
    uint16_t Address;
    uint8_t SearchFlag;
    char Header[ DATE_TIME_STRING_BUF + 1 ];
    uint32_t Power[ 8 ];
} power_profile_t;

typedef struct
{
	sqlite3 *DB;		    // --db={ПУТЬ}
	int RS232_FD;		    // --port="{ПУТЬ} {ФОРМУЛА КОНФИГА}"
	FILE *Log;		        // --log={ПУТЬ}
	uint8_t Device;	    // --device={НОМЕР УСТРОЙСТВА}
	long int Timeout;	    // --timeout={ТАЙМАУТ СОЕДИНЕНИЯ}
	uint8_t DebugFlag;		// --debug 
    power_profile_t *PowerProfile;
} script_cfg_t;

// запись в лог
static
void __log_write ( FILE *F, const char *format, ... )
{
   	time_t t = time ( NULL );
	char asctime_str[ DATE_TIME_STRING_BUF + 1 ];
	int len = strftime ( asctime_str, DATE_TIME_STRING_BUF + 1, "%Y-%m-%d %H:%M:%S", localtime ( &t ) );

	if ( len != DATE_TIME_STRING_BUF )
	{
		return;
	}
	
	fprintf ( F, "%s ", asctime_str );

    va_list vl;
    va_start ( vl, format );
    vfprintf ( F, format, vl );
    va_end ( vl );
}

// обёртка для пишущих в базу запросов
static
bool_t sqlite3_transaction ( script_cfg_t *SCfg, const char *sql )
{
	char *sqlite3_emsg = NULL;
	//выполнить запрос
	if ( sqlite3_exec ( SCfg->DB, sql, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, sqlite3_emsg );
		sqlite3_free ( sqlite3_emsg );
		return FALSE;
	}
	else
		return TRUE;
}

// настройка конфига
static
void script_cfg_init ( script_cfg_t *SCfg )
{
	SCfg->DB = NULL;
	SCfg->RS232_FD = -1;
    /*
	SCfg->Log = fopen ( ASKUE_LOG, "a" );

	if ( SCfg->Log == NULL )
	{
		exit ( EXIT_FAILURE );
	}
	*/
    SCfg->Log = stdout;
	SCfg->Device = 0;
	SCfg->Timeout = 0;
	SCfg->DebugFlag = 0;
}

// закрытие файлов
static
void script_cfg_destroy ( script_cfg_t *SCfg )
{
	if ( SCfg->DB != NULL )
		sqlite3_close ( SCfg->DB );
	
	if ( SCfg->RS232_FD != -1 )
		rs232_close ( SCfg->RS232_FD );
	
	if ( SCfg->Log != NULL )
		fclose ( SCfg->Log );
}


// Обработчик результатов запроса
static
int callback ( void *ptr, int amount, char **value, char**cols )
{
	return sprintf ( ( char* ) ptr, "%s %s", value[ 1 ], value[ 0 ] ) < 0;
} 

#define SQL_CreateDateTbl \
"CREATE TEMPORARY TABLE IF NOT EXISTS date_tbl ( date text );"
#define SQL_CreateDateId \
"CREATE TEMPORARY UNIQUE INDEX data_id ON date_tbl ( date );"
#define SQL_InsertIntoDateTbl \
"INSERT INTO date_tbl ( date ) VALUES ( ( SELECT DATE ( 'now', '-%d day' ) ) );"
#define SQL_DropDateTbl \
"DROP TABLE date_tbl"

static 
bool_t prepare_date_tbl ( script_cfg_t *SCfg )
{
	char *sqlite3_emsg = NULL;
	if ( sqlite3_exec ( SCfg->DB, SQL_CreateDateTbl, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s в запросе:\n%s\n", SCRIPT_NAME, sqlite3_emsg, SQL_CreateDateTbl );					
		sqlite3_free ( sqlite3_emsg );
		return FALSE;
	}
	else if ( sqlite3_exec ( SCfg->DB, SQL_CreateDateId, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s в запросе:\n%s\n", SCRIPT_NAME, sqlite3_emsg, SQL_CreateDateId );					
		sqlite3_free ( sqlite3_emsg );
		return FALSE;
	}
	
	if ( !sqlite3_transaction ( SCfg, "BEGIN TRANSACTION;" ) ) return FALSE;
	
	bool_t Result = TRUE;
	for ( int i = 0; i < 32 && Result; i++ )
	{
		char *sql = sqlite3_mprintf ( SQL_InsertIntoDateTbl, i );
		if ( sqlite3_exec ( SCfg->DB, sql, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
		{
			__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s в запросе:\n%s\n", SCRIPT_NAME, sqlite3_emsg, sql );					
			sqlite3_free ( sqlite3_emsg );
			Result = FALSE;
		}
		sqlite3_free ( sql );
	}
	if ( !Result ) return FALSE;
	
	if ( !sqlite3_transaction ( SCfg, "END TRANSACTION;" ) ) return FALSE;
	
	return TRUE;
}

// удалить таблицу
static
bool_t drop_date_tbl ( script_cfg_t *SCfg )
{
	char *sqlite3_emsg = NULL;
	if ( sqlite3_exec ( SCfg->DB, SQL_DropDateTbl, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s в запросе:\n%s\n", SCRIPT_NAME, sqlite3_emsg, SQL_DropDateTbl );					
		sqlite3_free ( sqlite3_emsg );
		return FALSE;
	}
	
	return TRUE;
}

// поиск потеряных записей
static
bool_t get_lost_datetime ( script_cfg_t *SCfg, char *LostDatetime )
{
	if ( !prepare_date_tbl ( SCfg ) ) return FALSE;

	int Result = FALSE;
    uint8_t Dev = SCfg->Device;
    // формирование запроса
	char *sql = sqlite3_mprintf ( SQL_FindLostRecord, 
                                    Dev, Dev, Dev, Dev,
                                    Dev, Dev, Dev, Dev ); 
	
	if ( sql != NULL )
	{	
		char *sqlite3_emsg = NULL;
		//выполнить запрос
		if ( sqlite3_exec ( SCfg->DB, sql, callback, LostDatetime, &sqlite3_emsg ) != SQLITE_OK )
		{
			__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, sqlite3_emsg );					
			sqlite3_free ( sqlite3_emsg );
		}
		else if ( SCfg->DebugFlag )
		{
			__log_write ( SCfg->Log, "[ %s ]: Найден пропуск: %s\n", SCRIPT_NAME, LostDatetime );
			Result = TRUE;
		}
				    
		sqlite3_free ( sql );	
		
		
	}
    else
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка: sqlite3_mprintf\n", SCRIPT_NAME );		
    }
	
	drop_date_tbl ( SCfg );
	return Result;
}

// Вывод сообщения в лог
static
bool_t IO_Terminal ( const byte_array_t *cmd, void *file )
{
	script_cfg_t *SCfg = *( script_cfg_t** )file;
	FILE *output = ( SCfg->DebugFlag ) ? stdout : SCfg->Log;
	
	time_t t = time ( NULL );
	char asctime_str[ DATE_TIME_STRING_BUF + 1 ];
	int len = strftime ( asctime_str, DATE_TIME_STRING_BUF + 1, "%Y-%m-%d %H:%M:%S", localtime ( &t ) );

	if ( len != DATE_TIME_STRING_BUF )
	{
		return FALSE;
	}
	
	fprintf ( output, "%s ", asctime_str );
	if ( cmd != NULL )
	{
		for ( size_t i = 0; i < cmd->len; i++ )
			fprintf ( output, "%x ", cmd->data[ i ] );
	}
	fprintf ( output, "\n" );
	fflush ( output );

	return TRUE;
}

// callback разбирающий ответ счётчика
static
bool_t search_status_encode ( const byte_array_t *input, void *ptr )
{
    if ( !IO_Terminal ( input, ptr ) ) return FALSE;
    
    script_cfg_t *SCfg = *( script_cfg_t** )ptr;
    power_profile_t *PP = SCfg->PowerProfile;
    
    bool_t Result = TRUE;
    switch ( input -> data[ 1 ] )
    {
        case 0x00:
            {
                PP->SearchFlag = SEARCH_STATUS_YES;
            
                SET_BYTE ( PP->Address, 1, input -> data[ 4 ] );
                SET_BYTE ( PP->Address, 0, input -> data[ 5 ] );
            }	
            break;		
        case 0x01:
            PP->SearchFlag = SEARCH_STATUS_IN_PROGRESS;
            break;
                    
        case 0x02:
            PP->SearchFlag = SEARCH_STATUS_NO;
            break;
                    
        default:
            Result = FALSE;
            PP->SearchFlag = SEARCH_STATUS_ERROR;
            break;
    }
    
    return Result;
}

// задать вопрос о статусе поиске
static
bool_t search_get_status ( script_cfg_t *SCfg )
{		
	char *emsg = NULL;
	uint8_t data[] = { SCfg->Device, 0x08, 0x18, 0x00 };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 4 );
	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( ( const byte_array_t* ) cmd, SCfg ); 
	}

	//выполнить команду
    bool_t Result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, search_status_encode, SCfg, &emsg );
	if ( Result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
    if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return Result;
}

// ожидание ответа от счётчика по поиску профиля
static
bool_t search_end ( script_cfg_t *SCfg )
{
    bool_t Result;
    do {
        Result = search_get_status ( SCfg );
    } while ( Result && SCfg->PowerProfile->SearchFlag == SEARCH_STATUS_IN_PROGRESS );
    return Result;
}

// открыть канал
static
bool_t chanel_open ( script_cfg_t *SCfg )
{
	char *emsg = NULL;
	uint8_t data[] = { SCfg->Device, 0x01, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 8 );
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( ( const byte_array_t* ) cmd, SCfg ); 
	}
		
	bool_t result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, IO_Terminal, SCfg, &emsg );		
	if ( result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return result;
}

//закрыть канал
static
bool_t chanel_close ( script_cfg_t *SCfg )
{	
	char *emsg = NULL;
	uint8_t data[] = { SCfg->Device, 0x02 };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 2 );
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );
	
	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( cmd, SCfg ); 
	}
		
	bool_t result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, IO_Terminal, SCfg, &emsg );		
	if ( result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return result;
}

//добавить дату и время
static
byte_array_t *add_datetime ( byte_array_t *_cmd, const char *_datetime )
{
	// msb ---> lsb
	// час, день, месяц, год
	uint32_t d = 0, m = 0, y = 0, h = 0; //день, месяц, год

	//смещение в дате чтобы захватить последние две цифры года
	sscanf ( _datetime + 2, "%2u-%2u-%2u %2u", &y, &m, &d, &h ); 
			    
	//добавить к команде адрес профиля ( время его начала )
	uint8_t data[] =
	{
		( uint8_t ) dec_to_bcd ( h ), ( uint8_t ) dec_to_bcd ( d ),
		( uint8_t ) dec_to_bcd ( m ), ( uint8_t ) dec_to_bcd ( y ),
		0xff, 0x1e
	};
	return byte_array_append_data ( _cmd, data, 6 ); 		     
}

//задать вопрос о поиске
static
bool_t search_start ( script_cfg_t *SCfg, char *datetime )
{
	char *emsg = NULL;
	uint8_t data[] = { SCfg->Device, 0x03, 0x28, 0x00, 0x00, 0x00 };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 6 );
	//контрольная сумма
	cmd = append_checksum ( add_datetime ( cmd, datetime ), modbus_crc16, CNT_TM_checksum_order );

	//выполнить команду
	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( cmd, SCfg ); 
	}
		
	bool_t result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, IO_Terminal, SCfg, &emsg );
	//сообщение об ошибке
	if ( result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return result;
}

#define PROFILE_HEADER_LENGTH 8
#define PROFILE_POWER_LENGTH 16

//извлечь из фрейма запрощенные из памяти байты
static
bool_t extract_profile_power ( const byte_array_t *input, void *ptr )
{
    if ( !IO_Terminal ( input, ptr ) ) return FALSE;
    power_profile_t *PowerProfile = ( *( script_cfg_t** )ptr )->PowerProfile;
    
    for ( int i = 0; i < 8; i++ )
    {
        // запись старшего бита без флага неполного профиля
        SET_BYTE ( PowerProfile->Power[ i ], 1, ( input->data[ i * 2 ] & 0x7f ) );
        SET_BYTE ( PowerProfile->Power[ i ], 0, input->data[ i * 2 + 2 ] );
        
        if ( PowerProfile->Power[ i ] == 0x7fff ) PowerProfile->Power[ i ] = 0;
    }
    
    return TRUE;
}

// чтение памяти профилей
// данные: значения мощностей P+, P-, Q+, Q-
static
bool_t read_profile_power ( script_cfg_t *SCfg )
{
	char *emsg = NULL;
    uint16_t MemoryAddress = SCfg->PowerProfile->Address + PROFILE_HEADER_LENGTH + 1;
	uint8_t data[] = { SCfg->Device, 0x06, 0x03, GET_BYTE ( MemoryAddress, 1 ), GET_BYTE ( MemoryAddress, 0 ), PROFILE_POWER_LENGTH };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 6 );
	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( cmd, SCfg ); 
	}

	//выполнить команду
	bool_t Result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, extract_profile_power, ( void* ) SCfg, &emsg );
	if ( Result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );	
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return Result;
}

// проверить контрольную сумму заголовка
static
bool_t valid_profile_checksum ( const byte_array_t *header )
{
	// Контрольная сумма заголовка
	byte_array_t *header_checksum = byte_array_update_data ( NULL, &( header -> data [ header -> len - 1 ] ), 1 );
	// проверить контрольную сумму
	bool_t result =  valid_checksum ( header -> data, header -> len - 1, header_checksum, simple_checksum, NULL );
	byte_array_delete ( header_checksum );
	return result;
}

// перевести заголовок в строку
static
bool_t header2str ( script_cfg_t *SCfg, const byte_array_t *input )
{
    #define HDR_HOUR input->data[ 1 ]
    #define HDR_DAY input->data[ 2 ]
    #define HDR_MONTH input->data[ 3 ]
    #define HDR_YEAR input->data[ 4 ]
    #define HDR_TEMPLATE "20%.2u-%.2u-%.2u %.2u:00:00"
    #define HDR_LEN DATE_TIME_STRING_BUF + 1
    
    int len = snprintf ( SCfg->PowerProfile->Header, 
                         HDR_LEN, HDR_TEMPLATE, HDR_YEAR, HDR_MONTH, HDR_DAY, HDR_HOUR );
    if ( len < 0 )
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, strerror ( errno ) );
        return FALSE;
    }
    else
    {
        return TRUE;
    }
    
    #undef HDR_HOUR
    #undef HDR_DAY
    #undef HDR_MONTH
    #undef HDR_YEAR
    #undef HDR_TEMPLATE
    #undef HDR_LEN
}

// извлечь из фрейма запрощенные из памяти байты
static
bool_t extract_profile_header ( const byte_array_t *input, void *ptr )
{
    if ( !IO_Terminal ( input, ptr ) ) return FALSE;
    script_cfg_t *SCfg = *( script_cfg_t** )ptr;
    
    if ( !valid_profile_checksum ( input ) ) return FALSE;

    return header2str ( SCfg, input );
}

// чтение памяти профилей
// данные: заголовок
static
bool_t read_profile_header ( script_cfg_t *SCfg )
{
	char *emsg = NULL;
    uint16_t MemoryAddress = SCfg->PowerProfile->Address;
	uint8_t data[] = { SCfg->Device, 0x06, 0x03, GET_BYTE ( MemoryAddress, 1 ), GET_BYTE ( MemoryAddress, 0 ), PROFILE_HEADER_LENGTH };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 6 );
	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( cmd, SCfg ); 
	}

	//выполнить команду
	bool_t Result = execute ( SCfg->RS232_FD, cmd, SCfg->Timeout, CNT_TM_valid_func, extract_profile_header, ( void* ) SCfg, &emsg );
	if ( Result == FALSE )
	{
		__log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );	
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return Result;
}

// чтение профиля
static
bool_t read_profile ( script_cfg_t *SCfg, char *LostDatetime )
{
    search_status_t SearchStatus = SCfg->PowerProfile->SearchFlag;
    if ( SearchStatus == SEARCH_STATUS_NO )
    {
        __log_write ( SCfg->Log, "[ %s ]: Профиль не найден\n", SCRIPT_NAME );
        return TRUE;
    }
    else if ( SearchStatus & SEARCH_STATUS_ERROR )
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка поиска профиля\n", SCRIPT_NAME );
        return TRUE;
    }
    else
    {
        return read_profile_header ( SCfg ) &&
                read_profile_power ( SCfg );
    }
}

#undef PROFILE_HEADER_LENGTH
#undef PROFILE_POWER_LENGTH

// получить получасие
void get_halfhour ( const char *src, int number, char *dest )
{
	bzero ( dest, TIME_STRING_BUF + 1 );
	if ( number )
	{
		// второе получасие
		strncpy ( dest, src + DATE_STRING_BUF + 1, TIME_STRING_BUF );
		dest[ 3 ] = '3';	
	}
	else
	{
		// первое получасие
		strncpy ( dest, src + DATE_STRING_BUF + 1, TIME_STRING_BUF );
	}
}

// получить дату
void get_date ( const char *src, char *dest )
{
	bzero ( dest, DATE_STRING_BUF + 1 );
	strncpy ( dest, src, DATE_STRING_BUF );
}

// сохранить в базу
static
bool_t save_profile ( script_cfg_t *SCfg )
{
    if ( sqlite3_transaction ( SCfg, "BEGIN TRAHSACTION;" ) )
    {
        bool_t Result = TRUE;
        
        const char *energy_type [ 4 ] = { "p+", "p-", "q+", "q-" };
        char Halfhour[ TIME_STRING_BUF ];
        char Date[ DATE_STRING_BUF ];
        char *emsg = NULL;
        
        for ( int i = 0; i < 8 && Result; i++ )
        {
            get_halfhour ( SCfg->PowerProfile->Header, ( i > 4 ), Halfhour );
            get_date ( SCfg->PowerProfile->Header, Date );
            Result = save ( SCfg->DB, SCfg->Device, SCfg->PowerProfile->Power[ i ], energy_type[ i/2 ], Date, Halfhour, &emsg );
            
            if ( Result == FALSE )
            {
                __log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
            }
            if ( emsg != NULL ) free ( emsg );
        }
        
        Result = sqlite3_transaction ( SCfg, "END TRAHSACTION;" );
        
        return Result;
    }
	else
    {
        return FALSE;
    }
}

/*                  Обработчики опций командной строки                */

// открыть базу
int cli_handler_open_db ( void *ptr, int *flag, const char *arg )
{    
    script_cfg_t* SCfg = ( script_cfg_t* )ptr;
    sqlite3 *db;
    if ( sqlite3_open ( arg, &db ) != SQLITE_OK )
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка открытия базы данных: %s\n", SCRIPT_NAME, sqlite3_errmsg ( db ) );
        sqlite3_close ( db );
        return -1;
    }
    else
    {
        ( ( script_cfg_t* )SCfg )->DB = db;
        return 0;
    }
}

// открыть порт
int cli_handler_open_port ( void *ptr, int *flag, const char *arg )
{
    char Buffer[ 512 ];
    strcpy ( Buffer, arg );

    script_cfg_t* SCfg = ( script_cfg_t* )ptr;
    int RS232 = -1;
    
    char *RS232_Opts[ 5 ];
    RS232_Opts[ 0 ] = Buffer;
    
    char *NextToken = NULL;
    for ( int i = 1; i < 5; i++ )
    {
    	char *NextToken = strchr ( RS232_Opts[ i - 1 ], ';' );
	    if ( NextToken == NULL ) 
	    {
	    	__log_write ( SCfg->Log, "[ %s ]: Ошибка разбора опции --port\n" )
	    	return -1;
	    }
        
        RS232_Opts[ i ] = NextToken + 1;
        NextToken[ 0 ] = '\0';
    }
    
    #define RS232_Path RS232_Opts[ 0 ]
    #define RS232_Speed RS232_Opts[ 1 ]
    #define RS232_DBits RS232_Opts[ 2 ]
    #define RS232_Parity RS232_Opts[ 3 ]
    #define RS232_SBits RS232_Opts[ 4 ]
    
    RS232 = rs232_open ( RS232_Path );
    if ( RS232 < 0 )
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка открытия rs232: %s\n", 
                    SCRIPT_NAME, strerror ( errno ) );
        return -1;
    }
  
    struct termios T;
    if ( rs232_init ( RS232, &T ) )
    {
    	close ( RS232 );
    	__log_write ( SCfg->Log, "[ %s ]: Ошибка инициализации порта rs232: %s\n", 
                    SCRIPT_NAME, strerror ( errno ) );
    	return -1;
    }
    
    rs232_set_speed ( &T, RS232_Speed );
    rs232_set_databits ( &T, RS232_DBits );
    rs232_set_parity ( &T, RS232_Parity );
    rs232_set_stopbits ( &T, RS232_SBits );
    
    if ( rs232_apply ( RS232, &T ) ) 
    {
    	close ( RS232 );
    	__log_write ( SCfg->Log, "[ %s ]: Ошибка настройки порта rs232: %s\n", 
                    SCRIPT_NAME, strerror ( errno ) );
    	return -1;
    }
    
    SCfg->RS232_FD = RS232;
    
    #undef RS232_Path
    #undef RS232_Speed
    #undef RS232_DBits
    #undef RS232_Parity
    #undef RS232_SBits
    
    return 0;
}

// открыть лог
int cli_handler_open_log ( void *ptr, int *flag, const char *arg )
{
    script_cfg_t* SCfg = ( script_cfg_t* )ptr;
    FILE *newLog = NULL;
    newLog = fopen ( arg, "a" );
    if ( newLog == NULL )
    {
        __log_write ( SCfg->Log, "[ %s ]: Ошибка открытия rs232: %s\n", 
                    SCRIPT_NAME, strerror ( errno ) );
        return -1;
    }
    else
    {
        fclose ( ( ( script_cfg_t* )SCfg )->Log );
        ( ( script_cfg_t* )SCfg )->Log = newLog;
        return 0;
    }
}

// установить номер обрабатываемого устройства
int cli_handler_set_device ( void *ptr, int *flag, const char *arg )
{
    long int liDevice;
    liDevice = strtol ( arg, NULL, 10 );
    ( *( uint8_t* )ptr ) = ( uint8_t )liDevice;
    return 0;
}

// таймаут ожидания ответа
int cli_handler_set_timeout ( void *ptr, int *flag, const char *arg )
{
    long int liTimeout;
    liTimeout = strtol ( arg, NULL, 10 );
    ( *( long int* )ptr ) = liTimeout;
    return 0;
}

// установка флага отладки
int cli_handler_set_debug ( void *ptr, int *flag, const char *arg )
{
    ( *( uint8_t* )ptr ) = 1;
    return 0;
}

/*                          Точка входа скрипта                       */

int main(int argc, char **argv)
{
    // настройка конфигурации скрипта
    power_profile_t PowerProfile;
	script_cfg_t SCfg;
	script_cfg_init ( &SCfg );
    SCfg.PowerProfile = &PowerProfile;

    // получение опций скрипта
	cli_option_t CliOpts[] =
	{
        { "db", 'd', CLI_REQUIRED_ARG, cli_handler_open_db, &(SCfg), NULL },
        { "port", 'p', CLI_REQUIRED_ARG, cli_handler_open_port, &(SCfg), NULL },
        { "log", 'l', CLI_REQUIRED_ARG, cli_handler_open_log, &(SCfg), NULL },
        { "device", 'n', CLI_REQUIRED_ARG, cli_handler_set_device, &(SCfg.Device), NULL },
        { "timeout", 't', CLI_REQUIRED_ARG, cli_handler_set_timeout, &(SCfg.Timeout), NULL },
        { "debug", 'v', CLI_REQUIRED_ARG, cli_handler_set_debug, &(SCfg.DebugFlag), NULL },
        CLI_LAST_OPTION
	};
	cli_result_t CliResult = cli_parse ( CliOpts, argc, argv );
	if ( CliResult != CLI_SUCCESS )
	{
        	script_cfg_destroy ( &SCfg );
		exit ( EXIT_FAILURE );
	}
	
	// ищем пропуск а затем работаем со счётчиком
    int exit_status;
	char LostDatetime[ DATE_TIME_STRING_BUF + 1 ];
	if ( get_lost_datetime ( &SCfg, LostDatetime ) &&
         	chanel_open ( &SCfg ) && 
		 search_start ( &SCfg, LostDatetime ) &&
		 search_end ( &SCfg ) &&
		 read_profile ( &SCfg, LostDatetime ) &&
		 chanel_close ( &SCfg ) &&
         	save_profile ( &SCfg ) )
    {
		exit_status = EXIT_FAILURE;
    }
    else
    {
        exit_status = EXIT_SUCCESS;
    }
    
    script_cfg_destroy ( &SCfg );

	return exit_status;
}
