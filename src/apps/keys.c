#include "apps/keys.h"

#include <stdlib.h>
#include <string.h>

#include "apps/numeral.h"
#include "apps/phonetic.h"

/* Shorter parts say too little. */
#define PART_MIN 3

/* Folded name cut into words. */
struct name {
	char text[FOLD_MAX];
	unsigned char start[FOLD_MAX];
	unsigned char end[FOLD_MAX];
	int words;
	int app;
};

static void add_key(struct catalog *c, struct key k)
{
	if (k.len == 0 || (!k.full && k.len < PART_MIN))
		return;
	if (c->n_keys == KEYS_MAX) {
		c->dropped++;
		return;
	}
	c->keys[c->n_keys++] = k;
}

static struct key key_for(int app, const char *folded, size_t len)
{
	struct key k = { .app = (unsigned short)app };

	k.len = (unsigned char)phonetic(folded, len, k.text);
	return k;
}

static bool word_char(unsigned char c)
{
	return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c >= 0x80;
}

static struct name name_of(const char *label, int app)
{
	struct name n = { .app = app };

	fold(label, strlen(label), n.text);
	for (size_t i = 0; n.text[i];) {
		if (!word_char((unsigned char)n.text[i])) {
			i++;
			continue;
		}
		n.start[n.words] = (unsigned char)i;
		while (word_char((unsigned char)n.text[i]))
			i++;
		n.end[n.words++] = (unsigned char)i;
	}
	return n;
}

/* Word runs starting at first. */
static void add_runs(struct catalog *c, const struct name *n, int first)
{
	size_t from = n->start[first];

	for (int last = first; last < n->words; last++)
		add_key(c, key_for(n->app, n->text + from, n->end[last] - from));
}

static struct key whole_key(int app, const char *folded)
{
	struct key k = key_for(app, folded, strlen(folded));

	k.full = true;
	return k;
}

static void add_label(struct catalog *c, int app, const char *label)
{
	struct name n = name_of(label, app);

	add_key(c, whole_key(app, n.text));
	for (int first = 0; first < n.words; first++)
		add_runs(c, &n, first);
}

/* Number words only as whole names. */
static void add_numerals(struct catalog *c, int app, const char *label)
{
	char spoken[DESKTOP_VALUE_MAX];
	char folded[FOLD_MAX];

	if (!numeral_words(label, spoken))
		return;
	fold(spoken, strlen(spoken), folded);
	add_key(c, whole_key(app, folded));
}

static void add_program(struct catalog *c, int app, const char *program)
{
	const char *slash = strrchr(program, '/');
	const char *base = slash ? slash + 1 : program;
	char folded[FOLD_MAX];

	fold(base, strlen(base), folded);
	add_key(c, key_for(app, folded, strlen(folded)));
}

void keys_add(struct catalog *c, int app, const struct desktop *d)
{
	for (int i = 0; i < DESKTOP_NAMES; i++) {
		add_label(c, app, d->names[i]);
		add_numerals(c, app, d->names[i]);
	}
	add_program(c, app, d->program);
}

static int by_sound(const void *a, const void *b)
{
	const struct key *x = a;
	const struct key *y = b;
	int text;

	if (x->len != y->len)
		return x->len - y->len;
	text = memcmp(x->text, y->text, x->len);
	if (text != 0)
		return text;
	if (x->app != y->app)
		return x->app - y->app;
	return y->full - x->full;
}

static uint64_t label_of(const struct catalog *c, const struct key *k)
{
	return c->apps[k->app].label;
}

static int group_end(const struct catalog *c, int from)
{
	const struct key *k = &c->keys[from];
	int to = from + 1;

	while (to < c->n_keys && c->keys[to].len == k->len &&
	       memcmp(c->keys[to].text, k->text, k->len) == 0)
		to++;
	return to;
}

/* Shared parts name no app. */
static bool shared(const struct catalog *c, int from, int to)
{
	for (int k = from + 1; k < to; k++)
		if (label_of(c, &c->keys[k]) != label_of(c, &c->keys[from]))
			return true;
	return false;
}

/* Keeps one key per sound. */
static void prune(struct catalog *c)
{
	int kept = 0;
	int from = 0;
	int to = 0;
	bool clash = false;

	for (int k = 0; k < c->n_keys; k++) {
		if (k == to) {
			from = k;
			to = group_end(c, k);
			clash = shared(c, from, to);
		}
		if (clash ? c->keys[k].full : k == from)
			c->keys[kept++] = c->keys[k];
	}
	c->n_keys = kept;
}

static void index_lengths(struct catalog *c)
{
	int k = 0;

	for (int len = 0; len <= FOLD_MAX; len++) {
		while (k < c->n_keys && c->keys[k].len < len)
			k++;
		c->by_len[len] = k;
	}
}

/* Same-length keys sort by text. */
static bool extended_at(const struct catalog *c, const struct key *k, int len)
{
	int lo = c->by_len[len];
	int hi = c->by_len[len + 1];

	while (lo < hi) {
		int mid = lo + (hi - lo) / 2;

		if (memcmp(c->keys[mid].text, k->text, k->len) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	for (; lo < c->by_len[len + 1] && memcmp(c->keys[lo].text, k->text, k->len) == 0; lo++)
		if (label_of(c, &c->keys[lo]) != label_of(c, k))
			return true;
	return false;
}

static void mark_open(struct catalog *c)
{
	for (int i = 0; i < c->n_keys; i++) {
		struct key *k = &c->keys[i];

		for (int len = k->len + 1; len < FOLD_MAX && !k->open; len++)
			k->open = extended_at(c, k, len);
	}
}

void keys_index(struct catalog *c)
{
	qsort(c->keys, (size_t)c->n_keys, sizeof *c->keys, by_sound);
	prune(c);
	index_lengths(c);
	mark_open(c);
}
