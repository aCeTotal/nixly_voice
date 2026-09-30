#ifndef PHONETIC_H
#define PHONETIC_H

#include <stddef.h>

#include "command/fold.h"

/* Folded spelling to rough sound. */
size_t phonetic(const char *folded, size_t len, char out[FOLD_MAX]);

#endif
