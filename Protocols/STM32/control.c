/*
 *  sloadhost:  interacts with microcontroller boot load protocols.
 *  Copyright 2009, 2011 Alex Faveluke
 *  portions from aducprog Copyright 2002, 2005 Alex Faveluke
 *  http://www.faveluke.com					
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *------------------------------------------------------------------------*/

//#define DUMPFLAGSTOSCREEN 1
#undef DUMPFLAGSTOSCREEN

/*control.c*/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>  
#include "actions.h"
#include "hostsystem.h"

#ifdef WINDOWSHOST
#include <fcntl.h>
#endif

#include "../../Common/serial.h" //only for serial_init(); 
#include "globaldecs.h"


#ifdef WINDOWSHOST
/*This is necessary in the mingw wrapping of the windows API open and write
 * calls to make sure the files are accessed binary.  I couldn't get it to work
 * any other way.  Without this special variable set in main context, the files
 * written to disk in a "read memory" always had an 0x0d (carriage return)
 * inserted in front of any 0x10 characters in the actual read memory
 * The compiler I'm using for Windows reports itself as being:
 * gcc.exe (GCC) 3.4.2 (mingw-special)
 */
unsigned int _CRT_fmode = _O_BINARY;
#endif

void hostopts_init(void);
void argtoolongexit(void);





Hostoptsstruct hostopts;
Targoptsstruct targopts;
Hostresourcesstruct hostresources;

int main (int argc, char **argv)
{
hostopts_init();
targopts_init();





while (1) {
	
    int optionseen;  
    static struct option long_options[] =
	{
		{"help", no_argument, &hostopts.help_request, 1},
		{"version", no_argument, &hostopts.version_request, 1},

		/*action requesting options--------------------------------------*/
		{"globalerase", no_argument, &hostopts.globalerase_request,1},


		{"setreadprotect", no_argument, &hostopts.set_readprotect_request, 1},
		{"clearreadprotect", no_argument, &hostopts.clear_readprotect_request, 1},
		
		{"clearwriteprotect", no_argument, &hostopts.clear_writeprotect_request, 1},
		
		{"get", no_argument, &hostopts.getcommands_request, 1},
		{"getversion", no_argument, &hostopts.getversion_request, 1},
		{"getid", no_argument, &hostopts.getid_request, 1},
		{"skipautobaud", no_argument, &hostopts.skipautobaud_request, 1},
		{"dtrrtsconfigboot", no_argument, &hostopts.dtrrtsconfigboot_request, 1},

		

		/*---------------------------------------------------------------*/

		/*resource and filename options----------------------------------*/
		{"serialdev", required_argument, 0, 'd'},
		{"loadfile", required_argument, 0, 'f'},
		{"savefile", required_argument, 0, 's'},
	    {"baudrate", required_argument, 0, 'b'},	
		{"bytestoread", required_argument, 0, 'q'},

		{"setwriteprotect", required_argument, 0, 'z'},
		{"pageserase", required_argument, 0, 'p'},
		{"go", required_argument, 0, 'g'},
		{"write", required_argument, 0, 'w'},
		{"read", required_argument, 0, 'r'},
		{"test", required_argument, 0, 't'}, 
		

		/*---------------------------------------------------------------*/ 
		{0,0,0,0}	//special terminating option
	};

    int option_index = 0;	
    
    optionseen = getopt_long (argc, argv, "d:f:m:a:", long_options, &option_index);

    if (optionseen == -1) {
	  //printf("DEBUG:  end of options parsed\n");
	  break;
	}
    
     switch (optionseen)
	{
	case 0 :   //what is going on here?
	   if (long_options[option_index].flag != 0) {
		//printf("DEBUG, caught case 0 in switch at %i, flag not 0\n",__LINE__);
		break;
	   }
	   printf ("option %s", long_options[option_index].name);
	   if (optarg)
		printf(" with arg %s",optarg);
	   printf("\n");
	   break;

	case 'd' :
	  	if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.serialdevname,optarg);
	  break;

	case 'f' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.loadfilename,optarg);	
	  break;

	case 's' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.savefilename,optarg);	
	  break;	

	case 'a' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.targetaddressstring, optarg);	
          break;

	case 'b' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.baudratestring, optarg);	
          break;
	
	case 'q' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		strcpy(hostopts.bytestoreadstring, optarg);
          break;

	case 'p' :   //p means "page erase"
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.pageserase_request = 1;
		strcpy(hostopts.pagestoerasestring, optarg);
          break;

	case 'z' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.set_writeprotect_request = 1;
		strcpy(hostopts.writeprotectsectorsstring, optarg);
          break;

	case 'g' :
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.go_request = 1;
		strcpy(hostopts.jumpaddressstring, optarg);
          break;

	case 'w' : 
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.program_request = 1;
		strcpy(hostopts.targetaddressstring, optarg);
          break;
	
	case 'r' : 
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.read_request= 1;
		strcpy(hostopts.targetaddressstring, optarg);
	  break;

	case 't' :   //easy hook for test
		if (strlen(optarg) > OPTARGSTRINGLENMAX) argtoolongexit();
		hostopts.test_request= 1;
		strcpy(hostopts.testparamstring, optarg);
	  break;


	default:
	   printf("\nerror in command line options, exiting.\n");
	   exit(1);
	}	//end case
    }	//end options parsing while
		

