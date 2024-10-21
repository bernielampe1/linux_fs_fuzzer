#include<linux/loop.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/mount.h>
#include<unistd.h>

#include"common.h"

/* This function mounts a fuzzed filesystem image via the loopback
 * interface (/dev/loop?). */

// Specify the filesystem type and mount point on target
#define FSTYPE "vfat"
#define MOUNTPOINT "/mnt"

void mount_fuzzed(char *buffer, int buffer_len)
{
    char *blockdev = "/dev/loop?";
    char *image = "/tmp/image.fs";
    struct loop_info64 li;
    int imagefd = -1; 
    int loopfd = -1; 
    int MAX_LOOP = 8;
    int i = 0;
    char *str = NULL;
    char *outbuffer = NULL;
    int outbuf_len = 0;

    // try all 8 loopback devices until we find one
    str = (char*)calloc(1, 32);
    if (!str) goto out1;

    memcpy(str, blockdev, strlen(blockdev));
    for(i = 0; i < MAX_LOOP; i++) {
        str[9] = (char)i + '0';
        if ((loopfd = open(str, O_RDWR | O_CLOEXEC, 0)) < 0) {
            fprintf(stderr, "failed to open %s\n", str);
            goto out1;
        }

        // is this loop device attached?
        if (ioctl(loopfd, LOOP_GET_STATUS64, &li) < 0) {
            printf("found loopdev %s\n", str);
            break;
        }

        if (close(loopfd) < 0) {
            fprintf(stderr, "close failed\n");
            goto out1;
        }
    }   

    if (i == MAX_LOOP) {
        fprintf(stderr, "failed to find free loop device\n");
        goto out1;
    }

    printf("found loop device loopfd = %d\n", loopfd);

    // uncompress
    outbuf_len = tagged_unrle(buffer, buffer_len, &outbuffer);

    // open image
    if ((imagefd = open(image, O_RDWR | O_CREAT | O_CLOEXEC, 0)) < 0) {
        fprintf(stderr, "failed to open image file %s\n", image);
        goto out0;
    }

    // write the image data
    if (write(imagefd, outbuffer, outbuf_len) < 0) {
        fprintf(stderr, "write of image file failed\n");
        goto out0;
    }

    // seek to begin
    if (lseek(imagefd, 0, SEEK_SET) < 0) {
        fprintf(stderr, "seek of image file failed\n");
        goto out0;
    }

    printf("wrote image to %s\n", image);

    // configure loopback
    if (ioctl(loopfd, LOOP_SET_FD, imagefd) < 0) {
        fprintf(stderr, "ioctl LOOP_SET_FD failed\n");
        goto out0;
    }

    // set loopback status
    memset(&li, 0, sizeof(li));
    strcpy((char*)li.lo_file_name, image);
    if (ioctl(loopfd, LOOP_SET_STATUS64, &li) < 0) {
        fprintf(stderr, "ioctl LOOP_SET_STATUS64 failed\n");
        goto out0;
    }

    // mount loopback
    if (mount(str, MOUNTPOINT, FSTYPE, 0, 0) < 0) {
        fprintf(stderr, "mount over %s failed\n", MOUNTPOINT);
    }

    // unmount image
    if (umount2(MOUNTPOINT, 0) < 0) {
        fprintf(stderr, "call to unmount2 failed\n");
    }

out0:
    // delete image
    if (unlink(image) < 0) {
        fprintf(stderr, "failed to unlink image file %s\n", image);
    }

    // detach the loopback
    if (ioctl(loopfd, LOOP_CLR_FD) < 0) {
        fprintf(stderr, "ioctl LOOP_CLR_FD failed\n");
    }

out1:
    if (str) free(str);
    if (outbuffer) free(outbuffer);

    if (imagefd != -1) close(imagefd);
    if (loopfd != -1 ) close(loopfd);
}

