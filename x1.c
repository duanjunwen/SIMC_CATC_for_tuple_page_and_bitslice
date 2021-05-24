// Test Bits ADT

#include <stdio.h>
#include "defs.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"

int main(int argc, char **argv)
{
    Bits a = newBits(6);
    setBit(a,0);
    setBit(a,2);
    showBits(a); printf("\n");

    Bits b = newBits(6);
    setBit(b,0);
    setBit(b,1);
    showBits(b); printf("\n");

    Bits c = newBits(6);
    setBit(c,1);
    setBit(c,2);
    showBits(c); printf("\n");

    orBits(a,b);
    orBits(a,c);
    showBits(a); printf("\n");

    /*
    shiftBits(a,6);
    showBits(a); printf("\n");
    showBits(a); printf("\n");



	Bits b = newBits(60);

	printf("t=0: "); showBits(b); printf("\n");
	setBit(b, 5);
	printf("t=1: "); showBits(b); printf("\n");
	setBit(b, 0);
	setBit(b, 50);
	setBit(b, 59);
	printf("t=2: "); showBits(b); printf("\n");
	if (bitIsSet(b,5)) printf("Bit 5 is set\n");
	if (bitIsSet(b,10)) printf("Bit 10 is set\n");
	setAllBits(b);
	printf("t=3: "); showBits(b); printf("\n");
	unsetBit(b, 40);
	printf("t=4: "); showBits(b); printf("\n");
	if (bitIsSet(b,20)) printf("Bit 20 is set\n");
	if (bitIsSet(b,40)) printf("Bit 40 is set\n");
	if (bitIsSet(b,50)) printf("Bit 50 is set\n");
	setBit(b, 59);
	return 0;
	 */
}
