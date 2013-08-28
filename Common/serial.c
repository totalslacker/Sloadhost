/*
 *  sloadhost:  interacts with microcontroller boot load protocols.
 *  Copyright 2009, 2011 Alex Faveluke
 *  Windows DTR/RTS code (C) 2011 Marten Petchske
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

/*-----------------------------------------------------------------------
 * Changes:
 * 2010-7-7 (bugfix): fixed up serial port initialization to eliminate bzeroing 
 * termios struct.   
 *
 * 2012-1-4 added DTR/RTS control functions for --dtrrtsconfigboot option
 *
 *
 *-----------------------------------------------------------------------*/

//serial.c  Provides serial port access.
//includes hostsystem.h to switch between Windows and Linux  serial
//api.  May eventually split into separate files and chose file at build.


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hostsystem.h"		//define the host system as GNU or WINDOWS 
#ifdef WINDOWSHOST
 /*include headers for windows API functions...*/
/*thanks Robertson Bayer for your nice Windows Serial Port Programming pdf...*/

#include <windows.h>

#else	/*assume we have Unix-like environment*/
#include <unistd.h>	//for read/write
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif /*WINDOWSHOST*/

#include "serial.h"	//function prototypes
//#include "globaldecs.h"

//  define USELOCALTESTINGFIFOS to listen to 
// ./testserialinfifo and write to ./testserialoutfifo


//#define USELOCALTESTINGFIFOS 1
#undef USELOCALTESTINGFIFOS

//define COPYTOSERIALVIEWFIFOS to copy serial output to ./serialoutcopyfifo and 
//copy serial input to ./serialincopyfifo

//#define COPYTOSERIALVIEWFIFOS
#undef COPYTOSERIALVIEWFIFOS

/*-------------definitions for serial port*------------------*/
#define READTIMEOUT 20	//sec/10 to wait for read. 
#define _POSIX_SOURCE 1

/*-----------------------------------------------------------*/


int serial_open(char *serialportname);	//opens serial port, gets fd.
int serial_config(unsigned int baudrate);	//sets up parameters

#ifdef WINDOWSHOST
static struct serial_port {
	int open;
	int status;
	HANDLE hSerial;
	DCB OldDCBParams;
	DCB NewDCBParams;
} serialport1;

int serial_init(char *serialportname, unsigned int baudrate) //windows version
{
if (serial_open(serialportname)) return (1);
if (serial_config(baudrate)) return (2);
return(0);
}

int serial_open(char *serialportname) 	//windows version
			//open up the serial port.. windows version
{
  serialport1.open=0;
  serialport1.status=0;
  serialport1.hSerial = CreateFile (	serialportname,
  					GENERIC_READ | GENERIC_WRITE,
					0,
					0,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					0 );
 if (serialport1.hSerial == INVALID_HANDLE_VALUE) {
 	if (GetLastError()==ERROR_FILE_NOT_FOUND) {
		printf("\nSerial port not found by Create_File in serial_open!\n");
		fflush(stdout);
		return(1);
	}
	else	{
		printf("\nError from CreateFile in serial_open!\n");
		fflush(stdout);
		return(1);
	}
}			//end if INVALID_HANDLE_VALUE
return(0);		//success!
}


