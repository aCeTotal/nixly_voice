#ifndef CATALOG_H
#define CATALOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "apps/dirs.h"
#include "command/fold.h"

#define APPS_MAX 2048
#define KEYS_MAX 16384
#define PATHS_MAX (APPS_MAX * 256)

/* Spoken form of an app. */
struct key {
	char text[FOLD_MAX];
	unsigned char len;
	bool full;
	/* Another app's key extends it. */
	bool open;
	unsigned short app;
};

struct app {
	size_t path;
	dev_t dev;
	ino_t ino;
	uint64_t label;
	uint64_t id;
};

struct catalog {
	struct dirs dirs;
	struct app apps[APPS_MAX];
	int n_apps;
	struct key keys[KEYS_MAX];
	int n_keys;
	/* First key per length. */
	int by_len[FOLD_MAX + 1];
	int dropped;
	size_t used;
	char paths[PATHS_MAX];
};

/* Rescans when a directory changed. */
void catalog_refresh(struct catalog *c);
const char *catalog_path(const struct catalog *c, int app);

#endif
