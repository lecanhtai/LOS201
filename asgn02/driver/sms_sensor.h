#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

#define SMS_MAGIC 's'
#define SMS_SET_INTERVAL _IOW(SMS_MAGIC, 1, int)

struct sms_sensor_data {
    unsigned long long timestamp_ms;
    float temperature;
    float humidity;
};

#endif
