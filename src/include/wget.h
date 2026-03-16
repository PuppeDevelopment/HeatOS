#ifndef WGET_H
#define WGET_H

#include "kat.h" /* For term_hooks_t */

void wget_run(const char *url, term_hooks_t *hooks);

#endif
