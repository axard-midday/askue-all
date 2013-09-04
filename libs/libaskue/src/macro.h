
#ifndef ASKUE_MACRO_H
#define ASKUE_MACRO_H


#include <stdint.h>
#include <stddef.h>

#define ASKUE_FLAG_VERBOSE 0
#define ASKUE_FLAG_DEBUG 1
#define ASKUE_FLAG_PROTOCOL 2


/*
 * СТРОКОВЫЕ БУФЕРЫ
 */
// специальные буферы (без учёта конечного символа)
#define TIME_STRBUF                 8
#define DATE_STRBUF                 10

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

#endif /* ASKUE_MACRO_H */
