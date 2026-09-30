#ifndef LEXICON_H
#define LEXICON_H

#include "command/intent.h"

struct stem {
	const char *text;
	enum intent_kind kind;
};

extern const char *const open_words[];
extern const char *const search_words[];
extern const struct stem stems[];

#endif
