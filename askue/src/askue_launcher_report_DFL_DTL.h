#ifndef ASKUE_LAUNCHER_REPORT_DFL_DTL_H_
#define ASKUE_LAUNCHER_REPORT_DFL_DTL_H_

#include <stdio.h>
#include <sqlite3.h>
#include <libaskue.h>

/*
 * СФормировать отчёт DTL
 */
bool_t askue_launcher_report_DTL ( sqlite3 *db, const char *date, FILE *report );

/*
 * СФормировать отчёт DFL
 */
bool_t askue_launcher_report_DFL ( sqlite3 *db, const char *date, FILE *report );

#endif /* ASKUE_LAUNCHER_REPORT_DFL_DTL_H_ */
