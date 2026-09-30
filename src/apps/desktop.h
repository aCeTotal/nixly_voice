#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdbool.h>
#include <stddef.h>

/* Name, then Name[nb], [nn], [no]. */
#define DESKTOP_NAMES 4
#define DESKTOP_VALUE_MAX 256

struct desktop {
	char names[DESKTOP_NAMES][DESKTOP_VALUE_MAX];
	char program[DESKTOP_VALUE_MAX];
	bool no_display;
	bool hidden;
};

/* True when nixly_launcher lists it. */
bool desktop_parse(const char *text, size_t size, struct desktop *out);

#endif
