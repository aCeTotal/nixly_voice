#ifndef RING_H
#define RING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#define RING_SIZE (1 << 16)

struct ring {
	float buf[RING_SIZE];
	atomic_size_t head;
	atomic_size_t tail;
	atomic_bool closed;
	int wake;
};

bool ring_init(struct ring *r);
void ring_push(struct ring *r, const float *pcm, size_t n);
bool ring_pop(struct ring *r, float *out, size_t n);
bool ring_wait(struct ring *r);
void ring_close(struct ring *r);

#endif
