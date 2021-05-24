// psig.c ... functions on page signatures (psig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "psig.h"
#include "hash.h"
#include <string.h>

Bits page_codeword(char *attr_value, int m, int k) {
    int nbits = 0;
    Bits cword = newBits(m);
    srand(hash_any(attr_value, strlen(attr_value)));
    while (nbits < k) {
        int i = rand() % m;
        if (!bitIsSet(cword, i)) {
            setBit(cword, i);
            nbits++;
        }
    }
    return cword;
}


Bits page_codewords_catc(char *attr_value, int m, int k, int start, int end) {
    int nbits = 0;
    int length_str = strlen(attr_value);
    Bits cword = newBits(m);

    srand(hash_any(attr_value, length_str));
    while (nbits < k) {
        int i = rand() % m;
        if((i <= end) & (i >= start)){  // 每次set 位数在 start和 end之间
            if (!bitIsSet(cword, i)) {
                setBit(cword, i);
                nbits++;
            }
        }
    }
    return cword;
}


Bits makePageSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
    int p_m = psigBits(r);  // page cw 长度 t_m
    int p_k = codeBits(r);  // page cw 中置为1的个数
    Bits desc = newBits(p_m);
    int nattr = nAttrs(r);
    char **attr_val = tupleVals(r, t);
    int max_tuple_per_page = maxTupsPP(r);  // 最大tuple数
    int catc_p_k_1 = (p_m/nattr + p_m % nattr)/2/max_tuple_per_page; // 第一个的tk
    int catc_p_k = (p_m/nattr)/2/max_tuple_per_page;
    int shift_distance;

    //如果是SIMC
    if (sigType(r) == 's') {
        for (int i = 0; i < nattr; i++) {
            if (strcmp(attr_val[i], "?") != 0) {
                Bits cw = page_codeword(attr_val[i], p_m, p_k);
                orBits(desc, cw);
                freeBits(cw);
            }
        }
    } else{
        //如果是CATC
        for (int i = 0; i < nattr; i++) {
            if (strcmp(attr_val[i], "?") != 0) {
                if(i == 0){
                    // 比如m = 10,n=3, 第一次set 0 - 3 前4位
                    Bits cw = page_codewords_catc(attr_val[i], p_m, catc_p_k_1, 0 , (p_m / nattr + p_m % nattr) - 1);
                    orBits(desc, cw);
                    freeBits(cw);
                } else if(i == 1){
                    //第二次set 0-2，前3位，shift 4位
                    Bits cw = page_codewords_catc(attr_val[i], p_m, catc_p_k, 0 , (p_m / nattr) - 1);
                    shift_distance = p_m / nattr + p_m % nattr;
                    shiftBits(cw, shift_distance);  // 每次shift m/n 位
                    orBits(desc, cw);
                    freeBits(cw);
                }else{
                    //第三次之后set 0-2，前3位，shift 4 + i * 3位
                    Bits cw = page_codewords_catc(attr_val[i], p_m, catc_p_k, 0 , (p_m / nattr) - 1);
                    shift_distance = (i * p_m / nattr) + p_m % nattr;
                    shiftBits(cw, shift_distance);  // 每次shift m/n 位
                    orBits(desc, cw);
                    freeBits(cw);
                }
            }
        }

    }
    freeVals(attr_val,nattr);
    return desc;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	//TODO

	//signature page里存的是 page 的 signature

    Reln r = q->rel;
    Bits Querysig = makePageSig(r, q->qstring);
    Bits psig = newBits(psigBits(r));

    int number_of_signature_page = nPsigPages(r); //当前relation r的psig page个数
    File tuple_sig_file = psigFile(r);  // 当前relation r的psig page所在file

    Bool psig_matched = FALSE;

    int sig_pages_read = 0;  //读了多少sig_pages
    int signatures_read = 0;  //读了多少条signatures

    for (int pid = 0; pid < number_of_signature_page; pid++) {
        Page p = getPage(tuple_sig_file, pid);
        for (int num = 0; num < pageNitems(p); num++) {
            getBits(p, num, psig);

            if(isSubset(Querysig, psig)){
                psig_matched = TRUE;  // query 的 page sig 和当前psig matched
            } else{
                psig_matched = FALSE;  // not matched
            }

            if (psig_matched == FALSE){
                signatures_read ++;
                // continue;
            }else{
                setBit(q->pages, signatures_read);
                signatures_read ++;
            }
        }
        sig_pages_read ++ ;
        free(p);
    }

    q->nsigpages = sig_pages_read;
    q->nsigs = signatures_read;
    freeBits(psig);
    freeBits(Querysig);
}

