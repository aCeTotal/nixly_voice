#ifndef KEYS_H
#define KEYS_H

#include "apps/catalog.h"
#include "apps/desktop.h"

/* Every way to say it. */
void keys_add(struct catalog *c, int app, const struct desktop *d);
/* Sorts, drops clashes, marks prefixes. */
void keys_index(struct catalog *c);

#endif
