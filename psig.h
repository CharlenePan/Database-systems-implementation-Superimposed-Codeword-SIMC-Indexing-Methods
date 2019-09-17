// psig.h ... interface to functions on page signatures
// part of SIMC signature files
// Written by John Shepherd, September 2018
// Fuctions written by MeiyanPAN for database systems implementation assignment
#ifndef PSIG_H
#define PSIG_H 1

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "bits.h"

Bits makePageSig(Reln, Tuple);
void findPagesUsingPageSigs(Query);

#endif
