#ifndef PRESENCE_H
#define PRESENCE_H

/* Speech-band energy over noise floor. */
struct presence {
	float b0;
	float a1;
	float a2;
	float z1;
	float z2;
	float floor;
};

void presence_init(struct presence *p, float rate);
float presence_prob(struct presence *p, const float *window, int n);

#endif
