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

typedef struct _profile_t
{
	uint16_t value [ 8 ];
} profile_t;

/*
 * открыть канал
 */
bool_t open_chanel ( int RS232, uint8_t dev_name, long int timeout );

/*
 * закрыть канал
 */
bool_t close_chanel ( int RS232, uint8_t dev_name, long int timeout );

bool_t sqlite3_transaction ( sqlite3 *SQLite3_DB, const char *sql );

/*
 * генерация строки с датой на day_ago дней назад
 */
const char *generate_date ( int day_ago );

/*
 * задать вопрос о поиске
 */
bool_t search_start ( int RS232, uint8_t dev_name, long int timeout, const char *datetime );

// задать вопрос о статусе поиске
// 0 - не найден
// 1 в поиске
// 2 найден
// -1 ошибка
search_status_t search_get_status ( int RS232, uint8_t dev_name, long int timeout, uint16_t *memory_addr );

/*
 * Прочитать заданное число байт из памяти счётчика
 */
bool_t cnt_read_mem ( int RS232, uint8_t dev_name, long int timeout, uint16_t mem_addr, uint8_t byte_amount, uint8_t *mem_content );

/*
 * Вывод лога
 */
bool_t __log_input ( const byte_array_t *input, void *ptr );

/*
 * проверить контрольную сумму заголовка
 */
bool_t valid_profile_checksum ( byte_array_t *header );

/*
 * Для обратного вызова
 */
int callback ( void *ptr, int amount, char **value, char**cols );

/*
 * Прочитать профиль
 */
bool_t get_profile ( int RS232, uint8_t dev_name, int timeout, const char *datetime, profile_t *profile );

/*
 * Найти отсутствующие в базе срезы профилей
 */
const char *get_lost_datetime ( sqlite3 *SQLite3_DB, uint32_t dev_name, const char *date, const char *max_time );

/*
 * Сгенерировать максимальное время для данной даты
 */
const char* generate_max_time ( int day_ago );

/*
 * Сохранить профиль в базу
 */
bool_t save_profile ( sqlite3 *SQLite3_db, uint32_t dev_name, const char *datetime, const profile_t *profile );

/*
 * Загрузить профиль мощности со счётчика
 */
bool_t download_profile ( int RS232, uint8_t dev, int timeout, const char *lost_datetime, sqlite3 *SQLite_db );

/*
 *  Число получасий в дне
 */
int halfhour_amount ( int day_ago );

typedef struct
{
	sqlite3 *DB;		// --db={ПУТЬ}
	int RS232_FD;		// --port="{ПУТЬ} {ФОРМУЛА КОНФИГА}"
	FILE *Log;		// --log={ПУТЬ}
	uint8_t Device;		// --device={НОМЕР УСТРОЙСТВА}
	long int Timeout;	// --timeout={ТАЙМАУТ СОЕДИНЕНИЯ}
	char DebugFlag;		// --debug 
} script_cfg_t;

// настройка конфига
static
void script_cfg_init ( script_cfg_t *SCfg )
{
	SCfg->DB = NULL;
	SCfg->RS232_FD = -1;
	SCfg->Log = fopen ( ASKUE_LOG, "a" );

	if ( SCfg->Log == NULL )
	{
		exit ( EXIT_FAILURE );
	}
	
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

// поиск потеряных записей
static
int get_lost_datetime ( script_cfg_t *SCfg, char *LostDatetime )
{
	int Result = -1;
	char *sql = sqlite3_mprintf ( SQL_FindLostRecord, SCfg->Device, SCfg->Device, SCfg->Device, SCfg->Device,
							  SCfg->Device, SCfg->Device, SCfg->Device, SCfg->Device ); // формирование запроса
	
	if ( sql != NULL )
	{	
		char *sqlite3_emsg = NULL;
		//выполнить запрос
		if ( sqlite3_exec ( SCfg->DB, sql, callback, LostDatetime, &sqlite3_emsg ) != SQLITE_OK )
		{
			log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, sqlite3_emsg );					
			sqlite3_free ( sqlite3_emsg );

		}
		else if ( SCfg->DebugFlag )
		{
			log_write ( Log_File, "[ %s ]: Найден пропуск: %s\n", SCRIPT_NAME, LostDatetime );
			Result = 0;
		}
				    
		sqlite3_free ( sql );	
	}
	
	return Result;
}

