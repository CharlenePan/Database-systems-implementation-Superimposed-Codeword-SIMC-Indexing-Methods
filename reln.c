// reln.c ... functions on Relations
// part of SIMC signature files
// Written by John Shepherd, September 2018
// Fuctions written by MeiyanPAN for database systems implementation assignment
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "tsig.h"
#include "bits.h"
#include "hash.h"
#include "psig.h"
#include "bsig.h"
// open a file with a specified suffix
// - always open for both reading and writing

File openFile(char *name, char *suffix)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.%s",name,suffix);
	File f = open(fname,O_RDWR|O_CREAT,0644);
	assert(f >= 0);
	return f;
}

// create a new relation (five files)
// data file has one empty data page

Status newRelation(char *name, Count nattrs, float pF,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
    Page b_sp;  PageID b_spid;
	p->nattrs = nattrs;
	p->pF = pF,
	p->tupsize = 28 + 7*(nattrs-2);
	Count available = (PAGESIZE-sizeof(Count));
	p->tupPP = available/p->tupsize;
	p->tk = tk; 
	if (tm%8 > 0) tm += 8-(tm%8); // round up to byte size
	p->tm = tm; p->tsigSize = tm/8; p->tsigPP = available/(tm/8);
	if (pm%8 > 0) pm += 8-(pm%8); // round up to byte size
	p->pm = pm; p->psigSize = pm/8; p->psigPP = available/(pm/8);
	if (p->psigPP < 2) { free(r); return -1; }
	if (bm%8 > 0) bm += 8-(bm%8); // round up to byte size
	p->bm = bm; p->bsigSize = bm/8; p->bsigPP = available/(bm/8);
	if (p->bsigPP < 2) { free(r); return -1; }
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	addPage(r->dataf); p->npages = 1; p->ntups = 0;
	addPage(r->tsigf); p->tsigNpages = 1; p->ntsigs = 0;
	addPage(r->psigf); p->psigNpages = 1; p->npsigs = 0;
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
    p->bsigNpages=0;
    p->nbsigs=0;
    for(b_spid =0;b_spid<iceil(pm,p->bsigPP);b_spid++){
        addPage(r->bsigf);
        b_sp = getPage(r->bsigf,b_spid);
        for(int m=0;m<p->bsigPP;m++){
            if(p->nbsigs==pm)
                break;
            Bits slice = newBits(bm);
            unsetAllBits(slice);
            putBits(b_sp,pageNitems(b_sp),slice);
            addOneItem(b_sp);
            p->nbsigs++;
        }
        putPage(r->bsigf,b_spid,b_sp);
        p->bsigNpages++;
    }

	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	File f = open(fname,O_RDONLY);
	if (f < 0)
		return FALSE;
	else {
		close(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name)
{
	Reln r = malloc(sizeof(RelnRep));
	assert(r != NULL);
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	read(r->infof, &(r->params), sizeof(RelnParams));
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info file
	lseek(r->infof, 0, SEEK_SET);
	int n = write(r->infof, &(r->params), sizeof(RelnParams));
	assert(n == sizeof(RelnParams));
	close(r->infof); close(r->dataf);
	close(r->tsigf); close(r->psigf); close(r->bsigf);
	free(r);
}

// insert a new tuple into a relation
// returns page where inserted
// returns NO_PAGE if insert fails completely

PageID addToRelation(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL && strlen(t) == tupSize(r));
	Page p;  PageID pid;
    Page sp;  PageID spid;
    Page p_sp;  PageID p_spid;
    Page b_sp;  PageID b_spid;
	RelnParams *rp = &(r->params);
	
	// add tuple to last page
	pid = rp->npages-1;
	p = getPage(r->dataf, pid);
	// check if room on last page; if not add new page
	if (pageNitems(p) == rp->tupPP) {
		addPage(r->dataf);
		rp->npages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
	}
	addTupleToPage(r, p, t);
	rp->ntups++;  //written to disk in closeRelation()
	putPage(r->dataf, pid, p);

	// compute tuple signature and add to tsigf
	Bits tsig = makeTupleSig(r,t);
    spid = rp->tsigNpages-1;
    sp = getPage(r->tsigf,spid);
    if (pageNitems(sp) == rp->tsigPP){// check if room on last page; if not add new page
        addPage(r->tsigf);
        rp->tsigNpages++;
        spid++;
        free(sp);
        sp = newPage();
        if (sp == NULL) return NO_PAGE;
    }
    
    putBits(sp,pageNitems(sp),tsig);
    addOneItem(sp);
    rp->ntsigs++;
    putPage(r->tsigf,spid,sp);
    
	// compute page signature and add to psigf
    Bits psig = makePageSig(r,t);
    p_spid = rp->psigNpages-1;
    p_sp = getPage(r->psigf,p_spid);
    
    //check if psigPP exits ,if yes ,merge
    if((rp->npsigs-1)==pid){
        Bits ppsig=newBits(rp->pm);
        getBits(p_sp,pageNitems(p_sp)-1,ppsig);
        orBits(ppsig,psig);
        putBits(p_sp,pageNitems(p_sp)-1,ppsig);
    }
    else{
        if(pid % rp->psigPP==0 && pid!=0){
            addPage(r->psigf);
            rp->psigNpages++;
            p_spid++;
            free(p_sp);
            p_sp = newPage();
            if (p_sp == NULL) return NO_PAGE;
        }
        putBits(p_sp,pageNitems(p_sp),psig);
        addOneItem(p_sp);
        rp->npsigs++;
    }
	
    putPage(r->psigf,p_spid,p_sp);
    
	// use page signature to update bit-slices
    
    Bits b_psig = makePageSig(r,t);
    for(int i=0;i<rp->pm;i++){
        if(bitIsSet(b_psig,i)){
            b_spid =i/rp->bsigPP;
            b_sp = getPage(r->bsigf,b_spid);
            Bits slice = newBits(rp->npages);
            getBits(b_sp,i%rp->bsigPP,slice);
            setBit(slice,pid);
            putBits(b_sp,i%rp->bsigPP,slice);
            putPage(r->bsigf,b_spid,b_sp);
        }
    }

	return nPages(r)-1;
}

// displays info about open Reln (for debugging)

void relationStats(Reln r)
{
	RelnParams *p = &(r->params);
	printf("Global Info:\n");
	printf("Dynamic:\n");
    printf("  #items:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->ntups, p->ntsigs, p->npsigs, p->nbsigs);
    printf("  #pages:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->npages, p->tsigNpages, p->psigNpages, p->bsigNpages);
	printf("Static:\n");
    printf("  tups   #attrs: %d  size: %d bytes  max/page: %d\n",
			p->nattrs, p->tupsize, p->tupPP);
	printf("  sigs   bits/attr: %d\n", p->tk);
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}

