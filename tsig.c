// tsig.c ... functions on Tuple Signatures (tsig's)
// part of SIMC signature files
// Written by John Shepherd, September 2018

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

Bits codeword(char *attr_value, int m, int k)
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

// make a tuple signature

Bits makeTupleSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	Bits tsig=newBits(tsigBits(r));
    Bits cw;
    unsetAllBits(tsig);
    char **tuple_value = tupleVals(r,t);
    for(int i=0;i<nAttrs(r);i++){
        cw = codeword(tuple_value[i],tsigBits(r),strlen(tuple_value[i]));
        orBits(tsig,cw);
    }
    freeVals(tuple_value,nAttrs(r));
	return tsig;
}

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);
	Bits querysig = makeTupleSig(q->rel,q->qstring);
	unsetAllBits(q->pages);
    PageID pid=0;
    for(PageID spid=0;spid<nTsigPages(q->rel);spid++){
        Page p = getPage(tsigFile(q->rel),spid);
        q->nsigpages++;
        for(int i=0;i<pageNitems(p);i++){  //ith sig in tsigpage p
            Bits tsig = newBits(tsigBits(q->rel));
            getBits(p,i,tsig);
            q->nsigs++;
            if(isSubset(querysig,tsig)){//matches
                pid = maxTsigsPP(q->rel)*spid/maxTupsPP(q->rel);
                setBit(q->pages,pid);
            }
        }
    }
	// The printf below is primarily for debugging
	// Remove it before submitting this function
	//printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
