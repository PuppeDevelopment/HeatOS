#include "ramdisk.h"
#include "string.h"

static uint8_t  fs_cwd = 0;
static uint16_t fs_data_free = 0;

static uint8_t  fs_node_type[RAMDISK_MAX_NODES];
static uint8_t  fs_node_parent[RAMDISK_MAX_NODES];
static uint8_t  fs_node_first_child[RAMDISK_MAX_NODES];
static uint8_t  fs_node_next_sibling[RAMDISK_MAX_NODES];
static uint16_t fs_node_size[RAMDISK_MAX_NODES];
static uint16_t fs_node_data_off[RAMDISK_MAX_NODES];
static char     fs_node_name[RAMDISK_MAX_NODES][RAMDISK_NAME_LEN];

static uint8_t  fs_data_pool[RAMDISK_DATA_CAP];

static char to_lower_c(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c - 'A' + 'a');
    return c;
}

static void fs_copy_name12_lower(char *dst, const char *src) {
    size_t i = 0;
    for (; i < RAMDISK_NAME_LEN - 1 && src[i]; i++) {
        dst[i] = to_lower_c(src[i]);
    }
    for (; i < RAMDISK_NAME_LEN; i++) {
        dst[i] = '\0';
    }
}

static fs_node_t fs_alloc_node(void) {
    for (uint8_t i = 1; i < RAMDISK_MAX_NODES; i++) {
        if (fs_node_type[i] == FS_TYPE_FREE) {
            return i;
        }
    }
    return 0;
}

static fs_node_t fs_find_child(fs_node_t parent, const char *token) {
    uint8_t n = fs_node_first_child[parent];
    while (n) {
        if (fs_node_type[n] != FS_TYPE_FREE &&
            strncmp(fs_node_name[n], token, RAMDISK_NAME_LEN) == 0) {
            return n;
        }
        n = fs_node_next_sibling[n];
    }
    return 0;
}

static bool fs_next_token(const char **pp, char *buf) {
    const char *p = *pp;

    while (*p == ' ' || *p == '/') p++;
    if (*p == '\0') {
        buf[0] = '\0';
        *pp = p;
        return false;
    }

    size_t i = 0;
    while (*p && *p != ' ' && *p != '/' && i < RAMDISK_NAME_LEN - 1) {
        buf[i++] = to_lower_c(*p++);
    }
    buf[i] = '\0';

    while (*p && *p != ' ' && *p != '/') p++;

    *pp = p;
    return true;
}

void ramdisk_init(void) {
    /* clear node tables */
    memset(fs_node_type,        0, sizeof(fs_node_type));
    memset(fs_node_parent,      0, sizeof(fs_node_parent));
    memset(fs_node_first_child, 0, sizeof(fs_node_first_child));
    memset(fs_node_next_sibling,0, sizeof(fs_node_next_sibling));
    memset(fs_node_size,        0, sizeof(fs_node_size));
    memset(fs_node_data_off,    0, sizeof(fs_node_data_off));
    memset(fs_node_name,        0, sizeof(fs_node_name));
    memset(fs_data_pool,        0, sizeof(fs_data_pool));

    fs_data_free = 0;
    fs_cwd = 0;

    fs_node_type[0] = FS_TYPE_DIR;
    fs_node_parent[0] = 0;

    (void)fs_mkdir_child(0, "apps");
    (void)fs_mkdir_child(0, "docs");
    (void)fs_mkdir_child(0, "home");
    (void)fs_mkdir_child(0, "system");
}

fs_node_t fs_resolve(const char *path) {
    if (!path || !*path)
        return fs_cwd;

    fs_node_t cur;

    if (*path == '/') {
        cur = 0; 
        path++;
    } else {
        cur = fs_cwd; 
    }

    char token[RAMDISK_NAME_LEN];
    const char *p = path;

    while (fs_next_token(&p, token)) {
        if (token[0] == '\0')
            break;

        if (strcmp(token, ".") == 0) {
        } else if (strcmp(token, "..") == 0) {
            if (cur != 0)
                cur = fs_node_parent[cur];
        } else {
            fs_node_t child = fs_find_child(cur, token);
            if (!child)
                return 0;
            cur = child;
        }

        while (*p == '/') p++;
        if (*p == '\0')
            break;
    }

    return cur;
}

bool fs_is_dir(fs_node_t node) {
    if (node >= RAMDISK_MAX_NODES) return false;
    return fs_node_type[node] == FS_TYPE_DIR;
}

bool fs_is_file(fs_node_t node) {
    if (node >= RAMDISK_MAX_NODES) return false;
    return fs_node_type[node] == FS_TYPE_FILE;
}

fs_node_t fs_cwd_get(void) {
    return fs_cwd;
}

void fs_cwd_set(fs_node_t node) {
    if (node < RAMDISK_MAX_NODES && fs_is_dir(node))
        fs_cwd = node;
}

bool fs_mkdir_child(fs_node_t parent, const char *name) {
    if (parent >= RAMDISK_MAX_NODES || !fs_is_dir(parent) || !name)
        return false;

    char token[RAMDISK_NAME_LEN];
    fs_copy_name12_lower(token, name);
    if (token[0] == '\0')
        return false;

    if (fs_find_child(parent, token))
        return false;

    fs_node_t n = fs_alloc_node();
    if (!n)
        return false;

    fs_node_type[n] = FS_TYPE_DIR;
    fs_node_parent[n] = parent;
    fs_node_first_child[n] = 0;
    fs_node_next_sibling[n] = fs_node_first_child[parent];
    fs_node_first_child[parent] = n;
    fs_node_size[n] = 0;
    fs_node_data_off[n] = 0;
    fs_copy_name12_lower(fs_node_name[n], token);

    return true;
}

bool fs_list_begin(fs_node_t dir, fs_node_t *iter_out) {
    if (!iter_out || !fs_is_dir(dir))
        return false;
    *iter_out = fs_node_first_child[dir];
    return *iter_out != 0;
}

bool fs_list_next(fs_node_t *iter_in_out, fs_node_t *child_out) {
    if (!iter_in_out || !child_out || *iter_in_out == 0)
        return false;

    fs_node_t n = *iter_in_out;
    *child_out = n;
    *iter_in_out = fs_node_next_sibling[n];
    return true;
}

void fs_build_path(fs_node_t node, char *buf, size_t buf_size) {
    if (!buf || buf_size == 0) return;

    if (node >= RAMDISK_MAX_NODES) {
        buf[0] = '\0';
        return;
    }

    fs_node_t stack[RAMDISK_MAX_NODES];
    size_t depth = 0;
    fs_node_t cur = node;

    while (1) {
        stack[depth++] = cur;
        if (cur == 0) break;
        cur = fs_node_parent[cur];
        if (depth >= RAMDISK_MAX_NODES) break;
    }

    size_t pos = 0;
    buf[pos++] = '/';

    for (size_t i = depth; i-- > 0; ) {
        fs_node_t n = stack[i];
        if (n == 0) continue;

        const char *name = fs_node_name[n];
        for (size_t j = 0; j < RAMDISK_NAME_LEN && name[j]; j++) {
            if (pos + 1 >= buf_size) {
                buf[buf_size - 1] = '\0';
                return;
            }
            buf[pos++] = name[j];
        }

        if (i != 0) {
            if (pos + 1 >= buf_size) {
                buf[buf_size - 1] = '\0';
                return;
            }
            buf[pos++] = '/';
        }
    }

    if (pos >= buf_size)
        pos = buf_size - 1;
    buf[pos] = '\0';
}

