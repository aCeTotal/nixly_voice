#include "speech/endpoint.h"

#define VAD_ON 0.5f
#define VAD_OFF 0.35f
/* Windows of 32 ms each. */
#define END_GAP 12
#define AWAIT_GAP 31
/* Pause ending an app name. */
#define HOLD_GAP 8
#define SEARCH_TAIL 3
#define MIN_SPEECH 6
/* Short fragments mishear whispers. */
#define SETTLE_SPEECH 31
#define PROBE_EVERY 15

static enum step begin(struct endpoint *e)
{
	*e = (struct endpoint){
		.phase = PHASE_LISTEN,
		.voiced = true,
		.windows = 1,
		.speech = 1,
		.fresh = 1,
	};
	return STEP_BEGIN;
}

/* Searches fire when speech ends. */
static enum step close_segment(struct endpoint *e, enum phase next, enum step search)
{
	enum phase was = e->phase;

	e->phase = next;
	return was == PHASE_SEARCH ? search : STEP_NONE;
}

static void count(struct endpoint *e)
{
	e->windows++;
	if (!e->voiced) {
		e->silence++;
		return;
	}
	e->speech++;
	e->fresh++;
	e->silence = 0;
}

static bool probe_due(const struct endpoint *e)
{
	return e->phase == PHASE_LISTEN && e->speech >= MIN_SPEECH && e->fresh > 0 &&
	       (e->silence == 1 || e->fresh >= PROBE_EVERY);
}

/* Decoded in pause, committed later. */
static bool refresh_due(const struct endpoint *e)
{
	return e->phase == PHASE_SEARCH && e->fresh > 0 && e->silence == SEARCH_TAIL;
}

static bool hold_due(const struct endpoint *e)
{
	return e->phase == PHASE_LISTEN && e->holding && e->silence == HOLD_GAP;
}

enum step endpoint_push(struct endpoint *e, float prob)
{
	e->voiced = prob >= (e->voiced ? VAD_OFF : VAD_ON);
	if (e->phase == PHASE_IDLE)
		return e->voiced ? begin(e) : STEP_NONE;
	count(e);
	if (e->silence >= (e->awaiting ? AWAIT_GAP : END_GAP))
		return close_segment(e, PHASE_IDLE, STEP_COMMIT);
	if (e->windows >= ENDPOINT_MAX_WINDOWS && e->phase != PHASE_DONE)
		return close_segment(e, PHASE_DONE, STEP_FINAL);
	if (hold_due(e)) {
		e->phase = PHASE_DONE;
		return STEP_COMMIT;
	}
	if (refresh_due(e)) {
		e->fresh = 0;
		return STEP_REFRESH;
	}
	if (!probe_due(e))
		return STEP_NONE;
	e->fresh = 0;
	return STEP_PROBE;
}

static enum phase after(const struct endpoint *e, enum intent_kind kind)
{
	static const enum phase next[] = {
		[INTENT_NONE] = PHASE_DONE,
		[INTENT_UNSURE] = PHASE_LISTEN,
		[INTENT_OPEN] = PHASE_LISTEN,
		[INTENT_SEARCH] = PHASE_SEARCH,
		[INTENT_APP_PREFIX] = PHASE_LISTEN,
		[INTENT_HOME] = PHASE_DONE,
		[INTENT_CALCULATOR] = PHASE_DONE,
		[INTENT_BROWSER] = PHASE_DONE,
		[INTENT_APP] = PHASE_DONE,
	};

	if (kind == INTENT_NONE && e->speech < SETTLE_SPEECH)
		return PHASE_LISTEN;
	return next[kind];
}

void endpoint_heard(struct endpoint *e, const struct intent *in)
{
	/* Probes are not search decodes. */
	if (e->phase == PHASE_LISTEN && in->kind == INTENT_SEARCH)
		e->fresh = 1;
	if (e->phase == PHASE_LISTEN)
		e->phase = after(e, in->kind);
	e->holding = e->phase == PHASE_LISTEN && in->kind == INTENT_APP_PREFIX;
	e->awaiting = e->phase != PHASE_DONE && intent_awaits(in);
}
