#ifndef ASKUE_LAUNCHER_GLOBAL_H_
#define ASKUE_LAUNCHER_GLOBAL_H_

#include <stdint.h>

/*
 * Список флагов
 */
#define Debug_Flag 0
#define Exit_Flag 1
#define Terminal_Flag 2
#define Log_Flag 3
#define Log_Cut 4
#define Help_Flag 5

/*
 * Переменная флагов
 */
 
extern uint32_t Askue_Launcher_Flags;

extern size_t Askue_Launcher_Log_Line_Amount;

/*
 * Первоначальная настройка флагов
 */
 
void askue_launcher_flags_to_default ( void );

/*
 * Установить число строк в логе по умолчанию
 */
void askue_launcher_log_line_default_amount ( void );


#endif /* ASKUE_LAUNCHER_MACRO_H_ */
