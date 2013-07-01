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
typedef bool_t (*uint8_array_compare_fptr) ( const uint8_array_t*, const uint8_array_t* );

/*
 * Создание массива из данных
 */
typedef uint8_array_t* (*uint8_array_generate_fptr) ( const uint8_t*, size_t );

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
uint8_array_t* uint8_array_append_ba ( uint8_array_t* dest, const uint8_array_t* src );

/*
 * Добавить в начало данные
 */
uint8_array_t* uint8_array_prepend_data ( uint8_array_t* dest, const uint8_t *data, size_t data_size );

/*
 * Добавить в начало массив
 */
uint8_array_t* uint8_array_prepend_ba ( uint8_array_t* dest, const uint8_array_t* src );

/*
 * Вставить данные перед индексом
 */
uint8_array_t* uint8_array_insert_data_before ( uint8_array_t *dest, const uint8_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив перед индексом
 */
uint8_array_t* uint8_array_insert_ba_before ( uint8_array_t *dest, const uint8_array_t *src, size_t index );

/*
 * Вставить данные после индекса
 */
uint8_array_t* uint8_array_insert_data_after ( uint8_array_t *dest, const uint8_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив после индекса
 */
uint8_array_t* uint8_array_insert_ba_after ( uint8_array_t *dest, const uint8_array_t *src, size_t index );

/*
 * Удалить промежуток
 */
uint8_array_t* uint8_array_remove_range ( uint8_array_t *dest, size_t start_index, size_t range_len );

/*
 * Перезаписать массив данными
 */
uint8_array_t* uint8_array_update_data ( uint8_array_t *dest, const uint8_t *data, size_t data_size );

/*
 * Перезаписать другим массивом 
 */
uint8_array_t* uint8_array_update_ba ( uint8_array_t *dest, const uint8_array_t *src );

/*
 * Создать массив байт из форматной строки
 */
uint8_array_t* uint8_array_printf_in ( const char *fmt, ... );

/*
 * Вывод в файл массива байт
 */
int uint8_array_printf_out ( FILE *output, const char *elem_fmt, const uint8_array_t *u8a );

#endif /* ASKUE_uint8_array_H_ */
