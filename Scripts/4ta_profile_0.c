#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <libaskue.h>

#define PROGRAM_NAME "cnt_4ta_profile"

#define SELECT_TEMPLATE1 "SELECT MAX ( time_tbl.time ) FROM time_tbl WHERE NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl WHERE time_tbl.time = reg_tbl.time AND reg_tbl.date = '%s' AND reg_tbl.cnt = %s ) AND time < '%s';"

#define SELECT_TEMPLATE2 "SELECT COUNT ( time_tbl.time ) FROM time_tbl WHERE NOT EXISTS ( SELECT reg_tbl.time FROM reg_tbl WHERE time_tbl.time = reg_tbl.time AND reg_tbl.date = '%s' AND reg_tbl.cnt = %s );"

typedef struct 
{
	uint8_t m;
	uint8_t d;
	uint8_t h;
} profile_address_t;

/*
 * Получить данные от устройства
 */
bool_t get_profile ( int RS232, const char *dev_name, int timeout, profile_address_t profile_address, uint32_t *energy );

/*
 * Вычленить данные
 */
bool_t extract_data ( const byte_array_t *ba, void *ptr );

/*
 * Время начала данного профиля
 */
const char* get_sql_time ( const char *lost_datetime );

/*
 * Дата начала данного профиля
 */
const char* get_sql_date ( const char *lost_datetime );

/* 
 * Загрузка потерянных профилей
 */
bool_t download_lost_profiles ( int RS232, sqlite3 *db, const char *dev_name, int timeout );

/*
 * Загрузить профиль
 */
bool_t download_profile ( int RS232, sqlite3 *db, const char *dev_name, int timeout, const char *lost_datetime );

/*
 * Получить адрес профиля в формате понятном счётчику
 */
profile_address_t profile_address_eval ( const char *datetime );

/*
 * Найти отсутствующие в базе срезы профилей
 */
const char* get_lost_datetime ( sqlite3 *SQLite3_DB, const char *dev_name, const char *date, const char *max_time );

/*
 * Работа с трансакциями БД
 */
bool_t sqlite3_transaction ( sqlite3 *SQLite3_DB, const char *sql );

/*
 * Сгенерировать даты на N дней назад
 */
const char* generate_date ( int day_ago );

/*
 * Сгенерировать максимальное время для данной даты
 */
const char* generate_max_time ( int day_ago );

/*
 * Найти отсутствующие в базе срезы профилей
 */
int get_lost_datetime_amount ( sqlite3 *SQLite3_DB, const char *dev_name, const char *date );

/*
 * Флаг отладки, Версия исполнения счётчика
 */
int Debug_Flag;

const char *Log_File;

int main ( int argc, char **argv )
{
	Debug_Flag = ( argc == 3 ) ? !strcmp ( "debug", argv[ 2 ] ) : 0;

	Log_File = argv[ 1 ];

	int result = -1;
	
	// лог, порт и база

	int RS232 = init_port ( "/etc/askue/rs232.cfg" );

	if ( RS232 < 0 )
	{
		//log_write (  Log_File, "[ %s ]: Ошибка открытия порта: %s( %d )\n", PROGRAM_NAME, strerror ( errno ), errno );
		printf ( "[ %s ]: Ошибка открытия порта: %s( %d )\n", PROGRAM_NAME, strerror ( errno ), errno );

		_exit ( -1 );
	}
		
	sqlite3 *SQLite3_db = init_db ( "/etc/askue/db.cfg" );

	if ( SQLite3_db == NULL )
	{
		//log_write ( Log_File, "[ %s ]: Ошибка открытия базы данных: %s\n", PROGRAM_NAME, sqlite3_errmsg ( SQLite3_db ) );
		printf ( "[ %s ]: Ошибка открытия базы данных: %s\n", PROGRAM_NAME, sqlite3_errmsg ( SQLite3_db ) );
		_exit ( -1 );
	}

	// получить имя устройства
	config_t cfg;

	config_init ( &cfg );

	if ( config_read_file ( &cfg, "/var/askue/device.cfg" ) == CONFIG_TRUE )
	{
		
		const char *dev_name;
		int timeout;
		
		//считать значение
		if ( ( config_lookup_string ( &cfg, "device", &dev_name ) == CONFIG_TRUE ) &&
		     ( config_lookup_int ( &cfg, "device_timeout", &timeout ) == CONFIG_TRUE ) )
		{	
			result = download_lost_profiles ( RS232, SQLite3_db, dev_name, timeout );
		}
		else
		{
			log_write ( Log_File, "[ %s ]: Ошибка %s\n", PROGRAM_NAME, "В файле конфигурации нет необходимых полей" );
			printf ( "[ %s ]: Ошибка %s\n", PROGRAM_NAME, "В файле конфигурации нет необходимых полей" );
		}

	}
	
	rs232_close ( RS232 );

	config_destroy ( &cfg );

	sqlite3_close ( SQLite3_db );

	return result;
}

