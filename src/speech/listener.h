#ifndef LISTENER_H
#define LISTENER_H

#include <stdbool.h>

#include "apps/catalog.h"
#include "command/intent.h"
#include "speech/endpoint.h"
#include "speech/transcribe.h"

/* Silero's window at 16 kHz. */
#define VAD_WINDOW 512
#define IDLE_SAMPLES (32 * VAD_WINDOW)
#define AUDIO_MAX (IDLE_SAMPLES + ENDPOINT_MAX_WINDOWS * VAD_WINDOW)

struct listener {
	struct endpoint endpoint;
	struct transcriber transcriber;
	struct catalog apps;
	struct whisper_vad_context *vad;
	/* Fires once the pause settles. */
	struct intent held;
	bool chatty;
	int len;
	int start;
	float audio[AUDIO_MAX];
};

bool listener_open(struct listener *l, const char *whisper_model, const char *vad_model);
bool listener_window(struct listener *l, const float *window, struct intent *out);

#endif
