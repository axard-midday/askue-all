/*
 * double_array.h
 *
 *  Created on: 05.12.2012
 *      Author: andrey
 */

#ifndef ASKUE_double_array_H_
#define ASKUE_double_array_H_

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
typedef struct _double_array_t
{
	size_t len;
	double data[ 0 ];
} double_array_t;

/*
 * Сравнение двух каких-либо массивов
 */
typedef bool_t (*double_array_compare_fptr) ( const double_array_t*, const double_array_t* );

/*
 * Создание массива из данных
 */
typedef double_array_t* (*double_array_generate_fptr) ( const double*, size_t );

/*
 * Изменение массива
 */
typedef double_array_t* (*double_array_modify_fptr) ( double_array_t* );

/*
 * Создать массив
 */
double_array_t* double_array_new ( size_t size );

/*
 * Удалить массив
 */
double_array_t* double_array_delete ( double_array_t* ptr );

/*
 * Изменить размер массива
 */
double_array_t* double_array_resize ( double_array_t* ptr, size_t size );

/*
 * Добавить в конец данные
 */
double_array_t* double_array_append_data ( double_array_t* dest, const double *data, size_t data_size );

/*
 * Добавить в конец массив
 */
double_array_t* double_array_append_ba ( double_array_t* dest, const double_array_t* src );

/*
 * Добавить в начало данные
 */
double_array_t* double_array_prepend_data ( double_array_t* dest, const double *data, size_t data_size );

/*
 * Добавить в начало массив
 */
double_array_t* double_array_prepend_ba ( double_array_t* dest, const double_array_t* src );

/*
 * Вставить данные перед индексом
 */
double_array_t* double_array_insert_data_before ( double_array_t *dest, const double *data, size_t data_size, size_t index );

/*
 * Вставить другой массив перед индексом
 */
double_array_t* double_array_insert_ba_before ( double_array_t *dest, const double_array_t *src, size_t index );

/*
 * Вставить данные после индекса
 */
double_array_t* double_array_insert_data_after ( double_array_t *dest, const double *data, size_t data_size, size_t index );

/*
 * Вставить другой массив после индекса
 */
double_array_t* double_array_insert_ba_after ( double_array_t *dest, const double_array_t *src, size_t index );

/*
 * Удалить промежуток
 */
double_array_t* double_array_remove_range ( double_array_t *dest, size_t start_index, size_t range_len );

/*
 * Перезаписать массив данными
 */
double_array_t* double_array_update_data ( double_array_t *dest, const double *data, size_t data_size );

/*
 * Перезаписать другим массивом 
 */
double_array_t* double_array_update_ba ( double_array_t *dest, const double_array_t *src );

/*
 * Создать массив байт из форматной строки
 */
double_array_t* double_array_printf_in ( const char *fmt, ... );

/*
 * Вывод в файл массива байт
 */
int double_array_printf_out ( FILE *output, const char *elem_fmt, const double_array_t *u8a );

#endif /* ASKUE_double_array_H_ */
