#ifndef ASKUE_PLUGIN_H_
#define ASKUE_PLUGIN_H_

#include <sqlite3.h>
/*
 * Открыть все возможные плагины
 */
void askue_plugin_init ( sqlite3 *db );

/*
 * Получить ссылку на плагин по его имени
 */
void* askue_plugin_invoke ( const char *name );

/*
 * Закрыть плагины
 */
void askue_plugin_close ( void );

#endif /* ASKUE_PLUGIN_H_ */
