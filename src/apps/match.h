#ifndef MATCH_H
#define MATCH_H

#include <stdbool.h>
#include <stddef.h>

#include "apps/catalog.h"

struct match {
	int app;
	int cost;
	int len;
	bool open;
};

#define MATCH_NONE ((struct match){ .app = -1 })

/* Closest app to folded span. */
struct match match_span(const struct catalog *c, const char *folded, size_t len);
/* Cheaper wins, then longer key. */
struct match match_better(struct match a, struct match b);

#endif
