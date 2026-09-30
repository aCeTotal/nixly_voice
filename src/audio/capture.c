#include "audio/capture.h"

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>

/* One Silero window per quantum. */
#define LATENCY "512/16000"
/* Raw mic, ahead of mute. */
#define TARGET "nixly-mic-tap"

struct capture {
	struct pw_thread_loop *loop;
	struct pw_stream *stream;
	struct ring *ring;
};

static struct capture capture;

static void on_process(void *data)
{
	struct capture *c = data;
	struct pw_buffer *b = pw_stream_dequeue_buffer(c->stream);
	struct spa_data *d;

	if (b == NULL)
		return;
	d = &b->buffer->datas[0];
	if (d->data && d->chunk->size)
		ring_push(c->ring, (const float *)((const uint8_t *)d->data + d->chunk->offset),
			  d->chunk->size / sizeof(float));
	pw_stream_queue_buffer(c->stream, b);
}

/* PipeWire gone: supervisor restarts us. */
static void on_state_changed(void *data, enum pw_stream_state old,
			     enum pw_stream_state state, const char *error)
{
	struct capture *c = data;

	if (error)
		fprintf(stderr, "nixly-voice: capture: %s\n", error);
	if (state == PW_STREAM_STATE_ERROR ||
	    (state == PW_STREAM_STATE_UNCONNECTED && old != PW_STREAM_STATE_UNCONNECTED))
		ring_close(c->ring);
}

static const struct pw_stream_events stream_events = {
	PW_VERSION_STREAM_EVENTS,
	.state_changed = on_state_changed,
	.process = on_process,
};

static bool connect_stream(struct capture *c)
{
	uint8_t buf[512];
	struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buf, sizeof(buf));
	const struct spa_pod *params[1];
	struct spa_audio_info_raw fmt = {
		.format = SPA_AUDIO_FORMAT_F32,
		.rate = CAPTURE_RATE,
		.channels = 1,
		.position = { SPA_AUDIO_CHANNEL_MONO },
	};

	c->stream = pw_stream_new_simple(pw_thread_loop_get_loop(c->loop), "nixly-voice",
		pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
				  PW_KEY_MEDIA_CATEGORY, "Capture",
				  PW_KEY_NODE_NAME, "nixly-voice",
				  PW_KEY_NODE_DESCRIPTION, "Nixly Voice",
				  PW_KEY_NODE_LATENCY, LATENCY,
				  PW_KEY_TARGET_OBJECT, TARGET,
				  NULL),
		&stream_events, c);
	if (c->stream == NULL)
		return false;
	params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &fmt);
	return pw_stream_connect(c->stream, SPA_DIRECTION_INPUT, PW_ID_ANY,
				 PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS |
				 PW_STREAM_FLAG_RT_PROCESS,
				 params, 1) == 0;
}

bool capture_start(struct ring *ring)
{
	struct capture *c = &capture;
	bool connected;

	pw_init(NULL, NULL);
	c->ring = ring;
	c->loop = pw_thread_loop_new("nixly-voice-capture", NULL);
	if (c->loop == NULL)
		return false;
	pw_thread_loop_lock(c->loop);
	connected = connect_stream(c) && pw_thread_loop_start(c->loop) == 0;
	pw_thread_loop_unlock(c->loop);
	return connected;
}
