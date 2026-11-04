#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "asgn1_ioctl.h"

int main() {
    int fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    // Test GET_VERSION
    char version[64];
    if (ioctl(fd, ASGN1_GET_VERSION, version) == 0) {
        printf("Driver Version: %s\n", version);
    }

    // Test SET_MODE (Mode 0: Kernel mode)
    int mode = 0;
    ioctl(fd, ASGN1_SET_MODE, &mode);
    write(fd, "Test Kernel Mode", 16);

    char buf[128] = {0};
    read(fd, buf, sizeof(buf));
    printf("Read back (Kernel Mode): %s\n", buf);

    // Test GET_STATS
    struct asgn1_stats stats;
    ioctl(fd, ASGN1_GET_STATS, &stats);
    printf("Stats -> Opens: %d, Writes: %d, Reads: %d, Len: %d\n", 
           stats.open_count, stats.write_count, stats.read_count, stats.buffer_len);

    // Test RESET_BUFFER
    ioctl(fd, ASGN1_RESET_BUFFER);
    printf("Buffer reset executed.\n");

    close(fd);
    return 0;
}
