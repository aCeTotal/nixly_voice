#ifndef FOLD_H
#define FOLD_H

#include <stddef.h>

#define FOLD_MAX 48

void fold(const char *src, size_t len, char dst[FOLD_MAX]);

#endif
