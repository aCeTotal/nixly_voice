#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <stdbool.h>

#include "command/intent.h"

/* Twelve seconds of windows. */
#define ENDPOINT_MAX_WINDOWS 375

enum phase {
	PHASE_IDLE,
	PHASE_LISTEN,
	PHASE_SEARCH,
	PHASE_DONE,
};

enum step {
	STEP_NONE,
	STEP_BEGIN,
	STEP_PROBE,
	STEP_REFRESH,
	STEP_COMMIT,
	STEP_FINAL,
};

struct endpoint {
	enum phase phase;
	bool voiced;
	bool awaiting;
	int windows;
	int speech;
	int silence;
	int fresh;
};

enum step endpoint_push(struct endpoint *e, float prob);
void endpoint_heard(struct endpoint *e, const struct intent *in);

#endif
