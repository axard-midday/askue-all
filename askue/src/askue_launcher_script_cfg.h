#ifndef ASKUE_LAUNCHER_SCRIPT_CFG_H_
#define ASKUE_LAUNCHER_SCRIPT_CFG_H_

#include <sqlite3.h>
/*
 * Занесение данных из конфига в таблицу базы
 */
int askue_scripts_config ( sqlite3 *db );

#endif /* ASKUE_LAUNCHER_SCRIPT_CFG_H_ */
