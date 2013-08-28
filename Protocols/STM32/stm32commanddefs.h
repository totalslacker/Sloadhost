
/*-----------------------------------------------------------------
 * sloadhost:  interacts with microcontroller boot load protocols
 * Copyright 2009 Alex Faveluke
 *
 * This is free software under the terms of the GNU GPL v3 or later.
 * See the file COPYING for full details.
 -----------------------------------------------------------------*/
/*stm32bootloadcommanddefs.h*/


/*these are codes from the STM32 protocol*/
/*documented by ST application note AN2606 Rev 4*/

#define AUTOBAUDSTARTUP 0x7f
#define STM32ACK 0x79
#define STM32NACK 0x1f

#define STM32GET 0x00
#define STM32GETVERSION 0x01
#define STM32GETID 0x02
#define STM32READMEMORY 0x11
#define STM32GO 0x21
#define STM32WRITEMEMORY 0x31
#define STM32ERASE 0x43
#define STM32WRITEPROTECT 0x63
#define STM32WRITEUNPROTECT 0x73
#define STM32READOUTPROTECT 0x82
#define STM32READOUTUNPROTECT 0x92

#define STM32GETVERSIONBYTESTOREAD 3
#define STM32GETVERSION_VERSIONBYTE 0
#define STM32GETVERSION_RDPROTDISABLEDQTYBYTE 1
#define STM32GETVERSION_RDPROTENABLEDQTYBYTE 2

#define STM32ERASEGLOBALPAGEQTYFIELD 0xff

//these are some behavior setting parameters
//This will be the number of data bytes a read will ask the uC for.
//all heck seems to break loose trying to program if WRITEPACKETMAXDATA is 0x0a!  
//Perhaps their write routine doesn't like addresses not on word boundaries???  
//Things work fine with WRITEPACKETMAXDATA set to 0x10, or 0x80, and probably 
//anything else that's a multiple of 4.  I don't think 0x100 is working, this is 
//possibly a bug in my code.

#define READPACKETMAXDATA 0x80
#define WRITEPACKETMAXDATA 0x80
#define READPACKETREADCALLSMAX 20 

#define STM32FLASHSTART 0x08000000
//COMMANDWAIT is the milliseconds to wait after any command.
//wait timer is implemented in Linux, not yet in Windows version..
//COMMANDRETRYCOUNT is the number of times to try a command before giving up
#define COMMANDWAIT 10 
#define COMMANDRETRYCOUNT 4

#define SENDPACKETWAIT 0
#define PACKETREADWAIT 0

//AUTOBAUDRETRYCOUNT is the number of times to send the autobaud char before giving up.
//AUTOBAUDRETRYWAIT is the milliseconds to wait between autobaud attempts.
#define AUTOBAUDRETRYCOUNT 4
#define AUTOBAUDRETRYWAIT 1000