// Вывод сообщения в лог
static
bool_t IO_Terminal ( const byte_array_t *cmd, void *file )
{
	script_cfg_t *SCfg = ( script_cfg_t* )file;
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
		for ( size_t i = 0; i < src->len; i++ )
			fprintf ( output, "%x ", src->data[ i ] );
	}
	fprintf ( output, "\n" );
	fflush ( output );

	return TRUE;
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
		IO_Terminal ( cmd, SCfg ); 
	}
		
	bool_t result = execute ( SCfg->RS232_FD, cmd, SCfg->timeout, CNT_TM_valid_func, IO_Terminal, SCfg, &emsg );		
	if ( result == FALSE )
	{
		log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return result;
}

//закрыть канал
static
bool_t close_chanel ( script_cfg_t *SCfg )
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
		log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
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
bool_t search_start ( script_cfg_t *SCfg, char *datetime )
{
	char *emsg = NULL;
	uint8_t data[] = { dev_name, 0x03, 0x28, 0x00, 0x00, 0x00 };
	byte_array_t *cmd = byte_array_update_data ( NULL, data, 6 );
	//контрольная сумма
	cmd = append_checksum ( add_datetime ( cmd, datetime ), modbus_crc16, CNT_TM_checksum_order );

	//выполнить команду
	if ( SCfg->DebugFlag )
	{
		IO_Terminal ( cmd, SCfg ); 
	}
		
	bool_t result = execute ( RS232, cmd, timeout, CNT_TM_valid_func, IO_Terminal, SCfg, &emsg );
	//сообщение об ошибке
	if ( result == FALSE )
	{
		log_write ( SCfg->Log, "[ %s ]: Ошибка: %s\n", SCRIPT_NAME, emsg );
	}
	if ( emsg != NULL ) free ( emsg );
	byte_array_delete ( cmd );
	
	return result;


int download_profile ( script_cfg_t *SCfg, char *LostDatetime, tmPowerProfile_t *PowerProfile )
{
	uint32_t address = 0;
	bool_t Result = chanel_open ( SCfg ) && 
		        search_start ( SCfg, LostDatetime ) &&
		        search_end ( SCfg, &address ) &&
		        read_profile ( SCfg, &address, PowerProfile ) &&
		        chanel_close ( SCfg );
	return ( Result ) ? 0 : -1;
}

int main(int argc, char **argv)
{
	script_cfg_t ScriptCfg;
	script_cfg_init ( &ScriptCfg );

	cli_option_t CliOpts[] =
	{
		// ЗДЕСЬ ДОЛЖНЫ БЫТЬ ОПЦИИ
	};
	cli_result_t CliResult = cli_parse ( CliOpts, argc, argv );
	if ( CliResult != CLI_SUCCESS )
	{
		exit ( EXIT_FAILURE );
	}
	
	// ищем 12 пропусков
	char LostDatetime[ DATE_TIME_STRING_BUF + 1 ];
	int TmpResult = get_lost_datetime ( &ScriptCfg, LostDatetime );
	if ( TmpResult != 0 )
	{
		script_cfg_destroy ( &ScriptCfg );
		exit ( EXIT_FAILURE );
	}
	
	tmPowerProfile_t PowerProfile;
	tm_power_profile_init ( &PowerProfile );
	
	TmpResult = download_profile ( &ScriptCfg, LostDatetime, &PowerProfile );
	if ( TmpResult != 0 )
	{
		script_cfg_destroy ( &ScriptCfg );
		exit ( EXIT_FAILURE );
	}
	
	TmpResult = cmp_profile ( LostDatetime, &PowerProfile );
	if ( TmpResult != 0 )
	{
		script_cfg_destroy ( &ScriptCfg );
		exit ( EXIT_FAILURE );
	}
	
	TmpResult = save_profile ( &ScriptCfg, &PowerProfile );
	if ( TmpResult != 0 )
	{
		script_cfg_destroy ( &ScriptCfg );
		exit ( EXIT_FAILURE );
	}
	
	script_cfg_destroy ( &ScriptCfg );

	return EXIT_SUCCESS;
}

bool_t sqlite3_transaction ( sqlite3 *SQLite3_DB, const char *sql )
{
	char *sqlite3_emsg = NULL;

	//выполнить запрос
	if ( sqlite3_exec ( SQLite3_DB, sql, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, sqlite3_emsg );
								
		sqlite3_free ( sqlite3_emsg );
		
		return FALSE;
	}
	else
		return TRUE;
}

int halfhour_amount ( int day_ago )
{
	if ( day_ago == 0 )
	{
		time_t t = time ( NULL );
		
		struct tm *stm_ptr = localtime ( &t );
		
		int x = ( stm_ptr -> tm_hour - 1 ) * 2;
		
		return ( x > 0 ) ? x : 
		       ( x == 0 ) ? 2 : 0;
	}
	else
		return 48;
}

bool_t download_profile ( int RS232, uint8_t dev, int timeout, const char *lost_datetime, sqlite3 *SQLite3_db )
{
	if ( lost_datetime != NULL )
	{	
		profile_t profile = { .value[ 0 ... 7 ] = 0 };
		if ( get_profile ( RS232, dev, timeout, lost_datetime, &profile ) == TRUE )
		{
			return save_profile ( SQLite3_db, dev, lost_datetime, &profile );
		}
		else
		{
			return FALSE;
		}
	}
	else
		return TRUE; // пропусков нет - ищем дальше
}


//открыть канал
bool_t open_chanel ( int RS232, uint8_t dev_name, long int timeout )
{
	char *emsg = NULL;
	
	byte_array_t *cmd = byte_array_update_data ( NULL, ( uint8_t [] ){ dev_name, 0x01, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 }, 8 );
	
	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( Debug_Flag )
		byte_array_fprintx ( stderr, cmd );
		
	bool_t result = execute ( RS232, cmd, timeout, CNT_TM_valid_func, __log_input, NULL, &emsg );
			
	if ( result == FALSE && emsg != NULL )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
		
		free ( emsg );
	}
	
	byte_array_delete ( cmd );
	
	return result;
}

