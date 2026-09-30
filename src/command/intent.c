#include "command/intent.h"

#include <stddef.h>
#include <string.h>

#include "apps/match.h"
#include "command/fold.h"
#include "command/lexicon.h"

#define MAX_WORDS 48
/* Target must follow within these. */
#define TARGET_WORDS 4
#define JOINED_MAX (FOLD_MAX * (TARGET_WORDS + 1))

enum trigger_kind {
	TRIGGER_NONE,
	TRIGGER_OPEN,
	TRIGGER_SEARCH,
};

struct trigger {
	enum trigger_kind kind;
	const char *rest;
};

/* Target joined, cut per word. */
struct target {
	char text[JOINED_MAX];
	size_t ends[TARGET_WORDS];
	int n;
};

struct found {
	enum intent_kind kind;
	bool leading;
};

struct word {
	const char *at;
	size_t len;
	char folded[FOLD_MAX];
};

struct words {
	struct word w[MAX_WORDS];
	int n;
};

/* Separator length; 0 in words. */
static size_t separator(const unsigned char *s)
{
	bool alnum = (s[0] >= 'a' && s[0] <= 'z') || (s[0] >= 'A' && s[0] <= 'Z') ||
		     (s[0] >= '0' && s[0] <= '9');

	if (s[0] < 0x80)
		return alnum ? 0 : 1;
	if (s[0] == 0xC2 && s[1] >= 0x80 && s[1] <= 0xBF)
		return 2;
	if (s[0] == 0xE2 && s[1] == 0x80 && s[2] >= 0x80 && s[2] <= 0xBF)
		return 3;
	return 0;
}

static void add(struct words *ws, const unsigned char *start, const unsigned char *end)
{
	struct word *w = &ws->w[ws->n++];

	w->at = (const char *)start;
	w->len = (size_t)(end - start);
	fold(w->at, w->len, w->folded);
}

static const unsigned char *word_end(const unsigned char *s)
{
	while (*s && !separator(s))
		s++;
	return s;
}

static void split(const char *text, struct words *ws)
{
	const unsigned char *s = (const unsigned char *)text;

	ws->n = 0;
	while (*s && ws->n < MAX_WORDS) {
		const unsigned char *start = s;
		size_t skip = separator(s);

		if (skip) {
			s += skip;
			continue;
		}
		s = word_end(s);
		add(ws, start, s);
	}
}

static bool listed(const char *const *list, const char *word)
{
	for (; *list; list++)
		if (strcmp(*list, word) == 0)
			return true;
	return false;
}

/* Whisper doubles letters inconsistently. */
static void squeeze(const char *s, char *out)
{
	char prev = '\0';

	for (; *s; prev = *s++)
		if (*s != prev)
			*out++ = *s;
	*out = '\0';
}

/* Earliest stem wins. */
static struct found target_in(const char *joined)
{
	char text[JOINED_MAX];
	char stem[FOLD_MAX];
	enum intent_kind kind = INTENT_NONE;
	const char *first = NULL;

	squeeze(joined, text);
	for (const struct stem *s = stems; s->text; s++) {
		const char *at;

		squeeze(s->text, stem);
		at = strstr(text, stem);

		if (at && (!first || at < first)) {
			first = at;
			kind = s->kind;
		}
	}
	return (struct found){ kind, first == text };
}

/* Whisper may fuse verb, target. */
static struct trigger trigger_of(const char *word)
{
	char plain[FOLD_MAX];
	char verb[FOLD_MAX];

	if (listed(search_words, word))
		return (struct trigger){ TRIGGER_SEARCH, "" };
	squeeze(word, plain);
	for (const char *const *o = open_words; *o; o++) {
		size_t n = strlen(*o);

		squeeze(*o, verb);
		if (strcmp(plain, verb) == 0)
			return (struct trigger){ TRIGGER_OPEN, "" };
		if (strncmp(word, *o, n) == 0 && target_in(word + n).kind != INTENT_NONE)
			return (struct trigger){ TRIGGER_OPEN, word + n };
	}
	return (struct trigger){ TRIGGER_NONE, "" };
}

static struct target join(const struct words *ws, const char *rest)
{
	struct target t = { .n = 0 };
	size_t at = strlen(rest);
	int last = ws->n < TARGET_WORDS + 1 ? ws->n : TARGET_WORDS + 1;

	memcpy(t.text, rest, at);
	for (int i = 1; i < last; i++) {
		size_t n = strlen(ws->w[i].folded);

		memcpy(t.text + at, ws->w[i].folded, n);
		at += n;
		t.ends[t.n++] = at;
	}
	t.text[at] = '\0';
	return t;
}

static struct match target_app(const struct catalog *apps, const struct target *t)
{
	struct match best = MATCH_NONE;

	for (int i = 0; i < t->n; i++)
		best = match_better(best, match_span(apps, t->text, t->ends[i]));
	return best;
}

/* Leading builtin word beats apps. */
static struct intent open_target(const struct catalog *apps, const struct words *ws,
				 const char *rest)
{
	struct target t = join(ws, rest);
	struct found builtin = target_in(t.text);
	struct match app;

	if (builtin.leading)
		return (struct intent){ .kind = builtin.kind };
	app = target_app(apps, &t);
	if (app.app >= 0)
		return (struct intent){ .kind = app.open ? INTENT_APP_PREFIX : INTENT_APP,
					.app = app.app };
	if (builtin.kind != INTENT_NONE)
		return (struct intent){ .kind = builtin.kind };
	return (struct intent){ .kind = ws->n <= TARGET_WORDS ? INTENT_OPEN : INTENT_NONE };
}

/* Query is words after trigger. */
static struct intent search(const struct words *ws)
{
	struct intent in = { .kind = INTENT_SEARCH };
	const struct word *first;
	const struct word *last;
	size_t len;

	if (ws->n < 2)
		return in;
	first = &ws->w[1];
	last = &ws->w[ws->n - 1];
	while (last > first && (size_t)(last->at + last->len - first->at) >= QUERY_MAX)
		last--;
	len = (size_t)(last->at + last->len - first->at);
	if (len >= QUERY_MAX)
		return in;
	for (size_t i = 0; i < len; i++)
		in.query[i] = (unsigned char)first->at[i] < ' ' ? ' ' : first->at[i];
	in.query[len] = '\0';
	return in;
}

struct intent intent_parse(const struct catalog *apps, const char *text)
{
	struct words ws;
	struct trigger t;

	split(text, &ws);
	if (ws.n == 0)
		return (struct intent){ .kind = INTENT_UNSURE };
	t = trigger_of(ws.w[0].folded);
	if (t.kind == TRIGGER_SEARCH)
		return search(&ws);
	if (t.kind == TRIGGER_OPEN)
		return open_target(apps, &ws, t.rest);
	return (struct intent){ .kind = ws.n > 1 ? INTENT_NONE : INTENT_UNSURE };
}
