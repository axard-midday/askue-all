#ifndef LIBASKUE_JOURNAL_H
#define LIBASKUE_JOURNAL_H

#include <sqlite3.h>
#include <stdlib.h>

typedef struct _askue_journal_t
{
    sqlite3 *File;
    size_t Flashback;
} askue_journal_t;

#endif /* LIBASKUE_JOURNAL_H */
