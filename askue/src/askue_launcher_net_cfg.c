#include <stdio.h>
#include <sqlite3.h>
#include <libconfig.h>
#include <sqlite3.h>
#include <libaskue.h>

#include "askue_launcher_db.h"
#include "askue_launcher_macro.h"
#include "askue_launcher_log_error.h"
/*
 * Получить флаг удалённости
 */
static bool_t get_modem_remote_flag ( const config_setting_t *modem )
{
	const char *modem_id = config_setting_name ( modem );
	
        if ( modem_id[ 1 ] == 'R' ) // определить местоположение модема в сети
        {
               return TRUE; // удалённый модем
        }
        else
        {
               return FALSE; // местный модем
        }            
}

/*
 * Получить тип модема
 */
static const char* get_modem_type ( const config_setting_t *modem )
{      
        return config_setting_name ( modem ) + 3; // возврат   
}

/*
 * Добавить счётчик в базу
 */
static bool_t askue_network_config_device ( sqlite3 *db, const char *type, const config_setting_t *device, const char *modem_id )
{
	int cnt_id = config_setting_get_int_elem ( device, 0 ); // номер устройства
	       
	int cnt_delay = config_setting_get_int_elem ( device, 1 ); // задержка чтения

	// формирование запроса на добавление данных о счётчике в базу
	char *sql = sqlite3_mprintf ( "INSERT INTO cnt_tbl ( cnt, type, timeout, modem ) VALUES ( %d, '%s', %d, '%s' );", cnt_id, type, cnt_delay, modem_id );
		       
	bool_t status = sqlite3_exec_decor ( db, sql, NULL, NULL );
	
	sqlite3_free ( sql );
	
	return status;
}

/*
 * Добавить модем в базу
 */
static bool_t askue_network_config_modem ( sqlite3 *db, const config_setting_t *modem )
{
	const char *modem_id = config_setting_get_string ( modem ); // номер модема

	const char *modem_type = get_modem_type ( modem ); // тип модема

	int remote_flag = get_modem_remote_flag ( modem ); // определить является ли модем удалённым

       	// формирование запроса на добавление данных о счётчике в базу
        char *sql = sqlite3_mprintf ( "INSERT INTO modem_tbl ( modem, type, remote_flag ) VALUES ( '%s', '%s', %d );", 
        				modem_id, modem_type, remote_flag );

        bool_t status = sqlite3_exec_decor ( db, sql, NULL, NULL );

	sqlite3_free ( sql );

	return status;
}

/*
 * Обход списка устройств одного типа
 */
static bool_t askue_network_config_device_list ( sqlite3 *db, const config_setting_t *device_list, const char *modem_id )
{
        bool_t status = TRUE;
        
        const char *type = config_setting_name ( device_list ); // тип устройства
        
        size_t length = config_setting_length ( device_list ); // кол-во устройств в списке
        
	for ( size_t i = 0; i < length && status; i++ ) // перебор устройств
	{
	       status = askue_network_config_device ( db, type, config_setting_get_elem ( device_list, i ), modem_id ); // добавить в таблицу устройств
	}
	
	return status;
}

/*
 * Найти модем в ветви
 */
static config_setting_t* lookup_plc_modem ( const config_setting_t *branch )
{
        config_setting_t *modem = NULL;

        for ( size_t i = 0; i < config_setting_length ( branch ) && modem == NULL; i++ ) // перебор устройств в сети
	{
		config_setting_t *elem = config_setting_get_elem ( branch, i );
	
		const char *modem_id = config_setting_name ( elem );
	
	        if ( modem_id[ 0 ] == 'm' )
	        {
	                modem = elem;
	        }
	}
	
	return modem;
}

/*
 * Обход ветви
 */
static bool_t askue_network_config_branch ( sqlite3 *db, const config_setting_t *branch )
{
        bool_t status = TRUE;
        
        config_setting_t *modem = lookup_plc_modem ( branch ); // поиск модема в ветви
        
        if ( modem != NULL )
        {
                // модем найден
        
                if ( ( status = askue_network_config_modem ( db, modem ) ) ) // записать в таблицу модемов
                {
                	const char *modem_name = config_setting_get_string ( modem ); // имя модема
                	
                	size_t length = config_setting_length ( branch ); // длина ветви
                
		        for ( size_t i = 0; i < length && status; i++ ) // перебор устройств в сети
			{
				config_setting_t *elem = config_setting_get_elem ( branch, i ); // следующий список устройств
			               
			        if ( elem != modem ) // проверить: не пытаемся ли мы записать модем как счётчик
			        	status = askue_network_config_device_list ( db, elem, modem_name ); // запись в базу устройств одного типа
			}
	        }
	}
	else
	{
	       // модем отсутствует
	       
	       size_t length = config_setting_length ( branch ); // длина ветви
	
	       for ( size_t i = 0; i < length && status; i++ ) // перебор устройств в сети
	       {
                      status = askue_network_config_device_list ( db, config_setting_get_elem ( branch, i ), "" ); // запись в базу устройств одного типа
	       }        
	}
	
	return status;
}

/*
 * Занесение данных из конфига в таблицу базы
 */
int askue_network_config ( sqlite3 *db )
{
	bool_t status = FALSE;
	
        config_t cfg;

	config_init ( &cfg ); // выделить память под переменную с конфигурацией
	
	if ( config_read_file ( &cfg, ASKUE_NET_FILE ) == CONFIG_TRUE ) // открыть и прочитать файл
	{
	        config_setting_t *net = config_lookup ( &cfg, "Network" ); // поиск сети
	        
	        if ( net != NULL )
	        {
	        	if ( ( status = sqlite3_exec_decor ( db, "BEGIN TRANSACTION;", NULL, NULL ) ) ) // обёртка из транзакций
	        	{
				size_t length = config_setting_length ( net ); // длина сети
	
				for ( size_t i = 0; i < length && status; i++ ) //пройтись по ветвям сети
				{ 
					status = askue_network_config_branch ( db, config_setting_get_elem ( net, i ) );
				}
				
				sqlite3_exec_decor ( db, "END TRANSACTION;", NULL, NULL );
			}
	        }
	        else
	        {
	        	askue_log_error ( "Отсутствует поле 'Network' в конфигурации\n" );
	        }      
	}
	else
	{
	        askue_log_error ( "Ошибка конфигурации: %s ( %d )\n", config_error_text ( &cfg ), config_error_line ( &cfg ) );
	}
	
	config_destroy ( &cfg );
	 
	return status;
}

