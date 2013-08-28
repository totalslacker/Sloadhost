
/*-----------------------------------------------------------------
 * bytesend: takes text-encoded hex input, outputs binary bytes
 * Copyright 2009 Alex Faveluke
 *
 *Copying and distribution of this file, with or without modification,
 *are permitted in any medium without royalty provided the copyright
 *notice and this notice are preserved.  This file is offered as-is,
 *without warranty of any kind. 
 -----------------------------------------------------------------*/

/*you probably want to redirect it to a file or fifo.*/
/*this utility expects bytes in hex format..
give it one byte per line.   
0x01, 0x02... 0x0a..  actually, you can get away
without the 0x formating code  because the base 
16 is specified in strtol.  */

/*to read bytes from a file... use od -t x1 -Ax filename */
/*or use the handy byterx... cat file | ./byterx */


#define _GNU_SOURCE
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

int main(void)
{
unsigned int c;
char linebuf[256];
char *linebufptr;
//bzero(linebuf, 256);
linebuf[0] = '\0';

while (1)  {
	linebufptr = fgets(linebuf, 256, stdin);
	if (linebufptr == NULL) break;
//	fprintf(stderr, "got line of length %i :", (int) strlen(linebuf));
	//fprintf(stderr, "%s\n", linebuf);
	c = (unsigned char) strtol(linebuf, NULL, 16);
	//fprintf(stderr, "converts to 0x%.2x \n", c);
	putchar(c);
	fflush(stdout);
}
return(0);
}
