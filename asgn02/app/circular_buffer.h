#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <pthread.h>

#define BUFFER_SIZE 64

typedef struct {
    unsigned long long timestamp_ms;
    float temperature;
    float humidity;
} SensorData;

typedef struct {
    SensorData buffer[BUFFER_SIZE];
    int head;
    int tail;
    int count;
    int total_produced;
    int total_consumed;
    int overflow_count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} CircularBuffer;

void cb_init(CircularBuffer *cb);
void cb_destroy(CircularBuffer *cb);
void cb_push(CircularBuffer *cb, SensorData data);
SensorData cb_pop(CircularBuffer *cb);

#endif
