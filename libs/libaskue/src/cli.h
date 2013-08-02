#ifndef COMMAND_LINE_INTERFACE_H_
#define COMMAND_LINE_INTERFACE_H_

typedef int ( *cli_handler_f )( void *ptr, int *flag, const char *arg );

typedef enum 
{
    CLI_SHORT_OPT = 1,
    CLI_LONG_OPT = 2,
    CLI_NO_OPT = 3
} cli_option_type_t;

typedef enum 
{
    CLI_REQUIRED_ARG = 1,
    CLI_OPTIONAL_ARG = 2,
    CLI_NO_ARG = 3
} cli_argument_type_t;

typedef struct _cli_option_t
{
    const char *longname;
    char shortname;
    cli_argument_type_t has_arg;
    cli_handler_f handler;
    void *outvar;
    int *flag;
} cli_option_t;

#define CLI_LAST_OPTION ((cli_option_t){ NULL, 0, 0, NULL, NULL, NULL })

typedef enum
{
    CLI_SUCCESS = 0,
    CLI_ERROR_NOARG = 1, // нет требуемого за опцией аргумента
    CLI_ERROR_ARG = 2, // неправильный аргумент
    CLI_ERROR_NF_OPT = 3, // в списке cli_option_t* не найдено опций
    CLI_ERROR_HANDLER = 4, // ошибка обработчика
    CLI_ERROR_OPTYPE = 5 // неправильно указана опция
} cli_result_t;

cli_result_t cli_parse ( cli_option_t *Option, int argc, char** argv );

#endif /* COMMAND_LINE_INTERFACE_H_ */
