#include "instrument.h"
#include <stdlib.h>

void destroyInstrument(instrument* inst) {
    free(inst->wf);
    free(inst->env);
    free(inst);
}