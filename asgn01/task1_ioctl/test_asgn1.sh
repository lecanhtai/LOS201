#!/bin/sh
echo "=== Starting Assignment 01 Task 1 Test ==="
insmod asgn1_driver.ko
echo "Module inserted."

# Ghi dữ liệu kiểm tra Echo Mode
echo "Echo Mode Test:" > /dev/asgn1
cat /dev/asgn1
echo ""

# Unload module test sạch sẽ
rmmod asgn1_driver
echo "=== Task 1 Test Completed ==="
