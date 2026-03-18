#include "shell_stubs.h"
#include "terminal.h"
#include "string.h"

typedef struct {
    const char *name;
    const char *note;
} stub_command_t;

static const stub_command_t g_stub_commands[] = {
    {"cd", "change working directory"},
    {"pwd", "print working directory"},
    {"tree", "print directory tree"},
    {"cp", "copy file"},
    {"mv", "move or rename file"},
    {"rmdir", "remove empty directory"},
    {"find", "search files by name"},
    {"grep", "search file content"},
    {"echo", "print text"},
    {"uname", "print kernel info"},
    {"whoami", "print active user"},
    {"date", "print RTC date and time"},
    {"reboot", "restart the system"},
    {"shutdown", "halt the system"},
    {"ifconfig", "show network interfaces"},
    {"netstat", "show network sockets"},
    {"nslookup", "resolve DNS names"},
    {"traceroute", "trace route hops"},
    {"curl", "download URL"},
    {"telnet", "open TCP connection"},
    {"ps", "list running tasks"},
    {"kill", "terminate task"},
    {"top", "live task monitor"},
    {"pong", "launch Pong game"}
};

bool shell_try_stub_command(const char *cmd, const char *args) {
    (void)args;

    if (!cmd || !*cmd) {
        return false;
    }

    for (size_t i = 0; i < (sizeof(g_stub_commands) / sizeof(g_stub_commands[0])); i++) {
        if (strcmp(cmd, g_stub_commands[i].name) == 0) {
            term_puts("Command found: ");
            term_puts(cmd);
            term_puts("\n");
            term_puts("This command is a stub and does nothing yet.\n");
            term_puts("Planned behavior: ");
            term_puts(g_stub_commands[i].note);
            term_puts("\n");
            return true;
        }
    }

    return false;
}
