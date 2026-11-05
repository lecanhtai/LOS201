#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Lab");
MODULE_DESCRIPTION("LAB-02: Simple Character Device Driver with procfs & sysfs");
MODULE_VERSION("1.0");

#define DEVICE_NAME "lab2"
#define CLASS_NAME "lab2_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 240

static int major_number = DRIVER_MAJOR;
static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;
static int open_count = 0;

/* Bien them moi - tat ca duoc bao ve boi lab2_mutex */
static int reverse_read = 0;              /* 0 hoac 1 */
static unsigned int overwrite_count = 0;  /* so lan ghi de */
static int case_mode = 0;                 /* 0 giu nguyen, 1 HOA, 2 thuong */

static struct class *lab2_class = NULL;
static struct device *lab2_device = NULL;

static DEFINE_MUTEX(lab2_mutex);
static struct proc_dir_entry *proc_entry;

/* ===== PROCFS Implementation ===== */
static int lab2_proc_show(struct seq_file *m, void *v) {
    mutex_lock(&lab2_mutex);
    seq_printf(m, "LAB-02 Driver Statistics\n");
    seq_printf(m, "========================\n");
    seq_printf(m, "Driver name     : %s\n", DEVICE_NAME);
    seq_printf(m, "Major number    : %d\n", major_number);
    seq_printf(m, "Buffer size     : %d bytes\n", BUFFER_SIZE);
    seq_printf(m, "Data length     : %d bytes\n", buffer_len);
    seq_printf(m, "Open count      : %d\n", open_count);
    seq_printf(m, "Overwrites    : %u\n", overwrite_count);
    if (buffer_len > 0)
        seq_printf(m, "Last data       : [%s]\n", device_buffer);
    mutex_unlock(&lab2_mutex);
    return 0;
}

static int lab2_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, lab2_proc_show, NULL);
}

static const struct proc_ops lab2_proc_ops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ===== SYSFS Implementation ===== */
static ssize_t buffer_len_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int v;
    mutex_lock(&lab2_mutex);
    v = buffer_len;
    mutex_unlock(&lab2_mutex);
    return sprintf(buf, "%d\n", v);
}

static ssize_t open_count_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int v;
    mutex_lock(&lab2_mutex);
    v = open_count;
    mutex_unlock(&lab2_mutex);
    return sprintf(buf, "%d\n", v);
}

static ssize_t last_data_show(struct device *dev, struct device_attribute *attr, char *buf) {
    ssize_t n;
    mutex_lock(&lab2_mutex);
    if (buffer_len > 0)
        n = sprintf(buf, "%s\n", device_buffer);
    else
        n = sprintf(buf, "(empty)\n");
    mutex_unlock(&lab2_mutex);
    return n;
}

/* B2: reverse_read (RW) */
static ssize_t reverse_read_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int v;
    mutex_lock(&lab2_mutex);
    v = reverse_read;
    mutex_unlock(&lab2_mutex);
    return sprintf(buf, "%d\n", v);
}

static ssize_t reverse_read_store(struct device *dev, struct device_attribute *attr,
                                  const char *buf, size_t count) {
    int v;
    if (kstrtoint(buf, 10, &v) || (v != 0 && v != 1))
        return -EINVAL;
    mutex_lock(&lab2_mutex);
    reverse_read = v;
    mutex_unlock(&lab2_mutex);
    return count;
}

/* B3: overwrite_count (RO) */
static ssize_t overwrite_count_show(struct device *dev, struct device_attribute *attr, char *buf) {
    unsigned int v;
    mutex_lock(&lab2_mutex);
    v = overwrite_count;
    mutex_unlock(&lab2_mutex);
    return sprintf(buf, "%u\n", v);
}

/* case_mode (RW): 0 giu nguyen, 1 HOA, 2 thuong */
static ssize_t case_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
    int v;
    mutex_lock(&lab2_mutex);
    v = case_mode;
    mutex_unlock(&lab2_mutex);
    return sprintf(buf, "%d\n", v);
}

static ssize_t case_mode_store(struct device *dev, struct device_attribute *attr,
                               const char *buf, size_t count) {
    int v;
    if (kstrtoint(buf, 10, &v) || v < 0 || v > 2)
        return -EINVAL;
    mutex_lock(&lab2_mutex);
    case_mode = v;
    mutex_unlock(&lab2_mutex);
    return count;
}

static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);
static DEVICE_ATTR_RW(reverse_read);
static DEVICE_ATTR_RO(overwrite_count);
static DEVICE_ATTR_RW(case_mode);

static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    &dev_attr_reverse_read.attr,
    &dev_attr_overwrite_count.attr,
    &dev_attr_case_mode.attr,
    NULL,
};

static struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};

/* ===== File Operations ===== */
static int lab2_open(struct inode *inode, struct file *file) {
    int c;
    mutex_lock(&lab2_mutex);
    open_count++;
    c = open_count;
    mutex_unlock(&lab2_mutex);
    pr_info("lab2_driver: device opened (count=%d)\n", c);
    return 0;
}

static int lab2_release(struct inode *inode, struct file *file) {
    pr_info("lab2_driver: device closed\n");
    return 0;
}

static ssize_t lab2_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    size_t i;

    mutex_lock(&lab2_mutex);
    if (*offset < 0 || *offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }
    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    for (i = 0; i < len; i++) {
        size_t idx = reverse_read ? (size_t)buffer_len - 1 - (size_t)(*offset + i)
                                  : (size_t)(*offset + i);
        char c = device_buffer[idx];

        if (case_mode == 1 && c >= 'a' && c <= 'z')
            c -= 'a' - 'A';
        else if (case_mode == 2 && c >= 'A' && c <= 'Z')
            c += 'a' - 'A';

        if (put_user(c, buf + i)) {
            mutex_unlock(&lab2_mutex);
            pr_err("lab2_driver: put_user failed\n");
            return -EFAULT;
        }
    }
    *offset += len;
    mutex_unlock(&lab2_mutex);
    pr_info("lab2_driver: sent %zu bytes to user\n", len);
    return len;
}

static ssize_t lab2_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    if (len > BUFFER_SIZE - 1) {
        pr_warn("lab2_driver: write too large, truncating\n");
        len = BUFFER_SIZE - 1;
    }
    mutex_lock(&lab2_mutex);
    if (copy_from_user(device_buffer, buf, len)) {
        mutex_unlock(&lab2_mutex);
        return -EFAULT;
    }
    if (buffer_len > 0)          /* B3: ghi de du lieu da co */
        overwrite_count++;
    buffer_len = len;
    device_buffer[buffer_len] = '\0';
    pr_info("lab2_driver: received %zu bytes: [%s]\n", len, device_buffer);
    mutex_unlock(&lab2_mutex);
    return len;
}

/* B1: llseek */
static loff_t lab2_llseek(struct file *file, loff_t off, int whence) {
    loff_t newpos;

    mutex_lock(&lab2_mutex);
    switch (whence) {
    case SEEK_SET:
        newpos = off;
        break;
    case SEEK_CUR:
        newpos = file->f_pos + off;
        break;
    case SEEK_END:
        newpos = (loff_t)buffer_len + off;
        break;
    default:
        mutex_unlock(&lab2_mutex);
        return -EINVAL;
    }
    if (newpos < 0 || newpos > (loff_t)buffer_len) {
        mutex_unlock(&lab2_mutex);
        return -EINVAL;          /* f_pos giu nguyen */
    }
    file->f_pos = newpos;
    mutex_unlock(&lab2_mutex);
    return newpos;
}

static struct file_operations lab2_fops = {
    .owner   = THIS_MODULE,
    .open    = lab2_open,
    .release = lab2_release,
    .read    = lab2_read,
    .write   = lab2_write,
    .llseek  = lab2_llseek,
};

static int __init lab2_init(void) {
    int ret;
    pr_info("lab2_driver: initializing module\n");
    ret = register_chrdev(major_number, DEVICE_NAME, &lab2_fops);
    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n", ret);
        return ret;
    }
    lab2_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(lab2_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_class);
    }
    lab2_device = device_create(lab2_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(lab2_device)) {
        class_destroy(lab2_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(lab2_device);
    }

    /* Dang ky sysfs attribute group */
    ret = sysfs_create_group(&lab2_device->kobj, &lab2_attr_group);
    if (ret)
        pr_warn("lab2_driver: sysfs_create_group failed\n");

    /* Tao /proc/lab2_info */
    proc_entry = proc_create("lab2_info", 0444, NULL, &lab2_proc_ops);
    if (!proc_entry)
        pr_warn("lab2_driver: failed to create /proc/lab2_info\n");
    else
        pr_info("lab2_driver: /proc/lab2_info created\n");

    pr_info("lab2_driver: loaded, major=%d\n", major_number);
    return 0;
}

static void __exit lab2_exit(void) {
    sysfs_remove_group(&lab2_device->kobj, &lab2_attr_group);
    if (proc_entry)
        proc_remove(proc_entry);
    device_destroy(lab2_class, MKDEV(major_number, 0));
    class_destroy(lab2_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    pr_info("lab2_driver: module unloaded\n");
}

module_init(lab2_init);
module_exit(lab2_exit);