int serial_config(unsigned int requestedbaudrate) 	//windows version
{

 
memset (&serialport1.OldDCBParams, 0, sizeof(DCB));
serialport1.OldDCBParams.DCBlength = sizeof(DCB);
 
memset (&serialport1.NewDCBParams, 0, sizeof(DCB));
serialport1.NewDCBParams.DCBlength = sizeof (DCB);	//?
								//error in pdf?
if (!GetCommState(serialport1.hSerial, &serialport1.OldDCBParams)) {
	printf("\nGetCommState error in serial_config!\n");
	fflush(stdout);
	return(1);
}

switch (requestedbaudrate) {		
	case 2400 : 
		serialport1.NewDCBParams.BaudRate = CBR_2400;
		break;
	case 4800 :
		serialport1.NewDCBParams.BaudRate = CBR_4800;
		break;
	case 9600 :
		serialport1.NewDCBParams.BaudRate = CBR_9600;
		break;
	case 19200 :
		serialport1.NewDCBParams.BaudRate = CBR_19200;
		break;
	case 38400 :
		serialport1.NewDCBParams.BaudRate = CBR_38400;
		break;
	case 57600 :
		serialport1.NewDCBParams.BaudRate = CBR_57600;
		break;
	case 115200 :
		serialport1.NewDCBParams.BaudRate = CBR_115200;
		break;
	default :
		printf("Unsupported baudrate in serial_config (windows version)!\n");
		fflush(stdout);
		return(1);
		exit(1);
} 

serialport1.NewDCBParams.ByteSize = 8;
serialport1.NewDCBParams.StopBits = ONESTOPBIT;
serialport1.NewDCBParams.Parity = EVENPARITY;	//required by STM32 bootloader protocol

if (!SetCommState(serialport1.hSerial, &serialport1.NewDCBParams)) {
	printf("\nSetCommState error in serial_config!\n");
	fflush(stdout);
	return(1);
}  

//set up timeouts

COMMTIMEOUTS timeouts={0};
//timeouts.ReadIntervalTimeout = 50;
timeouts.ReadIntervalTimeout = 1000;
timeouts.ReadTotalTimeoutConstant = 5000;	//5 sec for testing..
timeouts.ReadTotalTimeoutMultiplier=10;
timeouts.WriteTotalTimeoutConstant=50;
timeouts.WriteTotalTimeoutMultiplier=10;

if (!SetCommTimeouts(serialport1.hSerial, &timeouts)) {
	printf("\nSetCommTimeouts error in serial_config()\n");
	fflush(stdout);
	return(1);
}

return(0);  //end of windows version serial_config();
}


int serial_setdtr()	
{
if (!EscapeCommFunction(serialport1.hSerial, SETDTR)) {
	printf ("\nEscapeCommFunction error in serial_setdtr\n");
	fflush(stdout);                                   //STM32 BOOT0 = H
	return(1);
}
return(0);
}

int serial_clrdtr()
{
if (!EscapeCommFunction(serialport1.hSerial, CLRDTR)) {
	printf ("\nEscapeCommFunction error in serial_clrdtr\n");
	fflush(stdout);                                   //STM32 BOOT0 = L
	return(1);
}
return(0);
}

int serial_setrts() 
{
if (!EscapeCommFunction(serialport1.hSerial, SETRTS)) {
	printf ("\nEscapeCommFunction error in serial_setrts\n");
	fflush(stdout);                                   //STM32 RESET = L
	return(1);
}
return(0);
}
 
int serial_clrrts()
{
if (!EscapeCommFunction(serialport1.hSerial, CLRRTS)) {
	printf ("\nEscapeCommFunction error in serial_clrrts\n");
	fflush(stdout);                                   //STM32 RESET = H
	return(1);
}
return(0);
}

ssize_t serial_read(unsigned char * buffer, size_t byteqty) //windows version
{
DWORD bytesread;
if (!ReadFile(serialport1.hSerial, buffer, byteqty, &bytesread, NULL)) {
	printf("error in ReadFile in serial_read, exiting\n");
	exit(1);
}
return(bytesread);
}

ssize_t serial_write(unsigned char * buffer, size_t byteqty) //windows version		
{
DWORD lengthwrote;
if (!WriteFile(serialport1.hSerial, buffer, byteqty, &lengthwrote, NULL)) {
	printf("error in WriteFile in serial_write, exiting\n");
	exit(1);
}
return(lengthwrote);
}


int serial_release(void) //windows version
{
CloseHandle(serialport1.hSerial);	//should try to recover state...
return(0);
}


int serial_flush()	//windows version
					//flushes the serial port, returns 1 on error, 0 on success.
{
if (!FlushFileBuffers(serialport1.hSerial)) {
	printf("\nFlushFileBuffers fail in serial_flush!\n");
	fflush(stdout);
	return(1);
}
return(0);
}

#else			//  #WINDOWSHOST NOT DEFINED
			// we are assuming UNIX facilities from library 
			//if not WINDOWSHOST

static struct serial_port {		//linux serial port
	int open;
	int status;
	int fd;
	int fdtestread;		//just for testing
	int fdtestwrite;	//just for testing
	int fdincopy;		//observes serial input
	int fdoutcopy;		//observes serial output
	int ioctlstatus_old;
	struct termios oldtio;
	struct termios newtio;
} serialport1;			//save serial port info.


int serial_init(char *serialportname, unsigned int baudrate)	//linux version	
{
if (serial_open(serialportname)) return (1);
if (serial_config(baudrate)) return (2);
if (serial_flush()) return(3);
return(0);
}

