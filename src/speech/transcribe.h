#ifndef TRANSCRIBE_H
#define TRANSCRIBE_H

#include <stdbool.h>

#include <whisper.h>

#include "command/intent.h"

#define LANGUAGES 1
#define TEXT_MAX QUERY_MAX
#define PREFIX_MAX 4

struct pcm {
	const float *data;
	int len;
};

struct heard {
	struct intent intent;
	const char *language;
	char text[TEXT_MAX];
};

struct transcriber {
	const struct catalog *apps;
	struct whisper_context *ctx;
	struct whisper_state *state;
	int n_vocab;
	whisper_token sot;
	whisper_token eot;
	whisper_token nosp;
	whisper_token task;
	whisper_token notimestamps;
	whisper_token lang[LANGUAGES];
	whisper_token google[PREFIX_MAX];
	int n_google;
};

bool transcriber_open(struct transcriber *t, const char *model, const struct catalog *apps);
bool transcribe(struct transcriber *t, struct pcm audio, struct heard *out);
/* Decodes after a forced Google. */
bool transcribe_search(struct transcriber *t, struct pcm audio, struct heard *out);

#endif
