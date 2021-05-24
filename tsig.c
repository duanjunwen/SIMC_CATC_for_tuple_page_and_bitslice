// tsig.c ... functions on Tuple Signatures (tsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"

Bits codewords(char *attr_value, int m, int k);
//给一个属性生成codeword
//一个tuple是所有属性的codeword 取or操作
Bits codewords(char *attr_value, int m, int k) {
    int nbits = 0;
    int length_str = strlen(attr_value);
    Bits cword = newBits(m);

    srand(hash_any(attr_value, length_str));
    while (nbits < k) {
        int i = rand() % m;
        if (!bitIsSet(cword, i)) {
            setBit(cword, i);
            nbits++;
        }
    }
    return cword;
}

Bits codewords_catc(char *attr_value, int m, int k, int start, int end) {
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

// make a tuple signature
//一个tuple是所有属性的codeword 取or操作
Bits makeTupleSig(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL);
	//TODO
    int t_m = tsigBits(r);  // tuple cw 长度 t_m
    int t_k = codeBits(r);  // tuple cw 中置为1的个数
    int nattr = nAttrs(r);

    int catc_t_k_1 = (t_m/nattr +  t_m % nattr)/2; // 第一个的tk
    int catc_t_k = t_m/nattr/2;


    Bits desc = newBits(t_m);

    char **attr_val = tupleVals(r, t);

    //CATC的偏移位数
    int shift_distance;

    //如果是SIMC
    if (sigType(r) == 's') {
        for (int i = 0; i < nattr; i++) {
            if (strcmp(attr_val[i], "?") != 0) {
                Bits cw = codewords(attr_val[i], t_m, t_k);
                orBits(desc, cw);
                freeBits(cw);
            }
        }
    }else{
        //如果是CATC
        for (int i = 0; i < nattr; i++) {
            if (strcmp(attr_val[i], "?") != 0) {
                //每次生成m长的，但只set 前t_m / nattr + t_m % nattr位或者t_m / nattr 位
                //然后shift
                if(i == 0){
                    // 比如m = 10,n=3, 第一次set 0 - 3 前4位
                    Bits cw = codewords_catc(attr_val[i], t_m, catc_t_k_1, 0 , (t_m / nattr + t_m % nattr) - 1);
                    orBits(desc, cw);
                    freeBits(cw);
                } else if(i == 1){
                    //第二次set 0-2，前3位，shift 4位
                    Bits cw = codewords_catc(attr_val[i], t_m, catc_t_k, 0 , (t_m / nattr) - 1);
                    shift_distance = t_m / nattr + t_m % nattr;
                    shiftBits(cw, shift_distance);  // 每次shift m/n 位
                    orBits(desc, cw);
                    freeBits(cw);
                }else{
                    //第三次之后set 0-2，前3位，shift 4 + i * 3位
                    Bits cw = codewords_catc(attr_val[i], t_m, catc_t_k, 0 , (t_m / nattr) - 1);
                    shift_distance = (i * t_m / nattr) + t_m % nattr;
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

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)
{
	assert(q != NULL);
	//TODO

    Reln r = q->rel;
    Bits Querysig = makeTupleSig(r, q->qstring);
    int m_of_tuple = tsigBits(r);  // tuple m 长度
    Bits tsig = newBits(m_of_tuple);
    Count number_of_signature_page = nTsigPages(r);  //当前relation r的signature page个数
    int max_tuple_per_page = maxTupsPP(r);  //当前relation r的最大tuple个数每页
    File tuple_sig_file = tsigFile(r);  // 当前relation r的file

    Count sig_pages_read = 0;  //读了多少sig_pages
    int signatures_read = 0;  //读了多少条signatures
    int pageid;

    Bool tsig_matched = FALSE;  // query sig和当前tsig是否匹配

    for (int pid = 0; pid < number_of_signature_page; pid++) {
        Page p = getPage(tuple_sig_file, pid);
        for (int tid = 0; tid < pageNitems(p); tid++) {
            getBits(p, tid, tsig);

            if(isSubset(Querysig, tsig)) {
                tsig_matched = TRUE;  // 匹配
            }else{
                tsig_matched = FALSE;  // 不匹配
            }

            if (tsig_matched == FALSE) {
                signatures_read ++;
            }else{
                pageid = signatures_read / max_tuple_per_page;
                setBit(q->pages, pageid);
                signatures_read ++;
            }
        }
        sig_pages_read ++;
        free(p);
    }
    q->nsigpages = sig_pages_read;
    q->nsigs = signatures_read;
    freeBits(tsig);
    freeBits(Querysig);
}
