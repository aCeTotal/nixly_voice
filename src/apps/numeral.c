#include "apps/numeral.h"

#include <string.h>

#define DIGITS "0123456789"
/* Longer runs are years, versions. */
#define DIGITS_MAX 2

/* Folded, like the keys. */
static const char *const ones[] = {
	"null", "en", "to", "tre", "fire", "fem", "seks", "sju", "atte", "ni",
};

static const char *const teens[] = {
	"ti", "elleve", "tolv", "tretten", "fjorten",
	"femten", "seksten", "sytten", "atten", "nitten",
};

static const char *const tens[] = {
	"", "", "tjue", "tretti", "forti", "femti", "seksti", "sytti", "atti", "nitti",
};

static size_t put(char *out, size_t at, const char *s, size_t n)
{
	if (at + n >= DESKTOP_VALUE_MAX)
		return at;
	memcpy(out + at, s, n);
	return at + n;
}

static size_t put_word(char *out, size_t at, const char *word)
{
	return put(out, at, word, strlen(word));
}

static size_t put_number(char *out, size_t at, int n)
{
	if (n < 10)
		return put_word(out, at, ones[n]);
	if (n < 20)
		return put_word(out, at, teens[n - 10]);
	at = put_word(out, at, tens[n / 10]);
	return n % 10 ? put_word(out, at, ones[n % 10]) : at;
}

bool numeral_words(const char *label, char out[DESKTOP_VALUE_MAX])
{
	size_t at = 0;
	bool spelled = false;

	while (*label) {
		size_t digits = strspn(label, DIGITS);
		size_t plain = digits ? digits : 1;

		if (digits == 0 || digits > DIGITS_MAX) {
			at = put(out, at, label, plain);
			label += plain;
			continue;
		}
		at = put_number(out, at, digits == 1 ? label[0] - '0' :
						       (label[0] - '0') * 10 + label[1] - '0');
		label += digits;
		spelled = true;
	}
	out[at] = '\0';
	return spelled;
}
