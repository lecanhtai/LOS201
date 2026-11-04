#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include "sms_sensor.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LeCanhTai - SE200651");
MODULE_DESCRIPTION("Assignment 02: SMS Sensor Kernel Module");
MODULE_VERSION("1.0");

#define DEVICE_NAME "sms_sensor"
#define CLASS_NAME "sms_class"

static int major_number;
static struct class *sms_class = NULL;
static struct device *sms_device = NULL;
static struct proc_dir_entry *proc_sms_entry = NULL;

static unsigned int read_count = 0;
static int last_temp_int = 0;
static int last_hum_int = 0;
static int sampling_interval_ms = 100;
static unsigned long start_jiffies;

static DEFINE_MUTEX(sms_mutex);

static int sms_open(struct inode *inode, struct file *file) {
    return 0;
}

static int sms_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t sms_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char kbuf[128];
    int datalen;
    unsigned int rand_val;
    unsigned long ts_ms;
    int temp_whole, temp_frac, hum_whole, hum_frac;

    if (*offset > 0)
        return 0;

    msleep(sampling_interval_ms);

    mutex_lock(&sms_mutex);
    read_count++;
    ts_ms = jiffies_to_msecs(jiffies);

    rand_val = get_random_u32();
    last_temp_int = 1500 + (rand_val % 3000);

    rand_val = get_random_u32();
    last_hum_int = 3000 + (rand_val % 6000);

    temp_whole = last_temp_int / 100;
    temp_frac = last_temp_int % 100;
    hum_whole = last_hum_int / 100;
    hum_frac = last_hum_int % 100;

    datalen = snprintf(kbuf, sizeof(kbuf), "%lu,%d.%02d,%d.%02d\n", ts_ms, temp_whole, temp_frac, hum_whole, hum_frac);
    mutex_unlock(&sms_mutex);

    if (len < datalen)
        return -EINVAL;

    if (copy_to_user(buf, kbuf, datalen))
        return -EFAULT;

    *offset += datalen;
    return datalen;
}

static long sms_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case SMS_SET_INTERVAL: {
            int interval;
            if (copy_from_user(&interval, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (interval < 10)
                return -EINVAL;

            mutex_lock(&sms_mutex);
            sampling_interval_ms = interval;
            mutex_unlock(&sms_mutex);
            break;
        }
        default:
            return -ENOTTY;
    }
    return 0;
}

static int sms_proc_show(struct seq_file *m, void *v) {
    unsigned long uptime_sec = (jiffies - start_jiffies) / HZ;

    mutex_lock(&sms_mutex);
    seq_printf(m, "SMS Sensor Driver Statistics\n");
    seq_printf(m, "============================\n");
    seq_printf(m, "Read Count       : %u\n", read_count);
    seq_printf(m, "Last Temperature : %d.%02d C\n", last_temp_int / 100, last_temp_int % 100);
    seq_printf(m, "Last Humidity    : %d.%02d %%\n", last_hum_int / 100, last_hum_int % 100);
    seq_printf(m, "Sampling Interval: %d ms\n", sampling_interval_ms);
    seq_printf(m, "Driver Uptime    : %lu seconds\n", uptime_sec);
    mutex_unlock(&sms_mutex);

    return 0;
}

static int sms_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, sms_proc_show, NULL);
}

static const struct proc_ops sms_proc_ops = {
    .proc_open    = sms_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static const struct file_operations sms_fops = {
    .owner   = THIS_MODULE,
    .open    = sms_open,
    .release = sms_release,
    .read    = sms_read,
    .unlocked_ioctl = sms_ioctl,
};

static int __init sms_init(void) {
    start_jiffies = jiffies;

    major_number = register_chrdev(0, DEVICE_NAME, &sms_fops);
    if (major_number < 0) return major_number;

    sms_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sms_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(sms_class);
    }

    sms_device = device_create(sms_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(sms_device)) {
        class_destroy(sms_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(sms_device);
    }

    proc_sms_entry = proc_create("sms_stats", 0444, NULL, &sms_proc_ops);
    return 0;
}

static void __exit sms_exit(void) {
    if (proc_sms_entry)
        proc_remove(proc_sms_entry);
    device_destroy(sms_class, MKDEV(major_number, 0));
    class_destroy(sms_class);
    unregister_chrdev(major_number, DEVICE_NAME);
}

module_init(sms_init);
module_exit(sms_exit);
