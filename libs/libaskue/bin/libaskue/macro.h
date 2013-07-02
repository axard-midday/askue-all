/*
 * common_macro.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef LIBASKUE_MACRO_H
#define LIBASKUE_MACRO_H


#include <stdint.h>
#include <stddef.h>

/*
 * ФАЙЛЫ ИСПОЛЬЗУЕМЫЕ ПРОГРАММОЙ
 */

#define LOG_FILE "/mnt/base/reports/askue.log"
#define DB_FILE "/mnt/base/askue.db.sqlite3"
#define CFG_FILE "/etc/askue/askue.cfg"
#define HELP_FILE "/etc/askue/askue.help"

/*
 * СТРОКОВЫЕ БУФЕРЫ
 */

// общецелевой буфер
#define STRING_BUF 512

// специальные буферы (без учёта конечного символа)
#define TIME_STRING_BUF 8
#define DATE_STRING_BUF 10

// с учётом разделительного символа (без учёта конечного символа)
#define DATE_TIME_STRING_BUF ( TIME_STRING_BUF + DATE_STRING_BUF + 1 )

/*
 * ПРОВЕРКА БИТОВ
 */

// установить бит
#define SETBIT(bitfield, bitindex) \
	( ( bitfield ) |= 1 << ( bitindex ) )

// снять бит
#define UNSETBIT(bitfield, bitindex) \
	( ( bitfield ) &= ~( 1 << ( bitindex ) ) )

// переключить бит
#define TOGGLEBIT(bitfield, bitindex) \
	( ( bitfield ) ^= 1 << ( bitindex ) )

// выделить бит вернёт 1 если установлен
#define TESTBIT(bitfield, bitindex) \
	( !!( ( bitfield ) & ( 1 << ( bitindex ) ) ) )

/*
 * УСТАНОВКА / СНЯТИЕ БАЙТ
 */

// вытащить байт из переменной
#define GET_BYTE(X, POSITION) \
 ({\
   __typeof__ (X) _X = (X); \
   uint8_t _X_OUT = 0x00; \
   size_t _POSITION = (size_t)(POSITION); \
   if ( sizeof(_X) > _POSITION ) \
    { \
	   _X_OUT = (((uint8_t*)&_X)[_POSITION] ); \
    }  \
    ( _X_OUT ); \
 })

// записать байт в переменную
#define SET_BYTE(X, POSITION, VALUE) \
 ({\
  __typeof__ (X) _X = (X); \
  __typeof__ (VALUE) _VALUE = VALUE;\
  __typeof__ (POSITION) _POSITION = (POSITION); \
  if ( sizeof(_X) > _POSITION ) \
	{ \
		((uint8_t*)&_X)[_POSITION] = ((uint8_t*)&_VALUE)[0];\
		X = _X;\
	}\
  ( _X ); \
 })

#endif /* LIBASKUE_MACRO_H */
