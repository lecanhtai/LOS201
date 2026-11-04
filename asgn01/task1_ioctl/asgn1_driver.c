#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SinhVien - MSSV");
MODULE_DESCRIPTION("Assignment 01: Character Driver with ioctl");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024

static int major_number;
static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;

static int open_count = 0;
static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;
static int current_mode = 1;

static struct class *asgn1_class = NULL;
static struct device *asgn1_device = NULL;
static DEFINE_MUTEX(asgn1_mutex);

static int asgn1_open(struct inode *inode, struct file *file) {
    mutex_lock(&asgn1_mutex);
    open_count++;
    mutex_unlock(&asgn1_mutex);
    pr_info("asgn1_driver: device opened\n");
    return 0;
}

static int asgn1_release(struct inode *inode, struct file *file) {
    pr_info("asgn1_driver: device closed\n");
    return 0;
}

static ssize_t asgn1_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    int bytes_read = 0;
    mutex_lock(&asgn1_mutex);
    if (*offset >= buffer_len) {
        mutex_unlock(&asgn1_mutex);
        return 0;
    }
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf, device_buffer + *offset, len)) {
        mutex_unlock(&asgn1_mutex);
        return -EFAULT;
    }
    *offset += len;
    bytes_read = len;
    read_count++;
    mutex_unlock(&asgn1_mutex);
    pr_info("asgn1_driver: read %d bytes\n", bytes_read);
    return bytes_read;
}

static ssize_t asgn1_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    char temp_buf[BUFFER_SIZE];
    size_t actual_len = len;

    if (actual_len > BUFFER_SIZE - 1)
        actual_len = BUFFER_SIZE - 1;

    if (copy_from_user(temp_buf, buf, actual_len))
        return -EFAULT;
    temp_buf[actual_len] = '\0';

    mutex_lock(&asgn1_mutex);
    last_write_size = actual_len;
    write_count++;

    if (current_mode == 0) {
        int prefix_len = snprintf(device_buffer, BUFFER_SIZE, "[KERNEL] ");
        if (actual_len + prefix_len < BUFFER_SIZE) {
            memcpy(device_buffer + prefix_len, temp_buf, actual_len);
            buffer_len = actual_len + prefix_len;
            device_buffer[buffer_len] = '\0';
        } else {
            memcpy(device_buffer, temp_buf, actual_len);
            buffer_len = actual_len;
            device_buffer[buffer_len] = '\0';
        }
    } else {
        memcpy(device_buffer, temp_buf, actual_len);
        buffer_len = actual_len;
        device_buffer[buffer_len] = '\0';
    }
    mutex_unlock(&asgn1_mutex);
    pr_info("asgn1_driver: wrote %zu bytes (mode=%d)\n", actual_len, current_mode);
    return len;
}

static long asgn1_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pr_info("asgn1_driver: ioctl cmd 0x%X received\n", cmd);
    switch (cmd) {
        case ASGN1_RESET_BUFFER:
            mutex_lock(&asgn1_mutex);
            memset(device_buffer, 0, BUFFER_SIZE);
            buffer_len = 0;
            write_count = 0;
            read_count = 0;
            mutex_unlock(&asgn1_mutex);
            pr_info("asgn1_driver: RESET_BUFFER executed\n");
            break;
        case ASGN1_GET_STATS: {
            struct asgn1_stats stats;
            mutex_lock(&asgn1_mutex);
            stats.open_count = open_count;
            stats.write_count = write_count;
            stats.read_count = read_count;
            stats.buffer_len = buffer_len;
            stats.last_write_size = last_write_size;
            mutex_unlock(&asgn1_mutex);
            if (copy_to_user((struct asgn1_stats __user *)arg, &stats, sizeof(stats)))
                return -EFAULT;
            pr_info("asgn1_driver: GET_STATS executed\n");
            break;
        }
        case ASGN1_SET_MODE: {
            int mode;
            if (copy_from_user(&mode, (int __user *)arg, sizeof(int)))
                return -EFAULT;
            if (mode != 0 && mode != 1)
                return -EINVAL;
            mutex_lock(&asgn1_mutex);
            current_mode = mode;
            mutex_unlock(&asgn1_mutex);
            pr_info("asgn1_driver: SET_MODE changed to %d\n", mode);
            break;
        }
        case ASGN1_GET_VERSION: {
            char version_str[64];
            snprintf(version_str, sizeof(version_str), "asgn1_driver v1.0");
            if (copy_to_user((char __user *)arg, version_str, strlen(version_str) + 1))
                return -EFAULT;
            pr_info("asgn1_driver: GET_VERSION executed\n");
            break;
        }
        default:
            return -ENOTTY;
    }
    return 0;
}

static struct file_operations asgn1_fops = {
    .owner   = THIS_MODULE,
    .open    = asgn1_open,
    .release = asgn1_release,
    .read    = asgn1_read,
    .write   = asgn1_write,
    .unlocked_ioctl = asgn1_ioctl,
};

static int __init asgn1_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &asgn1_fops);
    if (major_number < 0) return major_number;

    asgn1_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(asgn1_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_class);
    }

    asgn1_device = device_create(asgn1_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(asgn1_device)) {
        class_destroy(asgn1_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(asgn1_device);
    }

    pr_info("asgn1_driver loaded with major %d\n", major_number);
    return 0;
}

static void __exit asgn1_exit(void) {
    device_destroy(asgn1_class, MKDEV(major_number, 0));
    class_destroy(asgn1_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("asgn1_driver unloaded\n");
}

module_init(asgn1_init);
module_exit(asgn1_exit);
