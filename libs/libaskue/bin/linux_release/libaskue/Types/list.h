#ifndef ASKUE_LIST_H_
#define ASKUE_LIST_H_

typedef struct _list_t
{
	struct _list_t *prev;
	struct _list_t *next;
} list_t;

/*
 * Создать новый элемент списка
 */
list_t* list_new ( void );

/*
 * Связать
 */
list_t* list_bind ( list_t **l1, list_t **l2 );

/*
 * Развязать
 */
void list_unbind ( list_t **l1, list_t **l2 );

/*
 * Удалить элемент списка
 */
void list_delete ( list_t *L );

#endif /* ASKUE_LIST_H_ */
