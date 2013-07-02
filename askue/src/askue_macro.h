#ifndef ASKUE_LAUNCHER_MACRO_H_
#define ASKUE_LAUNCHER_MACRO_H_

//#define ASKUE_DEBUG
/*
 * Различные макроопределения
 */

#ifdef ASKUE_RELEASE 
	#define ASKUE_HELP_FILE "/etc/askue/askue.help"

	#define ASKUE_DEVICE_FILE "/var/askue/device.cfg"

	#define ASKUE_NET_FILE "/etc/askue/net.cfg"

	#define ASKUE_DB_FILE "/mnt/base/askue.db.sqlite3"

	#define ASKUE_LOG_FILE "/mnt/base/reports/askue.log"
#else /* Отладочная версия */
	#define ASKUE_HELP_FILE "./askue.help"

	#define ASKUE_DEVICE_FILE "./device.cfg"

	#define ASKUE_NET_FILE "./net.cfg"

	#define ASKUE_DB_FILE "./askue.db.sqlite3"

	#define ASKUE_LOG_FILE "./askue.log"
#endif
		
// кол-во строк в логе 
#define ASKUE_LOG_LINE_AMOUNT 1000

#endif /* ASKUE_LAUNCHER_MACRO_H_ */
