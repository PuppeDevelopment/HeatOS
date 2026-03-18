#include "terminal.h"
#include "mamu.h"
#include "catnake.h"
#include "kat.h"
#include "desktop.h"

void cmd_mamu(const char *args) {
    mamu_run(args ? args : "");
    term_reset_screen();
}

void cmd_catnake(const char *args) {
    (void)args;
    catnake_run();
    term_reset_screen();
}

void cmd_kat(const char *args) {
    kat_run(args);
}

void cmd_cat(const char *args) {
    kat_run(args);
}

void cmd_desktop(const char *args) {
    (void)args;
    desktop_run();
    term_reset_screen();
}
