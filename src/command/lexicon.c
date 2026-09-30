#include "command/lexicon.h"

#include <stddef.h>

/* Folded: lowercase, accents stripped. */
const char *const open_words[] = {
	"apne", "opne", "apen", "start",
	"abn", "aben", "abne",
	"oppna",
	"avaa", "avatkaa",
	"offne", "offnen", "oeffne",
	"ouvre", "ouvrir", "ouvrez",
	"abre", "abrir", "abra",
	"open",
	NULL,
};

const char *const search_words[] = {
	"google", "googla", "googlaa", "googlea", "googler", "googel", "googeln",
	NULL,
};

/* Found inside joined, squeezed words. */
const struct stem stems[] = {
	{ "hjem", INTENT_HOME },
	{ "home", INTENT_HOME },
	{ "hemmapp", INTENT_HOME },
	{ "hemkatalog", INTENT_HOME },
	{ "kotikansi", INTENT_HOME },
	{ "kotihakemisto", INTENT_HOME },
	{ "heimordner", INTENT_HOME },
	{ "heimverzeichnis", INTENT_HOME },
	{ "benutzerordner", INTENT_HOME },
	{ "personlich", INTENT_HOME },
	{ "dossierpersonnel", INTENT_HOME },
	{ "repertoirepersonnel", INTENT_HOME },
	{ "dossierutilisateur", INTENT_HOME },
	{ "carpetapersonal", INTENT_HOME },
	{ "carpetadeinicio", INTENT_HOME },
	{ "carpetadeusuario", INTENT_HOME },
	{ "directoriopersonal", INTENT_HOME },

	{ "kalk", INTENT_CALCULATOR },
	{ "calc", INTENT_CALCULATOR },
	{ "rechner", INTENT_CALCULATOR },
	{ "laskin", INTENT_CALCULATOR },
	{ "laskim", INTENT_CALCULATOR },
	{ "laskuri", INTENT_CALCULATOR },
	{ "lommeregn", INTENT_CALCULATOR },
	{ "rakna", INTENT_CALCULATOR },
	{ "regnemaskin", INTENT_CALCULATOR },

	{ "nettles", INTENT_BROWSER },
	{ "netlaes", INTENT_BROWSER },
	{ "browser", INTENT_BROWSER },
	{ "webblas", INTENT_BROWSER },
	{ "selain", INTENT_BROWSER },
	{ "selaim", INTENT_BROWSER },
	{ "navigat", INTENT_BROWSER },
	{ "navegad", INTENT_BROWSER },

	{ NULL, INTENT_NONE },
};
