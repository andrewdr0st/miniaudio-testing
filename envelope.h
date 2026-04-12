#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
} asdr_env;

float sampleASDREnvelope(asdr_env env, float time, float end_time);

#endif