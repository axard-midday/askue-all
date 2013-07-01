/*
 * int32_array.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_int32_array_H_
#define ASKUE_int32_array_H_

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
typedef struct _int32_array_t
{
	size_t len;
	int32_t data[ 0 ];
} int32_array_t;

/*
 * Сравнение двух каких-либо массивов
 */
typedef bool_t (*int32_array_compare_fptr) ( const int32_array_t*, const int32_array_t* );

/*
 * Создание массива из данных
 */
typedef int32_array_t* (*int32_array_generate_fptr) ( const int32_t*, size_t );

/*
 * Изменение массива
 */
typedef int32_array_t* (*int32_array_modify_fptr) ( int32_array_t* );

/*
 * Создать массив
 */
int32_array_t* int32_array_new ( size_t size );

/*
 * Удалить массив
 */
int32_array_t* int32_array_delete ( int32_array_t* ptr );

/*
 * Изменить размер массива
 */
int32_array_t* int32_array_resize ( int32_array_t* ptr, size_t size );

/*
 * Добавить в конец данные
 */
int32_array_t* int32_array_append_data ( int32_array_t* dest, const int32_t *data, size_t data_size );

/*
 * Добавить в конец массив
 */
int32_array_t* int32_array_append_ba ( int32_array_t* dest, const int32_array_t* src );

/*
 * Добавить в начало данные
 */
int32_array_t* int32_array_prepend_data ( int32_array_t* dest, const int32_t *data, size_t data_size );

/*
 * Добавить в начало массив
 */
int32_array_t* int32_array_prepend_ba ( int32_array_t* dest, const int32_array_t* src );

/*
 * Вставить данные перед индексом
 */
int32_array_t* int32_array_insert_data_before ( int32_array_t *dest, const int32_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив перед индексом
 */
int32_array_t* int32_array_insert_ba_before ( int32_array_t *dest, const int32_array_t *src, size_t index );

/*
 * Вставить данные после индекса
 */
int32_array_t* int32_array_insert_data_after ( int32_array_t *dest, const int32_t *data, size_t data_size, size_t index );

/*
 * Вставить другой массив после индекса
 */
int32_array_t* int32_array_insert_ba_after ( int32_array_t *dest, const int32_array_t *src, size_t index );

/*
 * Удалить промежуток
 */
int32_array_t* int32_array_remove_range ( int32_array_t *dest, size_t start_index, size_t range_len );

/*
 * Перезаписать массив данными
 */
int32_array_t* int32_array_update_data ( int32_array_t *dest, const int32_t *data, size_t data_size );

/*
 * Перезаписать другим массивом 
 */
int32_array_t* int32_array_update_ba ( int32_array_t *dest, const int32_array_t *src );

/*
 * Создать массив байт из форматной строки
 */
int32_array_t* int32_array_printf_in ( const char *fmt, ... );

/*
 * Вывод в файл массива байт
 */
int int32_array_printf_out ( FILE *output, const char *elem_fmt, const int32_array_t *u8a );

#endif /* ASKUE_int32_array_H_ */
