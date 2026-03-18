#include "terminal.h"
#include "ramdisk.h"
#include "string.h"

static const char *skip_spaces(const char *s) {
    while (s && *s == ' ') s++;
    return s;
}

static void trim_right_spaces(char *s) {
    size_t n;
    if (!s) return;
    n = strlen(s);
    while (n > 0 && s[n - 1] == ' ') {
        s[n - 1] = '\0';
        n--;
    }
}

static bool split_parent_and_name(const char *path, char *parent, size_t psz,
                                  char *name, size_t nsz) {
    char tmp[256];
    size_t n = 0;
    size_t split = 0;

    path = skip_spaces(path);
    if (!path || !*path) return false;

    while (*path && n + 1 < sizeof(tmp)) {
        tmp[n++] = *path++;
    }
    tmp[n] = '\0';
    trim_right_spaces(tmp);
    n = strlen(tmp);
    if (n == 0) return false;

    for (size_t i = 0; i < n; i++) {
        if (tmp[i] == '/') split = i + 1;
    }

    if (split == 0) {
        if (psz < 2) return false;
        parent[0] = '.';
        parent[1] = '\0';
        strncpy(name, tmp, nsz - 1);
        name[nsz - 1] = '\0';
        return name[0] != '\0';
    }

    if (split == n) return false;

    if (split == 1) {
        if (psz < 2) return false;
        parent[0] = '/';
        parent[1] = '\0';
    } else {
        size_t plen = split - 1;
        if (plen + 1 > psz) return false;
        memcpy(parent, tmp, plen);
        parent[plen] = '\0';
    }

    strncpy(name, &tmp[split], nsz - 1);
    name[nsz - 1] = '\0';
    return name[0] != '\0';
}

static bool resolve_or_create_file(const char *path, fs_node_t *out_file) {
    fs_node_t node;

    if (!out_file || !path || !*path) return false;

    if (fs_resolve_checked(path, &node)) {
        if (!fs_is_file(node)) return false;
        *out_file = node;
        return true;
    }

    char parent_path[256];
    char child_name[RAMDISK_NAME_LEN];
    fs_node_t parent;

    if (!split_parent_and_name(path, parent_path, sizeof(parent_path), child_name, sizeof(child_name))) {
        return false;
    }

    if (!fs_resolve_checked(parent_path, &parent) || !fs_is_dir(parent)) {
        return false;
    }

    if (!fs_create_file(parent, child_name)) {
        return false;
    }

    if (!fs_resolve_checked(path, &node) || !fs_is_file(node)) {
        return false;
    }

    *out_file = node;
    return true;
}

void cmd_echo(const char *args) {
    char work[256];
    char *redir;

    if (!args || !*args) {
        term_puts("\n");
        return;
    }

    strncpy(work, args, sizeof(work) - 1);
    work[sizeof(work) - 1] = '\0';

    redir = strchr(work, '>');
    if (!redir) {
        term_puts(work);
        term_puts("\n");
        return;
    }

    *redir = '\0';
    redir++;

    trim_right_spaces(work);
    redir = (char *)skip_spaces(redir);

    if (!*redir) {
        term_puts("echo: missing redirect target\n");
        return;
    }

    fs_node_t node;
    if (!resolve_or_create_file(redir, &node)) {
        term_puts("echo: invalid redirect target\n");
        return;
    }

    if (!fs_write(node, work, (int)strlen(work))) {
        term_puts("echo: write failed\n");
        return;
    }
}