int serial_open(char *serialportname) 	//open up the serial port linux version
{
	printf("\nInfo:  serialdevname in serial_open is %s\n",serialportname);
	
  serialport1.open=0;
  serialport1.status=0;
#ifdef USELOCALTESTINGFIFOS 
printf("WARNING.. testing serial read and or write, not going to real port!!\n");
printf("opening fdtestread...\n"); fflush(stdout);
serialport1.fdtestread = open ("testserialinfifo", O_RDONLY);
printf("fdtestread opened .. opening fdtestwrite... \n");
fflush(stdout);
serialport1.fdtestwrite = open ("testserialoutfifo", O_WRONLY); 
printf("fdtestwrite opened\n");
return(0);
#endif

#ifdef COPYTOSERIALVIEWFIFOS
serialport1.fdoutcopy = open ("serialoutcopyfifo", O_WRONLY);
printf("serialoutcopyfifo opened\n");
serialport1.fdincopy = open ("serialincopyfifo", O_WRONLY);
printf("serialincopyfifo opened\n");
#endif

serialport1.fd = open (serialportname, O_RDWR | O_NOCTTY | O_NDELAY);  //NDELAY option, don't block on DCD status.  
  									 //added 2010-7-7
  


  if (serialport1.fd < 0)
        {
          perror (serialportname);
          return (1);
        }
  else 
      {
	  fcntl(serialport1.fd, F_SETFL, 0);			//make read block / timeout (restore blocking behavior from O_NDELAY)
	  serialport1.open=1;
      }

  return(0);

}

