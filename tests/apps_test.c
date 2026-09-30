#define _XOPEN_SOURCE 700

#include <ftw.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "apps/catalog.h"
#include "command/intent.h"

#define LISTED 17

struct file {
	const char *name;
	const char *text;
};

struct case_ {
	const char *text;
	enum intent_kind kind;
	const char *file;
};

static const struct file system_files[] = {
	{ "google-chrome.desktop", "[Desktop Entry]\nName=Google Chrome\n"
				   "Exec=/nix/store/x-chrome/bin/google-chrome-stable %U\n" },
	{ "org.pulseaudio.pavucontrol.desktop", "[Desktop Entry]\nName=Volume Control\n"
						"Name[nn]=Lydstyrkekontroll\nExec=pavucontrol\n" },
	{ "com.saivert.pwvucontrol.desktop", "[Desktop Entry]\nName=pwvucontrol\nExec=pwvucontrol\n" },
	{ "calc.desktop", "[Desktop Entry]\nName=LibreOffice Calc\nExec=libreoffice --calc %U\n" },
	{ "writer.desktop", "[Desktop Entry]\nName=LibreOffice Writer\nExec=libreoffice --writer %U\n" },
	{ "nixlykalk.desktop", "[Desktop Entry]\nName=Kalkulator\nExec=nixlykalk\n" },
	{ "steam.desktop", "[Desktop Entry]\nName=Steam\nExec=steam %U\n" },
	{ "steamlink.desktop", "[Desktop Entry]\nName=Steam Link\nExec=steamlink\n" },
	{ "Arma 3.desktop", "[Desktop Entry]\nName=Arma 3\nExec=steam steam://rungameid/107410\n" },
	{ "Squad.desktop", "[Desktop Entry]\nName=Squad\nExec=steam steam://rungameid/393380\n" },
	{ "Squad 44.desktop", "[Desktop Entry]\nName=Squad 44\nExec=steam steam://rungameid/736220\n" },
	{ "brave-browser.desktop", "[Desktop Entry]\nName=Brave Web Browser\nExec=brave %U\n" },
	{ "org.gnome.Nautilus.desktop", "[Desktop Entry]\nName=Files\nName[nb]=Filer\n"
					"Exec=nautilus --new-window %U\n" },
	{ "teams-for-linux.desktop", "[Desktop Entry]\nName=Microsoft Teams for Linux\n"
				     "Exec=teams-for-linux %U\n" },
	{ "vlc.desktop", "[Desktop Entry]\nName=VLC media player\nExec=vlc --started-from-file %U\n" },
	{ "mpv.desktop", "[Desktop Entry]\nName=mpv Media Player\nExec=mpv -- %U\n" },
	{ "hidden.desktop", "[Desktop Entry]\nName=Skjult\nExec=skjult\nNoDisplay=true\n" },
	{ "gone.desktop", "[Desktop Entry]\nName=Borte\nExec=borte\nHidden=true\n" },
	{ "game.desktop", "[Desktop Entry]\n# Spill\nName = Spill \nExec=\"/opt/my game/spill\"\n"
			  "[Desktop Action discord]\nName=Discord\nExec=discord\n" },
	{ "notes.txt", "[Desktop Entry]\nName=Notater\nExec=notater\n" },
};

static const struct file user_files[] = {
	{ "chrome-copy.desktop", "[Desktop Entry]\nName=google chrome\n"
				 "Exec=/nix/store/x-chrome/bin/google-chrome-stable\n" },
};

static const struct file later_files[] = {
	{ "spotify.desktop", "[Desktop Entry]\nName=Spotify\nExec=spotify %U\n" },
};

