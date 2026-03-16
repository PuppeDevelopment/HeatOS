/*=============================================================================
 * HeatOS Mini JVM - Java Bytecode Interpreter
 * Supports a subset of real JVM opcodes + HeatOS extensions for I/O.
 * Includes built-in demo programs (HelloWorld, Fibonacci, Factorial).
 *===========================================================================*/
extern "C" {
#include "plasma_java.h"
#include "string.h"
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* JVM Opcodes                                                                */
/* ═══════════════════════════════════════════════════════════════════════════ */
#define OP_NOP        0x00
#define OP_ICONST_M1  0x02
#define OP_ICONST_0   0x03
#define OP_ICONST_1   0x04
#define OP_ICONST_2   0x05
#define OP_ICONST_3   0x06
#define OP_ICONST_4   0x07
#define OP_ICONST_5   0x08
#define OP_BIPUSH     0x10
#define OP_SIPUSH     0x11
#define OP_ILOAD      0x15
#define OP_ILOAD_0    0x1A
#define OP_ILOAD_1    0x1B
#define OP_ILOAD_2    0x1C
#define OP_ILOAD_3    0x1D
#define OP_ISTORE     0x36
#define OP_ISTORE_0   0x3B
#define OP_ISTORE_1   0x3C
#define OP_ISTORE_2   0x3D
#define OP_ISTORE_3   0x3E
#define OP_DUP        0x59
#define OP_IADD       0x60
#define OP_ISUB       0x64
#define OP_IMUL       0x68
#define OP_IDIV       0x6C
#define OP_IREM       0x70
#define OP_IINC       0x84
#define OP_IFEQ       0x99
#define OP_IFNE       0x9A
#define OP_IFLT       0x9B
#define OP_IFGE       0x9C
#define OP_IFGT       0x9D
#define OP_IFLE       0x9E
#define OP_IF_ICMPEQ  0x9F
#define OP_IF_ICMPNE  0xA0
#define OP_IF_ICMPLT  0xA1
#define OP_IF_ICMPGE  0xA2
#define OP_IF_ICMPGT  0xA3
#define OP_IF_ICMPLE  0xA4
#define OP_GOTO       0xA7
#define OP_IRETURN    0xAC
#define OP_RETURN     0xB1

/* HeatOS extensions */
#define OP_PRINT_STR  0xFC   /* <len_byte> <chars...> */
#define OP_PRINT_CHAR 0xFD   /* pop, print as ASCII */
#define OP_PRINT_INT  0xFE   /* pop, print decimal + newline */

#define STACK_MAX  64
#define LOCAL_MAX  16

/* ═══════════════════════════════════════════════════════════════════════════ */
/* VM State                                                                   */
/* ═══════════════════════════════════════════════════════════════════════════ */
struct JVM {
    int32_t       stack[STACK_MAX];
    int           sp;
    int32_t       locals[LOCAL_MAX];
    const uint8_t *code;
    int           pc;
    int           code_len;
    char         *out;
    int           out_pos;
    int           out_max;
    bool          running;
    bool          error;
    char          err_msg[64];
    int           insn_count;
};

static void jvm_init(JVM *vm, const uint8_t *code, int len, char *out, int out_max) {
    vm->sp = 0;
    for (int i = 0; i < LOCAL_MAX; i++) vm->locals[i] = 0;
    vm->code = code;
    vm->pc = 0;
    vm->code_len = len;
    vm->out = out;
    vm->out_pos = 0;
    vm->out_max = out_max;
    vm->running = true;
    vm->error = false;
    vm->err_msg[0] = '\0';
    vm->insn_count = 0;
    if (out_max > 0) out[0] = '\0';
}

static void jvm_emit(JVM *vm, char c) {
    if (vm->out_pos < vm->out_max - 1) {
        vm->out[vm->out_pos++] = c;
        vm->out[vm->out_pos] = '\0';
    }
}

static void jvm_emit_str(JVM *vm, const char *s) {
    while (*s) jvm_emit(vm, *s++);
}

static void jvm_emit_int(JVM *vm, int32_t v) {
    char buf[16];
    itoa((int)v, buf, 10);
    jvm_emit_str(vm, buf);
}

static void jvm_error(JVM *vm, const char *msg) {
    vm->error = true;
    vm->running = false;
    strncpy(vm->err_msg, msg, 63);
    vm->err_msg[63] = '\0';
}

static void jvm_push(JVM *vm, int32_t val) {
    if (vm->sp >= STACK_MAX) { jvm_error(vm, "Stack overflow"); return; }
    vm->stack[vm->sp++] = val;
}

static int32_t jvm_pop(JVM *vm) {
    if (vm->sp <= 0) { jvm_error(vm, "Stack underflow"); return 0; }
    return vm->stack[--vm->sp];
}

static uint8_t jvm_fetch(JVM *vm) {
    if (vm->pc >= vm->code_len) { jvm_error(vm, "PC out of bounds"); return 0; }
    return vm->code[vm->pc++];
}

static int16_t jvm_fetch16(JVM *vm) {
    uint8_t hi = jvm_fetch(vm);
    uint8_t lo = jvm_fetch(vm);
    return (int16_t)((uint16_t)hi << 8 | lo);
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Interpreter                                                                */
/* ═══════════════════════════════════════════════════════════════════════════ */
static void jvm_run(JVM *vm) {
    while (vm->running && !vm->error) {
        if (vm->insn_count++ > 100000) {
            jvm_error(vm, "Execution limit exceeded");
            return;
        }

        int insn_pc = vm->pc;
        uint8_t op = jvm_fetch(vm);
        if (vm->error) return;

        switch (op) {
        case OP_NOP: break;

        case OP_ICONST_M1: jvm_push(vm, -1); break;
        case OP_ICONST_0:  jvm_push(vm, 0); break;
        case OP_ICONST_1:  jvm_push(vm, 1); break;
        case OP_ICONST_2:  jvm_push(vm, 2); break;
        case OP_ICONST_3:  jvm_push(vm, 3); break;
        case OP_ICONST_4:  jvm_push(vm, 4); break;
        case OP_ICONST_5:  jvm_push(vm, 5); break;

        case OP_BIPUSH: {
            int8_t val = (int8_t)jvm_fetch(vm);
            jvm_push(vm, (int32_t)val);
            break;
        }
        case OP_SIPUSH: {
            int16_t val = jvm_fetch16(vm);
            jvm_push(vm, (int32_t)val);
            break;
        }

        case OP_ILOAD: {
            uint8_t idx = jvm_fetch(vm);
            if (idx >= LOCAL_MAX) { jvm_error(vm, "Invalid local index"); return; }
            jvm_push(vm, vm->locals[idx]);
            break;
        }
        case OP_ILOAD_0: jvm_push(vm, vm->locals[0]); break;
        case OP_ILOAD_1: jvm_push(vm, vm->locals[1]); break;
        case OP_ILOAD_2: jvm_push(vm, vm->locals[2]); break;
        case OP_ILOAD_3: jvm_push(vm, vm->locals[3]); break;

        case OP_ISTORE: {
            uint8_t idx = jvm_fetch(vm);
            if (idx >= LOCAL_MAX) { jvm_error(vm, "Invalid local index"); return; }
            vm->locals[idx] = jvm_pop(vm);
            break;
        }
        case OP_ISTORE_0: vm->locals[0] = jvm_pop(vm); break;
        case OP_ISTORE_1: vm->locals[1] = jvm_pop(vm); break;
        case OP_ISTORE_2: vm->locals[2] = jvm_pop(vm); break;
        case OP_ISTORE_3: vm->locals[3] = jvm_pop(vm); break;

        case OP_DUP: {
            int32_t val = jvm_pop(vm);
            jvm_push(vm, val);
            jvm_push(vm, val);
            break;
        }

        case OP_IADD: { int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); jvm_push(vm, a + b); break; }
        case OP_ISUB: { int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); jvm_push(vm, a - b); break; }
        case OP_IMUL: { int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); jvm_push(vm, a * b); break; }
        case OP_IDIV: {
            int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm);
            if (b == 0) { jvm_error(vm, "Division by zero"); return; }
            jvm_push(vm, a / b);
            break;
        }
        case OP_IREM: {
            int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm);
            if (b == 0) { jvm_error(vm, "Division by zero"); return; }
            jvm_push(vm, a % b);
            break;
        }

        case OP_IINC: {
            uint8_t idx = jvm_fetch(vm);
            int8_t  c   = (int8_t)jvm_fetch(vm);
            if (idx >= LOCAL_MAX) { jvm_error(vm, "Invalid local index"); return; }
            vm->locals[idx] += (int32_t)c;
            break;
        }

        /* Branch instructions: offset is relative to the opcode position */
        case OP_IFEQ: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) == 0) vm->pc = insn_pc + off; break; }
        case OP_IFNE: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) != 0) vm->pc = insn_pc + off; break; }
        case OP_IFLT: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) <  0) vm->pc = insn_pc + off; break; }
        case OP_IFGE: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) >= 0) vm->pc = insn_pc + off; break; }
        case OP_IFGT: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) >  0) vm->pc = insn_pc + off; break; }
        case OP_IFLE: { int16_t off = jvm_fetch16(vm); if (jvm_pop(vm) <= 0) vm->pc = insn_pc + off; break; }

        case OP_IF_ICMPEQ: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a == b) vm->pc = insn_pc + off; break; }
        case OP_IF_ICMPNE: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a != b) vm->pc = insn_pc + off; break; }
        case OP_IF_ICMPLT: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a <  b) vm->pc = insn_pc + off; break; }
        case OP_IF_ICMPGE: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a >= b) vm->pc = insn_pc + off; break; }
        case OP_IF_ICMPGT: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a >  b) vm->pc = insn_pc + off; break; }
        case OP_IF_ICMPLE: { int16_t off = jvm_fetch16(vm); int32_t b = jvm_pop(vm); int32_t a = jvm_pop(vm); if (a <= b) vm->pc = insn_pc + off; break; }

        case OP_GOTO: {
            int16_t off = jvm_fetch16(vm);
            vm->pc = insn_pc + off;
            break;
        }

        case OP_RETURN:  vm->running = false; break;
        case OP_IRETURN: (void)jvm_pop(vm); vm->running = false; break;

        /* HeatOS extensions */
        case OP_PRINT_STR: {
            uint8_t len = jvm_fetch(vm);
            for (int i = 0; i < len && !vm->error; i++) {
                uint8_t ch = jvm_fetch(vm);
                jvm_emit(vm, (char)ch);
            }
            break;
        }
        case OP_PRINT_CHAR: {
            int32_t v = jvm_pop(vm);
            jvm_emit(vm, (char)(v & 0xFF));
            break;
        }
        case OP_PRINT_INT: {
            int32_t v = jvm_pop(vm);
            jvm_emit_int(vm, v);
            jvm_emit(vm, '\n');
            break;
        }

        default:
            jvm_error(vm, "Unknown opcode");
            break;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Built-in Demo Programs                                                     */
