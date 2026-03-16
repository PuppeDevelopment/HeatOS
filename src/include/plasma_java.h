#ifndef PLASMA_JAVA_H
#define PLASMA_JAVA_H

#include "types.h"
#include "ramdisk.h"

#define JAVA_OUTPUT_MAX 1024

typedef struct {
    char output[JAVA_OUTPUT_MAX];
    int  output_len;
    bool success;
    char error[64];
} java_result_t;

#ifdef __cplusplus
extern "C" {
#endif

void        java_vm_init(void);
bool        java_vm_run(const char *demo_name, java_result_t *result);
bool        java_vm_run_file(fs_node_t node, java_result_t *result);
bool        java_vm_run_path(const char *path, java_result_t *result);
int         java_vm_demo_count(void);
const char *java_vm_demo_name(int index);

#ifdef __cplusplus
}
#endif

#endif
