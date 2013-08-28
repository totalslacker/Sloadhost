
/*-----------------------------------------------------------------
 * byterx: takes binary bytes on stdin, outputs ascii coded hex stdout.
 * Copyright 2009 Alex Faveluke
 *
 *Copying and distribution of this file, with or without modification,
 *are permitted in any medium without royalty provided the copyright
 *notice and this notice are preserved.  This file is offered as-is,
 *without warranty of any kind.
 -----------------------------------------------------------------*/


/*you probably want to run it as cat testfifo | byterx*/

#define _GNU_SOURCE
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

int main(void)
{
unsigned int c;

while(1) {
	c = getchar();
	if (c==EOF) break; 
	printf("got byte: 0x%.2x\n",(int) c);
	fflush(stdout);
}
return(0);
}
