#include "apps/phonetic.h"

#include <stdbool.h>
#include <string.h>

struct rule {
	const char *from;
	const char *to;
};

/* Spellings whisper mixes up. */
static const struct rule rules[] = {
	{ "ch", "k" },
	{ "ce", "se" },
	{ "ci", "si" },
	{ "cy", "sy" },
	{ "c", "k" },
	{ "ph", "f" },
	{ "th", "t" },
	{ "q", "k" },
	{ "w", "v" },
	{ "x", "ks" },
	{ "z", "s" },
};

static bool sounded(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

/* Doubled letters squeeze. */
static size_t put(char *out, size_t at, const char *s)
{
	for (; *s && at < FOLD_MAX - 1; s++)
		if (sounded(*s) && (at == 0 || out[at - 1] != *s))
			out[at++] = *s;
	return at;
}

static const struct rule *rule_at(const char *s, size_t left)
{
	for (size_t i = 0; i < sizeof rules / sizeof *rules; i++) {
		size_t n = strlen(rules[i].from);

		if (n <= left && memcmp(s, rules[i].from, n) == 0)
			return &rules[i];
	}
	return NULL;
}

size_t phonetic(const char *folded, size_t len, char out[FOLD_MAX])
{
	size_t at = 0;

	for (size_t i = 0; i < len;) {
		const struct rule *r = rule_at(folded + i, len - i);
		char plain[2] = { folded[i], '\0' };

		at = put(out, at, r ? r->to : plain);
		i += r ? strlen(r->from) : 1;
	}
	out[at] = '\0';
	return at;
}
