// reln.c ... functions on Relations
// part of signature indexed files
// Written by John Shepherd, March 2019

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

Status newRelation(char *name, Count nattrs, float pF, char sigtype,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
	p->nattrs = nattrs;
	p->pF = pF,
	p->sigtype = sigtype;
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
	//addPage(r->bsigf); p->bsigNpages = 1; p->nbsigs = 0; // replace this
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
	//TODO
	//初始化生成一个bitslice sig page
    addPage(r->bsigf);
    p->bsigNpages = 1;
    p->nbsigs = 0;

    int the_pm = psigBits(r);
    int the_bm = bsigBits(r);
    Count curr_page_items = 0;

    Count max_bsig_per_page = maxBsigsPP(r);
    File bsig_file = bsigFile(r);

    Bool bsig_page_is_full = FALSE;


    for (int i = 0; i < the_pm; i++) {
        PageID pid = p->bsigNpages - 1;
        Page bpage = getPage(bsig_file, pid);
        curr_page_items = pageNitems(bpage); // 当前page有多少items

        if ((max_bsig_per_page - curr_page_items) > 0){
            bsig_page_is_full = FALSE;  // 没有满
        } else{
            bsig_page_is_full = TRUE;  // 满了
        }

        if (bsig_page_is_full){
            addPage(bsig_file);  //增加一个page
            pid++;
            free(bpage);  // free 上一个满了的bpage
            bpage = newPage();  //
            if (bpage == NULL) return NO_PAGE;

            Bits bsig = newBits(the_bm);
            putBits(bpage, pageNitems(bpage), bsig);
            addOneItem(bpage);
            putPage(bsig_file, pid, bpage);
            freeBits(bsig);
            p->bsigNpages++;
            p->nbsigs++;
        } else{
            Bits bsig = newBits(the_bm);
            putBits(bpage, pageNitems(bpage), bsig);
            addOneItem(bpage);
            putPage(bsig_file, pid, bpage);
            freeBits(bsig);
            p->nbsigs++;
        }
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
	
	//TODO

    PageID t_pid = rp->tsigNpages - 1;
    Page tuplepage = getPage(r->tsigf, t_pid);
    // check if room on last page; if not add one new page
    Count max_sig_per_page = maxTsigsPP(r);
    Count number_of_tsig_in_tsigpage =  pageNitems(tuplepage);
    Bool tsig_page_is_full = FALSE;

    //判断 tuple signature page 有没有满
    if (number_of_tsig_in_tsigpage == max_sig_per_page){
        tsig_page_is_full = TRUE;  //满了
    } else{
        tsig_page_is_full = FALSE;  //没满
    }
    if (tsig_page_is_full) {
        // 如果tsig page满了
        addPage(r->tsigf);  // 新增一个tsig page 到tsig file
        t_pid++;
        free(tuplepage);  //free 原来那个满了的
        tuplepage = newPage();  //生成一个新的
        if (tuplepage == NULL) return NO_PAGE;
        Bits t_sig = makeTupleSig(r, t); 
        putBits(tuplepage, pageNitems(tuplepage), t_sig);
        rp->tsigNpages++;
    } else{
        // 如果没满
        Bits t_sig = makeTupleSig(r, t);
        putBits(tuplepage, pageNitems(tuplepage), t_sig);
    }
    addOneItem(tuplepage);
    putPage(r->tsigf, t_pid, tuplepage);
    rp->ntsigs++;

	// compute page signature and add to psigf

	//TODO

    Bits Psig = makePageSig(r, t);
    int p_m = psigBits(r);
    Bits last_psig = newBits(p_m);
    PageID p_pid = nPsigPages(r) - 1;
    p = getPage(r->psigf, p_pid);

    Count number_of_data_pages = nPages(r);  // data page 个数
    Count number_of_psig = nPsigs(r); // page signature个数

    Bool add_new_data_page = FALSE;  //如果之前有新datapage，那么npage已经+1，而page sig没更新
    Bool sig_page_is_full = FALSE;
    Count number_of_psig_in_page = pageNitems(p);  //当前psig page有多少 page sig
    Count max_psig_per_page = maxPsigsPP(r); // psig page最大容纳page sig数

    if ((number_of_data_pages - number_of_psig) == 0){
        add_new_data_page = TRUE;
    } else{
        add_new_data_page = FALSE;
    }

    if ((max_psig_per_page - number_of_psig_in_page) == 0){  // psig page 满了
        sig_page_is_full = TRUE;
    } else{
        sig_page_is_full = FALSE;
    }

    if (add_new_data_page) {  // 没有新增data page
        getBits(p, pageNitems(p)-1, last_psig);  // 获取最后一页最后一个psig
        orBits(Psig, last_psig);  // or 新的psig
        putBits(p, pageNitems(p)-1, Psig);  // 写回psig page
    }
    else {  // 新增了data page
        // reference from addToRelation the part of data page
        if (sig_page_is_full) {
            addPage(r->psigf);
            free(p);
            p = newPage();
            if (p == NULL) return NO_PAGE;
            putBits(p, pageNitems(p), Psig);
            addOneItem(p);
            p_pid++;
            rp->psigNpages++;
            rp->npsigs++;
        }else {
            putBits(p, pageNitems(p), Psig);
            addOneItem(p);
            rp->npsigs++;
        }
    }
    putPage(r->psigf, p_pid, p);
    freeBits(last_psig);

	// use page signature to update bit-slices

	//TODO

	Page bsig_page;
    int bsig_page_id = nPsigs(r) - 1;
    PageID pp_id = 0;
    bsig_page = getPage(bsigFile(r), pp_id);  // get 第一个 page
    int max_bsig_per_page = maxBsigsPP(r);
    int b_m = bsigBits(r);
    Bool curr_bit_is_set = FALSE;

    for (int i = 0; i < psigBits(r); i++) {

        if (bitIsSet(Psig, i)) {  //当前位是1
            curr_bit_is_set = TRUE;
        } else{
            curr_bit_is_set = FALSE;
        }
        if (curr_bit_is_set) {
            if (pp_id != i / max_bsig_per_page) { //如果到了下一个page
                putPage(bsigFile(r), pp_id, bsig_page);  // put上一个page
             	pp_id = i / max_bsig_per_page;
                bsig_page = getPage(bsigFile(r), pp_id);
                Count off_t = i % max_bsig_per_page;
                Bits bsig = newBits(b_m);
                getBits(bsig_page, off_t, bsig);
                setBit(bsig, bsig_page_id);
                putBits(bsig_page, off_t, bsig);
                freeBits(bsig);
            } else{
                //否则只是写入
                Count off_t = i % max_bsig_per_page;
                Bits bsig = newBits(b_m);
                getBits(bsig_page, off_t, bsig);
                setBit(bsig, bsig_page_id);
                putBits(bsig_page, off_t, bsig);
                freeBits(bsig);
            }
        }else{
            continue;
        }
    }
    putPage(bsigFile(r), pp_id, bsig_page);  // put上一个page
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
	printf("  sigs   %s",
            p->sigtype == 'c' ? "catc" : "simc");
    if (p->sigtype == 's')
	    printf("  bits/attr: %d", p->tk);
    printf("\n");
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}
