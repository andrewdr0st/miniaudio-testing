#include "envelope.h"
#include "math_utils.h"

float sampleASDREnvelope(asdr_env env, float elapsed_time, float end_time) {
    if (elapsed_time < env.attack) {
        return LERP(0, 1, elapsed_time / env.attack);
    } else if (elapsed_time < env.attack + env.decay) {
        return LERP(1, env.sustain, (elapsed_time - env.attack) / env.decay);
    } else if (elapsed_time > end_time) {
        float val = LERP(env.sustain, 0, (elapsed_time - end_time) / env.release);
        return FMAX(0, val);
    } else {
        return env.sustain;
    }
}
