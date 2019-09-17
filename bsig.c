// bsig.c ... functions on Tuple Signatures (bsig's)
// part of SIMC signature files
// Written by John Shepherd, September 2018

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"

void findPagesUsingBitSlices(Query q)
{
    assert(q != NULL);
    Page b_sp;  PageID b_spid;
    int temp=-1;
    Bits querysig = makePageSig(q->rel,q->qstring);
    setAllBits(q->pages);
    for(int i=0;i<psigBits(q->rel);i++){
        if(bitIsSet(querysig,i)){
            b_spid =i/maxBsigsPP(q->rel);
            if(temp!=b_spid){
                b_sp = getPage(bsigFile(q->rel),b_spid);
                q->nsigpages++;
            }
            temp=b_spid;
            Bits slice = newBits(nPages(q->rel));
            getBits(b_sp,i%maxBsigsPP(q->rel),slice);
            q->nsigs++;
            for(int j=0;j<nPages(q->rel);j++){
                if(!(bitIsSet(slice,j)))
                    unsetBit(q->pages,j);
            }
        }
    }
    //printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}

