#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>

#include"common.h"

/* The tagged_rle() and tagged_unrle() functions will compress input files
 * in a way that is robust to fuzzing. The functions will remove all zero
 * bytes AFL works better on smaller files. */

#define RLE_TAG 0xEFAC

// these minimum and maximum custom run lens are customizable
#define MIN_RUNLEN 0x10
#define MAX_RUNLEN 0x400

typedef unsigned short u16;

int tagged_rle(char *inbuf, int inbuflen, char **outbuf)
{
    int run_len;
    char *start_ptr = NULL; // pointer to beginning of run
    char *run_ptr = NULL; // pointer used to find end of zero run
    char *outbuf_ptr = NULL; // position in output buffer

    char *inbuf_ptr = inbuf; // input buf pointer
    char *end_ptr = inbuf + inbuflen; // mark end of input buffer

    // allocate max space for output
    *outbuf = calloc(1, inbuflen);
    if (*outbuf == NULL) {
        LOGE("could not allocated %d bytes\n", inbuflen);
        return -1;
    }
    outbuf_ptr = *outbuf;

    // loop over all data and find runs
    while(inbuf_ptr < end_ptr) {

        // detect a run start
        if (*inbuf_ptr == 0) {
            // scan to end of run
            start_ptr = inbuf_ptr;
            run_ptr = inbuf_ptr;
            do {
                run_ptr++;
                run_len = run_ptr - start_ptr;
            } while(*run_ptr == 0 &&
                    run_len < MAX_RUNLEN &&
                    run_ptr < end_ptr);

            // if minimum run len found
            if (run_len >= MIN_RUNLEN) {
                // emit a run to outbuf and advance pointers
                (*(u16*)(outbuf_ptr)) = RLE_TAG;
                (*(u16*)(outbuf_ptr + sizeof(u16))) = run_len;
                outbuf_ptr += 2 * sizeof(u16);
                inbuf_ptr = run_ptr;
                continue;
            }
        }

        // just copy the data byte for byte
        *outbuf_ptr++ = *inbuf_ptr++;
    }

    return outbuf_ptr - *outbuf;
}

int tagged_unrle(char *inbuf, int inbuflen, char **outbuf)
{
    char *inbuf_ptr = inbuf;
    char *end_ptr = inbuf_ptr + inbuflen;
    char *outbuf_ptr = NULL;
    int len, outputlen = 0;

    // compute the output size by scanning the input
    while(inbuf_ptr < end_ptr) {
        if ((*(u16*)inbuf_ptr) == RLE_TAG) {
            len = *(u16*)(inbuf_ptr + sizeof(u16));
            if (len >= MIN_RUNLEN && len <= MAX_RUNLEN) {
                outputlen += len;
                inbuf_ptr += 2 * sizeof(u16);
                continue;
            }
        }

        outputlen++;
        inbuf_ptr++;
    }

    // allocate output buffer
    *outbuf = calloc(1, outputlen);
    if (!*outbuf) {
        LOGE("failed to calloc outbuf\n");
        return -1;
    }

    // expand the input to the full size
    inbuf_ptr = inbuf;
    outbuf_ptr = *outbuf;
    while(inbuf_ptr < end_ptr) {
        if ((*(u16*)inbuf_ptr) == RLE_TAG) {
            len = *(u16*)(inbuf_ptr + sizeof(u16));
            if (len >= MIN_RUNLEN && len <= MAX_RUNLEN) {
                for(int i = 0; i < len; i++) {
                    *outbuf_ptr++ = 0;
                }

                inbuf_ptr += 2 * sizeof(u16);
                continue;
            }
        }

        *outbuf_ptr++ = *inbuf_ptr++;
    }

    return outputlen;
}

