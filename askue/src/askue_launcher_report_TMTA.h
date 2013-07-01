#ifndef ASKUE_LAUNCHER_REPORT_TMTA_H_
#define ASKUE_LAUNCHER_REPORT_TMTA_H_


#include <stdio.h>
#include <sqlite3.h>
#include <libaskue.h>

/*
 * Запись запись отчёта counterТМТА
 */
bool_t askue_launcher_report_TMTA ( sqlite3 *db, const char *date, FILE *report );

#endif /* ASKUE_LAUNCHER_REPORT_TMTA_H_ */
