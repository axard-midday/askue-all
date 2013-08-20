#ifndef _GNU_SOURCE
    #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

#include "journal.h"
#include "my.h"

// обработчик результатов
static
int __check_callback ( void *ptr, int amount, char **vals, char **cols )
{
    *( int* ) ptr = ( int ) strtol ( vals[ 0 ], NULL, 10 );
    return 0;
}

// обновить журнал
void journal_refresh ( askue_jnl_t *Jnl )
{
    myfree ( Jnl->SQL );
    Jnl->SQL = NULL;
    
    myfree ( Jnl->Error );
    Jnl->Error = NULL;
}

// проверить наличие записи по ключу.
int journal_check ( askue_jnl_t *Jnl, const askue_jnl_key_t *Key )
{
    const char *Template = "SELECT EXISTS ( SELECT * FROM reg_tbl WHERE device=%llu AND type='%s' AND date='%s' AND time='%s' );";

    if ( asprintf ( &(Jnl->SQL), Template, Key->Device, Key->Type, Key->Date, Key->Time ) == -1 )
    {
        exit ( EXIT_FAILURE );
    }
    
    int R;
    char *errorstr;
    if ( sqlite3_exec ( Jnl->File, Jnl->SQL, __check_callback, &R, &errorstr ) != SQLITE_OK )
    {
        Jnl->Error = mystrdup ( errorstr );
        sqlite3_free ( errorstr );
        return -1;
    }
    
    journal_refresh ( Jnl );
    
    return R;
}

// вставка N записей в журнал
int journal_insert ( askue_jnl_t *Jnl, const askue_jnl_rec_t *Recv, size_t Amount )
{
    int Result = 0;
    while ( Amount && !Result )
    {
        const char *Template = "INSERT INTO reg_tbl ( device, value, type, date, time ) VALUES ( %llu, %f, '%s', '%s', '%s' );";

        if ( asprintf ( &(Jnl->SQL), Template, Recv->Device, Recv->Value, Recv->Type, Recv->Date, Recv->Time ) == -1 )
        {
            exit ( EXIT_FAILURE );
        }
        
        char *errorstr;
        if ( sqlite3_exec ( Jnl->File, Jnl->SQL, NULL, NULL, &errorstr ) != SQLITE_OK )
        {
            Jnl->Error = mystrdup ( errorstr );
            sqlite3_free ( errorstr );
            Result = -1;
        }
        
        journal_refresh ( Jnl );
    
        Recv ++; // к следующей записи
        Amount --;
    }
    return Result;
}

// обработчик запроса на подсчёт найденных записей
int __find_count_callback ( void *Amount, int len, char **vals, char **cols )
{
    *( size_t* ) Amount = ( size_t ) strtoul ( vals[ 0 ], NULL, 10 );
    return 0;
}

// обработчик запроса на выборку
int __find_select_callback ( void *Rec, int len, char **vals, char **cols )
{
    #define DEVICE  vals[ 0 ]
    #define VALUE   vals[ 1 ]
    #define TYPE    vals[ 2 ]
    #define DATE    vals[ 3 ]
    #define TIME    vals[ 4 ]
    
    ( *( askue_jnl_rec_t** )Rec )->Device = strtoull ( DEVICE, NULL, 10 );
    ( *( askue_jnl_rec_t** )Rec )->Value = strtod ( VALUE, NULL );
    ( *( askue_jnl_rec_t** )Rec )->Type = mystrdup ( TYPE );
    ( *( askue_jnl_rec_t** )Rec )->Date = mystrdup ( DATE );
    ( *( askue_jnl_rec_t** )Rec )->Time = mystrdup ( TIME );
    
    ( *( askue_jnl_rec_t** )Rec )++;
    
    return 0;
    
    #undef DEVICE
    #undef VALUE
    #undef TYPE
    #undef DATE
    #undef TIME
}

int journal_find ( askue_jnl_t *Jnl, askue_jnl_rec_t **Recv, size_t *Amount, const askue_jnl_key_t *Key, size_t Limit )
{
    char Cond [ 5 ] [ 256 ];
    const char *TemplateSelect = "SELECT device, value, type, date, time FROM reg_tbl WHERE %s AND %s AND %s AND %s %s;";
    const char *TemplateCount = "SELECT count (*) FROM reg_tbl WHERE %s AND %s AND %s AND %s %s;";
    if ( Key->Device != ( uint64_t ) 0 )
    {
        snprintf ( Cond[ 0 ], 256, "device=%llu", Key->Device );
    }
    else
    {
        strcpy ( Cond[ 0 ], "1" );
    }
    
    if ( Key->Type != NULL )
    {
        snprintf ( Cond[ 1 ], 256, "type='%s'", Key->Type );
    }
    else
    {
        strcpy ( Cond[ 1 ], "1" );
    }
    
    if ( Key->Date != NULL )
    {
        snprintf ( Cond[ 2 ], 256, "date='%s'", Key->Date );
    }
    else
    {
        strcpy ( Cond[ 2 ], "1" );
    }
    
    if ( Key->Time != NULL )
    {
        snprintf ( Cond[ 3 ], 256, "time='%s'", Key->Time );
    }
    else
    {
        strcpy ( Cond[ 3 ], "1" );
    }
    
    if ( Limit != 0 )
    {
        snprintf ( Cond[ 4 ], 256, "LIMIT %u", Limit );
    }
    else
    {
        strcpy ( Cond[ 4 ], "" );
    }
    
    if ( asprintf ( &(Jnl->SQL), TemplateCount, Cond[ 0 ], Cond[ 1 ], Cond[ 2 ], Cond[ 3 ], Cond[ 4 ] ) == -1 )
    {
        exit ( EXIT_FAILURE );
    }
    
    *Amount = 0;
    char *errorstr;
    if ( sqlite3_exec ( Jnl->File, Jnl->SQL, __find_count_callback, Amount, &errorstr ) != SQLITE_OK )
    {
        Jnl->Error = mystrdup ( errorstr );
        sqlite3_free ( errorstr );
        return -1;
    }
    
    journal_refresh ( Jnl );
    (*Amount) = ( ( Limit > 0 ) && ( (*Amount) > Limit ) ) ? Limit : (*Amount);
    
    if ( asprintf ( &(Jnl->SQL), TemplateSelect, Cond[ 0 ], Cond[ 1 ], Cond[ 2 ], Cond[ 3 ], Cond[ 4 ] ) == -1 )
    {
        exit ( EXIT_FAILURE );
    }
    
    askue_jnl_rec_t *Record = ( askue_jnl_rec_t* ) mymalloc ( sizeof ( askue_jnl_rec_t ) * (*Amount) );
    
    askue_jnl_rec_t *XRec = Record;
    
    if ( sqlite3_exec ( Jnl->File, Jnl->SQL, __find_select_callback, &XRec, &errorstr ) != SQLITE_OK )
    {
        Jnl->Error = mystrdup ( errorstr );
        sqlite3_free ( errorstr );
        return -1;
    }
    
    journal_refresh ( Jnl );
    
    *Recv = Record;
    
    return 0;
}



































