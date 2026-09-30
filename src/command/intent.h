#ifndef INTENT_H
#define INTENT_H

#include <stdbool.h>

#define QUERY_MAX 256

struct catalog;

/* Order: least to most heard. */
enum intent_kind {
	INTENT_NONE,
	INTENT_UNSURE,
	INTENT_OPEN,
	INTENT_SEARCH,
	/* Longer app name may follow. */
	INTENT_APP_PREFIX,
	INTENT_HOME,
	INTENT_CALCULATOR,
	INTENT_BROWSER,
	INTENT_APP,
};

struct intent {
	enum intent_kind kind;
	/* Catalog index for app kinds. */
	int app;
	char query[QUERY_MAX];
};

struct intent intent_parse(const struct catalog *apps, const char *text);

static inline bool intent_launches(enum intent_kind kind)
{
	return kind >= INTENT_HOME;
}

/* Trigger heard, argument still missing. */
static inline bool intent_awaits(const struct intent *in)
{
	return in->kind == INTENT_OPEN || (in->kind == INTENT_SEARCH && in->query[0] == '\0');
}

#endif
