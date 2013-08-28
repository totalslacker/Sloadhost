
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

/*actions.c*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>  
#include "actions.h"
#include "hostsystem.h"
#ifdef WINDOWSHOST
#include <sys/time.h>
#endif
#include "serial.h"  //prototypes for serial functions
#include "globaldecs.h"
#include "stm32commanddefs.h"

//global storage.. defined in control file
extern Hostoptsstruct hostopts;
extern Targoptsstruct targopts;
extern Hostresourcesstruct hostresources;
  

//functions local to this file

void packet_append(Packet * packet, unsigned char c);
void packet_checksumappend(Packet * packet);
void packet_zero(Packet * packet);
void packet_appendcommandandcheck(Packet * packet, unsigned char command);
int  packet_sendandgetack(Packet * packet, int trycount);
int stringlisttobytes(unsigned char * list, char * stringlist);

void packet_send(Packet * packet);
void packet_read(Packet * packet, int bytestoread);
void targopts_init(void);
void waitms(int mstowait);


void autobaudstart(void)
{
int trycount=0;
unsigned char receivedbyte;
flag autobaudestablished=0;
while (1) {
   serial_write1byte(AUTOBAUDSTARTUP);
   waitms(COMMANDWAIT);
   receivedbyte = serial_read1byte();
   if (receivedbyte == STM32ACK) {
	printf("Got autobaud ack in autobaudstart()\n");
	autobaudestablished=1;
	break;
   }
   if (receivedbyte == STM32NACK) {
	printf("Got NACK from STM32 in autobaudstart()\n");
	printf("Autobaud was already likely established\n");
	autobaudestablished=1;
	break;
   }

        waitms(AUTOBAUDRETRYWAIT);
   if (++trycount > AUTOBAUDRETRYCOUNT) {
	printf("Made %i tries in autobaudstart w/o ack, exiting\n", trycount);
	exit(1); 
   }
}
}

void clear_writeprotect(void)
{
Packet packet;

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32WRITEUNPROTECT);
if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending writeunprotect command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}
printf("got first ACK from writeunprotect.. good\n");
packet_zero(&packet);
if (packet_sendandgetack(&packet,1)) {
	printf("didn't get second ack from writeunprotect, exiting\n");
	exit(1);
}
printf("got second ACK from writeunprotect, assumed successful\n");
return;
}

void set_writeprotect(void)  //prototype of using the packet utility functions..
{
Packet packet;
int protectsectorcount;
int i;
unsigned char sectorlist[MAXSECTORSINLIST-1];


if(hostopts.writeprotectsectorsstring[0] == '\0') { //shouldn't be possible to hit this
	printf("setwriteprotect requires a list of 1 or more sectors \n \
--setwriteprotect 1,2,3,...N.  Exiting\n");
	exit(1);
}

protectsectorcount = stringlisttobytes(sectorlist, hostopts.writeprotectsectorsstring);

printf("sending write protect command for sectors ");
for(i=0;i<protectsectorcount;i++) 
	printf("0x%.2x%c",sectorlist[i], i < protectsectorcount-1 ? ',' : '\n');

if (protectsectorcount <= 0) {
	printf("got zero or error parsing list of sectors to write protect, exiting\n");
	exit(1);
}

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32WRITEPROTECT);

if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending writeprotect command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}

packet_zero(&packet);
packet_append(&packet, (unsigned char)(protectsectorcount-1));
for(i=0;i<protectsectorcount;i++) 
	packet_append(&packet, sectorlist[i]);
packet_checksumappend(&packet);

if (packet_sendandgetack(&packet, 1)) {
	printf("didn't get ACK after sending writeprotect sector list, exiting\n");
	exit(1);
}
return;
}

void clear_readprotect(void)
{
Packet packet;

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32READOUTUNPROTECT);
if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending readunprotect command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}
printf("got first ACK from writeunprotect.. good\n");

packet_zero(&packet);
if (packet_sendandgetack(&packet, 1)) {
	printf("didn't get second ack from readoutunprotect, exiting\n");
	exit(1);
}
printf("got second ACK from readoutunprotect, assumed successful\n");
return;
}

void set_readprotect(void)
{
Packet packet;

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32READOUTPROTECT);
if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending readoutprotect command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}
printf("got first ACK from readoutprotect.. good\n");

packet_zero(&packet);
if (packet_sendandgetack(&packet, 1)) {
	printf("didn't get second ack from readoutprotect, exiting\n");
	exit(1);
}
printf("got second ACK from readoutprotect, assumed successful\n");
return;
}

void globalerase_flash(void)
{
Packet packet;

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32ERASE);
if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending erase command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}

packet_zero(&packet);

packet_append(&packet, STM32ERASEGLOBALPAGEQTYFIELD);
packet_append(&packet, (unsigned char) ~STM32ERASEGLOBALPAGEQTYFIELD); 
if (packet_sendandgetack(&packet, 1)) {
	printf("didn't get second ack from globalerase, exiting\n");
	exit(1);
}
printf("got second ACK from globalerase, assumed successful\n");
return;
}

void pageserase_flash(void) 
{
Packet packet;
int erasepagecount;
int i;
unsigned char pagelist[MAXSECTORSINLIST-1];


if(hostopts.pagestoerasestring[0] == '\0') { //shouldn't be possible to hit this
	printf("pageerase requires a list of 1 or more sectors \n \
--pagerase 1,2,3,...N.  Exiting\n");
	exit(1);
}

erasepagecount = stringlisttobytes(pagelist, hostopts.pagestoerasestring);

printf("sending erase command for pages ");
for(i=0;i<erasepagecount;i++) 
	printf("0x%.2x%c",pagelist[i], i < erasepagecount-1 ? ',' : '\n');

if (erasepagecount <= 0) {
	printf("got zero or error parsing list of pagestoerase, exiting\n");
	exit(1);
}

packet_zero(&packet);
packet_appendcommandandcheck(&packet, STM32ERASE);

if (packet_sendandgetack(&packet, COMMANDRETRYCOUNT)) {
	printf("tried sending erase command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}

packet_zero(&packet);
packet_append(&packet, (unsigned char)(erasepagecount-1));
for(i=0;i<erasepagecount;i++) 
	packet_append(&packet, pagelist[i]);
packet_checksumappend(&packet);

if (packet_sendandgetack(&packet, 1)) {
	printf("didn't get ACK after sending erase page list, exiting\n");
	exit(1);
}
printf("got final ACK from pageserase, assumed successful\n");
return;
}

void program_memory(void)
{
Packet datapacket; //for data
Packet outpacket; //for command
unsigned long long address;
flag hitEOFinloadfile = 0;
int packsendtrycount;
int bytestosendthispacket;
int bytessenttotal = 0;

printf("... starting program_memory()\n");
address = targopts.start_address;  //starting address
 
if (hostopts.loadfilename[0] == '\0') {
	printf("program_memory needs --loadfile <filename> defined, exiting\n");
	exit(1);
}	

#ifdef WINDOWSHOST
hostresources.inputfile_fd = open (hostopts.loadfilename, O_RDONLY | O_BINARY);
#else
hostresources.inputfile_fd = open (hostopts.loadfilename, O_RDONLY);
#endif
  if (hostresources.inputfile_fd < 0) {
	perror("open inputfile failed, exiting\n");
	exit(1);
  }

while (!hitEOFinloadfile) {
     
   bytestosendthispacket = read(hostresources.inputfile_fd, &datapacket.storage, WRITEPACKETMAXDATA);
   if (bytestosendthispacket < 0) {
		perror("trouble reading loadfile in program_memory!, exiting");
		exit(1);
   }

   if (bytestosendthispacket == 0) { 
	printf("bytestosendthispacket is 0, EOF in loadfile\n"); 
	hitEOFinloadfile = 1;
	break;	//avoid sending 0 length write packet.
   }

   datapacket.length = bytestosendthispacket;

			//now send the command packet.
   packet_zero(&outpacket);

packet_appendcommandandcheck(&outpacket, STM32WRITEMEMORY);

if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
	printf("tried sending writememory command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
	exit(1);
}


	//send address
	packet_zero(&outpacket);
	packet_append(&outpacket, ((address & 0xff000000) >> 24));
	packet_append(&outpacket, ((address & 0x00ff0000) >> 16));
	packet_append(&outpacket, ((address & 0x0000ff00) >> 8));
	packet_append(&outpacket, (address & 0x000000ff));
	packet_checksumappend(&outpacket);
	packsendtrycount = 0;


if (packet_sendandgetack(&outpacket, 1)) {
	printf("didn't get ack after address+checksum in program_memory exiting\n");
	exit(1);
}


	packet_zero(&outpacket);
	packet_append(&outpacket, (unsigned char) bytestosendthispacket-1); //number of bytes to write
	memcpy((outpacket.storage + (size_t) 1),datapacket.storage, bytestosendthispacket);
	outpacket.length += bytestosendthispacket;
	packet_checksumappend(&outpacket);

	packet_send(&outpacket);
	if (serial_read1byte() != STM32ACK) {
	   printf("didn't get ack after length,data,checksum in program_memory exiting\n");
	   exit(1);
	}
	bytessenttotal+=bytestosendthispacket;
	address+=bytestosendthispacket;		//address now at next memory location to write.
}  //end while !hitEOFinreadfile

printf("sent %i bytes total in program_memory()\n",bytessenttotal);
close(hostresources.inputfile_fd);
return;
}

void read_memory(void)
{
int byteslefttotal;
int bytestoreadthispacket;
int byteswritten;
//int packsendtrycount;
unsigned long long address;
Packet inpacket;
Packet outpacket;


address = targopts.start_address;  //starting address


if (hostopts.savefilename[0] == '\0') {
	printf("read_memory needs --savefile <filename> defined, exiting\n");
	exit(1);
}	

#ifdef WINDOWSHOST
//mingw or the windows API is adding a carriage return
//before every linefeed!  So I need to use an O_BINARY flag!
//But this didn't work!  I needed to resort to setting 
//unsigned int _CRT_fmode = _O_BINARY in the file where main() is defined.
//yuck.  Anyone knows the proper way to do this, plese let me know!!!
hostresources.outputfile_fd = open (hostopts.savefilename, 
	O_WRONLY | O_TRUNC | O_CREAT,
	S_IRUSR | S_IWUSR | O_BINARY);
#else

hostresources.outputfile_fd = open (hostopts.savefilename, 
	O_WRONLY | O_TRUNC | O_CREAT,
	S_IRUSR | S_IWUSR);
#endif


  if (hostresources.outputfile_fd < 0) {
	perror("open outputfile failed in read_memory, exiting\n");
	exit(1);
  }
//opened file to save to okay, now send read command..

byteslefttotal = targopts.bytestoread;
while(byteslefttotal>0) {

	if(byteslefttotal > READPACKETMAXDATA) 		//chose a number of bytes to read
		bytestoreadthispacket = READPACKETMAXDATA;
	else
		bytestoreadthispacket = byteslefttotal;

				     //send the read command sequence.
	packet_zero(&outpacket);

	packet_appendcommandandcheck(&outpacket, STM32READMEMORY);

	if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
		printf("tried sending readmemory command %i times w/o ack, exiting\n", COMMANDRETRYCOUNT);
		exit(1);
	}

	//send address
	packet_zero(&outpacket);
	packet_append(&outpacket, ((address & 0xff000000) >> 24));
	packet_append(&outpacket, ((address & 0x00ff0000) >> 16));
	packet_append(&outpacket, ((address & 0x0000ff00) >> 8));
	packet_append(&outpacket, (address & 0x000000ff));
	packet_checksumappend(&outpacket);

	if (packet_sendandgetack(&outpacket, 1)) {
		printf("didn't get ack after address+checksum in read_memory exiting\n");
	exit(1);
	}

	//send bytestoread
	packet_zero(&outpacket);
	if (bytestoreadthispacket <= 0) {
		printf("BUG... bytestoreadthispacket =%i in read command loop, exiting\n",bytestoreadthispacket);  //not likely, but could be a two weeker to debug if
			 //we start sending "negative" values to read.
		exit(1);
	}
	packet_append(&outpacket, (unsigned char)(bytestoreadthispacket - 1));
	packet_append(&outpacket,  ~((unsigned char) (bytestoreadthispacket -1)));
	packet_send(&outpacket);
	if (serial_read1byte() != STM32ACK) {
	   printf("didn't get ack after byteqty+checksum in read_memory exiting\n");
	   exit(1);
	}

	//now get the packet and write it to the savefile!
	packet_zero(&inpacket);
	packet_read(&inpacket, bytestoreadthispacket);

	byteswritten = write(hostresources.outputfile_fd, inpacket.storage, inpacket.length);
	if (byteswritten != inpacket.length)	{  
		//it's very lazy to not have this in loop.. will it cause 
		//problems in some environments?   Is there a way to set write to 
		//block until all bytes written or timeout? how to test?.
		perror("didn't write requested bytes!\n");
		printf("didn't write requested %i bytes to output file, exiting", inpacket.length);
		exit(1);
	}

	byteslefttotal -= byteswritten;
	address += byteswritten;
		
	
} //end while (byteslefttotal > 0)

if (close(hostresources.outputfile_fd)) {
	perror("error closing outputfile after serial_read.. wierd!, exiting\n");
	exit(1);
}

return;
}

void get_commands(void)
{
Packet outpacket, inpacket;
int packsendtrycount=0;
int bytestoreadfromtarget;
int bytesactuallyread=0;
unsigned char * packptr;
int i;


packet_zero(&outpacket);
packet_zero(&inpacket);
//build up the command packet



packet_appendcommandandcheck(&outpacket, STM32GET);

if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
	printf("tried get command %i times w/o ack, exiting\n",packsendtrycount);
	exit(1);
}


//now get the number of bytes the target says it's going to send us.
bytestoreadfromtarget = (int)serial_read1byte() + 1; 

packptr = inpacket.storage;
while (1) {
   bytesactuallyread = serial_read(packptr, bytestoreadfromtarget);
   inpacket.length+=bytesactuallyread;
   packptr += (size_t)bytesactuallyread;
   bytestoreadfromtarget -= bytesactuallyread;
   if (bytestoreadfromtarget == 0)  break;
}

if (serial_read1byte() != STM32ACK) 
{
	printf("didn't get ACK after data in get_command, exiting\n");
	exit(1);
}
 	
printf("got commands and version:  --------------------------\n");
for (i = 0; i < inpacket.length; i++) 
    printf("byte %.2i:   0x%.2x\n", i, inpacket.storage[i]);
printf("-----------------------------------------------------\n");
return;
}

void get_version(void)
{
Packet outpacket, inpacket;
int packsendtrycount=0;
int bytestoreadfromtarget;
int bytesactuallyread=0;
unsigned char * packptr;

packet_zero(&outpacket);
packet_zero(&inpacket);

packet_appendcommandandcheck(&outpacket, STM32GETVERSION);

if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
	printf("tried getversion command %i times w/o ack, exiting\n",packsendtrycount);
	exit(1);
}

//ST (AN2606) says the number of bytes to read here are 3
bytestoreadfromtarget = STM32GETVERSIONBYTESTOREAD; 

packptr = inpacket.storage;
while (1) {
   bytesactuallyread = serial_read(packptr, bytestoreadfromtarget);
   inpacket.length+=bytesactuallyread;
   packptr += (size_t)bytesactuallyread;
   bytestoreadfromtarget -= bytesactuallyread;
   if (bytestoreadfromtarget == 0)  break;
}
printf("... got %i data bytes from get_version, waiting for an ACK\n",inpacket.length);
if (serial_read1byte() == STM32ACK) 
	printf("got ACK after data in get_version, good\n");
else {
	printf("didn't get ACK after data in get_version, exiting\n");
	exit(1);
}
 	
printf("byte %.2i : bootloader version = %.2x\n", 
   STM32GETVERSION_VERSIONBYTE, inpacket.storage[STM32GETVERSION_VERSIONBYTE]);
printf("byte %.2i : bootloader version = %.2x\n", 
   STM32GETVERSION_RDPROTDISABLEDQTYBYTE, inpacket.storage[STM32GETVERSION_RDPROTDISABLEDQTYBYTE]);
printf("byte %.2i : bootloader version = %.2x\n", 
   STM32GETVERSION_RDPROTENABLEDQTYBYTE, inpacket.storage[STM32GETVERSION_RDPROTENABLEDQTYBYTE]);
}

void get_id(void)
{
Packet outpacket, inpacket;
int packsendtrycount=0;
int bytestoreadfromtarget;
int bytesactuallyread=0;
unsigned char * packptr;
int i;
packet_zero(&outpacket);
packet_zero(&inpacket);
/*
//build up the command packet
packet_append(&outpacket, STM32GETID);
packet_append(&outpacket, (unsigned char) ~STM32GETID);

//now send the command packet and wait for an ack.
while (1) {
   packet_send(&outpacket);
   if (serial_read1byte() == STM32ACK) break;
   if (++packsendtrycount > COMMANDRETRYCOUNT) {
	printf("tried getid command %i times w/o ack, exiting\n",packsendtrycount);
	exit(1);
   }
}
*/
packet_appendcommandandcheck(&outpacket, STM32GETID);

