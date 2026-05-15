#include "events.h"
#include <stdlib.h>

EventQueue* createEventQueue(uint16_t size) {
    EventQueue* queue = malloc(sizeof(EventQueue));
    queue->size = size;
    queue->tail = 0;
    queue->index = 0;
    queue->lock_size = 0;
    queue->events = malloc(sizeof(Event) * size);
    return queue;
}

void destroyEventQueue(EventQueue* queue) {
    free(queue->events);
    free(queue);
}

void addEventToQueue(EventQueue* queue, Event event) {
    if (!queue->lock_size && queue->tail >= queue->size) {
        if (queue->size <= UINT16_MAX / 2) {
            queue->size *= 2;
        } else {
            queue->size = UINT16_MAX;
        }
        queue->events = realloc(queue->events, sizeof(Event) * queue->size);
    }
    if (queue->tail == queue->size) {
        queue->tail = 0;
    }
    queue->events[queue->tail] = event;
    queue->tail++;
}

void trimAndLockEventQueue(EventQueue* queue) {
    if (queue->lock_size) {
        return;
    }
    queue->size = queue->tail;
    queue->events = realloc(queue->events, sizeof(Event) * queue->size);
    queue->lock_size = 1;
}