//закрыть канал
bool_t close_chanel ( int RS232, uint8_t dev_name, long int timeout )
{	
	char *emsg = NULL;
	
	byte_array_t *cmd = byte_array_update_data ( NULL, ( uint8_t [] ){ dev_name, 0x02 }, 2 );

	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );
	
	if ( Debug_Flag )
		byte_array_fprintx ( stderr, cmd );
		
	bool_t result = execute ( RS232, cmd, timeout, CNT_TM_valid_func, __log_input, NULL, &emsg );
			
	if ( result == FALSE && emsg != NULL )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
		
		free ( emsg );
	}
	
	byte_array_delete ( cmd );
	
	return result;
}

//задать вопрос о поиске
bool_t search_start ( int RS232, uint8_t dev_name, long int timeout, const char *datetime )
{
	//добавить дату и время
	byte_array_t *add_datetime ( byte_array_t *_cmd, const char *_datetime )
	{
		// msb ---> lsb
		// час, день, месяц, год
		uint32_t d = 0, m = 0, y = 0, h = 0; //день, месяц, год
	
		//смещение в дате чтобы захватить последние две цифры года
		sscanf ( _datetime + 2, "%2u-%2u-%2u %2u", &y, &m, &d, &h ); 
				    
		//добавить к команде адрес профиля ( время его начала )
		return byte_array_append_data ( _cmd, ( uint8_t [] ) { ( uint8_t ) dec_to_bcd ( h ), ( uint8_t ) dec_to_bcd ( d ), 
								         ( uint8_t ) dec_to_bcd ( m ), ( uint8_t ) dec_to_bcd ( y ), 0xff, 0x1e }, 6 ); 		     
	}


	char *emsg = NULL;
	
	byte_array_t *cmd = byte_array_update_data ( NULL, ( uint8_t [] ){ dev_name, 0x03, 0x28, 0x00, 0x00, 0x00 }, 6 );

	//контрольная сумма
	cmd = append_checksum ( add_datetime ( cmd, datetime ), modbus_crc16, CNT_TM_checksum_order );

	//выполнить команду
	if ( Debug_Flag )
		byte_array_fprintx ( stderr, cmd );
		
	bool_t result = execute ( RS232, cmd, timeout, CNT_TM_valid_func, __log_input, NULL, &emsg );

	//сообщение об ошибке
	if ( result == FALSE && emsg != NULL )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
		
		free ( emsg );
	}
	
	byte_array_delete ( cmd );
	
	return result;
}


