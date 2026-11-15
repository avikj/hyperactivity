/* src/drivers.c — the driver loop (PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.11 “D24–D27 — The world”, its part D26). Included by lang.c after the readers.
 *
 * (run e): e is a value of lib/world.cub's Step W A, a process as a tree of steps. The loop reads
 * the head: sdone a prints a and stops; scmd c k reads the command c (a constructor of WCmd with
 * word, Nat and byte-string fields), performs it against the operating system, builds the response
 * as a value and applies the continuation k to it, then reads again. Reading a field is an
 * observation of the net (synchronous: the process blocks on the command, as D26 says: PLAN.md revision 7.9, 6 Nov 2026, git 2ca8d747aaa5:PLAN.md, §18.11 “D24–D27 — The world”, its part D26). The
 * commands: w-now (nanoseconds since the epoch, a word), w-write fd n b (the count written),
 * w-read fd n (the count read and a buffer of n bytes), w-sleep ns (unit), w-exit code.
 * w-sys nr a1..a6 is the raw system call (the typed world over it is lib/os/linux.cub): the
 * response is the kernel's return value as a 64-bit word, an error the two's complement of its
 * errno, what the syscall instruction leaves in rax (libc's syscall(2) answers -1 with errno,
 * which is put back). w-alloc n is the address of n fresh zero bytes, never freed; w-load8 and
 * w-store8 are one byte at an address. */
