
/*-----------------------------------------------------------------
 * sloadhost:  interacts with microcontroller boot load protocols
 * Copyright 2009, 2011 Alex Faveluke
 *
 * This is free software under the terms of the GNU GPL v3 or later.
 * See the file COPYING for full details.
 -----------------------------------------------------------------*/
//version 0.14.. added --dtrrtsconfigboot option

#define SLOADHOSTVERSIONSTRING "sloadhost v0.14"
#define OPTARGSTRINGLENMAX 255
#define MAXPACKLEN 512
#define MAXSECTORSINLIST 256

//MAXPACKLEN defines the maximum length of this programs internal packet storage.
//it needs to be longer than the longest data+command packet sent to target
//or packet read from target. 
//the maximum target packet lengths are defined in stm32commandsdefs.h 
//if you're enough of a he-man to use more than 256 characters in an option 
//parameter, you can increase OPTARGSTRINGLENMAX and recompile this host.  

typedef int flag;

struct hostoptionsstructure {
	char serialdevname[OPTARGSTRINGLENMAX+1];
	char loadfilename[OPTARGSTRINGLENMAX+1];
	char savefilename[OPTARGSTRINGLENMAX+1];
	char targetaddressstring[OPTARGSTRINGLENMAX+1];
	char jumpaddressstring[OPTARGSTRINGLENMAX+1];
	char baudratestring[OPTARGSTRINGLENMAX+1];
	char bytestoreadstring[OPTARGSTRINGLENMAX+1];
	char pagestoerasestring[OPTARGSTRINGLENMAX+1];
	char writeprotectsectorsstring[OPTARGSTRINGLENMAX+1];
	char testparamstring[OPTARGSTRINGLENMAX+1];
	unsigned int baudrate;

	flag go_request;
	flag globalerase_request;
	flag pageserase_request;
	flag program_request;
	flag read_request;
	flag set_readprotect_request;
	flag clear_readprotect_request;
	flag set_writeprotect_request;
	flag clear_writeprotect_request;
	flag getcommands_request;
	flag getversion_request;
	flag getid_request;
	flag skipautobaud_request;
	flag dtrrtsconfigboot_request; 
	flag test_request;

	flag help_request;
	flag version_request;
};

typedef struct hostoptionsstructure Hostoptsstruct;	

struct hostresourcesstructure {
	int inputfile_fd;
	int outputfile_fd;
};
typedef struct hostresourcesstructure Hostresourcesstruct;

struct targoptionsstructure {
	long long unsigned int start_address;
	long long unsigned int bytestoread;
	long long unsigned int jump_address;
	int pagestoerase;
};

typedef struct targoptionsstructure Targoptsstruct;


struct packetstoragestructure {
	unsigned char storage[MAXPACKLEN];
	int length;
};

typedef struct packetstoragestructure Packet;

