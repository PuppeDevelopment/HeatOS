#include "commands.h"
#include "string.h"
#include "terminal.h"

static const cmd_entry_t g_commands[] = {
    {"cd", "change working directory", cmd_cd},
    {"pwd", "print working directory", cmd_pwd},
    {"tree", "print directory tree", cmd_tree},
    {"cp", "copy file", cmd_cp},
    {"mv", "move or rename file", cmd_mv},
    {"rmdir", "remove empty directory", cmd_rmdir},
    {"find", "search files by name", cmd_find},
    {"grep", "search file content", cmd_grep},
    {"echo", "print text", cmd_echo},
    {"cat", "display file", cmd_cat},
    {"uname", "print kernel info", cmd_uname},
    {"whoami", "print active user", cmd_whoami},
    {"date", "print RTC date and time", cmd_date},
    {"reboot", "restart the system", cmd_reboot},
    {"shutdown", "halt the system", cmd_shutdown},
    {"ifconfig", "show network interfaces", cmd_ifconfig},
    {"netstat", "show network sockets", cmd_netstat},
    {"nslookup", "resolve DNS names", cmd_nslookup},
    {"traceroute", "trace route hops", cmd_traceroute},
    {"curl", "download URL", cmd_curl},
    {"telnet", "open TCP connection", cmd_telnet},
    {"ps", "list running tasks", cmd_ps},
    {"kill", "terminate task", cmd_kill},
    {"top", "live task monitor", cmd_top},
    {"dmesg", "show kernel ring log", cmd_dmesg},
    {"kstats", "show kernel subsystem stats", cmd_kstats},
    {"catnake", "snake game", cmd_catnake},
    {"pong", "launch Pong game", cmd_pong},
    {"ls", "list files", cmd_ls},
    {"mkdir", "make directory", cmd_mkdir},
    {"rm", "remove file/dir", cmd_rm},
    {"touch", "create file", cmd_touch},
    {"ping", "ping IP address", cmd_ping},
    {"wget", "download file", cmd_wget},
    {"mamu", "text editor", cmd_mamu},
    {"desktop", "launch desktop environment", cmd_desktop},
    {"kat", "display file", cmd_kat},
};

bool cmd_dispatch(const char *cmd, const char *args) {
    if (!cmd || !*cmd) return false;

    for (size_t i = 0; i < (sizeof(g_commands) / sizeof(g_commands[0])); i++) {
        if (strcmp(cmd, g_commands[i].name) == 0) {
            if (g_commands[i].handler) {
                g_commands[i].handler(args);
            }
            return true;
        }
    }

    return false;
}
