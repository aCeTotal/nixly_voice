#include "apps/match.h"

#include <limits.h>

#include "apps/phonetic.h"

/* Costs in half edits. */
#define SOFT_COST 1
#define HARD_COST 2

enum group {
	GROUP_NONE,
	GROUP_VOWEL,
	GROUP_NASAL,
	GROUP_LIP,
	GROUP_TONGUE,
	GROUP_THROAT,
	GROUP_BREATH,
};

/* Letters whisper confuses. */
static const unsigned char groups[UCHAR_MAX + 1] = {
	['a'] = GROUP_VOWEL, ['e'] = GROUP_VOWEL, ['i'] = GROUP_VOWEL,
	['o'] = GROUP_VOWEL, ['u'] = GROUP_VOWEL, ['y'] = GROUP_VOWEL,
	['m'] = GROUP_NASAL, ['n'] = GROUP_NASAL,
	['b'] = GROUP_LIP, ['p'] = GROUP_LIP,
	['d'] = GROUP_TONGUE, ['t'] = GROUP_TONGUE,
	['g'] = GROUP_THROAT, ['k'] = GROUP_THROAT,
	['f'] = GROUP_BREATH, ['v'] = GROUP_BREATH,
};

struct sound {
	char text[FOLD_MAX];
	int len;
};

static int weight(char c)
{
	return groups[(unsigned char)c] == GROUP_VOWEL ? SOFT_COST : HARD_COST;
}

static int swap_cost(char a, char b)
{
	unsigned char group = groups[(unsigned char)a];

	if (a == b)
		return 0;
	return group != GROUP_NONE && group == groups[(unsigned char)b] ? SOFT_COST : HARD_COST;
}

static int least(int a, int b)
{
	return a < b ? a : b;
}

/* Next letter; returns row minimum. */
static int next_row(int row[FOLD_MAX], char c, const struct key *k)
{
	int diag = row[0];
	int low;

	row[0] += weight(c);
	low = row[0];
	for (int j = 1; j <= k->len; j++) {
		int up = row[j];

		row[j] = least(diag + swap_cost(c, k->text[j - 1]),
			       least(up + weight(c), row[j - 1] + weight(k->text[j - 1])));
		diag = up;
		low = least(low, row[j]);
	}
	return low;
}

/* Similar sounds cost half. */
static int distance(const struct sound *s, const struct key *k, int budget)
{
	int row[FOLD_MAX];

	row[0] = 0;
	for (int j = 1; j <= k->len; j++)
		row[j] = row[j - 1] + weight(k->text[j - 1]);
	for (int i = 0; i < s->len; i++)
		if (next_row(row, s->text[i], k) > budget)
			return budget + 1;
	return row[k->len];
}

struct match match_better(struct match a, struct match b)
{
	if (b.app < 0)
		return a;
	if (a.app < 0 || b.cost < a.cost || (b.cost == a.cost && b.len > a.len))
		return b;
	return a;
}

/* One edit per four letters. */
struct match match_span(const struct catalog *c, const char *folded, size_t len)
{
	struct sound s;
	struct match best = MATCH_NONE;
	int lo;
	int hi;

	s.len = (int)phonetic(folded, len, s.text);
	lo = (2 * s.len + 2) / 3;
	hi = least(s.len + s.len / 2, FOLD_MAX - 1);
	for (int i = c->by_len[lo]; i < c->by_len[hi + 1]; i++) {
		const struct key *k = &c->keys[i];
		int budget = least(k->len, s.len) / 2;
		int cost = distance(&s, k, budget);

		if (cost <= budget)
			best = match_better(best, (struct match){ k->app, cost, k->len, k->open });
	}
	return best;
}
