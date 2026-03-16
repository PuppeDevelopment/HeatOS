#ifndef POPEYE_PLASMA_H
#define POPEYE_PLASMA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void popeye_plasma_init(void);
bool popeye_plasma_is_ready(void);
const char *popeye_plasma_name(void);
bool popeye_plasma_should_boot_desktop(void);
void popeye_plasma_set_boot_desktop(bool enabled);
const char *popeye_plasma_boot_mode(void);
bool popeye_plasma_run(void);
void popeye_plasma_force_terminal_first(void);
bool popeye_plasma_try_enable_desktop_boot(void);
bool popeye_plasma_desktop_boot_allowed(void);
bool popeye_plasma_enable_java_support(void);
bool popeye_plasma_java_support_enabled(void);
const char *popeye_plasma_java_support_mode(void);

#ifdef __cplusplus
}
#endif

#endif
