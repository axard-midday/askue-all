#ifndef ASKUE_CONTAINER_H_
#define ASKUE_CONTAINER_H_

#include "list.h"

/*
 * Возможные типы данных контейнера
 */
typedef enum
{
	askue_type_no,					// контейнер пуст
	
	askue_type_ptr,					// указатель на что-либо					
	
	askue_type_uint8,				// uint8_t
	askue_type_uint16,				// uint16_t
	askue_type_uint32,				// uint32_t
	askue_type_uint64,				// uint64_t
	
	askue_type_double,				// double
	
	askue_type_int8,				// int8_t
	askue_type_int16,				// int16_t
	askue_type_int32,				// int32_t
	askue_type_int64,				// int64_t
	
	askue_type_uint8_array,				// массив uint8_t с кол-ва элементов
	askue_type_uint16_array,			// массив uint16_t с кол-ва элементов			
	askue_type_uint32_array,			// массив uint32_t с кол-ва элементов
	askue_type_uint64_array,			// массив uint64_t с кол-ва элементов
	
	askue_type_int8_array,				// массив int8_t с кол-ва элементов
	askue_type_int16_array,				// массив uint16_t с кол-ва элементов
	askue_type_int32_array,				// массив uint32_t с кол-ва элементов
	askue_type_int64_array,				// массив uint64_t с кол-ва элементов
	
	askue_type_double_array,			// массив double с кол-вом элементов
	
	askue_type_string,				// char*
} askue_type_t;

typedef struct
{
	askue_type_t type;
	void *content;
	list_t *item;
} askue_container_t;

/*
 * Получить контейнер следующий за данным
 */
askue_container_t* askue_container_next ( const askue_container_t *_0 );

/*
 * Получить контейнер предыдущий данному
 */
askue_container_t* askue_container_prev ( const askue_container_t *_0 );

/*
 * Создать новый контейнер
 */
askue_container_t* askue_container_new ( void );

/*
 * Удалить контейнер
 */
askue_container_t* askue_container_delete ( askue_container_t *src );

/*
 * Запись в контейнер
 */
askue_container_t* askue_container_in ( askue_container_t *src, const void *data, askue_type_t data_type );

/*
 * Чтение из контейнера
 */
void* askue_container_out ( const askue_container_t *src );

/*
 * Привязать к контейнеру 1 контейнер 2
 */
askue_container_t* askue_container_bind ( askue_container_t *_1, askue_container_t *_2 );

/*
 * Развязать контейнеры 1 и 2
 */
void askue_container_unbind ( askue_container_t *_1, askue_container_t *_2 );

/*
 * Запись в контейнер
 */
askue_container_t* askue_container_copy ( const askue_container_t *src );

/*
 * Удалить содержимое
 */
askue_container_t* askue_container_delete_content ( askue_container_t *src );

#endif /* ASKUE_CONTAINER_H_ */
