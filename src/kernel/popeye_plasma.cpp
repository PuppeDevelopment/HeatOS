#include "popeye_plasma.h"

bool popeye_plasma_desktop_run(bool java_enabled);

namespace {
static bool g_plasma_ready = false;
static bool g_boot_desktop = false;
static bool g_java_support_enabled = false;
}

extern "C" void popeye_plasma_init(void) {
    /* Terminal-first: desktop launched via 'popeye boot plasma' command. */
    g_boot_desktop = false;
    g_java_support_enabled = true;
    g_plasma_ready = true;
}

extern "C" bool popeye_plasma_is_ready(void) {
    return g_plasma_ready;
}

extern "C" const char *popeye_plasma_name(void) {
    return "Popeye Plasma";
}

extern "C" bool popeye_plasma_should_boot_desktop(void) {
    return g_boot_desktop;
}

extern "C" void popeye_plasma_set_boot_desktop(bool enabled) {
    g_boot_desktop = enabled;
}

extern "C" const char *popeye_plasma_boot_mode(void) {
    return g_boot_desktop ? "desktop-first" : "terminal-first";
}

extern "C" bool popeye_plasma_run(void) {
    if (!g_plasma_ready) popeye_plasma_init();
    return popeye_plasma_desktop_run(g_java_support_enabled);
}

extern "C" void popeye_plasma_force_terminal_first(void) {
    g_boot_desktop = false;
}

extern "C" bool popeye_plasma_try_enable_desktop_boot(void) {
    g_boot_desktop = true;
    return true;
}

extern "C" bool popeye_plasma_desktop_boot_allowed(void) {
    return true;
}

extern "C" bool popeye_plasma_enable_java_support(void) {
    g_java_support_enabled = true;
    return true;
}

extern "C" bool popeye_plasma_java_support_enabled(void) {
    return g_java_support_enabled;
}

extern "C" const char *popeye_plasma_java_support_mode(void) {
    return g_java_support_enabled ? "mini-vm" : "disabled";
}