// задать вопрос о статусе поиске
// 0 - не найден
// 1 в поиске
// 2 найден
// -1 ошибка
search_status_t search_get_status ( int RS232, uint8_t dev_name, long int timeout, uint16_t *memory_addr )
{
	//контейнер содержащий статус выполнения и адрес в памяти счётчика
	typedef struct _tmp_container_t
	{
		search_status_t search_status;
		uint16_t mem_addr;
	} tmp_container_t;
	
	//начальные настройки контейнера
	tmp_container_t tmp_container = { .search_status = SEARCH_STATUS_ERROR, .mem_addr = 0x0000 };

	// callback разбирающий ответ счётчика
	bool_t search_status_encode ( const byte_array_t *input, void *ptr )
	{
		__log_input ( input, NULL );
	
		bool_t bool_result = TRUE; 
		
		tmp_container_t *_tmp_container_ = ( tmp_container_t* ) ptr;
		
		switch ( input -> data[ 1 ] )
		{
			case 0x00:
				{
					_tmp_container_ -> search_status = SEARCH_STATUS_YES;
				
					SET_BYTE ( _tmp_container_ -> mem_addr, 1, input -> data[ 4 ] );
					SET_BYTE ( _tmp_container_ -> mem_addr, 0, input -> data[ 5 ] );
				}	
				break;		
			case 0x01:
				_tmp_container_ -> search_status = SEARCH_STATUS_IN_PROGRESS;
				break;
						
			case 0x02:
				_tmp_container_ -> search_status = SEARCH_STATUS_NO;
				break;
						
			default:
				bool_result = FALSE;
				_tmp_container_ -> search_status = SEARCH_STATUS_ERROR;
				break;
		}
		
		return bool_result;
	}
		
	char *emsg = NULL;
	
	byte_array_t *cmd = byte_array_update_data ( NULL, ( uint8_t [] ){ dev_name, 0x08, 0x18, 0x00 }, 4 );

	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( Debug_Flag )
		byte_array_fprintx ( stderr, cmd );

	//выполнить команду
	if ( execute ( RS232, cmd, timeout, CNT_TM_valid_func, search_status_encode, ( void* ) &tmp_container, &emsg ) == FALSE )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
		
		if ( emsg != NULL ) free ( emsg );
	}
	else
	{
		( *memory_addr ) = tmp_container.mem_addr;
	}
	
	byte_array_delete ( cmd );
	
	return tmp_container.search_status;
}

/*
 * Отладочный вывод
 */

bool_t __log_input ( const byte_array_t *input, void *ptr )
{
	if ( Debug_Flag )
		byte_array_fprintx ( stderr, input );
		
	return TRUE;
}


/*
 * Прочитать заданное число байт из памяти счётчика
 */
bool_t cnt_read_mem ( int RS232, uint8_t dev_name, long int timeout, uint16_t mem_addr, uint8_t byte_amount, uint8_t *mem_content )
{
	//извлечь из фрейма запрощенные из памяти байты
	bool_t get_mem_content ( const byte_array_t *input, void *ptr )
	{
		__log_input ( input, NULL );
	
		memcpy ( ptr, input -> data + 1, byte_amount );
		
		return TRUE;
	}

	char *emsg = NULL;
	
	byte_array_t *cmd = byte_array_update_data ( NULL, 
						     ( uint8_t [] ){ dev_name, 0x06, 0x03, GET_BYTE ( mem_addr, 1 ), GET_BYTE ( mem_addr, 0 ), byte_amount }, 6 );

	//контрольная сумма
	cmd = append_checksum ( cmd, modbus_crc16, CNT_TM_checksum_order );

	if ( Debug_Flag )
		byte_array_fprintx ( stderr, cmd );

	//выполнить команду
	bool_t result = execute ( RS232, cmd, timeout, CNT_TM_valid_func, get_mem_content, ( void* ) mem_content, &emsg );
	
	if ( result == FALSE )
	{
		log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
		
		if ( emsg != NULL ) free ( emsg );
	}
	
	byte_array_delete ( cmd );
	
	return result;
}

