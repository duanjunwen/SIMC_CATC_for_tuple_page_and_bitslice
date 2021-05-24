// bits.c ... functions on bit-strings
// part of signature indexed files
// Bit-strings are arbitrarily long byte arrays
// Least significant bits (LSB) are in array[0]
// Most significant bits (MSB) are in array[nbytes-1]

// Written by John Shepherd, March 2019

#include <assert.h>
#include "defs.h"
#include "bits.h"
#include "page.h"
#include <stdbool.h>

typedef struct _BitsRep {
	Count  nbits;		  // how many bits
	Count  nbytes;		  // how many bytes in array
	Byte   bitstring[1];  // array of bytes to hold bits
	                      // actual array size is nbytes
} BitsRep;

// create a new Bits object
//生成一个nbits长度的bit
Bits newBits(int nbits)
{
	Count nbytes = iceil(nbits,8);
	Bits new = malloc(2*sizeof(Count) + nbytes);
	new->nbits = nbits;
	new->nbytes = nbytes;
	memset(&(new->bitstring[0]), 0, nbytes);
	return new;
}

// release memory associated with a Bits object

void freeBits(Bits b)
{
	//TODO
    free(b);
}

// check if the bit at position is 1
//check这个bit的第position位是否为1
Bool bitIsSet(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO

    //&: 二进制“与”and(都为1时，结果是1，否则是0。)，比如:1010 & 1011 = 1010，1010 & 1000 = 1000。
    int byte_index = position / 8;
    int bit_index = position % 8;
    Byte mask = (1 << bit_index);
    Bool is_set;
    int byte_and = (b->bitstring[byte_index] & mask);

    if (byte_and != 0) {
        is_set = TRUE;
        //return TRUE;
    }
    else {
        is_set = FALSE;
        //return FALSE;
    }
    return is_set;
}

// check whether one Bits b1 is a subset of Bits b2

Bool isSubset(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO

	//假设是subset
    Count length = b1->nbytes;
    Bool is_sub_set = TRUE;

    if (b1->nbytes != b2->nbytes){
        is_sub_set = FALSE;
    } else {
        for (int i = 0; i < length; i++) {
            if ((b1->bitstring[i] & b2->bitstring[i]) != b1->bitstring[i]) {
                is_sub_set = FALSE;
                break;
            }
        }
    }
    return is_sub_set;
}

// set the bit at position to 1

void setBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO

    int byte_pos = position / 8;
    int bit_pos = position % 8;
    Byte mask = (1 << bit_pos);
    //按位或后赋值
    //代码示例为：
    //x = 0x02;
    //x  |= 0x01;
    //按位或的结果为：0x03 等同于0011
    b->bitstring[byte_pos] |= mask;
}

// set all bits to 1

void setAllBits(Bits b)
{
	assert(b != NULL);
	//TODO
    for (int i = 0; i < b->nbits; i++) {
        setBit(b,i);
    }


}

// set the bit at position to 0

void unsetBit(Bits b, int position)
{
	assert(b != NULL);
	assert(0 <= position && position < b->nbits);
	//TODO

    int byte_pos = position / 8;
    int bit_pos = position % 8;

    // ~是按位取反
    //a = 1010 1111
    //~a= 0101 0000
    Byte mask = ~(1 << bit_pos);
    b->bitstring[byte_pos] &= mask;
}

// set all bits to 0

void unsetAllBits(Bits b)
{
	assert(b != NULL);
	//TODO

    for (int i = 0; i < b->nbits; i++) {
        unsetBit(b,i);
    }

}

// bitwise AND ... b1 = b1 & b2

void andBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
    Count length = b1->nbytes;
    for (int i = 0; i < length; i++) {
        b1->bitstring[i] &= b2->bitstring[i];
    }
}

// bitwise OR ... b1 = b1 | b2

