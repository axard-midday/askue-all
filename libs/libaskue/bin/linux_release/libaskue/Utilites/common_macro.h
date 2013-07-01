/*
 * common_macro.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_COMMON_MACRO_H_
#define ASKUE_COMMON_MACRO_H_


#include <stdint.h>
#include <stddef.h>

#define __LIBNAME1 "libaskue"
#define __LIBASKUE__

//способный буфер
#define BUF256 256

//специальные буферы (без учёта конечного символа)
#define TIME_STRING_BUF 8
#define DATE_STRING_BUF 10

#define REGISTRATION_TYPE_STRING_BUF 4

//с учётом разделительного символа
#define DATE_TIME_STRING_BUF ( TIME_STRING_BUF + DATE_STRING_BUF + 1 )

//установить бит
#define SETBIT(bitfield, bitindex) \
	( ( bitfield ) |= 1 << ( bitindex ) )

//снять бит
#define UNSETBIT(bitfield, bitindex) \
	( ( bitfield ) &= ~( 1 << ( bitindex ) ) )

//переключить бит
#define TOGGLEBIT(bitfield, bitindex) \
	( ( bitfield ) ^= 1 << ( bitindex ) )

//выделить бит вернёт 1 если установлен
#define TESTBIT(bitfield, bitindex) \
	( !!( ( bitfield ) & ( 1 << ( bitindex ) ) ) )

//вытащить байт из переменной
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

//записать байт в переменную
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

#define REVERSE_BYTE_ORDER( X )\
({\
	__typeof__ (X) _X = (X); \
	__typeof__ (X) _X_OUT = 0; \
	for ( size_t i = 0; i < sizeof (_X); i++ )\
		((uint8_t*)&_X_OUT)[ i ] = ((uint8_t*)&_X)[ sizeof ( _X ) - 1 - i ];\
	(_X_OUT);\
})

#define TEMP_VAR( TYPE, X )\
({\
	TYPE _X = (TYPE)( X );\
	( _X );\
})

//дописывает байт на самый младший адрес
//всё остальное сдвигается влево на 8 бит
#define PUSH_BYTE(X, VALUE) \
 ({\
  __typeof__ (X) _X = ( (X) << 8 ); \
  uint8_t _VALUE = (VALUE & 0xff); \
  ( _X &= ~(0xff) ); \
  ( _X |= _VALUE ); \
  ( X = (_X) ); \
  ( _X ); \
 })

//вытащить байт с самого младщего адреса
//остальное сдвигается на 8 байт вправо
#define POP_BYTE(X) \
 ({ \
	__typeof__ (X) _X = (X); \
	uint8_t _OUT = 0; \
	__typeof__ (X) _SHIFT = 0;\
	( _OUT = _X & 0xff ); \
	for ( size_t __INDEX = 0; __INDEX < (sizeof(_X) - 1); __INDEX++ ) \
	{ \
		PUSH_BYTE(_SHIFT, 0xff); \
	} \
	( X = _SHIFT & (_X >> 8)); \
	( _OUT );\
 })


#endif /* ASKUE_COMMON_MACRO_H_ */
