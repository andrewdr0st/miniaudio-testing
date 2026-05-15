#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

#define EVENT_TIME_OFFSET 0x00
#define EVENT_NOTE_ON 0x01
#define EVENT_NOTE_OFF 0x02

typedef struct {
    uint8_t offset;
    uint8_t event_type;
    uint16_t value;
} Event;

typedef struct {
    uint16_t size;
    uint16_t tail;
    uint16_t index;
    uint8_t lock_size;
    Event* events;
} EventQueue;

EventQueue* createEventQueue(uint16_t size);
void destroyEventQueue(EventQueue*);
void addEventToQueue(EventQueue*, Event event);
void trimAndLockEventQueue(EventQueue*);

#endif