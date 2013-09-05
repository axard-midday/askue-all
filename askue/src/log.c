#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <libaskue.h/write_msg.h>
#include <libaskue/macro.h>

#include "config.h"
#include "flag.h"
#include "ecode.h"

// открыть временный лог
// сюда копируются остатки от обрезания
static
int tmp_log_open ( FILE **Ptr, const askue_cfg_t *Cfg, FILE *Log )
{
    char Buffer[ 256 ];
    if ( snprintf ( Buffer, 256, "%s.tmp", Cfg->Log->File ) == -1 )
    {
        write_msg ( Log, "Временный лог", "FAIL", "tmp_log_open(): snprintf()" );
        return ASKUE_ERROR;
    }
    
    FILE *_F = fopen ( Buffer, "w" );
    if ( _F == NULL )
    {
        if ( snprintf ( Buffer, 256, "Ошибка открытия временного лога: %d - %s", errno, strerror ( errno ) ) != -1 )
            write_msg ( Log, "Временный лог", "FAIL", Buffer );
        
        ( *Ptr ) = NULL;
        
        return ASKUE_ERROR;
    }
        
    ( *Ptr ) = _F;
        
    return ASKUE_SUCCESS;
}

// открыть лог
int askue_log_open ( FILE **Ptr, const askue_cfg_t *Cfg )
{
    FILE *_F = fopen ( Cfg->Log->File, Cfg->Log->Mode );
    if ( _F == NULL )
    {
        ( *Ptr ) = NULL;
        
        return ASKUE_ERROR;
    }
        
    ( *Ptr ) = _F;
        
    return ASKUE_SUCCESS;
}

// открыть лог заново
static
int askue_log_reopen ( FILE **Ptr, const char *File, const char *Mode )
{
    FILE *_F = fopen ( File, Mode );
    if ( _F == NULL )
    {
        ( *Ptr ) = NULL;
        
        return ASKUE_ERROR;
    }
        
    ( *Ptr ) = _F;
        
    return ASKUE_SUCCESS;
}


// закрыть лог
void askue_log_close ( FILE **Log )
{
    if ( ( *Log ) != NULL )
    {
        fclose ( *Log );
        ( *Log ) = NULL;
    }
}

// узнать кол-во строк в файле
static
size_t __get_lines_amount ( FILE *F )
{
    size_t All = 0; // всего строк
    int ch;
    while ( ( ch = fgetc ( F ) ) != EOF ) 
    {
        if ( ch == '\n' )
            All++;
    }
    return All;
}

// отсчитать число строк, 
// которые будут пропущены при копировании
static
void __skeep_lines ( FILE *F, size_t ToSkeep )
{
    int ch;
    size_t x = ToSkeep;
    while ( x > 0 &&
             ( ch = fgetc ( F ) ) != EOF )
    {
        if ( ch == '\n' )
            x--;
    }
}

// скопировать файл
static
int __copy_file ( FILE *New, FILE *Old )
{
    
    char buf[ 256 ];
    int FlagEOF = 0, FlagWError = 0;
    while ( !FlagEOF && 
             !FlagWError )
    {
        size_t ReadAmount = fread ( buf, sizeof ( char ), 256, Old );
        FlagEOF = feof ( Old );
        FlagWError = fwrite ( buf, sizeof ( char ), ReadAmount, New ) != ReadAmount;
    }
    
    return ( FlagWError ) ? ASKUE_ERROR : ASKUE_SUCCESS;
}

// новый лог заместо старого
static
int __rewind_log ( const askue_cfg_t *Cfg )
{
    if ( remove ( Cfg->Log->File ) )
    {
        return ASKUE_ERROR;
    }
    char Buffer[ 256 ];
    if ( snprintf ( Buffer, 256, "%s.tmp", Cfg->Log->File ) == -1 )
    {
        return ASKUE_ERROR;
    }
    
    return ( rename ( Buffer, Cfg->Log->File ) ) ? ASKUE_ERROR : ASKUE_SUCCESS;
}

// обрезать лог
int askue_log_stifle ( FILE **Log, const askue_cfg_t *Cfg )
{
    FILE *Old = ( *Log );
    FILE *New;
    
    // временный файл лога
    if ( tmp_log_open ( &New, Cfg, Old ) == -1 )
        return ASKUE_ERROR;
    // переоткрыть на чтение
    fclose ( Old );
    if ( askue_log_reopen ( &Old, Cfg->Log->File, "r" ) )
        return ASKUE_ERROR;
    // вернуться к началу файла лога
    rewind ( Old );
    // всего строк
    size_t All = __get_lines_amount ( Old ); 
    // кол-во строк на удаление 
    // чтобы осталось Cfg->Log->Lines строк
    size_t ToDelete = All - Cfg->Log->Lines + 1;
    // вернуться к началу файла лога
    rewind ( Old );
    // отсчитать число строк, 
    // которые будут пропущены при копировании
    __skeep_lines ( Old, ToDelete );
    // копировать оставщееся в новый файл
    if ( __copy_file ( New, Old ) )
        return ASKUE_ERROR;

    fclose ( New );
    fclose ( Old );
    
    if ( __rewind_log ( Cfg ) )
        return ASKUE_ERROR;

    if ( askue_log_reopen ( Log, Cfg->Log->File, "a" ) )
        return ASKUE_ERROR;
        
    if ( TESTBIT ( Cfg->Flag, ASKUE_FLAG_VERBOSE ) )
        write_msg ( (*Log), "АСКУЭ", "OK", "Лог успешно сжат." );
        
    return ASKUE_SUCCESS;
}