static const struct case_ cases[] = {
	{ "Åpne Chrome.", INTENT_APP, "google-chrome.desktop" },
	{ "Åpne krom.", INTENT_APP, "google-chrome.desktop" },
	{ "Oppne Chrome", INTENT_APP, "google-chrome.desktop" },
	{ "Åpne Google Chrome", INTENT_APP, "google-chrome.desktop" },
	{ "Åpne Pavucontrol.", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne Pavu Control.", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne pavukontroll.", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne Pavo-kontroll", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne volumkontroll", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne lydstyrkekontrollen", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne PW Vu Control", INTENT_APP, "com.saivert.pwvucontrol.desktop" },
	{ "Åpne LibreOffice Calc.", INTENT_APP, "calc.desktop" },
	{ "Åpne Writer", INTENT_APP, "writer.desktop" },
	{ "Åpne Steam.", INTENT_APP_PREFIX, "steam.desktop" },
	{ "Åpne Stim", INTENT_APP_PREFIX, "steam.desktop" },
	{ "Åpne Steam Link.", INTENT_APP, "steamlink.desktop" },
	{ "Start Arma 3.", INTENT_APP, "Arma 3.desktop" },
	{ "Åpne Arma 3", INTENT_APP, "Arma 3.desktop" },
	{ "Start Arma tre", INTENT_APP, "Arma 3.desktop" },
	{ "Start armatreet.", INTENT_APP, "Arma 3.desktop" },
	{ "Start Squad", INTENT_APP_PREFIX, "Squad.desktop" },
	{ "Start Squad 44", INTENT_APP, "Squad 44.desktop" },
	{ "Start Squad førtifire", INTENT_APP, "Squad 44.desktop" },
	{ "Start tre", INTENT_OPEN, NULL },
	{ "Start førtifire", INTENT_OPEN, NULL },
	{ "Start Spotify", INTENT_OPEN, NULL },
	{ "Start Chrome", INTENT_APP, "google-chrome.desktop" },
	{ "Start pavucontrol", INTENT_APP, "org.pulseaudio.pavucontrol.desktop" },
	{ "Åpne Steam takk", INTENT_APP_PREFIX, "steam.desktop" },
	{ "Åpne Brave.", INTENT_APP, "brave-browser.desktop" },
	{ "Åpne filer.", INTENT_APP, "org.gnome.Nautilus.desktop" },
	{ "Åpne Nautilus", INTENT_APP, "org.gnome.Nautilus.desktop" },
	{ "Åpne Teams", INTENT_APP, "teams-for-linux.desktop" },
	{ "Åpne VLC", INTENT_APP, "vlc.desktop" },
	{ "Åpne Spill", INTENT_APP, "game.desktop" },

	{ "Åpne kalkulator.", INTENT_CALCULATOR, NULL },
	{ "Åpne nettleser.", INTENT_BROWSER, NULL },
	{ "Åpne hjem", INTENT_HOME, NULL },
	{ "Åpne home.", INTENT_HOME, NULL },
	{ "Åpne hjemmemappe", INTENT_HOME, NULL },

	{ "Åpne media player", INTENT_OPEN, NULL },
	{ "Åpne LibreOffice", INTENT_OPEN, NULL },
	{ "Åpne skjult", INTENT_OPEN, NULL },
	{ "Åpne borte", INTENT_OPEN, NULL },
	{ "Åpne Discord", INTENT_OPEN, NULL },
	{ "Åpne notater", INTENT_OPEN, NULL },
	{ "Åpne den", INTENT_OPEN, NULL },
	{ "Åpne døra for meg nå", INTENT_NONE, NULL },
	{ "Kan du åpne Chrome?", INTENT_NONE, NULL },
};

static struct catalog catalog;
static int failed;

static void write_files(const char *dir, const struct file *files, size_t n)
{
	char path[PATH_MAX];

	mkdir(dir, 0700);
	for (size_t i = 0; i < n; i++) {
		FILE *f;

		snprintf(path, sizeof path, "%s/%s", dir, files[i].name);
		f = fopen(path, "w");
		fputs(files[i].text, f);
		fclose(f);
	}
}

static bool ends_with(const char *path, const char *file)
{
	size_t n = strlen(path);
	size_t m = strlen(file);

	return n > m && path[n - m - 1] == '/' && strcmp(path + n - m, file) == 0;
}

static void expect(const char *text, enum intent_kind kind, const char *file)
{
	struct intent in = intent_parse(&catalog, text);
	const char *got = in.kind == INTENT_APP || in.kind == INTENT_APP_PREFIX ?
				  catalog_path(&catalog, in.app) : "";

	if (in.kind == kind && (file == NULL || ends_with(got, file)))
		return;
	fprintf(stderr, "\"%s\": kind %d \"%s\", want %d \"%s\"\n", text, in.kind, got, kind,
		file ? file : "");
	failed++;
}

static int remove_one(const char *path, const struct stat *st, int flag, struct FTW *ftw)
{
	(void)st;
	(void)flag;
	(void)ftw;
	return remove(path);
}

int main(void)
{
	const char *tmp = getenv("TMPDIR");
	char root[256];
	char system_dir[512];
	char user_dir[512];
	char later_dir[512];
	char link[PATH_MAX];
	char target[PATH_MAX];

	snprintf(root, sizeof root, "%s/nixly-voice-XXXXXX", tmp ? tmp : "/tmp");
	if (mkdtemp(root) == NULL)
		return 1;
	snprintf(system_dir, sizeof system_dir, "%s/system", root);
	snprintf(user_dir, sizeof user_dir, "%s/user", root);
	snprintf(later_dir, sizeof later_dir, "%s/later", root);
	write_files(system_dir, system_files, sizeof system_files / sizeof *system_files);
	write_files(user_dir, user_files, sizeof user_files / sizeof *user_files);
	snprintf(target, sizeof target, "%s/google-chrome.desktop", system_dir);
	snprintf(link, sizeof link, "%s/google-chrome.desktop", user_dir);
	if (symlink(target, link) != 0)
		return 1;

	dirs_add(&catalog.dirs, system_dir);
	dirs_add(&catalog.dirs, user_dir);
	dirs_add(&catalog.dirs, system_dir);
	dirs_add(&catalog.dirs, later_dir);
	catalog_refresh(&catalog);
	if (catalog.n_apps != LISTED) {
		fprintf(stderr, "%d apps listed, want %d\n", catalog.n_apps, LISTED);
		failed++;
	}
	for (size_t i = 0; i < sizeof cases / sizeof *cases; i++)
		expect(cases[i].text, cases[i].kind, cases[i].file);

	expect("Åpne Spotify", INTENT_OPEN, NULL);
	write_files(later_dir, later_files, sizeof later_files / sizeof *later_files);
	catalog_refresh(&catalog);
	expect("Åpne Spotify", INTENT_APP, "spotify.desktop");
	expect("Åpne Chrome", INTENT_APP, "google-chrome.desktop");

	nftw(root, remove_one, 8, FTW_DEPTH | FTW_PHYS);
	return failed != 0;
}
