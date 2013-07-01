#ifndef ASKUE_LAUNCHER_DB_H_
#define ASKUE_LAUNCHER_DB_H_

#include <sqlite3.h>
#include <libaskue.h>

/*
 * декорирование обращения к базе
 */
bool_t sqlite3_exec_decor ( sqlite3 *db, const char *sql, int ( *callback ) ( void*, int, char **, char ** ), void *ptr );

/*
 * Первичная настройка базы данных
 */
int askue_db_init ( sqlite3 **db );

/*
 * Очистить базу от лишних записей
 */
bool_t askue_db_cut ( sqlite3 *db );

/*
 * Открыть базу
 */
bool_t askue_db_open ( void );

/*
 * Закрыть базу
 */
void askue_db_close ( void );

/*
 * Доступ к базе
 */
sqlite3* askue_db_access ( void );

#endif /* ASKUE_LAUNCHER_DB_H_ */
