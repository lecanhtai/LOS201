#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <mqueue.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/mman.h>
#include <sched.h>
#include "circular_buffer.h"
#include "../driver/sms_sensor.h"

#define SENSOR_DEV "/dev/sms_sensor"
#define MQ_NAME "/sms_alert_mq"

int running = 1;
CircularBuffer cb;
mqd_t alert_mq;

void handle_sigint(int sig) {
    running = 0;
}

// Sensor Thread (RT SCHED_FIFO)
void *sensor_thread_func(void *arg) {
    int interval_ms = *(int *)arg;
    int fd = open(SENSOR_DEV, O_RDONLY);
    if (fd < 0) {
        perror("Sensor Thread: Failed to open device");
        return NULL;
    }

    // Cấu hình ioctl interval cho driver
    ioctl(fd, SMS_SET_INTERVAL, interval_ms);

    struct sched_param param;
    param.sched_priority = 50;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    while (running) {
        char buf[128] = {0};
        ssize_t bytes = read(fd, buf, sizeof(buf) - 1);
        if (bytes > 0) {
            SensorData data;
            if (sscanf(buf, "%llu,%f,%f", &data.timestamp_ms, &data.temperature, &data.humidity) == 3) {
                cb_push(&cb, data);
            }
        }
    }
    close(fd);
    return NULL;
}

// Processor Thread
void *processor_thread_func(void *arg) {
    while (running) {
        SensorData data = cb_pop(&cb);
        // Kiểm tra ngưỡng bất thường (ví dụ: temp > 40 hoặc hum > 80)
        if (data.temperature > 40.0f || data.humidity > 80.0f) {
            char msg[256];
            snprintf(msg, sizeof(msg), "ALERT: Temp=%.2f, Hum=%.2f", data.temperature, data.humidity);
            mq_send(alert_mq, msg, strlen(msg) + 1, 0);
        }
    }
    return NULL;
}

// Alert Thread
void *alert_thread_func(void *arg) {
    char msg[256];
    struct mq_attr attr;
    mq_getattr(alert_mq, &attr);

    while (running) {
        ssize_t bytes = mq_receive(alert_mq, msg, attr.mq_msgsize, NULL);
        if (bytes > 0) {
            fprintf(stderr, "[ALERT] %s\n", msg);
        }
    }
    return NULL;
}

// Logger Thread
void *logger_thread_func(void *arg) {
    FILE *f = fopen("/tmp/sms.log", "w");
    if (!f) return NULL;

    while (running) {
        sleep(5); // Flush định kỳ mỗi 5 giây
        fflush(f);
    }
    fclose(f);
    return NULL;
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    mlockall(MCL_CURRENT | MCL_FUTURE);

    int interval_ms = 500;
    cb_init(&cb);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 256;
    attr.mq_curmsgs = 0;
    alert_mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr);

    pthread_t t_sensor, t_processor, t_alert, t_logger;
    pthread_create(&t_sensor, NULL, sensor_thread_func, &interval_ms);
    pthread_create(&t_processor, NULL, processor_thread_func, NULL);
    pthread_create(&t_alert, NULL, alert_thread_func, NULL);
    pthread_create(&t_logger, NULL, logger_thread_func, NULL);

    while (running) {
        sleep(1);
    }

    // Graceful shutdown
    pthread_join(t_sensor, NULL);
    pthread_join(t_processor, NULL);
    pthread_join(t_alert, NULL);
    pthread_join(t_logger, NULL);

    cb_destroy(&cb);
    mq_close(alert_mq);
    mq_unlink(MQ_NAME);

    printf("SMS Application shutdown cleanly.\n");
    return 0;
}