if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
	printf("tried getid command %i times w/o ack, exiting\n",packsendtrycount);
	exit(1);
}

//now get the number of bytes the target says it's going to send us.
bytestoreadfromtarget = (int)serial_read1byte() + 1; 

packptr = inpacket.storage;
while (1) {
   bytesactuallyread = serial_read(packptr, bytestoreadfromtarget);
   inpacket.length+=bytesactuallyread;
   packptr += (size_t)bytesactuallyread;
   bytestoreadfromtarget -= bytesactuallyread;
   if (bytestoreadfromtarget == 0)  break;
}
printf("... got %i data bytes from get_command, waiting for an ACK\n",inpacket.length);
if (serial_read1byte() == STM32ACK) 
	printf("got ACK after data in get_id, good\n");
else {
	printf("didn't get ACK after data in get_id, exiting\n");
	exit(1);
}
 	
printf("got PID\n");
for (i = 0; i < inpacket.length; i++) 
    printf("byte %.2i:   %.2x\n", i, inpacket.storage[i]);
return;
}

void go_jump(void)
{
Packet outpacket;
int packsendtrycount=0;
long long unsigned int address;

address = targopts.jump_address;
if (hostopts.jumpaddressstring[0] == '\0')  //user didn't set it, we left the initted default, warn
    printf("no specified jump address, jumping to default %lli\n", address);

packet_zero(&outpacket);
packet_appendcommandandcheck(&outpacket, STM32GO);

if (packet_sendandgetack(&outpacket, COMMANDRETRYCOUNT)) {
	printf("tried \"go\" command %i times w/o ack, exiting\n",packsendtrycount);
	exit(1);
}

packet_zero(&outpacket);

packet_append(&outpacket, ((address & 0xff000000) >> 24));
packet_append(&outpacket, ((address & 0x00ff0000) >> 16));
packet_append(&outpacket, ((address & 0x0000ff00) >> 8));
packet_append(&outpacket, (address & 0x000000ff));
packet_checksumappend(&outpacket);
packsendtrycount = 0;
packet_send(&outpacket);
if (serial_read1byte() != STM32ACK) {
   printf("didn't get ack after address+checksum in go_jump(), exiting\n");
	   exit(1);
	}
printf("got ack after address+checksum in go_jump()\n");

if (serial_read1byte() != STM32ACK) {
   printf("didn't get second ack (after address valid check, according to AN2606), exiting\n");
	   exit(1);
	}
printf("got second ack after address+checksum in go_jump() assumed jump is successful\n");

return;
}

