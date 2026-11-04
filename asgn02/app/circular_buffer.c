#include "circular_buffer.h"
#include <stdio.h>

void cb_init(CircularBuffer *cb) {
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    cb->total_produced = 0;
    cb->total_consumed = 0;
    cb->overflow_count = 0;
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->not_full, NULL);
    pthread_cond_init(&cb->not_empty, NULL);
}

void cb_destroy(CircularBuffer *cb) {
    pthread_mutex_destroy(&cb->mutex);
    pthread_cond_destroy(&cb->not_full);
    pthread_cond_destroy(&cb->not_empty);
}

void cb_push(CircularBuffer *cb, SensorData data) {
    pthread_mutex_lock(&cb->mutex);
    if (cb->count == BUFFER_SIZE) {
        cb->overflow_count++;
        // Bỏ qua mẫu cũ hoặc đợi, ở đây ta ghi đè / bỏ qua để tránh block sensor thread
        pthread_mutex_unlock(&cb->mutex);
        return;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % BUFFER_SIZE;
    cb->count++;
    cb->total_produced++;
    pthread_cond_signal(&cb->not_empty);
    pthread_mutex_unlock(&cb->mutex);
}

SensorData cb_pop(CircularBuffer *cb) {
    SensorData data;
    pthread_mutex_lock(&cb->mutex);
    while (cb->count == 0) {
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }
    data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % BUFFER_SIZE;
    cb->count--;
    cb->total_consumed++;
    pthread_cond_signal(&cb->not_full);
    pthread_mutex_unlock(&cb->mutex);
    return data;
}
