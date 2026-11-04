#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SinhVien - MSSV");
MODULE_DESCRIPTION("Assignment 01: Misc Device Counter Driver");
MODULE_VERSION("1.0");

#define DEVICE_NAME "counter"

static atomic_t counter_val = ATOMIC_INIT(0);
static atomic_t total_reads = ATOMIC_INIT(0);
static atomic_t total_resets = ATOMIC_INIT(0);
static struct proc_dir_entry *proc_counter_entry = NULL;

static ssize_t counter_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char buffer[32];
    int val = atomic_read(&counter_val);
    int len_str;

    if (*offset > 0)
        return 0;

    len_str = snprintf(buffer, sizeof(buffer), "%d\n", val);
    if (copy_to_user(buf, buffer, len_str))
        return -EFAULT;

    *offset += len_str;
    atomic_inc(&counter_val);
    atomic_inc(&total_reads);
    return len_str;
}

static ssize_t counter_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    char kbuf[32];
    int new_val;
    int ret;

    if (len > sizeof(kbuf) - 1)
        return -EINVAL;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;
    kbuf[len] = '\0';

    ret = kstrtoint(strim(kbuf), 10, &new_val);
    if (ret < 0 || new_val < 0)
        return -EINVAL;

    atomic_set(&counter_val, new_val);
    atomic_inc(&total_resets);
    pr_info("counter_driver: counter set to %d\n", new_val);
    return len;
}

static int counter_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Counter Driver Statistics\n");
    seq_printf(m, "=========================\n");
    seq_printf(m, "Current counter value : %d\n", atomic_read(&counter_val));
    seq_printf(m, "Total reads (increments): %d\n", atomic_read(&total_reads));
    seq_printf(m, "Total writes (resets) : %d\n", atomic_read(&total_resets));
    return 0;
}

static int counter_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, counter_proc_show, NULL);
}

static const struct proc_ops counter_proc_ops = {
    .proc_open    = counter_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static const struct file_operations counter_fops = {
    .owner   = THIS_MODULE,
    .read    = counter_read,
    .write   = counter_write,
};

static struct miscdevice counter_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &counter_fops,
};

static int __init counter_init(void) {
    int ret = misc_register(&counter_misc_device);
    if (ret) return ret;

    proc_counter_entry = proc_create("counter_info", 0444, NULL, &counter_proc_ops);
    pr_info("counter_driver loaded under major 10\n");
    return 0;
}

static void __exit counter_exit(void) {
    if (proc_counter_entry)
        proc_remove(proc_counter_entry);
    misc_deregister(&counter_misc_device);
    pr_info("counter_driver unloaded\n");
}

module_init(counter_init);
module_exit(counter_exit);