void packet_read(Packet * packet, int bytestoread)
{
unsigned char * packptr;
int bytesactuallyread;
int readcount = 0;

if (bytestoread > MAXPACKLEN) {
	printf("BUG: bytestoread %i exceeds MAXPACKLEN %i, exiting\n",bytestoread, MAXPACKLEN);
	exit(1);
}

packet_zero(packet);
packptr = packet->storage;
while (1) {
   bytesactuallyread = serial_read(packptr, bytestoread);
   packet->length += bytesactuallyread;
   packptr += (size_t)bytesactuallyread;
   if (packet->length == bytestoread) break;
   if (++readcount > READPACKETREADCALLSMAX) {
	printf("ran serial_read %i times in packet_read still don't have all bytes, exiting\n",readcount);
	exit(1);
   }
waitms(PACKETREADWAIT);
}
return;
}


#ifndef WINDOWSHOST
void waitms(int mstowait)
{
static struct timespec timestruct;
timestruct.tv_sec=mstowait/1000;
timestruct.tv_nsec = (mstowait % 1000) * 1000000;
if (nanosleep(&timestruct, NULL))
	perror("DANGER.. nanosleep problem! (maybe I just got a signal..)\n");
return;
}
#else
void waitms(int mstowait)
{
_sleep(mstowait);		//windows Sleep() takes ms.  
return;
}
#endif

