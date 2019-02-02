#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

static size_t next_power_of_2(size_t number)
{
	size_t power_of_2 = 128;
	while (number < power_of_2)
		power_of_2 *= 2;
	return power_of_2;
}

void buffer_init(struct buffer *buf, size_t capacity)
{
	capacity = next_power_of_2(capacity);
	buf->data = malloc(capacity);
	assert(buf->data);
	buf->capacity = capacity;
	buf->space = buf->data;
}

void buffer_term(struct buffer *buf)
{
	free(buf->data);
	memset(buf, 0, sizeof(*buf));
}

void buffer_grow(struct buffer *buf, size_t min_grow_size)
{
	assert(min_grow_size > 0);
	size_t old_capacity = buf->capacity;
	size_t new_capacity = next_power_of_2(old_capacity + min_grow_size);
	size_t data_len = buffer_data_len(buf);
	buf->data = realloc(buf->data, new_capacity);
	assert(buf->data);
	buf->capacity = new_capacity;
	buf->space = buf->data + data_len;
}