#include <time.h>
#include <errno.h>
/* unistd.h's link(2) clashes with the net's link(): the two system calls are declared here */
extern long read(int, void *, unsigned long);
extern long write(int, const void *, unsigned long);
extern int open(const char *, int, ...);
extern int close(int);
extern long syscall(long, ...);
static int drv_con(const char *name) { int c = con_lookup(name); if (c < 0) die("run: lib/world.cub is not loaded (missing constructor)", name); return c; }
/* a word field read: the value must be a literal word */
static uint64_t drv_word(Port p, const char *what) {
    Cell *c = obs_val(p);
    if (!c || c->kind != V_WORD || c->p) die("run: a word was expected for", what);
    return c->a;
}
/* a Nat field read: successors counted down to zero */
static uint64_t drv_nat(Port p, const char *what) {
    uint64_t n = 0;
    for (;;) {
        Cell *c = obs_val(p);
        if (!c) die("run: a natural number was expected for", what);
        if (c->kind == V_WORD && !c->p) return n + c->a;
        if (c->kind != V_CON) die("run: a natural number was expected for", what);
        const char *nm = CON[c->a].name;
        if (!strcmp(nm, "zero")) return n;
        if (strcmp(nm, "suc") || c->n < 2) die("run: a natural number was expected for", what);
        n++; p = c->port[1];
    }
}
static Bytes *drv_bytes(Port p, const char *what) {
    Cell *c = obs_val(p);
    if (!c || c->kind != V_BYTES) die("run: a byte string was expected for", what);
    return (Bytes *)c->p;
}
static Port drv_word_of(uint64_t n) { Cell *c = cell(V_WORD, 1); c->a = n; return P(c, 0); }
static Port drv_bytes_of(Bytes *b) { Cell *c = cell(V_BYTES, 1); c->x = PT_BYTES; c->p = b; return P(c, 0); }
static void drivers_run(Node *t) {
    int c_sdone = drv_con("sdone"), c_scmd = drv_con("scmd"), c_unit = drv_con("unit");
    int c_now = drv_con("w-now"), c_write = drv_con("w-write"), c_read = drv_con("w-read"), c_sleep = drv_con("w-sleep"), c_exit = drv_con("w-exit");
    int c_open = drv_con("w-open"), c_close = drv_con("w-close");
    int c_load = drv_con("w-load"), c_store = drv_con("w-store"), c_cas = drv_con("w-cas"), c_fence = drv_con("w-fence");
    int c_sys = drv_con("w-sys"), c_alloc = drv_con("w-alloc"), c_load8 = drv_con("w-load8"), c_store8 = drv_con("w-store8");
    Port p = build_top(t);
    long steps = 0, s0 = stat_steps;
    for (;;) {
        Cell *h = obs_val(p);
        if (!h) die("run: the process did not reduce to a step", NULL);
        if (h->kind != V_CON) die("run: a step (sdone a or scmd c k) was expected", NULL);
        if ((int)h->a == c_sdone) {
            char *r = read_port(h->port[1]);
            net_drain();
            printf("run: done %s   [%ld steps]\n", r, stat_steps - s0 + steps);
            free(r);
            return;
        }
        if ((int)h->a != c_scmd || h->n < 3) die("run: a step (sdone a or scmd c k) was expected", NULL);
        Port k = h->port[2];
        Cell *cm = obs_val(h->port[1]);
        if (!cm || cm->kind != V_CON) { char m[64]; snprintf(m, sizeof m, "kind %d", cm ? cm->kind : -1); die("run: a command of W was expected, found", m); }
        int c = (int)cm->a;
        Port resp;
        if (c == c_now) {
            struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
            resp = drv_word_of((uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec);
        } else if (c == c_write) {
            uint64_t fd = drv_word(cm->port[1], "w-write's descriptor"), n = drv_nat(cm->port[2], "w-write's length");
            Bytes *b = drv_bytes(cm->port[3], "w-write's bytes");
            size_t len = b ? b->n : 0; if (n < len) len = (size_t)n;
            fflush(stdout);
            long w = len ? write((int)fd, b->b, len) : 0;
            resp = drv_word_of(w < 0 ? 0 : (uint64_t)w);
        } else if (c == c_read) {
            uint64_t fd = drv_word(cm->port[1], "w-read's descriptor"), n = drv_nat(cm->port[2], "w-read's length");
            Bytes *b = calloc(1, sizeof(Bytes) + (size_t)n); b->rc = 1; b->n = (size_t)n;
            long r = n ? read((int)fd, b->b, (size_t)n) : 0;
            Cell *pr = cell(V_PAIR, 3);
            link(drv_word_of(r < 0 ? 0 : (uint64_t)r), P(pr, 1)); link(drv_bytes_of(b), P(pr, 2));
            resp = P(pr, 0);
        } else if (c == c_sleep) {
            uint64_t ns = drv_word(cm->port[1], "w-sleep's nanoseconds");
            struct timespec ts = { (time_t)(ns / 1000000000ull), (long)(ns % 1000000000ull) };
            nanosleep(&ts, NULL);
            resp = con_build(c_unit, NULL, NULL);
        } else if (c == c_exit) {
            uint64_t code = drv_word(cm->port[1], "w-exit's code");
            fflush(stdout);
            exit((int)code);
        } else if (c == c_open) {
            uint64_t n = drv_nat(cm->port[1], "w-open's length");
            Bytes *b = drv_bytes(cm->port[2], "w-open's path");
            uint64_t fl = drv_word(cm->port[3], "w-open's flags"), md = drv_word(cm->port[4], "w-open's mode");
            size_t len = b ? b->n : 0; if (n < len) len = (size_t)n;
            char *path = malloc(len + 1); memcpy(path, b ? (const char *)b->b : "", len); path[len] = 0;
            int fd = open(path, (int)fl, (int)md);
            free(path);
            resp = drv_word_of((uint64_t)(int64_t)fd);
        } else if (c == c_close) {
            uint64_t fd = drv_word(cm->port[1], "w-close's descriptor");
            resp = drv_word_of((uint64_t)(int64_t)close((int)fd));
        } else if (c == c_load) {
            uint64_t a = drv_word(cm->port[1], "w-load's address");
            resp = drv_word_of(__atomic_load_n((uint64_t *)(uintptr_t)a, __ATOMIC_ACQUIRE));
        } else if (c == c_store) {
            uint64_t a = drv_word(cm->port[1], "w-store's address"), v = drv_word(cm->port[2], "w-store's value");
            __atomic_store_n((uint64_t *)(uintptr_t)a, v, __ATOMIC_RELEASE);
            resp = con_build(c_unit, NULL, NULL);
        } else if (c == c_cas) {
            uint64_t a = drv_word(cm->port[1], "w-cas's address"), old = drv_word(cm->port[2], "w-cas's old"), nw = drv_word(cm->port[3], "w-cas's new");
            uint64_t seen = old;
            __atomic_compare_exchange_n((uint64_t *)(uintptr_t)a, &seen, nw, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
            resp = drv_word_of(seen);
        } else if (c == c_fence) {
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            resp = con_build(c_unit, NULL, NULL);
        } else if (c == c_sys) {
            uint64_t a[7];
            for (int i = 0; i < 7; i++) a[i] = drv_word(cm->port[1 + i], "w-sys's number and arguments");
            fflush(stdout);
            errno = 0;
            long r = syscall((long)a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
            resp = drv_word_of(r == -1 && errno ? (uint64_t)-(int64_t)errno : (uint64_t)r);
        } else if (c == c_alloc) {
            uint64_t n = drv_word(cm->port[1], "w-alloc's size");
            void *m = calloc(n ? (size_t)n : 1, 1);
            if (!m) die("run: w-alloc: out of memory", NULL);
            resp = drv_word_of((uint64_t)(uintptr_t)m);
        } else if (c == c_load8) {
            uint64_t a = drv_word(cm->port[1], "w-load8's address");
            resp = drv_word_of(*(volatile uint8_t *)(uintptr_t)a);
        } else if (c == c_store8) {
            uint64_t a = drv_word(cm->port[1], "w-store8's address"), v = drv_word(cm->port[2], "w-store8's value");
            *(volatile uint8_t *)(uintptr_t)a = (uint8_t)v;
            resp = con_build(c_unit, NULL, NULL);
        } else die("run: not a command of W:", CON[c].name);
        Cell *ap = cell(O_APP, 3);
        link(k, P(ap, 0)); link(resp, P(ap, 1));
        p = P(ap, 2);
    }
}