void packet_checksumappend (Packet * packet)
{
int i;
unsigned char checksum = 0;
for(i=0; i < packet->length; i++)  
   checksum = checksum ^ packet->storage[i];
packet_append(packet, checksum);
return;
}

void packet_appendcommandandcheck (Packet * packet, unsigned char command)
{
packet_append(packet, command);
packet_append(packet, ~command);	//checksum for commands is the inverted command.
return;
}


void packet_zero(Packet * packet)
{
packet->length = 0;
return;
}

void packet_send(Packet * packet)
{
int lengthleft;
int bytesactuallyread;
unsigned char * packptr;
packptr = packet->storage;

lengthleft = packet->length;
while (1) {
   bytesactuallyread = serial_write(packptr, lengthleft);
   lengthleft -= bytesactuallyread;
   packptr += (size_t)bytesactuallyread;
   if (lengthleft == 0) break;
}
waitms(SENDPACKETWAIT);
return;
}

int  packet_sendandgetack(Packet * packet, int trycount)
{
int packsendtrycount = 0;
while (1) {
   packet_send(packet);
   if (serial_read1byte() == STM32ACK) break;
   if (++packsendtrycount >= trycount) 
	return(1);  
}
return(0);//success if we got ACK
}

void targopts_init(void)
{
targopts.start_address = 0;
targopts.jump_address = STM32FLASHSTART;
return;
}