/*
 * Работа с трансакциями БД
 */
bool_t sqlite3_transaction ( sqlite3 *SQLite3_DB, const char *sql )
{
	char *sqlite3_emsg = NULL;

	//выполнить запрос
	if ( sqlite3_exec ( SQLite3_DB, sql, NULL, NULL, &sqlite3_emsg ) != SQLITE_OK )
	{
		//log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, sqlite3_emsg );
		printf ( "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, sqlite3_emsg );
								
		sqlite3_free ( sqlite3_emsg );
		
		return FALSE;
	}
	else
		return TRUE;
}

/* 
 * Загрузка потерянных профилей
 */
bool_t download_lost_profiles ( int RS232, sqlite3 *db, const char *dev_name, int timeout )
{
	bool_t result = FALSE;

	// перебор дней
	for ( int i = 0; i > -5; i-- )
	{
		if ( sqlite3_transaction ( db, "BEGIN TRANSACTION;" ) )
		{
		
			const char *_date_str = generate_date ( i );
			//printf ( "Обрабатываемая дата: %s\n", _date_str );
			const char *_max_time_str = generate_max_time ( i );
			//printf ( "Обрабатываемая дата: %s\n", _max_time_str );
			
			int lost_datetime_amount = get_lost_datetime_amount ( db, dev_name, _date_str );
			//printf ( "Число пропусков: %d\n", lost_datetime_amount );
		
			// перебор получасий
			for ( int j = 0; j < lost_datetime_amount; j++ )
			{
				//printf ( "Пропуск номер: %d\n", i );
				const char *lost_datetime = get_lost_datetime ( db, dev_name, _date_str, _max_time_str );
				//printf ( "Потеряное время: %s\n", lost_datetime );
				if ( lost_datetime == NULL )
				{
					result = TRUE;
					break;
				}
				else //if ( strlen ( lost_datetime ) == DATE_TIME_STRING_BUF )
				{	
					result = download_profile ( RS232, db, dev_name, timeout, lost_datetime );
				}
					
				if ( result == FALSE ) break;	
			}
		
			if ( result == FALSE ) break;	
		}
		sqlite3_transaction ( db, "END TRANSACTION;" );
	}
	
	return result;
}


/*
 * Загрузить профиль
 */
bool_t download_profile ( int RS232, sqlite3 *db, const char *dev_name, int timeout, const char *lost_datetime )
{
	profile_address_t profile_address = profile_address_eval ( lost_datetime );
	
	bool_t result = FALSE;
		
	uint32_t energy = 0, dev = str_to_uint32 ( dev_name );
			
	if ( get_profile ( RS232, dev_name, timeout, profile_address, &energy ) )
	{
		char *error_msg = NULL;
		const char *c1 = get_sql_date ( lost_datetime );
		const char *c1 = get_sql_time ( lost_datetime );
		
		
		if ( save ( db, dev, energy, "p+", , &error_msg ) ) 
		{
			result = TRUE;
		}
		else
		{
			printf
			log_write ( Log_File, "[ %s ]: Ошибка sqlite: %s\n", PROGRAM_NAME, error_msg );
	
			sqlite3_free ( error_msg );
		}
	}
		
	return result;
}

/*
 * Получить адрес профиля в формате понятном счётчику
 */
profile_address_t profile_address_eval ( const char *datetime )
{
	profile_address_t result;
	
	int month, day, hour, minute;
	
	sscanf ( datetime + 5, "%2d-%2d %2d:%2d", &month, &day, &hour, &minute );
	
	result.m = ( uint8_t ) dec_to_bcd ( month );
	
	result.d = ( uint8_t ) dec_to_bcd ( day );
	
	result.h = ( uint8_t ) dec_to_bcd ( hour * 2 + ( ( minute >= 30 ) ? 1 : 0 ) );
	
	return result;
}

/*
 * Дата начала данного профиля
 */
const char* get_sql_date ( const char* lost_datetime )
{
	static char sql[ DATE_STRING_BUF + 3 ];
	
	snprintf ( sql, DATE_STRING_BUF + 2, "'%s'", lost_datetime );
	
	return sql;
}

/*
 * Время начала данного профиля
 */
const char* get_sql_time ( const char* lost_datetime )
{
	static char sql[ TIME_STRING_BUF + 3 ];
	
	snprintf ( sql, TIME_STRING_BUF + 3, "'%s'", lost_datetime + DATE_STRING_BUF + 1 );
	
	return sql;
}

/*
 * Вычленить данные
 */
bool_t extract_data ( const byte_array_t *ba, void *ptr )
{
	int my_power ( unsigned int x, unsigned int degree )
	{
		int result = 1;
		
		for ( int i = degree; i > 0; i-- )
			result *= x;
			
		return result;
	}


	if ( Debug_Flag )
		byte_array_fprintc ( stderr, ba );

	if ( ba -> len == 14 )
	{
		char strbuf[ 6 ] = { [ 0 ... 5 ] = '\0' };
		
		memcpy ( strbuf, ba -> data + 6, 5 );
		
		unsigned int x, y;
		
		sscanf ( strbuf, "%4u%1u", &x, &y );
		
		*( int* ) ptr = x * my_power ( 10, y );
	
		return TRUE;
	}
	else
		return FALSE;
}

/*
 * Получить данные от устройства
 */
bool_t get_profile ( int RS232, const char *dev_name, int timeout, profile_address_t profile_address, uint32_t *energy )
{
	bool_t result = FALSE;

	char *com_str = NULL;
	
	asprintf ( &com_str, "#%s00000?%2x%2x%2x", dev_name, profile_address.m, profile_address.d, profile_address.h );
	
	if ( com_str != NULL )
	{
		for ( int i = 0; i < strlen ( com_str ); i++ )
			if ( com_str [ i ] == ' ' )
				com_str [ i ] = '0';
					
		byte_array_t *cmd = byte_array_update_data ( NULL, ( uint8_t* ) com_str, strlen ( com_str ) );

		free ( com_str );
		
		char *emsg = NULL;
			
		//контрольная сумма
		cmd = append_checksum ( cmd, simple_checksum, CNT_TA_checksum_order );
			
		cmd = byte_array_append_data ( cmd, ( uint8_t [] ) { 0x0d }, 1 );
			
		if ( Debug_Flag )
			byte_array_fprintc ( stderr, cmd );
		
		result = execute ( RS232, cmd, timeout, CNT_TA_valid_func, extract_data, energy, &emsg );
			
		if ( emsg != NULL )
		{
			log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, emsg );
	
			free ( emsg );
		}
	}
	else
	{
		log_write ( Log_File, "[ %s ]: Ошибка функции asprintf () \n", PROGRAM_NAME );
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
			log_write ( Log_File, "[ %s ]: Проверка даты: %s\n", PROGRAM_NAME, _date );
		
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
		sprintf_datetime ( _time, TIME_STRING_BUF + 1, "%H:%M:%S", ( const char *[] ) { "now", "start of hour", NULL } );
	}
	else
	{
		sprintf ( _time, "23:59:59" );
	}
	
	return _time;
}

