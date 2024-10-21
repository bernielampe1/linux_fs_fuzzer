#ifndef __LOGGGING_H__
#define __LOGGGING_H__

#include <assert.h>
#include <stdio.h>

#ifdef __ANDROID__
#include <android/log.h>
#undef LOG_TAG
#define LOG_TAG "FUZZER"
#else
#define __android_log_print(...)
#endif

/* clang-format off */

#ifdef DEBUG
#define LOGD(...)                                                   \
do                                                                  \
{                                                                   \
    __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);   \
    fprintf(stdout, "DEBUG: " __VA_ARGS__);                         \
    fprintf(stdout, "\n");                                          \
} while (0);

#define LOGI(...)                                                   \
do                                                                  \
{                                                                   \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);    \
    fprintf(stdout, "INFO: " __VA_ARGS__);                          \
    fprintf(stdout, "\n");                                          \
} while (0);

#define LOGW(...)                                                   \
do                                                                  \
{                                                                   \
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__);    \
    fprintf(stdout, "WARN: " __VA_ARGS__);                          \
    fprintf(stdout, "\n");                                          \
} while (0);

#define LOGE(...)                                                   \
do                                                                  \
{                                                                   \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);   \
    fprintf(stderr, "ERROR: " __VA_ARGS__);                         \
    fprintf(stdout, "\n");                                          \
} while (0);

#define TRACE                                                       \
do                                                                  \
{                                                                   \
    LOGD("%s:%i\n", __FILE__, __LINE__)                             \
} while (0);

#else
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#define TRACE
#undef assert
#define assert(...)    \
do                     \
{                      \
} while (0)
#endif

#endif // __LOGGGING_H__