#ifdef DUMPFLAGSTOSCREEN
//flag check
printf("\n~~~~~~~~~~flags are~~~~~~~~~~~\n");
printf("           go_request = %i\n", hostopts.go_request);
printf("  getcommands_request = %i\n", hostopts.getcommands_request);
printf("   getversion_request = %i\n", hostopts.getversion_request);
printf("        getid_request = %i\n", hostopts.getid_request);
printf("        globalerase_request = %i\n", hostopts.globalerase_request);
printf("      program_request = %i\n", hostopts.program_request);
printf("         read_request = %i\n", hostopts.read_request);
printf("  set_writeprotect_request = %i\n", hostopts.set_writeprotect_request);
printf("clear_writeprotect_request = %i\n", hostopts.clear_writeprotect_request);
printf("  set_readprotect_request = %i\n", hostopts.set_readprotect_request);
printf("clear_readprotect_request = %i\n", hostopts.clear_readprotect_request);
printf("test_request = %i\n", hostopts.test_request);
printf("dtrrtsconfigboot_request = %i\n", hostopts.dtrrtsconfigboot_request);

printf("have serial device name %s\n",hostopts.serialdevname);
printf("have loadfile name %s\n",hostopts.loadfilename);
printf("have savefilename %s\n",hostopts.savefilename);
printf("have targetaddressstring %s\n",hostopts.targetaddressstring);
printf("have baudratestring %s\n",hostopts.baudratestring);
printf("have bytestoreadstring %s\n",hostopts.bytestoreadstring);
printf("have pagestoerasestring %s\n", hostopts.pagestoerasestring);
printf("have writeprotectsectorsstring %s\n", hostopts.writeprotectsectorsstring);
printf("have testparamstring %s\n", hostopts.testparamstring);
printf("have jumpaddressstring %s\n",hostopts.jumpaddressstring);
#endif


if (hostopts.version_request) {
	printf("\nThis is sloadhost-stm32, part of %s\n\n", SLOADHOSTVERSIONSTRING);
	printf("Sloadhost is Free Software licensed under the GPL v3 or later.\n");
	printf("See the file COPYING for details.\n");
	exit(0);
}


if (hostopts.help_request) {
	printf("\nThis is sloadhost-stm32, part of %s\n\n", SLOADHOSTVERSIONSTRING);
	printf("Sloadhost is Free Software licensed under the GPL v3 or later.\n");
	printf("See the file COPYING for details.\n\n");
		
	printf("Host side downloader for STM32 USART bootloader protocol\n");
	printf("downloadable file format is straight binary\n");
	printf("\n\n");
	printf("==============================================================\n");
	printf("  Note:  Numerical parameters are interpreted in decimal\n");
	printf("            unless the base code is preappended.\n");
	printf(" 	\"11\" means eleven.  \"0x11\" means seventeen.\n");
	printf("==============================================================\n\n\n");
printf("options:\n");
	printf("--go <address> ...................jump to address and run\n");
	printf("--globalerase    .................globalerase target memory\n");
	printf("--pageserase <pagelist> ...........erase one or more pages\n");
	printf("  List is comma delimited, no spaces.\n\n");

	printf("--write <address>  ...............write data starting at <address> \n");
	printf("  (--write requires the --loadfile option to be set)\n\n");
	printf("--read  <address> ................read target memory\n");
	printf("  (--read requires the --savefile option to be set)\n\n");
	printf("--setwriteprotect <sectorlist> ...set write protection\n");
	printf("  on 1 or more sectors.  List is comma delimited, no spaces.\n\n");
	printf("--clearwriteprotect ..............clear write protection on memory\n");
	printf("--setreadprotect .................set read protection on target memory\n");
	printf("--clearreadprotect ...............clear read protection on memory\n");
	printf("--get .....................get target version and allowed commands\n");
	printf("--getversion ..............get target version and read protection status\n");
	printf("--getid ..........................get the chip ID\n");
	printf("--skipautobaud ...................do not send the autobaud sequence\n");
	printf("--dtrrtsconfigboot ...............config bootmode, reset with dtr and rts\n");


	printf("\n");
	printf("--loadfile <loadfilename>  ........set file to load <loadfilename>\n");
	printf("--savefile <savefilename>  ........set file to save <savefilename>\n");	
	printf("\n");
	printf("--bytestoread <numberofbytes> .....number of bytes to read\n");
	printf("--serialdev <serialportname> ......set serial port device to <serialportname>\n");
	printf("--baudrate <baudrate> .............set baud rate to <baudrate>\n");
	exit(0);
}  //end if helprequested

