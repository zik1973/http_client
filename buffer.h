#pragma once
#include <stddef.h>

struct buffer {
	size_t	capacity;
	char	*data;
	char	*space;
};

void buffer_init(struct buffer *buf, size_t capacity);
void buffer_term(struct buffer *buf);

static inline void *buffer_data(struct buffer *buf)
{
	return buf->data;
}

static inline size_t buffer_data_len(struct buffer *buf)
{
	return buf->space - buf->data;
}

static inline size_t buffer_space_len(struct buffer *buf)
{
	return buf->data + buf->capacity - buf->space;
}

void buffer_grow(struct buffer *buf, size_t min_grow_size);

static inline void buffer_reserve(struct buffer *buf, size_t size)
{
	if (buf->space + size > buf->data + buf->capacity)
		buffer_grow(buf, (buf->space + size) - (buf->data + buf->capacity));
}
