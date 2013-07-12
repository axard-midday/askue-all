#include <sqlite3.h>

// создать таблицу если отсутствует
static 
int __init_tbl ( sqlite3 *DB )
{
    
}

/*                Точка первоначальной настройки базы                 */
void askue_db_init ( askue_cfg_t *ACfg )
{
    sqlite3 *DB;
    if ( sqlite3_open ( ACfg->DB->File, &DB ) != SQLITE_OK )
    {
        char Buffer[ 256 ];
        snprintf ( Buffer, 256, "%s", sqlite3_errmsg ( DB ) );
        write_msg ( stderr, "Открытие базы данных", "FAIL", Buffer );
    }
    
    if ( !__init_reg_tbl ( DB ) &&
         !__init_time_tbl ( DB ) &&
         !__init_cnt_tbl ( DB ) &&
         !__init_log_tbl ( DB ) &&
         !__init_date_tbl ( DB ) )
}
