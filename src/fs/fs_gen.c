#include<dirent.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>

#include"common.h"

#define MAXCMDSIZE 255
#define VFATBIN "/usr/bin/mkfs.vfat"

/*
 * This program will programmatically generate compressed file system
 * imges to fuzz. If the size of the resulting compressed images is too
 * large, then it will be deleted.
 */

void addRandomFiles(char **corpus)
{
    // mount images
    // add random files
    return;
}

void compressfiles(char **corpus)
{
    // read file data
    // call tagged_rle
    // write out compressed buffer over file
    return;
}

void dellargefiles(char **corpus, int maxsize)
{
    // stat files
    // delete large files
    return;
}

void deduplicatefiles(char **corpus)
{
    // compute md5's
    // delete duplicates
    return;
}

int find_files(char *prefix, char ***corpus)
{


    return NULL;
}


void postprocess_corpus(char *prefix, int maxsize)
{
    char **corpus;
    int numfiles;

    // find all the files with prefix
    numfiles = find_files(prefix, &corpus);

    // process list
    deduplicatefiles(corpus);
    addRandomFiles(corpus);
    compressfiles(corpus);
    dellargefiles(corpus, maxsize);

    // clean up corpus list
}

void runcmd(char *cmd)
{
    printf("system(\"%s\")\n", cmd);
    system(cmd);
}

// Relies on mkfs.vfat binary to create compressed vfat images.
// This generates piecewise images addressing one parameter at a time.
// Potential progress could be made if generation combines parameters.
int gen_vfat(char *prefix, int maxsize)
{
    char cmdbuf[MAXCMDSIZE];
    int index = 0;

    // put -C in all to create file without dd
    int blockcounts[] = {2, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    for(int bc = 0; bc < ARRAYSIZE(blockcounts); bc++) {

        // -f number of fats
        int numfats[] = {2, 3, 4};
        for(int i = 0; i < ARRAYSIZE(numfats); i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -f %d %s_%d.vfat %d",
                     VFATBIN, numfats[i], prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }

        // -F fat size <12, 16, 32>
        int fatsizes[] = {12, 16, 32};
        for(int i = 0; i < ARRAYSIZE(fatsizes); i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -F %d %s_%d.vfat %d",
                     VFATBIN, fatsizes[i], prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }

        // -h <2, 4, 6, 8, ... 128>
        int hiddensecs_bits = 7;
        for(int i = 0; i < hiddensecs_bits; i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -h %d %s_%d.vfat %d",
                     VFATBIN, (1 << i), prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }

        // -s sectors-per-cluster <1, 2, 4, ... 128>
        int secspercluster_bits = 7;
        for(int i = 0; i < secspercluster_bits; i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -s %d %s_%d.vfat %d",
                     VFATBIN, (1 << i), prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }

        // -R number-of-reserved-sectors <1, 2, 4, 8, 16, 32, 64, 128, 256>
        int numreservedsecs_bits = 8;
        for(int i = 0; i < numreservedsecs_bits; i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -R %d %s_%d.vfat %d",
                     VFATBIN, (1 << i), prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }

        // -S logical-sectors-sizes <512, 1024, 2048, 4096, 8192, 16384, 32768>
        int logsecsizes[] = {512, 1024, 2048, 4096, 8192, 32768};
        for(int i = 0; i < ARRAYSIZE(logsecsizes); i++) {
            snprintf(cmdbuf, MAXCMDSIZE, "%s -C -S %d %s_%d.vfat %d",
                     VFATBIN, logsecsizes[i], prefix, index++, blockcounts[bc]);
            runcmd(cmdbuf);
        }
    }

    // compress, de-duplicate, add files, etc
    postprocess_corpus(prefix, maxsize);

    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <prefix> <maxsize>", argv[0]);
        return -1;
    }

    char *prefix = argv[1];
    int maxsize = atoi(argv[2]);

    if (gen_vfat(prefix, maxsize) < 0) {
        fprintf(stderr, "makeing vfat images failed");
        return -1;
    }

#if 0
    gen_ext4(prefix);
    gen_sdcardfs(prefix);
    gen_ecryptfs(prefix);
    gen_tmpfs(prefix);
    gen_f2fs(prefix);
#endif

    return 0;
}