hostopts.baudrate = strtol(hostopts.baudratestring, NULL, 0);
targopts.start_address = strtoll(hostopts.targetaddressstring, NULL, 0);
targopts.bytestoread = strtoll(hostopts.bytestoreadstring, NULL, 0);
targopts.jump_address = strtoll(hostopts.jumpaddressstring, NULL, 0);

//printf("DEBUG:  have bytestoread = %lli\n",targopts.bytestoread);
//printf("DEBUG:  have jump_address = 0x%08llx\n", targopts.jump_address);

//got to open serial for anything useful here... 
if (serial_init(hostopts.serialdevname, hostopts.baudrate)) {
	printf("serial_init on %s failed, exiting\n", hostopts.serialdevname);
	exit(1);
}

if (hostopts.dtrrtsconfigboot_request) {
	setdtr();		//pull BOOT0 high (DTR does NOT go thru inverting buffer)
	setrts();		//pull RESET low (RTS goes through inverting buffer)
	clrrts();		//deassert reset, leave boot0 high
}

printf("checking autobaudstart\n");
if (!hostopts.skipautobaud_request) 
	autobaudstart();		//start communications

printf("checking getcommands_request\n");
if (hostopts.getcommands_request) get_commands();
printf("checking getversion_request\n");
if (hostopts.getversion_request) get_version();
printf("checking getid_request\n");
if (hostopts.getid_request) get_id();

printf("checking protect clear requests\n");
if (hostopts.clear_writeprotect_request) clear_writeprotect();
if (hostopts.clear_readprotect_request) clear_readprotect();

printf("checking globalerase_request\n");
if (hostopts.globalerase_request) globalerase_flash(); 

printf("checking pageerase_request\n");
if(hostopts.pageserase_request) pageserase_flash(); 

printf("checking program_request\n");
if (hostopts.program_request) program_memory();
printf("checking read_request\n");
if (hostopts.read_request) read_memory();

printf("checking protect set requests\n");
if (hostopts.set_writeprotect_request) set_writeprotect();
if (hostopts.set_readprotect_request) set_readprotect();

if (hostopts.test_request) testfunction();

printf("checking go_request\n");
if (hostopts.go_request) go_jump();

printf("checking dtrrtsconfigboot_request\n");
if (hostopts.dtrrtsconfigboot_request) {
	clrdtr();		//pull BOOT0 low (DTR does NOT go thru inverting buffer)
	setrts();		//pull chips reset line low (RTS goes through inverting buffer)
	clrrts();		//deassert reset, leave boot0 high
							//reset brings into normal run mode, jump to 0x08000000
}

printf("checking go_request\n");
if (hostopts.go_request) go_jump();
 
return(0);  //made it through everything ..Yay!
}   //end main.

void argtoolongexit(void)
{
  printf("sorry, option argument longer than %i, exiting.\n\n", OPTARGSTRINGLENMAX);
  exit(1);
}

void hostopts_init(void)
{
hostopts.getcommands_request = 0;
hostopts.getversion_request = 0;
hostopts.getid_request = 0;
hostopts.globalerase_request = 0;
hostopts.pageserase_request = 0;
hostopts.program_request = 0;
hostopts.read_request = 0;
hostopts.set_readprotect_request = 0;
hostopts.clear_readprotect_request = 0;
hostopts.set_writeprotect_request = 0;
hostopts.clear_writeprotect_request = 0;
hostopts.help_request = 0;
hostopts.skipautobaud_request = 0;
hostopts.dtrrtsconfigboot_request = 0;


hostopts.serialdevname[0] = '\0';
hostopts.loadfilename[0] = '\0';
hostopts.pagestoerasestring[0] = '\0';
hostopts.jumpaddressstring[0] = '\0';

strcpy (hostopts.jumpaddressstring, "0xdeadbeef"); //testing here.
strcpy (hostopts.baudratestring, "57600");
strcpy (hostopts.targetaddressstring, "0x08000000");
strcpy (hostopts.bytestoreadstring, "1024");
return;
}


