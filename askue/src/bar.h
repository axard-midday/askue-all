#ifndef FOOBAR_H_
#define FOOBAR_H_

typedef struct 
{
	uint8_t id;
	uint16_t address;
	size_t amount;
} memory16_info_t;

typedef struct
{
	memory16_info_t info;
	byte_array_t *content;
} memory16_slice_t;

typedef struct
{
	char *value;
	uint8_t weekday;
	uint8_t season;
} datetime_t;

typedef struct
{
	uint8_t level;
	char *pwd;
	char *confirm;
} password_t

#endif
