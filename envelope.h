#ifndef ENVELOPE_H
#define ENVELOPE_H

typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
} asdr_env;

asdr_env* createASDREnvelope(float attack, float decay, float sustain, float release);
float sampleASDREnvelope(asdr_env* env, float time, float end_time);

#endif