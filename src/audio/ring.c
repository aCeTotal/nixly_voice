#include "audio/ring.h"

#include <errno.h>
#include <string.h>
#include <sys/eventfd.h>

#define RING_MASK (RING_SIZE - 1)

bool ring_init(struct ring *r)
{
	atomic_init(&r->head, 0);
	atomic_init(&r->tail, 0);
	atomic_init(&r->closed, false);
	r->wake = eventfd(0, EFD_CLOEXEC);
	return r->wake >= 0;
}

/* Realtime side; drops when full. */
void ring_push(struct ring *r, const float *pcm, size_t n)
{
	size_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
	size_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
	size_t room = RING_SIZE - (head - tail);
	size_t at = head & RING_MASK;
	size_t first;

	if (n > room)
		n = room;
	first = n < RING_SIZE - at ? n : RING_SIZE - at;
	memcpy(r->buf + at, pcm, first * sizeof *pcm);
	memcpy(r->buf, pcm + first, (n - first) * sizeof *pcm);
	atomic_store_explicit(&r->head, head + n, memory_order_release);
	eventfd_write(r->wake, 1);
}

bool ring_pop(struct ring *r, float *out, size_t n)
{
	size_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed);
	size_t head = atomic_load_explicit(&r->head, memory_order_acquire);
	size_t at = tail & RING_MASK;
	size_t first = n < RING_SIZE - at ? n : RING_SIZE - at;

	if (head - tail < n)
		return false;
	memcpy(out, r->buf + at, first * sizeof *out);
	memcpy(out + first, r->buf, (n - first) * sizeof *out);
	atomic_store_explicit(&r->tail, tail + n, memory_order_release);
	return true;
}

bool ring_wait(struct ring *r)
{
	eventfd_t pending;

	while (eventfd_read(r->wake, &pending) < 0)
		if (errno != EINTR)
			return false;
	return !atomic_load(&r->closed);
}

void ring_close(struct ring *r)
{
	atomic_store(&r->closed, true);
	eventfd_write(r->wake, 1);
}
