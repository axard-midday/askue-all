#ifndef ASKUE_JOURNAL_H_
#define ASKUE_JOURNAL_H_

#include "config.h"

/* Точка первоначальной настройки базы */
int askue_journal_init ( askue_cfg_t *ACfg, FILE *Log, int Verbose );

/* Сжать журнал */
int askue_journal_stifle ( journal_cfg_t *JCfg, FILE *Log, int Verbose );

#endif /* ASKUE_JOURNAL_H_ */
