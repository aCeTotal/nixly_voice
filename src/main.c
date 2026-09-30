#include <signal.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "audio/capture.h"
#include "audio/ring.h"
#include "speech/listener.h"

static struct ring ring;
static struct listener listener;

/* One line per command. */
static void emit(const struct intent *in)
{
	static const char *const lines[] = {
		[INTENT_SEARCH] = "search ",
		[INTENT_HOME] = "open home",
		[INTENT_CALCULATOR] = "open calculator",
		[INTENT_BROWSER] = "open browser",
		[INTENT_APP] = "launch ",
	};
	const char *arg = in->kind == INTENT_APP ? catalog_path(&listener.apps, in->app) : in->query;

	dprintf(STDOUT_FILENO, "%s%s\n", lines[in->kind], arg);
}

static void drain(void)
{
	float window[VAD_WINDOW];
	struct intent in;

	while (ring_pop(&ring, window, VAD_WINDOW))
		if (listener_window(&listener, window, &in))
			emit(&in);
}

int main(void)
{
	prctl(PR_SET_PDEATHSIG, SIGTERM);
	if (!listener_open(&listener, WHISPER_MODEL, VAD_MODEL) || !ring_init(&ring) ||
	    !capture_start(&ring)) {
		fputs("nixly-voice: startup failed\n", stderr);
		return 1;
	}
	fputs("nixly-voice: listening\n", stderr);
	while (ring_wait(&ring))
		drain();
	fputs("nixly-voice: audio capture lost\n", stderr);
	return 1;
}
