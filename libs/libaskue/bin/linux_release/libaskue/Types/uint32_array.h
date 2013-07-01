/*
 * uint32_array.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_uint32_array_H_
#define ASKUE_uint32_array_H_

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
typedef struct _uint32_array_t
{
	size_t len;
	uint32_t data[ 0 ];
} uint32_array_t;

/*
 * Сравнение двух каких-либо массивов
 */
typedef bool_t (*uint32_array_compare_fptr) ( const uint32_array_t*, const uint32_array_t* );

/*
 * Создание массива из данных
 */
typedef uint32_array_t* (*uint32_array_generate_fptr) ( const uint32_t*, size_t );

/*
 * Изменение массива
 */
typedef uint32_array_t* (*uint32_array_modify_fptr) ( uint32_array_t* );

/*
 * Создать массив
 */
uint32_array_t* uint32_array_new ( size_t size );

/*
 * Удалить массив
 */
uint32_array_t* uint32_array_delete ( uint32_array_t* ptr );

/*
 * Изменить размер массива
 */
uint32_array_t* uint32_array_resize ( uint32_array_t* ptr, size_t size );

/*
 * Добавить в конец данные
 */
uint32_array_t* uint32_array_append_data ( uint32_array_t* dest, const uint32_t *data, size_t data_size );

/*
 * Добавить в конец массив
 */
uint32_array_t* uint32_array_append_ba ( uint32_array_t* dest, const uint32_array_t* src );

/*
 * Добавить в начало данные
 */
uint32_array_t* uint32_array_prepend_data ( uint32_array_t* dest, const uint32_t *data, size_t data_size );

/*
 * Добавить в начало массив
 */
uint32_array_t* uint32_array_prepend_ba ( uint32_array_t* dest, const uint32_array_t* src );

/*
 * Вставить данные перед индексом
 */
uint32_array_t* uint32_array_insert_data_before ( uint32_array_t *dest, const uint32_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив перед индексом
 */
uint32_array_t* uint32_array_insert_ba_before ( uint32_array_t *dest, const uint32_array_t *src, size_t index );

/*
 * Вставить данные после индекса
 */
uint32_array_t* uint32_array_insert_data_after ( uint32_array_t *dest, const uint32_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив после индекса
 */
uint32_array_t* uint32_array_insert_ba_after ( uint32_array_t *dest, const uint32_array_t *src, size_t index );

/*
 * Удалить промежуток
 */
uint32_array_t* uint32_array_remove_range ( uint32_array_t *dest, size_t start_index, size_t range_len );

/*
 * Перезаписать массив данными
 */
uint32_array_t* uint32_array_update_data ( uint32_array_t *dest, const uint32_t *data, size_t data_size );

/*
 * Перезаписать другим массивом 
 */
uint32_array_t* uint32_array_update_ba ( uint32_array_t *dest, const uint32_array_t *src );

/*
 * Создать массив байт из форматной строки
 */
uint32_array_t* uint32_array_printf_in ( const char *fmt, ... );

/*
 * Вывод в файл массива байт
 */
int uint32_array_printf_out ( FILE *output, const char *elem_fmt, const uint32_array_t *u8a );


#endif /* ASKUE_uint32_array_H_ */
