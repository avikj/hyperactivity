/* libcubical.h — the call boundary, native → net (B18). A C program linked with build/libcubical.a
 * loads the libraries it needs with cubical_init, then calls checked definitions on word arguments:
 * the net computes the result to its normal form (synchronously). Words only in this revision: a
 * value of a declared representation is not marshaled. */
#ifndef LIBCUBICAL_H
#define LIBCUBICAL_H
#include <stdint.h>
/* loads the files (the libraries, in order; a definition is reachable after its file): 0, or -1 when a check failed */
int cubical_init(int nfiles, const char *const *files);
/* name applied to n words, read to a word in *out: 0; -1 when the normal form is not a word (its text: cubical_last_text) */
int cubical_call_w(const char *name, const uint64_t *args, int n, uint64_t *out);
const char *cubical_last_text(void);
/* the boxed form of the emitted C (lib/emitc.cub's V: tag 0 a word in n; tag c+1 constructor c with n fields): checked
 * definition k (its signature index) applied to boxed words, the result a boxed word, or NULL (a constructor argument
 * or result: not marshaled yet). This is the name the emitted programs call, cubical_call(key, n, (V *[]){…}) */
void *cubical_call(uint64_t k, uint64_t nargs, void **args);
long cubical_index(const char *name);   /* a checked definition's signature index, or -1 */
/* the general boxed call: name applied to boxed arguments whose types are named ("Word", "Bytes", or a data type's
 * name; a constructor is tag c+1 with its fields), the result read by the net and boxed with every field forced;
 * NULL when a type is unknown or the result is not a word, bytes or a constructor of such */
void *cubical_call_data(const char *name, uint64_t nargs, void **args, const char *const *types);
#endif