void orBits(Bits b1, Bits b2)
{
	assert(b1 != NULL && b2 != NULL);
	assert(b1->nbytes == b2->nbytes);
	//TODO
    Count length = b1->nbytes;
    for (int i = 0; i < length; i++) {
        b1->bitstring[i] |= b2->bitstring[i];
    }

}

// left-shift ... b1 = b1 << n
// negative n gives right shift

void shiftBits(Bits b, int n)
{
    // TODO
    int length = b->nbits;  //length of bit
    //printf("%d", length);
    //printf("\n");
    int i = length - 1; //从后往前

    //length是8, n=6
    //7 6 5 4 3 2 1 0
    //check i = 7 到 i = 0
    //0 0 0 0 0 1 1 1
    //1 1 0 0 0 0 0 0

    if (n > 0) {

        while (i >= 0) {
            if (bitIsSet(b, i)) {
                //如果当前这个位为1
                if (i + n < length) {
                    //且他要偏移的位在nbits范围之内
                    setBit(b, i + n);  //偏移的位设为1
                    unsetBit(b, i);  //当前位变为0
                    //printf("i = %d:", i); showBits(b); printf("\n");
                } else {
                    //要偏移的位超出范围
                    unsetBit(b, i);//当前位变为0
                    //printf("i = %d:", i); showBits(b); printf("\n");
                }
            } else {
                //如果当前这个位为0
                //printf("%d notset",i);
                //printf("\n");
                if (i + n < length) {
                    //且他要偏移的位在nbits范围之内
                    unsetBit(b, i + n);  //偏移的位设为0
                    //printf("i = %d:", i); showBits(b); printf("\n");
                } else {
                    //要偏移的位超出范围
                    unsetBit(b, i);//当前位变为0
                    //printf("i = %d:", i); showBits(b); printf("\n");
                }
            }
            i--;
        }
    } else if (n < 0){
        //向右偏移
        //00001111
        //shift -2
        //00000011
        n = -n; // 取反过来
        for (int j = 0; j < length; j++) {
            if (bitIsSet(b, j)) {
                //如果当前这个位为1
                if (j - n >= 0) {
                    //且他要偏移的位在nbits范围之内
                    setBit(b, j - n);  //偏移的位设为1
                    unsetBit(b, j);  //当前位变为0
                    //printf("i = %d:", j); showBits(b); printf("\n");
                } else {
                    //要偏移的位超出范围
                    unsetBit(b, j);//当前位变为0
                    //printf("i = %d:", j); showBits(b); printf("\n");
                }
            }else {
                //如果当前这个位为0
                //printf("%d notset",i);
                //printf("\n");
                if (j - n >= 0) {
                    //且他要偏移的位在nbits范围之内
                    unsetBit(b, j - n);  //偏移的位设为0
                    //printf("i = %d:", j); showBits(b); printf("\n");
                } else {
                    //要偏移的位超出范围
                    unsetBit(b, j);//当前位变为0
                    //printf("i = %d:", j); showBits(b); printf("\n");
                }
            }
        }
    }
}

// get a bit-string (of length b->nbytes)
// from specified position in Page buffer
// and place it in a BitsRep structure

void getBits(Page p, Offset pos, Bits b)
{
	//TODO
    Count length = b->nbytes;
    Byte *start = addrInPage(p, pos, length);
    memcpy(b->bitstring, start, length);
}

// copy the bit-string array in a BitsRep
// structure to specified position in Page buffer

void putBits(Page p, Offset pos, Bits b)
{
	//TODO
    Count length = b->nbytes;
    Byte *start = addrInPage(p, pos, length);
    memcpy(start, b->bitstring, length);
}

// show Bits on stdout
// display in order MSB to LSB
// do not append '\n'

void showBits(Bits b)
{
	assert(b != NULL);
    //printf("(%d,%d)",b->nbits,b->nbytes);
	for (int i = b->nbytes-1; i >= 0; i--) {
		for (int j = 7; j >= 0; j--) {
			Byte mask = (1 << j);
			if (b->bitstring[i] & mask)
				putchar('1');
			else
				putchar('0');
		}
	}
}
