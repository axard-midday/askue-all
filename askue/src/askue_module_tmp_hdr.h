#ifndef
#define

typedef struct
{
    char *Buffer;
    size_t BufferLen;
    FILE *LogFile;
    sqlite3 *DB;
    int PortFd;
} askue_module_enviroment_t;

#ifndef DeviceIdSize
    #define DeviceIdSize 32
#endif

/*
typedef struct
{
    char DevID[ DeviceIdSize ];
    long int Timeout;
} askue_module_in_params_t;
*/

typedef struct
{
    size_t ArgC;
    char **ArgV; // 2 должно быть обязательно: timeout & ID
} askue_module_argv_t;

typedef struct
{
    askue_log_callback_t LogCallback;
    askue_db_callback_t DBCallback;
    askue_journal_callback_t JnlCallback;
    askue_ioport_callback_t IOCallback;
} askue_module_callbacks_t;

typedef int ( *askue_module_method_t ) ( askue_module_enviroment_t*,
                                           askue_module_argv_t*,
                                           askue_module_callbacks_t* );

// болванка для конструирования функций обмена
typedef bool_t ( *askue_module_iomethod_t ) ( askue_ioport_callback_t, 
                                               askue_journal_callback_t,
                                               askue_plugin_get_request_t, 
                                               askue_plugin_valid_checksum_t,
                                               askue_plugin_valid_content_t,
                                               askue_plugin_get_result_t,
                                               askue_module_in_params_t,
                                               const void *parametr,
                                               void *output );


#endif /* */
