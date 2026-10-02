#include "speech/presence.h"

#include <math.h>

/* Whispers live at 1-4 kHz. */
#define CENTER_HZ 2000.0f
#define Q 0.7f
/* 1.5 dB/s per 32 ms window. */
#define FLOOR_RISE 1.011f
#define SNR_MIN_DB 6.0f
#define SNR_SPAN_DB 12.0f

void presence_init(struct presence *p, float rate)
{
	float w = 2.0f * (float)M_PI * CENTER_HZ / rate;
	float alpha = sinf(w) / (2.0f * Q);
	float a0 = 1.0f + alpha;

	*p = (struct presence){
		.b0 = alpha / a0,
		.a1 = -2.0f * cosf(w) / a0,
		.a2 = (1.0f - alpha) / a0,
		.floor = INFINITY,
	};
}

/* Bandpass with b1 = 0, b2 = -b0. */
static float band_power(struct presence *p, const float *x, int n)
{
	float z1 = p->z1;
	float z2 = p->z2;
	float sum = 0.0f;

	for (int i = 0; i < n; i++) {
		float y = p->b0 * x[i] + z1;

		z1 = z2 - p->a1 * y;
		z2 = -p->b0 * x[i] - p->a2 * y;
		sum += y * y;
	}
	p->z1 = z1;
	p->z2 = z2;
	return sum / (float)n;
}

float presence_prob(struct presence *p, const float *window, int n)
{
	float power = band_power(p, window, n);
	float snr_db;

	p->floor = fminf(power, p->floor * FLOOR_RISE);
	snr_db = 10.0f * log10f(power / p->floor);
	return fminf(fmaxf((snr_db - SNR_MIN_DB) / SNR_SPAN_DB, 0.0f), 1.0f);
}
