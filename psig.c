// psig.c ... functions on page signatures (psig's)
// part of SIMC signature files
// Written by John Shepherd, September 2018
// Fuctions written by MeiyanPAN for database systems implementation assignment
#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "hash.h"
#include "bits.h"

Bits codewordp(char *attr_value, int m, int k)
{
    int  nbits = 0;   // count of set bits
    Bits cword =newBits(m);
    unsetAllBits(cword);
    if(strcmp(attr_value,"?")){
        srandom(hash_any(attr_value,k));//Generating Codewords
        while (nbits < k) {
            int i = random() % m;
            if(!(bitIsSet(cword,i))){
                setBit(cword,i);
                nbits++;
            }
        }
    }
    return cword;
}

Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	Bits psig=newBits(psigBits(r));
    Bits cw;
    unsetAllBits(psig);
    char **tuple_value = tupleVals(r,t);
    for(int i=0;i<nAttrs(r);i++){
        cw = codewordp(tuple_value[i],psigBits(r),strlen(tuple_value[i]));
        orBits(psig,cw);
    }
    freeVals(tuple_value,nAttrs(r));
    return psig;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
    Bits querysig = makePageSig(q->rel,q->qstring);
    unsetAllBits(q->pages);
    PageID pid=0;
    for(PageID spid=0;spid<nPsigPages(q->rel);spid++){
        Page p = getPage(psigFile(q->rel),spid);
        q->nsigpages++;
        for(int i=0;i<pageNitems(p);i++){  //ith sig in tsigpage p
            Bits psig = newBits(psigBits(q->rel));
            getBits(p,i,psig);
            q->nsigs++;
            if(isSubset(querysig,psig)){//matches
                pid = (maxPsigsPP(q->rel)*spid)%maxTupsPP(q->rel);
                setBit(q->pages,pid);
            }
        }
    }
    //printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
