#ifndef KLOG_H
#define KLOG_H

#include "types.h"

typedef enum {
    KLOG_EMERG = 0,
    KLOG_ALERT = 1,
    KLOG_CRIT  = 2,
    KLOG_ERR   = 3,
    KLOG_WARN  = 4,
    KLOG_NOTICE = 5,
    KLOG_INFO  = 6,
    KLOG_DEBUG = 7
} klog_level_t;

void klog_init(void);
void klog_clear(void);
void klog_log(klog_level_t level, const char *subsys, const char *msg);
void klog_log_u32(klog_level_t level, const char *subsys, const char *prefix, uint32_t value, int base);
int  klog_read(char *out, int max_len);
uint32_t klog_bytes_used(void);
uint32_t klog_line_count(void);

#define klog_info(subsys, msg)  klog_log(KLOG_INFO, (subsys), (msg))
#define klog_warn(subsys, msg)  klog_log(KLOG_WARN, (subsys), (msg))
#define klog_error(subsys, msg) klog_log(KLOG_ERR, (subsys), (msg))

#endif