void packet_append (Packet * packet, unsigned char c)
{
if(packet->length >= MAXPACKLEN-1) {
   printf("packet too long in packet_append, exiting!");
   exit(1);
}
packet->storage[packet->length] = c;
packet->length++;
return;
}

int stringlisttobytes(unsigned char * list, char * stringlist)
{
//this will modify stringlist...
int bytesread=0;
char *singlebytestring;

if (stringlist[0] == '\0') {  //this should not be possible with req'd parameter
	printf("Got null string in stringlisttobytes, exiting\n");
	exit(1);
}

singlebytestring = strtok(stringlist, ",");
while (singlebytestring != NULL)
{
	list[bytesread++] = (unsigned char) strtol(singlebytestring, NULL, 0);
	singlebytestring = strtok(NULL, ",");
}
return(bytesread);
}

void testfunction(void)
{
unsigned long testwaittime;
 testwaittime = strtol(hostopts.testparamstring, NULL, 0);
printf("Hello from testfunction.. raising rts now testing waitms(%li)....\n",testwaittime);
fflush(stdout);
serial_setrts();
serial_clrdtr();
waitms(testwaittime);
serial_clrrts();
printf("waitms() returned just now, bye!\n");
fflush(stdout);
exit(0);
}

void setdtr(void)
{
    waitms(50);
    serial_setdtr();
    waitms(50);
}

void clrdtr(void)
{
    waitms(50);
    serial_clrdtr();
    waitms(50);
}

void setrts(void)
{
    waitms(200);
    serial_setrts();
    waitms(200);		//give at least 200ms on rts change, used for reset
}

void clrrts(void)
{
    waitms(50);
    serial_clrrts();
    waitms(50);		//give at least 50ms on rts change, used for reset 
}

    
