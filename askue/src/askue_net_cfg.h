#ifndef ASKUE_LAUNCHER_NET_CFG_H_
#define ASKUE_LAUNCHER_NET_CFG_H_

#include <stdio.h>
#include <sqlite3.h>
#include <libconfig.h>
#include <sqlite3.h>
#include <libaskue.h>

/*
 * Занесение данных из конфига в таблицу базы
 */
int askue_network_config ( sqlite3 *db );

#endif /* ASKUE_LAUNCHER_NET_CFG_H_ */
