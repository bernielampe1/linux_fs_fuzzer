#ifndef __COMMON_H__
#define __COMMON_H__

#include<stdint.h>

#include "logging.h"

/* FUZZER FUNCTION: this function is where the fuzzed buffer
 * is used (sent to kernel). You can override this function to fuzz
 * arbitrary binary blobs. */
void mount_fuzzed(char *buffer, int buffer_len); // mount a fuzzed fs image

// FUZZER HOOK: Preprocessor macro hook for fuzz_main.c
#define FUZZ_FUNCTION mount_fuzzed

/* kcov.c */
struct kcov;
struct kcov *kcov_new(void);
void kcov_enable(struct kcov *kcov);
int kcov_disable(struct kcov *kcov);
void kcov_free(struct kcov *kcov);
uint64_t *kcov_cover(struct kcov *kcov);

/* forksrv.c */
struct forksrv;
struct forksrv *forksrv_new(void);
int forksrv_on(struct forksrv *forksrv);
void forksrv_welcome(struct forksrv *forksrv);
int32_t forksrv_cycle(struct forksrv *forksrv, uint32_t child_pid);
void forksrv_status(struct forksrv *forksrv, uint32_t status);
uint8_t *forksrv_area_ptr(struct forksrv *forksrv);
void forksrv_free(struct forksrv *forksrv);

/* utils.c */
struct option;
const char *optstring_from_long_options(const struct option *opt);
int taskset(int taskset_cpu);

/* run length encoding robust to fuzzing to reduce input file sizes */
int tagged_rle(char *inbuf, int inbuflen, char **outbuf);
int tagged_unrle(char *inbuf, int inbuflen, char **outbuf);

/* siphash.c used for converting RIP's from kcov */
uint32_t hsiphash_static(const void *src, unsigned long src_sz);

/* network namespace */
int netns_save();
void netns_new();
void netns_restore(int net_ns);

/* mount namespace */
int mountns_save();
void mountns_new();
void mountns_restore(int mount_ns);

/* pid namespace */
int pidns_save();
void pidns_new();
void pidns_restore(int pid_ns);

/* ipc namespace */
int ipcns_save();
void ipcns_new();
void ipcns_restore(int ipc_ns);

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

#endif // __COMMON_H__
