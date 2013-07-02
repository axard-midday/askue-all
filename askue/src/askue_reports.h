#ifndef ASKUE_LAUNCHER_REPORTS_H_
#define ASKUE_LAUNCHER_REPORTS_H_

#include <stdio.h>
#include <sqlite3.h>
#include <libaskue.h>

/*
 * Запись отчётов за Пн, Вт, Ср, Чт, Пт, Сб, Вс
 */
bool_t askue_launcher_write_reports_according_weekday ( sqlite3 *SQLite3 );


/*
 * Запись отчётов за конкретную дату
 */
bool_t askue_launcher_write_reports_according_date ( sqlite3 *SQLite3, const char *date );

#endif /* ASKUE_LAUNCHER_REPORTS_H_ */
