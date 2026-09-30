#include "speech/transcribe.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define THREADS 4
#define MAX_TOKENS 48
#define HYPOTHESES 3
#define LANGUAGE_FLOOR 0.05f
#define NO_SPEECH_MAX 0.6f
#define TASK_TOKENS 3
#define GOOGLE " Google"

/* Norwegian only for now. */
static const char *const languages[LANGUAGES] = {
	"no",
};

struct choice {
	float prob;
	int lang;
};

static const struct heard nothing = { .intent = { .kind = INTENT_UNSURE } };

struct prompt {
	whisper_token tokens[TASK_TOKENS + PREFIX_MAX];
	int n;
	const char *text;
};

static float no_speech(const struct transcriber *t, const float *logits)
{
	float max = logits[0];
	double sum = 0.0;

	for (int i = 1; i < t->n_vocab; i++)
		max = fmaxf(max, logits[i]);
	for (int i = 0; i < t->n_vocab; i++)
		sum += expf(logits[i] - max);
	return (float)(expf(logits[t->nosp] - max) / sum);
}

static int by_prob(const void *a, const void *b)
{
	float pa = ((const struct choice *)a)->prob;
	float pb = ((const struct choice *)b)->prob;

	return (pa < pb) - (pa > pb);
}

/* Softmax over our languages only. */
static void rank(const struct transcriber *t, const float *logits, struct choice ranked[LANGUAGES])
{
	float max = -INFINITY;
	float sum = 0.0f;

	for (int i = 0; i < LANGUAGES; i++)
		max = fmaxf(max, logits[t->lang[i]]);
	for (int i = 0; i < LANGUAGES; i++) {
		ranked[i] = (struct choice){ expf(logits[t->lang[i]] - max), i };
		sum += ranked[i].prob;
	}
	for (int i = 0; i < LANGUAGES; i++)
		ranked[i].prob /= sum;
	qsort(ranked, LANGUAGES, sizeof *ranked, by_prob);
}

static whisper_token argmax(const float *logits, whisper_token last)
{
	whisper_token best = 0;

	for (whisper_token id = 1; id <= last; id++)
		if (logits[id] > logits[best])
			best = id;
	return best;
}

static bool append(char *text, const char *piece)
{
	size_t len = strlen(text);
	size_t n = strlen(piece);

	if (len + n >= TEXT_MAX)
		return false;
	memcpy(text + len, piece, n + 1);
	return true;
}

/* App names may run on. */
static bool settled(enum intent_kind kind)
{
	return kind == INTENT_NONE || (intent_launches(kind) && kind != INTENT_APP);
}

static struct prompt task_prompt(const struct transcriber *t, int lang)
{
	return (struct prompt){ { t->lang[lang], t->task, t->notimestamps }, TASK_TOKENS, "" };
}

static bool decode(struct transcriber *t, const struct prompt *p, struct heard *out)
{
	const whisper_token *next = p->tokens;
	int batch = p->n;
	/* SOT holds position 0. */
	int past = 1;
	whisper_token token;

	strcpy(out->text, p->text);
	out->intent = intent_parse(t->apps, out->text);
	for (int i = 0; i < MAX_TOKENS; i++) {
		const float *logits;

		if (whisper_decode_with_state(t->ctx, t->state, next, batch, past, THREADS) != 0)
			return false;
		past += batch;
		logits = whisper_get_logits_from_state(t->state) + (size_t)(batch - 1) * (size_t)t->n_vocab;
		token = argmax(logits, t->eot);
		if (token == t->eot || !append(out->text, whisper_token_to_str(t->ctx, token)))
			break;
		out->intent = intent_parse(t->apps, out->text);
		if (settled(out->intent.kind))
			break;
		next = &token;
		batch = 1;
	}
	return true;
}

/* Other languages may still command. */
static bool retry(enum intent_kind kind)
{
	return kind == INTENT_NONE || kind == INTENT_OPEN;
}

/* First language that commands wins. */
static bool guess(struct transcriber *t, const struct choice ranked[LANGUAGES], struct heard *out)
{
	struct heard h;

	for (int i = 0; i < HYPOTHESES && i < LANGUAGES &&
	     (i == 0 || ranked[i].prob >= LANGUAGE_FLOOR); i++) {
		struct prompt p = task_prompt(t, ranked[i].lang);

		if (!decode(t, &p, &h))
			return false;
		h.language = languages[ranked[i].lang];
		if (i == 0 || h.intent.kind > out->intent.kind)
			*out = h;
		if (!retry(h.intent.kind))
			return true;
	}
	return true;
}

/* Mel, encoder, then language logits. */
static const float *prepare(struct transcriber *t, struct pcm audio)
{
	if (whisper_pcm_to_mel_with_state(t->ctx, t->state, audio.data, audio.len, THREADS) != 0 ||
	    whisper_encode_with_state(t->ctx, t->state, 0, THREADS) != 0 ||
	    whisper_decode_with_state(t->ctx, t->state, &t->sot, 1, 0, THREADS) != 0)
		return NULL;
	return whisper_get_logits_from_state(t->state);
}

bool transcribe(struct transcriber *t, struct pcm audio, struct heard *out)
{
	struct choice ranked[LANGUAGES];
	const float *logits = prepare(t, audio);

	*out = nothing;
	if (logits == NULL)
		return false;
	if (no_speech(t, logits) > NO_SPEECH_MAX)
		return true;
	rank(t, logits, ranked);
	return guess(t, ranked, out);
}

bool transcribe_search(struct transcriber *t, struct pcm audio, struct heard *out)
{
	struct choice ranked[LANGUAGES];
	const float *logits = prepare(t, audio);
	struct prompt p;

	*out = nothing;
	if (logits == NULL)
		return false;
	rank(t, logits, ranked);
	p = task_prompt(t, ranked[0].lang);
	memcpy(p.tokens + p.n, t->google, (size_t)t->n_google * sizeof *t->google);
	p.n += t->n_google;
	p.text = GOOGLE;
	out->language = languages[ranked[0].lang];
	return decode(t, &p, out);
}

/* Compile GPU pipelines up front. */
static bool warm_up(struct transcriber *t)
{
	static const float silence[WHISPER_SAMPLE_RATE];
	struct heard h;

	return transcribe_search(t, (struct pcm){ silence, WHISPER_SAMPLE_RATE }, &h);
}

bool transcriber_open(struct transcriber *t, const char *model, const struct catalog *apps)
{
	struct whisper_context_params params = whisper_context_default_params();

	params.flash_attn = true;
	t->apps = apps;
	t->ctx = whisper_init_from_file_with_params_no_state(model, params);
	if (t->ctx == NULL)
		return false;
	t->state = whisper_init_state(t->ctx);
	if (t->state == NULL)
		return false;
	t->n_vocab = whisper_n_vocab(t->ctx);
	t->sot = whisper_token_sot(t->ctx);
	t->eot = whisper_token_eot(t->ctx);
	t->nosp = whisper_token_nosp(t->ctx);
	t->task = whisper_token_transcribe(t->ctx);
	t->notimestamps = whisper_token_not(t->ctx);
	for (int i = 0; i < LANGUAGES; i++)
		t->lang[i] = whisper_token_lang(t->ctx, whisper_lang_id(languages[i]));
	t->n_google = whisper_tokenize(t->ctx, GOOGLE, t->google, PREFIX_MAX);
	return t->n_google > 0 && warm_up(t);
}
