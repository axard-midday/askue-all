/*
 * uint8_array.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_uint8_array_H_
#define ASKUE_uint8_array_H_

#define _GNU_SOURCE

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "bool_type.h"

/*
 * Массив байт
 */
typedef struct _uint8_array_t
{
	size_t len;
	uint8_t data[ 0 ];
} uint8_array_t;

/*
 * Сравнение двух каких-либо массивов
 */
typedef bool_t (*uint8_array_compare_f) ( const uint8_array_t*, const uint8_array_t* );

/*
 * Изменение через входные данные
 */
typedef uint8_array_t* (*uint8_array_update_data_f) ( uint8_array_t*, const uint8_t*, size_t );

/*
 * Изменение через входной массив
 */
typedef uint8_array_t* (*uint8_array_update_u8a_f) ( uint8_array_t*, const uint8_array_t* );

/*
 * Добавление входных данных
 */
typedef uint8_array_t* (*uint8_array_append_data_f) ( uint8_array_t*, const uint8_t*, size_t );

/*
 * Добавление входного массива
 */
typedef uint8_array_t* (*uint8_array_append_u8a_f) ( uint8_array_t*, const uint8_array_t* );

/*
 * Изменение массива
 */
typedef uint8_array_t* (*uint8_array_modify_fptr) ( uint8_array_t* );

/*
 * Создать массив
 */
uint8_array_t* uint8_array_new ( size_t size );

/*
 * Удалить массив
 */
uint8_array_t* uint8_array_delete ( uint8_array_t* ptr );

/*
 * Изменить размер массива
 */
uint8_array_t* uint8_array_resize ( uint8_array_t* ptr, size_t size );

/*
 * Добавить в конец данные
 */
uint8_array_t* uint8_array_append_data ( uint8_array_t* dest, const uint8_t *data, size_t data_size );

/*
 * Добавить в конец массив
 */
uint8_array_t* uint8_array_append_u8a ( uint8_array_t* dest, const uint8_array_t* src );

/*
 * Перезаписать массив данными
 */
uint8_array_t* uint8_array_update_data ( uint8_array_t *dest, const uint8_t *data, size_t data_size );

/*
 * Перезаписать другим массивом 
 */
uint8_array_t* uint8_array_update_u8a ( uint8_array_t *dest, const uint8_array_t *src );


#endif /* ASKUE_uint8_array_H_ */