/*
 * Колбек
 */
int callback1 ( void *ptr, int amount, char **value, char**cols )
{
	if ( value[ 0 ] != NULL )
		return sprintf ( ( char* ) ptr, "%s", value[ 0 ] ) <= 0;
	else
		return 0;
}

/*
 * Найти отсутствующие в базе срезы профилей
 */
const char* get_lost_datetime ( sqlite3 *SQLite3_DB, const char *dev_name, const char *date, const char *max_time )
{
	static char datetime[ DATE_STRING_BUF + TIME_STRING_BUF + 2 ] = 
		{ [ 0 ... DATE_STRING_BUF + TIME_STRING_BUF + 1 ] = '\0' };
	
	char *sql = sqlite3_mprintf ( SELECT_TEMPLATE1, date, dev_name, max_time ); // формирование запроса
		
	if ( sql != NULL )
	{	
		char *sqlite3_emsg = NULL;
			
		char _time[ TIME_STRING_BUF + 1 ] = { [ 0 ... TIME_STRING_BUF ] = '\0' };

		//выполнить запрос
		if ( sqlite3_exec ( SQLite3_DB, sql, callback1, ( void* ) _time, &sqlite3_emsg ) != SQLITE_OK )
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
int callback2 ( void *ptr, int amount, char **value, char**cols )
{
	return sscanf ( value[ 0 ], "%d", ( int* ) ptr ) <= 0;
}

/*
 * Найти отсутствующие в базе срезы профилей
 */
int get_lost_datetime_amount ( sqlite3 *SQLite3_DB, const char *dev_name, const char *date )
{
	int amount = 0;
	
	char *sql = sqlite3_mprintf ( SELECT_TEMPLATE2, date, dev_name ); // формирование запроса
	
	if ( sql != NULL )
	{	
		char *sqlite3_emsg = NULL;

		//выполнить запрос
		if ( sqlite3_exec ( SQLite3_DB, sql, callback2, &amount, &sqlite3_emsg ) != SQLITE_OK )
		{
			log_write ( Log_File, "[ %s ]: Ошибка: %s\n", PROGRAM_NAME, sqlite3_emsg );
								
			sqlite3_free ( sqlite3_emsg );
		}
	    
		sqlite3_free ( sql );	
	}
	
	return amount;
}



