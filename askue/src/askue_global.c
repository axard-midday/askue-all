#include <stdio.h>
#include <stdint.h>

#include "askue_launcher_macro.h"

/*
 * Переменная флагов
 */
uint32_t Askue_Launcher_Flags;

/*
 * Кол-во строк в логе 
 */
size_t Askue_Launcher_Log_Line_Amount;

/*
 * Первоначальная настройка флагов
 */
void askue_launcher_flags_to_default ( void )
{
	Askue_Launcher_Flags = 0; // флагов нет
}

/*
 * Установить кол-во строк в логе по умолчанию
 */
void askue_launcher_log_line_default_amount ( void )
{
	Askue_Launcher_Log_Line_Amount = ASKUE_LOG_LINE_AMOUNT; // 1000 строк файла лога
}
