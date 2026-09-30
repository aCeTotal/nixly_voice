#include "speech/listener.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <ggml-backend.h>

/* 320 ms kept before speech. */
#define PREROLL (10 * VAD_WINDOW)
/* Mean power of -70 dBFS. */
#define SILENCE_POWER 1e-7f

typedef bool (*transcribe_fn)(struct transcriber *, struct pcm, struct heard *);

static void quiet(enum ggml_log_level level, const char *text, void *user)
{
	(void)user;
	if (level == GGML_LOG_LEVEL_WARN || level == GGML_LOG_LEVEL_ERROR)
		fputs(text, stderr);
}

bool listener_open(struct listener *l, const char *whisper_model, const char *vad_model)
{
	struct whisper_vad_context_params params = whisper_vad_default_context_params();

	whisper_log_set(quiet, NULL);
	ggml_backend_load_all();
	params.n_threads = 1;
	l->vad = whisper_vad_init_from_file_with_params(vad_model, params);
	if (l->vad == NULL)
		return false;
	/* Init leaves LSTM state uninitialized. */
	whisper_vad_reset_state(l->vad);
	l->chatty = isatty(STDERR_FILENO);
	return transcriber_open(&l->transcriber, whisper_model);
}

static bool collecting(const struct listener *l)
{
	return l->endpoint.phase == PHASE_LISTEN || l->endpoint.phase == PHASE_SEARCH;
}

/* Idle audio shrinks to preroll. */
static void keep(struct listener *l, const float *window)
{
	if (!collecting(l) && l->len + VAD_WINDOW > IDLE_SAMPLES) {
		memmove(l->audio, l->audio + l->len - PREROLL, PREROLL * sizeof *l->audio);
		l->len = PREROLL;
	}
	memcpy(l->audio + l->len, window, VAD_WINDOW * sizeof *window);
	l->len += VAD_WINDOW;
}

static long elapsed_ms(struct timespec from, struct timespec to)
{
	return (to.tv_sec - from.tv_sec) * 1000 + (to.tv_nsec - from.tv_nsec) / 1000000;
}

static bool hear(struct listener *l, transcribe_fn fn, struct heard *h)
{
	struct timespec from, to;
	bool ok;

	clock_gettime(CLOCK_MONOTONIC, &from);
	ok = fn(&l->transcriber, (struct pcm){ l->audio + l->start, l->len - l->start }, h);
	clock_gettime(CLOCK_MONOTONIC, &to);
	if (l->chatty)
		fprintf(stderr, "nixly-voice: [%s %ld ms] %s\n",
			h->language ? h->language : "--", elapsed_ms(from, to), h->text);
	if (ok)
		endpoint_heard(&l->endpoint, &h->intent);
	return ok;
}

static bool probe(struct listener *l, struct intent *out)
{
	struct heard h;

	if (!hear(l, transcribe, &h))
		return false;
	*out = h.intent;
	return intent_launches(h.intent.kind);
}

static void refresh(struct listener *l)
{
	struct heard h;

	if (hear(l, transcribe_search, &h))
		l->search = h.intent;
}

static bool commit(const struct listener *l, struct intent *out)
{
	*out = l->search;
	return out->kind == INTENT_SEARCH && out->query[0] != '\0';
}

/* Muted mics skip inference. */
static bool silent(const float *window)
{
	float energy = 0.0f;

	for (int i = 0; i < VAD_WINDOW; i++)
		energy += window[i] * window[i];
	return energy < VAD_WINDOW * SILENCE_POWER;
}

static float speech_prob(struct listener *l, const float *window)
{
	if (silent(window) || !whisper_vad_detect_speech_no_reset(l->vad, window, VAD_WINDOW))
		return 0.0f;
	return whisper_vad_probs(l->vad)[0];
}

bool listener_window(struct listener *l, const float *window, struct intent *out)
{
	float prob = speech_prob(l, window);

	keep(l, window);
	switch (endpoint_push(&l->endpoint, prob)) {
	case STEP_BEGIN:
		l->start = l->len > PREROLL + VAD_WINDOW ? l->len - PREROLL - VAD_WINDOW : 0;
		l->search.kind = INTENT_NONE;
		return false;
	case STEP_PROBE:
		return probe(l, out);
	case STEP_REFRESH:
		refresh(l);
		return false;
	case STEP_FINAL:
		refresh(l);
		return commit(l, out);
	case STEP_COMMIT:
		return commit(l, out);
	case STEP_NONE:
		return false;
	}
	return false;
}
