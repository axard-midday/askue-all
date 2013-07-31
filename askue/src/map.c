#include <stdio.h>

typedef struct
{
    void *Key;
    void *Value;
} map_item_t;

typedef struct
{
    size_t size;
    map_item_t item[ 0 ];
} map_t

void map_init ( map_t *map )
{
    map
}

int main(int argc, char **argv)
{
	
	return 0;
}

