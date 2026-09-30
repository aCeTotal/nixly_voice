#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdbool.h>

#include "audio/ring.h"

#define CAPTURE_RATE 16000

bool capture_start(struct ring *ring);

#endif
