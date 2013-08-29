#ifndef LIBASKUE_JOURNAL_H
#define LIBASKUE_JOURNAL_H

#include <sqlite3.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct _askue_jnl_t
{
    sqlite3 *File;
    size_t Flashback;
    char *SQL;
    char *Error;
} askue_jnl_t;

typedef struct _askue_jnl_rec_t
{
    uint64_t    Device;     // часть серийника устройства используемая в работе
    double      Value;      // данные ( показания )
    char        *Type;      // тип данных
    char        *Date;      // дата соответствующая фиксации показаний
    char        *Time;      // время соответствующее фиксации показаний
} askue_jnl_rec_t;

typedef struct _askue_jnl_key_t
{
    uint64_t    Device;     // часть серийника устройства используемая в работе
    char        *Type;      // тип данных
    char        *Date;      // дата соответствующая фиксации показаний
    char        *Time;      // время соответствующее фиксации показаний
} askue_jnl_key_t;

// проверить наличие записи
int journal_check ( askue_jnl_t *Jnl, const askue_jnl_key_t *Key );

// обновить журнал
void journal_refresh ( askue_jnl_t *Jnl );

// вставка N записей в журнал
int journal_insert ( askue_jnl_t *Jnl, const askue_jnl_rec_t *Recv, size_t Amount );

// поиск записей
int journal_find ( askue_jnl_t *Jnl, askue_jnl_rec_t **Recv, size_t *Amount, const askue_jnl_key_t *Key, size_t Limit );

#endif /* LIBASKUE_JOURNAL_H */