/* ═══════════════════════════════════════════════════════════════════════════ */

/* Demo 1: HelloWorld - prints greeting strings */
static const uint8_t demo_hello[] = {
    /* print "Hello from Java on HeatOS!\n" (27 bytes) */
    0xFC, 27,
    'H','e','l','l','o',' ','f','r','o','m',' ',
    'J','a','v','a',' ','o','n',' ',
    'H','e','a','t','O','S','!','\n',
    /* print "Bytecode VM running on bare metal!\n" (35 bytes) */
    0xFC, 35,
    'B','y','t','e','c','o','d','e',' ',
    'V','M',' ','r','u','n','n','i','n','g',' ',
    'o','n',' ','b','a','r','e',' ',
    'm','e','t','a','l','!','\n',
    /* print "Java support: active\n" (21 bytes) */
    0xFC, 21,
    'J','a','v','a',' ','s','u','p','p','o','r','t',':',' ',
    'a','c','t','i','v','e','\n',
    /* return */
    0xB1
};

/* Demo 2: Fibonacci - computes fib(10) = 55 */
/* locals: 0=a, 1=b, 2=temp, 3=counter */
static const uint8_t demo_fib[] = {
    /* offset 0: print header (28 bytes) */
    0xFC, 28,
    'C','o','m','p','u','t','i','n','g',' ',
    'F','i','b','o','n','a','c','c','i','(','1','0',')',
    '.','.','.','\n','\n',
    /* offset 30: a = 0 */
    0x03,              /* iconst_0 */
    0x3B,              /* istore_0 */
    /* offset 32: b = 1 */
    0x04,              /* iconst_1 */
    0x3C,              /* istore_1 */
    /* offset 34: counter = 9 (9 iterations: fib(10)=55) */
    0x10, 9,           /* bipush 9 */
    0x3E,              /* istore_3 */
    /* LOOP at offset 37 */
    0x1A,              /* iload_0   push a */
    0x1B,              /* iload_1   push b */
    0x60,              /* iadd      a+b */
    0x3D,              /* istore_2  temp = a+b */
    0x1B,              /* iload_1   push b */
    0x3B,              /* istore_0  a = b */
    0x1C,              /* iload_2   push temp */
    0x3C,              /* istore_1  b = temp */
    0x84, 3, 0xFF,     /* iinc 3, -1 */
    0x1D,              /* iload_3   push counter */
    /* offset 49: ifne -> offset 37 (49 + (-12) = 37) */
    0x9A, 0xFF, 0xF4,
    /* offset 52: print "Fibonacci(10) = " (16 bytes) */
    0xFC, 16,
    'F','i','b','o','n','a','c','c','i','(','1','0',')',' ','=',' ',
    /* offset 70: print result */
    0x1B,              /* iload_1   push b */
    0xFE,              /* print_int */
    0xB1               /* return */
};