/*
 * проверить контрольную сумму заголовка
 */
bool_t valid_profile_checksum ( byte_array_t *header )
{
	// Контрольная сумма заголовка
	byte_array_t *header_checksum = byte_array_update_data ( NULL, &( header -> data [ header -> len - 1 ] ), 1 );
	
	// проверить контрольную сумму
	bool_t result =  valid_checksum ( header -> data, header -> len - 1, header_checksum, simple_checksum, NULL );

	byte_array_delete ( header_checksum );

	return result;
}

/*
 * Сохранить профиль в базу
 */
bool_t save_profile ( sqlite3 *SQLite3_db, uint32_t dev_name, const char *datetime, const profile_t *profile )
{
	//типы полученных данных
	const char *energy_type [ 4 ] =
	{
		"p+",
		"p-",
		"q+",
		"q-"
	};
	
	//извлечь дату 
	const char* extract_date ( const char *_datetime )
	{
		static char buf[ DATE_STRING_BUF + 3 ];
		
		char subbuf[ DATE_STRING_BUF + 1 ] = { [ 0 ... DATE_STRING_BUF ] = '\0' };
		
		memcpy ( subbuf, _datetime, DATE_STRING_BUF );
		
		sprintf ( buf, "\'%s\'", subbuf );
		
		return ( const char* ) buf;
	}
	
	//извлечь первое получасие
	const char* extract_first_half_hour ( const char *_datetime )
	{
		static char buf[ TIME_STRING_BUF + 3 ];
		
		char subbuf[ TIME_STRING_BUF + 1 ] = { [ 0 ... TIME_STRING_BUF ] = '\0' };
		
		memcpy ( subbuf, _datetime + DATE_STRING_BUF + 1, TIME_STRING_BUF ); 
		
		subbuf[ 3 ] = subbuf[ 4 ] = '0';
		
		sprintf ( buf, "\'%s\'", subbuf );
		
		return ( const char* ) buf;
	}
	
	//извлечь второе получасие
	const char* extract_second_half_hour ( const char *_datetime )
	{
		static char buf[ TIME_STRING_BUF + 3 ];
		
		char subbuf[ TIME_STRING_BUF + 1 ] = { [ 0 ... TIME_STRING_BUF ] = '\0' };
		
		memcpy ( subbuf, _datetime + DATE_STRING_BUF + 1, TIME_STRING_BUF ); 
		
		subbuf[ 3 ] = '3';
		subbuf[ 4 ] = '0';
		
		sprintf ( buf, "\'%s\'", subbuf );
		
		return ( const char* ) buf;
	}
	/* код save_profile */

	bool_t result = FALSE;
	
	for ( int i = 0; i < 8; i++ )
	{
		char * error_msg = NULL;
		
		if ( ( result = save ( SQLite3_db, 
				       dev_name, profile -> value[ i ], energy_type[ i % 4 ], 
				       extract_date ( datetime ), 
				       ( ( i < 4 ) ? extract_first_half_hour ( datetime ) : extract_second_half_hour ( datetime ) ), &error_msg ) ) == FALSE )
		{
			log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, error_msg );
			
			if ( error_msg != NULL )
			{	
				sqlite3_free ( error_msg );
			}
			
			break;
		}
		
	}
	
	return result;
}

/*
 * Сгенерировать даты на N дней назад
 */
const char* generate_date ( int day_ago )
{
	static char _date[ DATE_STRING_BUF + 1 ];
	
	char *day_ago_str = NULL;
	
	asprintf ( &day_ago_str, "%d day", day_ago ); // строка смещения
	
	if ( day_ago_str != NULL )
	{
		sprintf_datetime ( _date, DATE_STRING_BUF + 1, "%Y-%m-%d", ( const char *[] ) { day_ago_str, NULL } ); // строка даты
		
		if ( Debug_Flag )
			log_write ( Log_File, "[ %s ]: проверка даты: %s\n", PROGRAM_NAME, _date );
		
		return ( const char* ) _date;
	}
	else
	{
		return NULL;
	}
}

/*
 * Сгенерировать максимальное время для данной даты
 */
