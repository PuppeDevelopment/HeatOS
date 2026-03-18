#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"

typedef void (*cmd_handler_t)(const char *args);

typedef struct {
    const char *name;
    const char *desc;
    cmd_handler_t handler;
} cmd_entry_t;

// Command handlers
void cmd_cd(const char *args);
void cmd_pwd(const char *args);
void cmd_tree(const char *args);
void cmd_cp(const char *args);
void cmd_mv(const char *args);
void cmd_rmdir(const char *args);
void cmd_find(const char *args);
void cmd_grep(const char *args);
void cmd_echo(const char *args);
void cmd_uname(const char *args);
void cmd_whoami(const char *args);
void cmd_date(const char *args);
void cmd_reboot(const char *args);
void cmd_shutdown(const char *args);
void cmd_ifconfig(const char *args);
void cmd_netstat(const char *args);
void cmd_nslookup(const char *args);
void cmd_traceroute(const char *args);
void cmd_curl(const char *args);
void cmd_telnet(const char *args);
void cmd_ps(const char *args);
void cmd_kill(const char *args);
void cmd_top(const char *args);
void cmd_pong(const char *args);
void cmd_ls(const char *args);
void cmd_mkdir(const char *args);
void cmd_rm(const char *args);
void cmd_touch(const char *args);
void cmd_ping(const char *args);
void cmd_wget(const char *args);
void cmd_mamu(const char *args);
void cmd_catnake(const char *args);
void cmd_kat(const char *args);

bool cmd_copy_file_path(const char *src_path, const char *dst_path, int *bytes_out);

bool cmd_dispatch(const char *cmd, const char *args);

#endif
