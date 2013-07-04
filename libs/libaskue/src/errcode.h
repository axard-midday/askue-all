#ifndef ASKUE_ERRORCODE_H_
#define ASKUE_ERRORCODE_H_

/* 
 * Коды ошибок:
 *  EE_{NAME} статусы выхода
 */
#define EE_SUCCESS EXIT_SUCCESS
#define EE_UNKNOWN EXIT_FAILURE
#define EE_MALLOC 2
#define EE_REALLOC 3
#define EE_LOG_FOPEN 4
#define EE_PRINTF 5 // ошибка какой-то из функций серии printf
#define EE_WRITE 6
#define EE_READ 7
#define EE_TIMER 8

#endif /* ASKUE_ERRORCODE_H_ */
