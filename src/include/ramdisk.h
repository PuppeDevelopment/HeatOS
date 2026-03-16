#ifndef RAMDISK_H
#define RAMDISK_H

#include "types.h"

#define RAMDISK_MAX_NODES   64
#define RAMDISK_NAME_LEN    12
#define RAMDISK_DATA_CAP    8192

typedef enum {
    FS_TYPE_FREE = 0,
    FS_TYPE_DIR  = 1,
    FS_TYPE_FILE = 2,
} fs_node_type_t;

typedef uint8_t fs_node_t;

void ramdisk_init(void);

fs_node_t fs_resolve(const char *path);

bool      fs_is_dir(fs_node_t node);
bool      fs_is_file(fs_node_t node);
fs_node_t fs_cwd_get(void);
void      fs_cwd_set(fs_node_t node);

bool      fs_mkdir_child(fs_node_t parent, const char *name);

bool        fs_list_begin(fs_node_t dir, fs_node_t *iter_out);
bool        fs_list_next(fs_node_t *iter_in_out, fs_node_t *child_out);

void        fs_build_path(fs_node_t node, char *buf, size_t buf_size);
const char *fs_get_name(fs_node_t node);

bool      fs_create_file(fs_node_t parent, const char *name);
int       fs_read(fs_node_t node, void *buf, int size);
bool      fs_write(fs_node_t node, const void *buf, int size);

#endif

