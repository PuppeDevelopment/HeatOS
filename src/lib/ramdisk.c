#include "ramdisk.h"
#include "string.h"

static void ramdisk_seed_demo_files(void);

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
    (void)fs_mkdir_child(0, "java");
    (void)fs_mkdir_child(0, "plasma");
    (void)fs_mkdir_child(0, "system");

    fs_node_t plasma_root = fs_resolve("/plasma");
    if (plasma_root != 0) {
        (void)fs_mkdir_child(plasma_root, "apps");
        (void)fs_mkdir_child(plasma_root, "config");
        (void)fs_mkdir_child(plasma_root, "session");
    }

    ramdisk_seed_demo_files();
}

static void ramdisk_write_demo_file(fs_node_t parent, const char *name, const void *data, int size) {
    fs_node_t node;

    if (!fs_create_file(parent, name)) {
        return;
    }

    node = fs_resolve(name);
    if (parent != 0) {
        char path[RAMDISK_NAME_LEN * 2];
        path[0] = '/';
        path[1] = '\0';
        fs_build_path(parent, path, sizeof(path));
        if (strlen(path) + 1 + strlen(name) + 1 < sizeof(path)) {
            if (strcmp(path, "/") != 0) strcat(path, "/");
            strcat(path, name);
            node = fs_resolve(path);
        }
    }

    if (node != 0) {
        (void)fs_write(node, data, size);
    }
}

static void ramdisk_seed_ppm_demo(fs_node_t parent, const char *name) {
    enum { DEMO_W = 32, DEMO_H = 24, HEADER_LEN = 13, FILE_LEN = HEADER_LEN + DEMO_W * DEMO_H * 3 };
    uint8_t ppm[FILE_LEN];
    int pos = 0;

    ppm[pos++] = 'P'; ppm[pos++] = '6'; ppm[pos++] = '\n';
    ppm[pos++] = '3'; ppm[pos++] = '2'; ppm[pos++] = ' '; ppm[pos++] = '2'; ppm[pos++] = '4'; ppm[pos++] = '\n';
    ppm[pos++] = '2'; ppm[pos++] = '5'; ppm[pos++] = '5'; ppm[pos++] = '\n';

    for (int y = 0; y < DEMO_H; y++) {
        for (int x = 0; x < DEMO_W; x++) {
            uint8_t r = (uint8_t)(40 + x * 5);
            uint8_t g = (uint8_t)(110 + y * 4);
            uint8_t b = (uint8_t)(160 + ((x + y) * 2));

            if (x > 9 && x < 22 && y > 7 && y < 18) {
                r = 245;
                g = 244;
                b = 232;
            }

            ppm[pos++] = r;
            ppm[pos++] = g;
            ppm[pos++] = b;
        }
    }

    ramdisk_write_demo_file(parent, name, ppm, pos);
}

static void ramdisk_seed_java_demo(fs_node_t parent, const char *name) {
    static const uint8_t hjar[] = {
        'H','J','A','R',1,
        0xFC, 25,
        'H','e','a','t','O','S',' ','J','A','R',' ','d','e','m','o',' ','s','t','a','r','t','e','d','\n',
        0xFC, 18,
        'R','u','n','n','i','n','g',' ','f','r','o','m',' ','/','j','a','v','a',
        0xFC, 1, '\n',
        0x10, 7,
        0x10, 5,
        0x60,
        0xFC, 8,
        '7',' ','+',' ','5',' ','=',' ',
        0xFE,
        0xB1
    };

    ramdisk_write_demo_file(parent, name, hjar, (int)sizeof(hjar));
}

static void ramdisk_seed_text_demo(fs_node_t parent, const char *name) {
    static const char text[] =
        "HeatOS demo files\n"
        "- sample.ppm : image preview test\n"
        "- hello.jar  : HeatOS mini-Java archive\n";

    ramdisk_write_demo_file(parent, name, text, (int)sizeof(text) - 1);
}

static void ramdisk_seed_demo_files(void) {
    fs_node_t home = fs_resolve("/home");
    fs_node_t java = fs_resolve("/java");

    if (home != 0) {
        ramdisk_seed_ppm_demo(home, "sample.ppm");
        ramdisk_seed_text_demo(home, "readme.txt");
    }

    if (java != 0) {
        ramdisk_seed_java_demo(java, "hello.jar");
    }
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

const char *fs_get_name(fs_node_t node) {
    if (node >= RAMDISK_MAX_NODES) return "";
    return fs_node_name[node];
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

bool fs_create_file(fs_node_t parent, const char *name) {
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

    fs_node_type[n] = FS_TYPE_FILE;
    fs_node_parent[n] = parent;
    fs_node_first_child[n] = 0;
    fs_node_next_sibling[n] = fs_node_first_child[parent];
    fs_node_first_child[parent] = n;
    fs_node_size[n] = 0;
    fs_node_data_off[n] = 0;
    fs_copy_name12_lower(fs_node_name[n], token);

    return true;
}

bool fs_delete(fs_node_t node) {
    if (node == 0 || node >= RAMDISK_MAX_NODES || fs_node_type[node] == FS_TYPE_FREE) return false;
    
    // Disallow deleting non-empty directories for simplicity
    if (fs_node_type[node] == FS_TYPE_DIR && fs_node_first_child[node] != 0) return false;

    fs_node_t parent = fs_node_parent[node];
    if (parent != 0) {
        fs_node_t cur = fs_node_first_child[parent];
        if (cur == node) {
            fs_node_first_child[parent] = fs_node_next_sibling[node];
        } else {
            while (cur != 0 && fs_node_next_sibling[cur] != node) {
                cur = fs_node_next_sibling[cur];
            }
            if (cur != 0) {
                fs_node_next_sibling[cur] = fs_node_next_sibling[node];
            }
        }
    }
    
    fs_node_type[node] = FS_TYPE_FREE;
    return true;
}

int fs_read(fs_node_t node, void *buf, int size) {
    if (node >= RAMDISK_MAX_NODES || !fs_is_file(node) || !buf)
        return -1;

    int actual = (int)fs_node_size[node];
    if (size < actual) actual = size;

    if (actual > 0) {
        memcpy(buf, &fs_data_pool[fs_node_data_off[node]], (size_t)actual);
    }
    return actual;
}

bool fs_write(fs_node_t node, const void *buf, int size) {
    if (node >= RAMDISK_MAX_NODES || !fs_is_file(node) || size < 0)
        return false;
    if (size > 0 && !buf) return false;

    /* Check if we have enough space in the pool */
    if (fs_data_free + size > RAMDISK_DATA_CAP)
        return false;

    /* For simplicity, we always allocate new space for write */
    fs_node_data_off[node] = fs_data_free;
    fs_node_size[node]     = (uint16_t)size;

    if (size > 0) {
        memcpy(&fs_data_pool[fs_data_free], buf, (size_t)size);
        fs_data_free += (uint16_t)size;
    }

    return true;
}

