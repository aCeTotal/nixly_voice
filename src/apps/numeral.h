#ifndef NUMERAL_H
#define NUMERAL_H

#include <stdbool.h>

#include "apps/desktop.h"

/* Digits said as Norwegian words. */
bool numeral_words(const char *label, char out[DESKTOP_VALUE_MAX]);

#endif
