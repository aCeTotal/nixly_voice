#ifndef DIRS_H
#define DIRS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

#define DIRS_MAX 64
#define DIRS_TEXT 16384

/* Zero when directory is missing. */
struct stamp {
	dev_t dev;
	ino_t ino;
	struct timespec ctime;
};

struct dirs {
	struct stamp stamps[DIRS_MAX];
	size_t at[DIRS_MAX];
	int n;
	size_t used;
	char text[DIRS_TEXT];
};

void dirs_add(struct dirs *d, const char *path);
/* nixly_launcher's list and order. */
void dirs_launcher(struct dirs *d);
/* Restamps; true when any changed. */
bool dirs_changed(struct dirs *d);
/* Exists and not listed earlier. */
bool dirs_first(const struct dirs *d, int i);
const char *dirs_path(const struct dirs *d, int i);

#endif