const char* generate_max_time ( int day_ago )
{
	static char _time[ TIME_STRING_BUF + 1 ];
	
	if ( day_ago == 0 )
	{
		sprintf_datetime ( _time, TIME_STRING_BUF + 1, "%H:%M:%S", ( const char *[] ) { "start of hour", NULL } );
	}
	else
	{
		sprintf ( _time, "23:59:59" );
	}
	
	return _time;
}

/*
 * Найти отсутствующие в базе срезы профилей
 */
const char *get_lost_datetime ( sqlite3 *SQLite3_DB, uint32_t dev_name, const char *date, const char *max_time )
{
	static char datetime[ DATE_STRING_BUF + TIME_STRING_BUF + 2 ] = 
		{ [ 0 ... DATE_STRING_BUF + TIME_STRING_BUF + 1 ] = '\0' };
	
	char *sql = sqlite3_mprintf ( SELECT_TEMPLATE, date, dev_name, max_time ); // формирование запроса
	
	if ( sql != NULL )
	{	
		char *sqlite3_emsg = NULL;
			
		char _time[ TIME_STRING_BUF + 1 ] = { [ 0 ... TIME_STRING_BUF ] = '\0' };

		//выполнить запрос
		if ( sqlite3_exec ( SQLite3_DB, sql, callback, ( void* ) _time, &sqlite3_emsg ) != SQLITE_OK )
		{
			log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, sqlite3_emsg );
								
			sqlite3_free ( sqlite3_emsg );
		}
		else
		{
			if ( _time[ 0 ] != '\0' )
			{
				sqlite3_free ( sql );	
				
				sprintf ( datetime, "%s %s", date, _time );
				
				if ( Debug_Flag )
					log_write ( Log_File, "[ %s ]: Найден пропуск: %s\n", PROGRAM_NAME, datetime );
				
				return ( const char* ) datetime;
			}
		}
				    
		sqlite3_free ( sql );	
	}
	
	return NULL;
}

/*
 * Колбек
 */
int callback ( void *ptr, int amount, char **value, char**cols )
{
	return sprintf ( ( char* ) ptr, "%s", value[ 0 ] ) < 0;
}

/*
 * Прочитать профиль
 */
bool_t get_profile ( int RS232, uint8_t dev_name, int timeout, const char *datetime, profile_t *profile )
{	
	bool_t result = FALSE;
	
	if ( search_start ( RS232, dev_name, timeout, datetime ) == TRUE ) //начать поиск заголовка
	{
		search_status_t status;
		
		uint16_t mem_addr = 0;
		
		//ждать пока найдётся
		while ( ( status = search_get_status ( RS232, dev_name, timeout, &mem_addr ) ) == SEARCH_STATUS_IN_PROGRESS ); 
			
		if ( status == SEARCH_STATUS_YES ) //если найден
		{
			byte_array_t *hdr = byte_array_new ( 7 );
				
			//читать заголовок
			if ( hdr != NULL && 
			     ( cnt_read_mem ( RS232, dev_name, timeout, mem_addr, 7, hdr -> data ) == TRUE ) &&
			     ( valid_profile_checksum ( hdr ) == TRUE ) ) 
			{		
				byte_array_t *raw_profile = byte_array_new ( 16 );
					
				//читать сам профиль
				if ( raw_profile != NULL && 
				     ( cnt_read_mem ( RS232, dev_name, timeout, mem_addr + 8, 16, raw_profile -> data ) == TRUE ) )
				{
					for ( int i = 0; i < 8; i++ )
					{
						// запись старшего бита без флага неполного профиля
						SET_BYTE ( profile -> value[ i ], 1, ( raw_profile -> data[ i * 2 ] & 0x7f ) );
						SET_BYTE ( profile -> value[ i ], 0, raw_profile -> data[ i * 2 + 1 ] );
						
						if ( profile->value[ i ] == 0x7fff )
							profile->value[ i ] = 0;
					}
						
					result = TRUE;
				}
					
				byte_array_delete ( raw_profile );
			}
				
			byte_array_delete ( hdr );
		}
		else if ( status == SEARCH_STATUS_NO )
		{
			for ( int i = 0; i < 8; i++ )
			{
				profile -> value[ i ] = 0;
			}
				
			result = TRUE;
		}
	}
	
	return result;
}









