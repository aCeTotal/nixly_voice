#include <stdio.h>
#include <string.h>

#include "speech/endpoint.h"

#define VOICE 0.9f
#define HUSH 0.1f

static int failed;

static void expect(int line, int got, int want)
{
	if (got == want)
		return;
	fprintf(stderr, "line %d: got %d, want %d\n", line, got, want);
	failed++;
}

#define EXPECT(got, want) expect(__LINE__, (int)(got), (int)(want))

/* Each step must be none. */
static void run(struct endpoint *e, float prob, int n)
{
	for (int i = 0; i < n; i++)
		EXPECT(endpoint_push(e, prob), STEP_NONE);
}

static void hear(struct endpoint *e, enum intent_kind kind, const char *query)
{
	struct intent in = { .kind = kind };

	strcpy(in.query, query);
	endpoint_heard(e, &in);
}

/* Speech long enough to probe. */
static void speak(struct endpoint *e)
{
	EXPECT(endpoint_push(e, VOICE), STEP_BEGIN);
	run(e, VOICE, 5);
	EXPECT(endpoint_push(e, HUSH), STEP_PROBE);
}

static void blip_is_ignored(void)
{
	struct endpoint e = { 0 };

	run(&e, HUSH, 5);
	EXPECT(endpoint_push(&e, VOICE), STEP_BEGIN);
	run(&e, VOICE, 2);
	run(&e, HUSH, 12);
	EXPECT(e.phase, PHASE_IDLE);
}

static void command_probes_at_the_dip(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_OPEN, "");
	run(&e, HUSH, 2);
	run(&e, VOICE, 4);
	EXPECT(endpoint_push(&e, HUSH), STEP_PROBE);
	hear(&e, INTENT_CALCULATOR, "");
	run(&e, VOICE, 20);
	run(&e, HUSH, 12);
	EXPECT(e.phase, PHASE_IDLE);
}

static void search_commits_after_the_pause(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_SEARCH, "været");
	run(&e, HUSH, 1);
	EXPECT(endpoint_push(&e, HUSH), STEP_REFRESH);
	hear(&e, INTENT_SEARCH, "været i Oslo");
	run(&e, HUSH, 8);
	EXPECT(endpoint_push(&e, HUSH), STEP_COMMIT);
	EXPECT(e.phase, PHASE_IDLE);
}

static void search_waits_for_its_query(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_SEARCH, "");
	run(&e, HUSH, 1);
	EXPECT(endpoint_push(&e, HUSH), STEP_REFRESH);
	hear(&e, INTENT_SEARCH, "");
	run(&e, HUSH, 20);
	run(&e, VOICE, 10);
	run(&e, HUSH, 2);
	EXPECT(endpoint_push(&e, HUSH), STEP_REFRESH);
	hear(&e, INTENT_SEARCH, "været");
	run(&e, HUSH, 8);
	EXPECT(endpoint_push(&e, HUSH), STEP_COMMIT);
}

static void open_waits_for_its_target(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_OPEN, "");
	run(&e, HUSH, 20);
	run(&e, VOICE, 6);
	EXPECT(endpoint_push(&e, HUSH), STEP_PROBE);
	hear(&e, INTENT_CALCULATOR, "");
	EXPECT(e.phase, PHASE_DONE);
}

static void unsure_word_does_not_wait(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_UNSURE, "");
	run(&e, HUSH, 11);
	EXPECT(e.phase, PHASE_IDLE);
}

static void fluent_speech_is_probed(void)
{
	struct endpoint e = { 0 };

	EXPECT(endpoint_push(&e, VOICE), STEP_BEGIN);
	run(&e, VOICE, 13);
	EXPECT(endpoint_push(&e, VOICE), STEP_PROBE);
	hear(&e, INTENT_NONE, "");
	run(&e, VOICE, 100);
	EXPECT(e.phase, PHASE_DONE);
}

static void long_search_is_cut(void)
{
	struct endpoint e = { 0 };

	speak(&e);
	hear(&e, INTENT_SEARCH, "været");
	run(&e, VOICE, ENDPOINT_MAX_WINDOWS - 8);
	EXPECT(endpoint_push(&e, VOICE), STEP_FINAL);
	EXPECT(e.phase, PHASE_DONE);
	run(&e, VOICE, 50);
	run(&e, HUSH, 12);
	EXPECT(e.phase, PHASE_IDLE);
}

static void hysteresis_holds_speech(void)
{
	struct endpoint e = { 0 };

	run(&e, 0.4f, 3);
	EXPECT(endpoint_push(&e, VOICE), STEP_BEGIN);
	run(&e, 0.4f, 5);
	EXPECT(e.speech, 6);
}

int main(void)
{
	blip_is_ignored();
	command_probes_at_the_dip();
	search_commits_after_the_pause();
	search_waits_for_its_query();
	open_waits_for_its_target();
	unsure_word_does_not_wait();
	fluent_speech_is_probed();
	long_search_is_cut();
	hysteresis_holds_speech();
	return failed != 0;
}
