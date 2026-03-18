/*
 * calc — integer expression evaluator.
 * Grammar (recursive descent):
 *   expr   := term   (('+' | '-') term)*
 *   term   := factor (('*' | '/') factor)*
 *   factor := NUMBER | '(' expr ')' | '-' factor
 *
 * Example: calc 2 + 3 * 4    => 14
 *          calc (10 - 2) / 4  => 2
 */
#include "terminal.h"
#include "string.h"

static const char *g_pos;
static bool        g_err;

static void skip_ws(void) {
    while (*g_pos == ' ' || *g_pos == '\t') g_pos++;
}

static int32_t parse_expr(void);
static int32_t parse_term(void);
static int32_t parse_factor(void);

static int32_t parse_factor(void) {
    skip_ws();
    if (*g_pos == '(') {
        g_pos++;
        int32_t v = parse_expr();
        skip_ws();
        if (*g_pos == ')') g_pos++;
        return v;
    }
    if (*g_pos == '-') {
        g_pos++;
        return -parse_factor();
    }
    int32_t v = 0;
    bool had = false;
    while (*g_pos >= '0' && *g_pos <= '9') {
        v = v * 10 + (*g_pos - '0');
        g_pos++;
        had = true;
    }
    if (!had) g_err = true;
    return v;
}

static int32_t parse_term(void) {
    int32_t v = parse_factor();
    for (;;) {
        skip_ws();
        if (*g_pos == '*') {
            g_pos++; v *= parse_factor();
        } else if (*g_pos == '/') {
            g_pos++;
            int32_t d = parse_factor();
            if (d == 0) { g_err = true; return 0; }
            v /= d;
        } else break;
    }
    return v;
}

static int32_t parse_expr(void) {
    int32_t v = parse_term();
    for (;;) {
        skip_ws();
        if (*g_pos == '+')      { g_pos++; v += parse_term(); }
        else if (*g_pos == '-') { g_pos++; v -= parse_term(); }
        else break;
    }
    return v;
}

void cmd_calc(const char *args) {
    if (!args || !*args) {
        term_puts("Usage: calc <expression>\n");
        term_puts("  e.g.  calc 2 + 3 * 4\n");
        return;
    }
    g_pos = args;
    g_err = false;
    int32_t result = parse_expr();
    if (g_err) {
        term_puts("calc: parse error\n");
        return;
    }
    char buf[16];
    itoa((int)result, buf, 10);
    term_puts("= ");
    term_puts(buf);
    term_putc('\n', 0x07);
}
