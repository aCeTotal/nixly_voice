#include "apps/catalog.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "apps/desktop.h"
#include "apps/keys.h"

#define SUFFIX ".desktop"
#define FNV_BASIS 0xcbf29ce484222325u
#define FNV_PRIME 0x100000001b3u

struct entry {
	struct desktop desktop;
	dev_t dev;
	ino_t ino;
};

static uint64_t mix(uint64_t h, unsigned char c)
{
	return (h ^ c) * FNV_PRIME;
}

/* Launcher labels compare case-blind. */
static uint64_t label_hash(const char *label)
{
	uint64_t h = FNV_BASIS;

	for (; *label; label++)
		h = mix(h, (unsigned char)tolower((unsigned char)*label));
	return h;
}

static uint64_t id_hash(uint64_t label, const char *program)
{
	uint64_t h = mix(label, 0);

	for (; *program; program++)
		h = mix(h, (unsigned char)*program);
	return h;
}

/* Duplicate file or launcher entry. */
static bool listed(const struct catalog *c, const struct app *a)
{
	for (int i = 0; i < c->n_apps; i++) {
		const struct app *b = &c->apps[i];

		if ((b->dev == a->dev && b->ino == a->ino) || b->id == a->id)
			return true;
	}
	return false;
}

static void enlist(struct catalog *c, const char *path, const struct entry *e)
{
	size_t n = strlen(path) + 1;
	uint64_t label = label_hash(e->desktop.names[0]);
	struct app a = { c->used, e->dev, e->ino, label, id_hash(label, e->desktop.program) };

	if (listed(c, &a))
		return;
	if (c->n_apps == APPS_MAX || c->used + n > PATHS_MAX) {
		c->dropped++;
		return;
	}
	memcpy(c->paths + c->used, path, n);
	c->used += n;
	c->apps[c->n_apps] = a;
	keys_add(c, c->n_apps++, &e->desktop);
}

static bool map_entry(int fd, size_t size, struct desktop *out)
{
	void *text = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	bool ok;

	if (text == MAP_FAILED)
		return false;
	ok = desktop_parse(text, size, out);
	munmap(text, size);
	return ok;
}

static bool read_entry(const char *path, struct entry *out)
{
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	struct stat st = { 0 };
	bool ok;

	if (fd < 0)
		return false;
	ok = fstat(fd, &st) == 0 && S_ISREG(st.st_mode) &&
	     map_entry(fd, (size_t)st.st_size, &out->desktop);
	close(fd);
	out->dev = st.st_dev;
	out->ino = st.st_ino;
	return ok;
}

static bool is_entry(const char *file)
{
	size_t n = strlen(file);
	size_t suffix = sizeof SUFFIX - 1;

	return n > suffix && memcmp(file + n - suffix, SUFFIX, suffix) == 0;
}

static void add_file(struct catalog *c, const char *dir, const char *file)
{
	char path[PATH_MAX];
	struct entry e;

	if (!is_entry(file) || snprintf(path, sizeof path, "%s/%s", dir, file) >= (int)sizeof path)
		return;
	if (read_entry(path, &e))
		enlist(c, path, &e);
}

static void scan_dir(struct catalog *c, const char *dir)
{
	DIR *d = opendir(dir);
	struct dirent *e;

	if (d == NULL)
		return;
	while ((e = readdir(d)) != NULL)
		add_file(c, dir, e->d_name);
	closedir(d);
}

static void scan(struct catalog *c)
{
	c->n_apps = 0;
	c->n_keys = 0;
	c->used = 0;
	c->dropped = 0;
	for (int i = 0; i < c->dirs.n; i++)
		if (dirs_first(&c->dirs, i))
			scan_dir(c, dirs_path(&c->dirs, i));
	if (c->dropped > 0)
		fprintf(stderr, "nixly-voice: catalog full, %d apps or names left out\n", c->dropped);
	keys_index(c);
}

void catalog_refresh(struct catalog *c)
{
	if (dirs_changed(&c->dirs))
		scan(c);
}

const char *catalog_path(const struct catalog *c, int app)
{
	return c->paths + c->apps[app].path;
}