int serial_config(unsigned int requestedbaudrate)   //linux version
{
speed_t baudrate;
int status_ioctl;

/*put baudrate in termios baudrate form*/

switch (requestedbaudrate) {
	case 2400 : 
		baudrate = B2400;
		break;
	case 4800 :
		baudrate = B4800;
		break;
	case 9600 :
		baudrate = B9600;
		break;
	case 19200 :
		baudrate = B19200;
		break;
	case 38400 :
		baudrate = B38400;
		break;
	case 57600 :
		baudrate = B57600;
		break;
	case 115200 :
		baudrate = B115200;
		break;
	default :
		printf("Unsupported baudrate in serial_config, exiting\n");
		exit(1);
}  //end baudrate integer to termios constant conversion switch case

#ifdef USELOCALTESTINGFIFOS 
printf("\nWARNING... USELOCALTESTINGFIFOS was defined, serial init is commented out, reading and writing to local fifos. \n");
return(0);
#endif
if (tcgetattr (serialport1.fd, &serialport1.oldtio)) {     //save existing settings
	perror("tcgetattr in serial_config");
	return(1);
}

if (tcgetattr (serialport1.fd, &serialport1.newtio)) {     //use existing settings as starting point for new config
	perror("tcgetattr in serial_config");
	return(1);
}


cfmakeraw(&serialport1.newtio);					//sets up for raw 
serialport1.newtio.c_cflag |= (CLOCAL | CREAD | PARENB) ; 	//enable parity output,
serialport1.newtio.c_cflag &= ~PARODD;  			//ensure even parity
serialport1.newtio.c_iflag |= IGNPAR;   			//ignore parity and frame errors
cfsetospeed(&serialport1.newtio, baudrate);			//proper way to set baudrate
cfsetispeed(&serialport1.newtio, baudrate);


  serialport1.newtio.c_cc[VTIME] = (unsigned int) READTIMEOUT;   //READTIMEOUT in msecs;
  //gotta fix this for nice timeout behavior
  serialport1.newtio.c_cc[VMIN] = 0;     //blocking read until 1 char recieved
 if (tcflush (serialport1.fd, TCIOFLUSH))  {  //ditches unread or unwrit data
 	perror("tcflush in serial_config");
	return(1);
 }
 if (tcsetattr (serialport1.fd, TCSANOW, &serialport1.newtio))  {   //apply new settings NOW..
 	perror("tcsetattr in serial_config");
	return(1);
 }

 if (ioctl(serialport1.fd, TIOCMGET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMGET");
	return(1);
 }
 serialport1.ioctlstatus_old = status_ioctl;
   status_ioctl |= TIOCM_DTR;
   status_ioctl &= ~TIOCM_RTS;  
 
 if (ioctl(serialport1.fd, TIOCMSET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMSET");
	return(1);
 }

  serialport1.status = 1;     
  return(0);	//should error check!
}

int serial_release(void)  //linux version
{
if (tcflush(serialport1.fd, TCIOFLUSH)) {
	perror ("tcflush in serial_release");
	return(1);
}

if (ioctl(serialport1.fd, TIOCMSET, &serialport1.ioctlstatus_old)) {
 	perror("ioctl failed on TIOCMSET in serial_release");
	return(1);
}

if (tcsetattr (serialport1.fd, TCSANOW, &serialport1.oldtio)) {
	perror ("tcsetattr in serial_release");
	return(1);
}
if (close (serialport1.fd))
    {
      perror ("close in serial_release");
      return(1);
    }
return(0);
}


int serial_flush()	//flushes the serial port, returns 1 on error, 0 on success.  Linux version
{
if (tcflush(serialport1.fd, TCIOFLUSH)) {
	perror ("tcflush error in serial_flush");
	return(1);
}
return(0);
}
   
int serial_setdtr()	//sets DTR
{

int status_ioctl;

if (ioctl(serialport1.fd, TIOCMGET, &status_ioctl)) {   //get existing status
 	perror("ioctl failed on TIOCMGET in serial_set_dtr");
	return(1);
 }

status_ioctl |= TIOCM_DTR;

if (ioctl(serialport1.fd, TIOCMSET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMSET in serial_set_dtr");
	return(1);
}
return(0);
}

int serial_clrdtr()	//clears DTR
{

int status_ioctl;

if (ioctl(serialport1.fd, TIOCMGET, &status_ioctl)) {   //get existing status
 	perror("ioctl failed on TIOCMGET in serial_clr_dtr");
	return(1);
 }

status_ioctl &= ~TIOCM_DTR;

if (ioctl(serialport1.fd, TIOCMSET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMSET in serial_clr_dtr");
	return(1);
}
return(0);
}

int serial_setrts()	//sets rts 
{

int status_ioctl;

if (ioctl(serialport1.fd, TIOCMGET, &status_ioctl)) {   //get existing status
 	perror("ioctl failed on TIOCMGET in serial_set_rts");
	return(1);
 }

status_ioctl |= TIOCM_RTS;

if (ioctl(serialport1.fd, TIOCMSET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMSET in serial_set_dtr");
	return(1);
}
return(0);
}

int serial_clrrts()	//clears RTS 
{

int status_ioctl;

if (ioctl(serialport1.fd, TIOCMGET, &status_ioctl)) {   //get existing status
 	perror("ioctl failed on TIOCMGET in serial_clr_rts");
	return(1);
 }

status_ioctl &= ~TIOCM_RTS;

if (ioctl(serialport1.fd, TIOCMSET, &status_ioctl)) {
 	perror("ioctl failed on TIOCMSET in serial_clr_rts");
	return(1);
}
return(0);
}


ssize_t serial_write(unsigned char * buffer, size_t byteqty)  //linux version
{
int byteswritten;
#ifdef USELOCALTESTINGFIFOS
return(write(serialport1.fdtestwrite, buffer, byteqty));
#endif
#ifdef COPYTOSERIALVIEWFIFOS
byteswritten = write(serialport1.fd, buffer, byteqty);
write(serialport1.fdoutcopy, buffer, byteswritten);
return(byteswritten);
#endif
byteswritten =  write(serialport1.fd, buffer, byteqty);
return(byteswritten);
}

ssize_t serial_read(unsigned char * buffer, size_t byteqty) //linux version 		
{
int bytesread;
#ifdef USELOCALTESTINGFIFOS
return(read(serialport1.fdtestread, buffer, byteqty));
#endif

//printf("DEBUG.. serial_read called with byteqty=%i\n",(int) byteqty);
#ifdef COPYTOSERIALVIEWFIFOS
bytesread = read(serialport1.fd, buffer, byteqty); 
write(serialport1.fdincopy, buffer, bytesread);
return(bytesread);
#endif
bytesread = read(serialport1.fd, buffer, byteqty);
return(bytesread);
}

#endif /*#ifdef WINDOWSHOST windows functions defined #else Unix-like functions defined)*/

unsigned char serial_read1byte(void)
{
unsigned char byte;
int bytesread;
int trycount;
bytesread = 0;
trycount = 0;
while (bytesread == 0 && trycount < 4) {
  bytesread = serial_read(&byte, 1);
  trycount++; 
  if(trycount > 1) {
	printf ("WARNING:  trycount in serial_read1byte = %i\n", trycount);
  }
}
if (bytesread != 1) {
	perror("serial_read1byte had problem, exiting\n");
	exit(1);
}
return(byte);
}

void serial_write1byte(unsigned char byte) 
{
if (serial_write(&byte, 1) != (ssize_t) 1) {
	perror("serial_write1byte had problem, exiting\n");
	exit(1);
}
else return;
}


	
