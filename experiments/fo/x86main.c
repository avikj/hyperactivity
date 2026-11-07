/* experiments/fo/x86main.c — runs a blob lib/emitx86.cub emitted (plan 18.7's JIT path): the
 * file's bytes are copied into executable memory and entered as
 *   uint64_t entry(uint64_t *arena, uint64_t arg0, …)
 * with the words on the command line as arguments (up to five); the result is printed.
 * Usage: x86main blob.bin [args…]. The arena is the blob's bump store for constructor cells. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
typedef uint64_t (*entry5)(uint64_t *, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: x86main blob.bin [args]\n"); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    void *code = mmap(NULL, (size_t) n + 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) { perror("mmap"); return 2; }
    if (fread(code, 1, (size_t) n, f) != (size_t) n) { perror("read"); return 2; }
    fclose(f);
    if (mprotect(code, (size_t) n + 4096, PROT_READ | PROT_EXEC)) { perror("mprotect"); return 2; }
    uint64_t *arena = calloc(1 << 20, sizeof(uint64_t));
    uint64_t a[5] = { 0, 0, 0, 0, 0 };
    for (int i = 2; i < argc && i < 7; i++) a[i - 2] = strtoull(argv[i], NULL, 10);
    uint64_t r = ((entry5) code)(arena, a[0], a[1], a[2], a[3], a[4]);
    printf("%llu\n", (unsigned long long) r);
    return 0;
}
