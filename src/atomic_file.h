// SPDX-License-Identifier: GPL-3.0-only
#ifndef ATOMIC_FILE_H
#define ATOMIC_FILE_H
#include <stdio.h>
#include <limits.h>
typedef struct { FILE *file; char temporary[PATH_MAX],destination[PATH_MAX]; } AtomicFile;
/* Unique sibling file; failure leaves the existing destination intact. */
int atomic_file_open(AtomicFile *output,const char *destination);
int atomic_file_commit(AtomicFile *output);
void atomic_file_abort(AtomicFile *output);
#endif
