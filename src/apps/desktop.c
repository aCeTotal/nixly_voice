#include "apps/desktop.h"

#include <string.h>
#include <strings.h>

#define SECTION "[Desktop Entry]"

enum field {
	FIELD_NAME,
	FIELD_NAME_NB,
	FIELD_NAME_NN,
	FIELD_NAME_NO,
	FIELD_EXEC,
	FIELD_NO_DISPLAY,
	FIELD_HIDDEN,
	FIELD_OTHER,
};

static const char *const keys[FIELD_OTHER] = {
	[FIELD_NAME] = "Name",
	[FIELD_NAME_NB] = "Name[nb]",
	[FIELD_NAME_NN] = "Name[nn]",
	[FIELD_NAME_NO] = "Name[no]",
	[FIELD_EXEC] = "Exec",
	[FIELD_NO_DISPLAY] = "NoDisplay",
	[FIELD_HIDDEN] = "Hidden",
};

struct span {
	const char *at;
	size_t len;
};

static bool blank(char c)
{
	return c == ' ' || c == '\t' || c == '\r';
}

static struct span trim(const char *at, size_t len)
{
	while (len > 0 && blank(*at)) {
		at++;
		len--;
	}
	while (len > 0 && blank(at[len - 1]))
		len--;
	return (struct span){ at, len };
}

static bool equals(struct span s, const char *text)
{
	return s.len == strlen(text) && memcmp(s.at, text, s.len) == 0;
}

static enum field field_of(struct span key)
{
	enum field f = FIELD_NAME;

	while (f < FIELD_OTHER && !equals(key, keys[f]))
		f++;
	return f;
}

static void copy(char out[DESKTOP_VALUE_MAX], struct span value)
{
	size_t n = value.len < DESKTOP_VALUE_MAX - 1 ? value.len : DESKTOP_VALUE_MAX - 1;

	memcpy(out, value.at, n);
	out[n] = '\0';
}

/* First Exec word, launcher-split. */
static void program_of(char out[DESKTOP_VALUE_MAX], struct span exec)
{
	size_t n = 0;
	bool quoted = false;

	for (size_t i = 0; i < exec.len && n < DESKTOP_VALUE_MAX - 1; i++) {
		char c = exec.at[i];
		bool gap = !quoted && blank(c);

		if (gap && n > 0)
			break;
		if (c == '"')
			quoted = !quoted;
		else if (!quoted && c == '%')
			i++;
		else if (!gap)
			out[n++] = c;
	}
	out[n] = '\0';
}

static void take(struct desktop *d, enum field f, struct span value)
{
	bool yes = value.len == 4 && strncasecmp(value.at, "true", 4) == 0;

	switch (f) {
	case FIELD_EXEC:
		program_of(d->program, value);
		break;
	case FIELD_NO_DISPLAY:
		d->no_display = yes;
		break;
	case FIELD_HIDDEN:
		d->hidden = yes;
		break;
	case FIELD_OTHER:
		break;
	default:
		copy(d->names[f], value);
	}
}

static void read_pair(struct desktop *d, struct span line)
{
	const char *eq = memchr(line.at, '=', line.len);
	struct span key;

	if (eq == NULL || line.at[0] == '#')
		return;
	key = trim(line.at, (size_t)(eq - line.at));
	take(d, field_of(key), trim(eq + 1, line.len - (size_t)(eq - line.at) - 1));
}

static bool is_section(struct span line)
{
	return line.len >= 2 && line.at[0] == '[' && line.at[line.len - 1] == ']';
}

bool desktop_parse(const char *text, size_t size, struct desktop *out)
{
	const char *end = text + size;
	bool in_entry = false;

	*out = (struct desktop){ 0 };
	for (const char *at = text; at < end;) {
		const char *eol = memchr(at, '\n', (size_t)(end - at));
		struct span line = trim(at, (size_t)((eol ? eol : end) - at));

		at = eol ? eol + 1 : end;
		if (is_section(line))
			in_entry = equals(line, SECTION);
		else if (in_entry)
			read_pair(out, line);
	}
	return out->names[0][0] && out->program[0] && !out->no_display && !out->hidden;
}