/* Demo 3: Factorial - computes 10! = 3628800 */
/* locals: 0=result, 1=i */
static const uint8_t demo_fact[] = {
    /* offset 0: print header (18 bytes) */
    0xFC, 18,
    'C','o','m','p','u','t','i','n','g',' ',
    '1','0','!','.','.','.','\n','\n',
    /* offset 20: result = 1 */
    0x04,              /* iconst_1 */
    0x3B,              /* istore_0 */
    /* offset 22: i = 1 */
    0x04,              /* iconst_1 */
    0x3C,              /* istore_1 */
    /* LOOP at offset 24 */
    0x1A,              /* iload_0   push result */
    0x1B,              /* iload_1   push i */
    0x68,              /* imul      result * i */
    0x3B,              /* istore_0  result = result * i */
    0x84, 1, 1,        /* iinc 1, 1  i++ */
    0x1B,              /* iload_1   push i */
    0x10, 11,          /* bipush 11 */
    /* offset 34: if_icmplt -> offset 24 (34 + (-10) = 24) */
    0xA1, 0xFF, 0xF6,
    /* offset 37: print "10! = " (6 bytes) */
    0xFC, 6,
    '1','0','!',' ','=',' ',
    /* offset 45: print result */
    0x1A,              /* iload_0   push result */
    0xFE,              /* print_int */
    0xB1               /* return */
};

