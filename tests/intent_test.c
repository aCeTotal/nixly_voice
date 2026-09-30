#include <stdio.h>
#include <string.h>

#include "command/intent.h"

struct case_ {
	const char *text;
	enum intent_kind kind;
	const char *query;
};

static const struct case_ cases[] = {
	{ "Åpne hjemmemappet.", INTENT_HOME, "" },
	{ "Åpne kalkulator.", INTENT_CALCULATOR, "" },
	{ "Åpne nettleser.", INTENT_BROWSER, "" },
	{ "Åpne Nixly Kalk", INTENT_CALCULATOR, "" },
	{ "Opne nettlesaren", INTENT_BROWSER, "" },
	{ "Google Være i Oslo i morgen.", INTENT_SEARCH, "Være i Oslo i morgen" },
	{ "Google, været i Oslo.", INTENT_SEARCH, "været i Oslo" },

	{ "Åbn hjemmemappen", INTENT_HOME, "" },
	{ "Åbn lommeregneren.", INTENT_CALCULATOR, "" },
	{ "Åbn browseren.", INTENT_BROWSER, "" },
	{ "Google vejret i København.", INTENT_SEARCH, "vejret i København" },

	{ "Avaa kotikansio.", INTENT_HOME, "" },
	{ "Avaalaskin.", INTENT_CALCULATOR, "" },
	{ "Avaa selain.", INTENT_BROWSER, "" },
	{ "Google Helsingin sää huomenna", INTENT_SEARCH, "Helsingin sää huomenna" },

	{ "Öppna hemmappen.", INTENT_HOME, "" },
	{ "Öppna hem appen.", INTENT_HOME, "" },
	{ "Öppna miniräknaren.", INTENT_CALCULATOR, "" },
	{ "Öppna webbläsaren.", INTENT_BROWSER, "" },
	{ "Google-Vädret i Stockholm.", INTENT_SEARCH, "Vädret i Stockholm" },

	{ "Öffne den persönlichen Ordner.", INTENT_HOME, "" },
	{ "Öffne den Taschenrechner.", INTENT_CALCULATOR, "" },
	{ "Öffne den Browser.", INTENT_BROWSER, "" },
	{ "Google Wetter in Berlin", INTENT_SEARCH, "Wetter in Berlin" },

	{ "Ouvre le dossier personnel.", INTENT_HOME, "" },
	{ "Ouvre la calculatrice.", INTENT_CALCULATOR, "" },
	{ "Ouvre le navigateur.", INTENT_BROWSER, "" },
	{ "Google Météo à Paris", INTENT_SEARCH, "Météo à Paris" },

	{ "Abre la carpeta personal.", INTENT_HOME, "" },
	{ "¿Abre la calculadora?", INTENT_CALCULATOR, "" },
	{ "Abre el navegador.", INTENT_BROWSER, "" },
	{ "Google el tiempo en Madrid.", INTENT_SEARCH, "el tiempo en Madrid" },

	{ "Open Home folder.", INTENT_HOME, "" },
	{ "Open calculator.", INTENT_CALCULATOR, "" },
	{ "Open browser.", INTENT_BROWSER, "" },
	{ "Google weather in London tomorrow.", INTENT_SEARCH, "weather in London tomorrow" },
	{ "Google foo\tbar", INTENT_SEARCH, "foo bar" },

	{ "Jeg skal google det senere.", INTENT_NONE, "" },
	{ "Kan du åpne døra for meg?", INTENT_NONE, "" },
	{ "Åpne døra for meg nå", INTENT_NONE, "" },
	{ "Åpnet kalkulatoren i går", INTENT_NONE, "" },
	{ "I will google it later.", INTENT_NONE, "" },
	{ "Lo buscaré en Google más tarde.", INTENT_NONE, "" },
	{ "Openly speaking", INTENT_NONE, "" },

	{ "", INTENT_UNSURE, "" },
	{ "...", INTENT_UNSURE, "" },
	{ "Takk.", INTENT_UNSURE, "" },
	{ "Åpne", INTENT_OPEN, "" },
	{ "Åpne den", INTENT_OPEN, "" },
	{ "Google.", INTENT_SEARCH, "" },
};

int main(void)
{
	int failed = 0;

	for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
		struct intent in = intent_parse(cases[i].text);

		if (in.kind == cases[i].kind && strcmp(in.query, cases[i].query) == 0)
			continue;
		fprintf(stderr, "\"%s\": kind %d query \"%s\", want %d \"%s\"\n",
			cases[i].text, in.kind, in.query, cases[i].kind, cases[i].query);
		failed++;
	}
	return failed != 0;
}
