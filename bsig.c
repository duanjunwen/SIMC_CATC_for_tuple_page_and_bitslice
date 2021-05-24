// bsig.c ... functions on Tuple Signatures (bsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"
#include "bits.h"

void findPagesUsingBitSlices(Query q)
{
	assert(q != NULL);
	//TODO


    Reln r = q->rel;
    //Tuple t = q->qstring;  // query对应的tuple
    Bits qsig = makePageSig(r, q->qstring);  //生成要查询的page signature
    int b_m = bsigBits(r);  // bsig 长度
    Bits bsig = newBits(b_m);  //
    Count max_bsig_per_page = maxBsigsPP(r);  //bsig page
    //Page curr_bsig_page;  //当前的bsig page
    unsigned int pid;
    unsigned int curr_bsig_position;
    unsigned int flag = -1;
    Bool qsig_bit_is_set = FALSE;

    setAllBits(q->pages);  //假设所有page都是结果 set all 1
    // iterate all the pages(bits page size)
    // 遍历 query 长度 m，或者说 bsig page 总 行数
    for (int i = 0; i < psigBits(r); i++) {

        if (bitIsSet(qsig, i)) {
            qsig_bit_is_set = TRUE;
        }else{
            qsig_bit_is_set = FALSE;
        }

        if (qsig_bit_is_set == FALSE){
            continue;
        }else{
            pid = i / max_bsig_per_page;  // bsig page 位置
            curr_bsig_position = i % max_bsig_per_page;  // 当前page 的 bsig 位置
            Page curr_bsig_page = getPage(r->bsigf, pid);  //读取当前bsig page
            getBits(curr_bsig_page, curr_bsig_position, bsig);
            // 取模得到横着的tuple对应位置，
            // 读取对应位置的bsig(横着的)
            if (pid != flag) {  // 如果往后读了一页
                flag = pid;
                q->nsigpages++;
            }
            // 查找 number of page signatures (psigs)
            // 用page 个数去遍历横着的bsig的每一位
            for (int j = 0; j < nPsigs(r); j++) {
                if (!bitIsSet(bsig, j)) {
                    unsetBit(q->pages, j);
                }
            }
            // the number of signature we scanned
            q->nsigs++;
            free(curr_bsig_page); // free page
        }
    }
    freeBits(qsig);
    freeBits(bsig);
}

