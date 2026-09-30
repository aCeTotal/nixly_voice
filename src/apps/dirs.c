#include "apps/dirs.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *const system_dirs[] = {
	"/run/current-system/sw/share/applications",
	"/var/lib/flatpak/exports/share/applications",
	"/usr/share/applications",
	"/usr/local/share/applications",
};

static const char *const home_dirs[] = {
	".nix-profile/share/applications",
	".local/share/applications",
};

void dirs_add(struct dirs *d, const char *path)
{
	size_t n = strlen(path) + 1;

	if (d->n == DIRS_MAX || d->used + n > DIRS_TEXT) {
		fprintf(stderr, "nixly-voice: too many app dirs, skipping %s\n", path);
		return;
	}
	memcpy(d->text + d->used, path, n);
	d->at[d->n++] = d->used;
	d->used += n;
}

static void add_data_dirs(struct dirs *d, const char *list)
{
	char path[PATH_MAX];

	while (list && *list) {
		size_t n = strcspn(list, ":");

		snprintf(path, sizeof path, "%.*s/applications", (int)n, list);
		if (n > 0)
			dirs_add(d, path);
		list += n + (list[n] == ':');
	}
}

void dirs_launcher(struct dirs *d)
{
	const char *home = getenv("HOME");
	const char *user = getenv("USER");
	char path[PATH_MAX];

	for (size_t i = 0; i < sizeof system_dirs / sizeof *system_dirs; i++)
		dirs_add(d, system_dirs[i]);
	for (size_t i = 0; home && i < sizeof home_dirs / sizeof *home_dirs; i++) {
		snprintf(path, sizeof path, "%s/%s", home, home_dirs[i]);
		dirs_add(d, path);
	}
	if (user) {
		snprintf(path, sizeof path, "/etc/profiles/per-user/%s/share/applications", user);
		dirs_add(d, path);
	}
	add_data_dirs(d, getenv("XDG_DATA_DIRS"));
}

/* Follows symlinks through profile switches. */
static struct stamp stamp_of(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0)
		return (struct stamp){ 0 };
	return (struct stamp){ st.st_dev, st.st_ino, st.st_ctim };
}

static bool same_dir(struct stamp a, struct stamp b)
{
	return a.dev == b.dev && a.ino == b.ino;
}

bool dirs_changed(struct dirs *d)
{
	bool changed = false;

	for (int i = 0; i < d->n; i++) {
		struct stamp now = stamp_of(dirs_path(d, i));
		struct stamp was = d->stamps[i];

		changed |= !same_dir(now, was) || now.ctime.tv_sec != was.ctime.tv_sec ||
			   now.ctime.tv_nsec != was.ctime.tv_nsec;
		d->stamps[i] = now;
	}
	return changed;
}

bool dirs_first(const struct dirs *d, int i)
{
	if (d->stamps[i].ino == 0)
		return false;
	for (int j = 0; j < i; j++)
		if (same_dir(d->stamps[j], d->stamps[i]))
			return false;
	return true;
}

const char *dirs_path(const struct dirs *d, int i)
{
	return d->text + d->at[i];
}
