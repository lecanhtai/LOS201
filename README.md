# LOS201 - Embedded Linux Systems

Repository chứa các bài lab và assignment môn Embedded Linux Systems.

- **Sinh viên:** Lê Cảnh Tài
- **MSSV:** SE200651

## Môi trường

- Host: Ubuntu Linux
- Target: ARMv7-A Cortex-A9
- Emulator: QEMU Versatile Express (vexpress-a9)
- Kernel: Linux 5.15
- Toolchain: arm-linux-gnueabihf

## Nội dung

- LAB-01: Cấu hình Kernel và khởi động hệ thống nhúng (ARMv7)
- LAB-02: Viết device driver và quản lý file system
- LAB-03: Debug kernel và phân tích hiệu năng ứng dụng nhúng
- ASSIGNMENT-01: How to Write a Device Driver
- ASSIGNMENT-02: Sensor Monitoring System

---

## LAB-01 - Cấu hình Kernel và khởi động hệ thống nhúng

**Phạm vi bài làm:**

- Cấu hình và cross-compile Linux kernel cho ARMv7.
- Tìm hiểu U-Boot, Device Tree Blob và quá trình boot.
- Xây dựng BusyBox root filesystem, initramfs và boot trên QEMU.

**Các file trong repository:**

- `lab01/configs/busybox.config`
- `lab01/configs/kernel.config`

Xem thư mục: [lab01/](lab01/)

---

## LAB-02 - Device driver và quản lý file system

**Phạm vi bài làm:**

- Viết character device driver, giao tiếp qua /dev/lab2.
- Xuất thông tin qua procfs và sysfs.
- Thực hành MTD/JFFS2 và BusyBox init.

**Các file trong repository:**


Xem thư mục: [lab02/](lab02/)

---

## LAB-03 - Debug và phân tích hiệu năng

**Phạm vi bài làm:**

- Debug bằng GDB (gdbserver), phân tích memory leak bằng Valgrind.
- Theo dõi system call bằng strace, phân tích CPU bằng perf.
- Đo scheduling latency bằng cyclictest.

**Các file trong repository:**

- `lab03/apps/buggy_app.c`
- `lab03/apps/latency_histogram.png`
- `lab03/apps/leaky_app.c`
- `lab03/apps/leaky_app_fixed.c`
- `lab03/apps/perf_target.c`
- `lab03/logs/gdb_session.txt`
- `lab03/logs/hist_standard.txt`
- `lab03/logs/latency_histogram.png`
- `lab03/logs/perf_report.txt`
- `lab03/logs/perf_stat.txt`
- `lab03/logs/plot_latency.gnuplot`
- `lab03/logs/strace_full.txt`
- `lab03/logs/strace_summary.txt`
- `lab03/logs/valgrind_fixed.txt`
- `lab03/logs/valgrind_report.txt`

Xem thư mục: [lab03/](lab03/)

---

## ASSIGNMENT-01 - How to Write a Device Driver

**Phạm vi bài làm:**

- Driver asgn1_driver với giao diện ioctl.
- Misc device driver counter_driver.

**Các file trong repository:**

- `asgn01/logs/dmesg_full.txt`
- `asgn01/logs/test_asgn1_output.txt`
- `asgn01/logs/test_counter_output.txt`
- `asgn01/task1_ioctl/asgn1_driver.c`
- `asgn01/task1_ioctl/asgn1_ioctl.h`
- `asgn01/task1_ioctl/Makefile`
- `asgn01/task1_ioctl/test_asgn1.sh`
- `asgn01/task1_ioctl/test_ioctl.c`
- `asgn01/task2_misc/counter_driver.c`
- `asgn01/task2_misc/Makefile`

Xem thư mục: [asgn01/](asgn01/)

---

## ASSIGNMENT-02 - Sensor Monitoring System

**Phạm vi bài làm:**

- Kernel driver sms_sensor_driver (/dev/sms_sensor, /proc/sms_stats).
- Ứng dụng đa luồng: Sensor, Processor, Alert, Logger.

**Các file trong repository:**

- `asgn02/app/circular_buffer.c`
- `asgn02/app/circular_buffer.h`
- `asgn02/app/Makefile`
- `asgn02/app/sms_app.c`
- `asgn02/driver/Makefile`
- `asgn02/driver/sms_sensor_driver.c`
- `asgn02/driver/sms_sensor.h`
- `asgn02/logs/jitter_report.txt`
- `asgn02/logs/perf_sms.txt`
- `asgn02/logs/sms_sample_run.txt`
- `asgn02/logs/strace_sms.txt`
- `asgn02/logs/valgrind_sms.txt`

Xem thư mục: [asgn02/](asgn02/)

---
