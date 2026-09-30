#include "command/fold.h"

#include <stdbool.h>
#include <string.h>

/* U+00C0..U+00FF without accents. */
static const char *const latin1[64] = {
	"a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
	"d", "n", "o", "o", "o", "o", "o", "", "o", "u", "u", "u", "u", "y", "th", "ss",
	"a", "a", "a", "a", "a", "a", "ae", "c", "e", "e", "e", "e", "i", "i", "i", "i",
	"d", "n", "o", "o", "o", "o", "o", "", "o", "u", "u", "u", "u", "y", "th", "y",
};

static bool latin1_pair(const unsigned char *s, size_t left)
{
	return left >= 2 && (s[0] == 0xC2 || s[0] == 0xC3) && s[1] >= 0x80 && s[1] <= 0xBF;
}

static size_t put(char *dst, size_t at, const char *s, size_t n)
{
	if (at + n >= FOLD_MAX)
		return at;
	memcpy(dst + at, s, n);
	return at + n;
}

/* U+0080..U+00BF is punctuation: dropped. */
static size_t put_latin1(char *dst, size_t at, const unsigned char *s)
{
	const char *plain = s[0] == 0xC3 ? latin1[s[1] - 0x80] : "";

	return put(dst, at, plain, strlen(plain));
}

void fold(const char *src, size_t len, char dst[FOLD_MAX])
{
	const unsigned char *s = (const unsigned char *)src;
	size_t at = 0;

	for (size_t i = 0; i < len; i++) {
		char c = (char)(s[i] >= 'A' && s[i] <= 'Z' ? s[i] + ('a' - 'A') : s[i]);

		if (!latin1_pair(s + i, len - i)) {
			at = put(dst, at, &c, 1);
			continue;
		}
		at = put_latin1(dst, at, s + i);
		i++;
	}
	dst[at] = '\0';
}