struct JavaDemo {
    const char *name;
    const uint8_t *code;
    int code_len;
};

static const JavaDemo g_demos[] = {
    { "hello", demo_hello, (int)sizeof(demo_hello) },
    { "fib",   demo_fib,   (int)sizeof(demo_fib)   },
    { "fact",  demo_fact,  (int)sizeof(demo_fact)   },
};

#define DEMO_COUNT ((int)(sizeof(g_demos) / sizeof(g_demos[0])))

/* ═══════════════════════════════════════════════════════════════════════════ */
/* Public API                                                                 */
/* ═══════════════════════════════════════════════════════════════════════════ */

extern "C" void java_vm_init(void) {
    /* Nothing to initialize - VM is stateless between runs */
}

extern "C" bool java_vm_run(const char *demo_name, java_result_t *result) {
    memset(result, 0, sizeof(java_result_t));

    const JavaDemo *demo = (const JavaDemo *)0;

    for (int i = 0; i < DEMO_COUNT; i++) {
        if (strcmp(demo_name, g_demos[i].name) == 0) {
            demo = &g_demos[i];
            break;
        }
    }

    /* Default to hello if empty name */
    if (!demo && (demo_name[0] == '\0')) {
        demo = &g_demos[0];
    }

    if (!demo) {
        strcpy(result->error, "Unknown demo program");
        return false;
    }

    JVM vm;
    jvm_init(&vm, demo->code, demo->code_len, result->output, JAVA_OUTPUT_MAX);
    jvm_run(&vm);

    result->output_len = vm.out_pos;
    result->success = !vm.error;
    if (vm.error) {
        strncpy(result->error, vm.err_msg, 63);
        result->error[63] = '\0';
    }
    return result->success;
}

extern "C" int java_vm_demo_count(void) {
    return DEMO_COUNT;
}

extern "C" const char *java_vm_demo_name(int index) {
    if (index < 0 || index >= DEMO_COUNT) return "";
    return g_demos[index].name;
}
