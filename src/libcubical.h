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
int cubical_call(const char *name, const uint64_t *args, int n, uint64_t *out);
const char *cubical_last_text(void);
#endif
