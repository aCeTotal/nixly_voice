#include <math.h>
#include <stdio.h>

#include "speech/presence.h"

#define RATE 16000.0f
#define WINDOW 512
#define HUM_HZ 100.0f
#define HISS_HZ 2000.0f
/* Hiss 5 dB under hum. */
#define HUM_AMP 0.014f
#define HISS_AMP 0.0078f
#define VOICE_ON 0.5f

static int failed;

static void fill(float *w, int at, float hiss)
{
	for (int i = 0; i < WINDOW; i++) {
		float t = (float)(at * WINDOW + i) / RATE;

		w[i] = HUM_AMP * sinf(2.0f * (float)M_PI * HUM_HZ * t) +
		       hiss * sinf(2.0f * (float)M_PI * HISS_HZ * t);
	}
}

static float listen(struct presence *p, int at, float hiss)
{
	float w[WINDOW];

	fill(w, at, hiss);
	return presence_prob(p, w, WINDOW);
}

int main(void)
{
	struct presence p;
	int at = 0;

	presence_init(&p, RATE);
	for (; at < 30; at++)
		if (listen(&p, at, 0.0f) > 0.0f) {
			fprintf(stderr, "window %d: loud hum heard as speech\n", at);
			failed++;
		}
	if (listen(&p, at, HISS_AMP) < VOICE_ON) {
		fputs("quiet speech-band hiss missed\n", stderr);
		failed++;
	}
	return failed != 0;
}
