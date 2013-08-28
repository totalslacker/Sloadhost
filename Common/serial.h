/*-----------------------------------------------------------------
 * sloadhost:  interacts with microcontroller boot load protocols
 * Copyright 2009 Alex Faveluke
 *
 * This is free software under the terms of the GNU GPL v3 or later.
 * See the file COPYING for full details.
 -----------------------------------------------------------------*/

//serial.h

int serial_init(char *serialportname, unsigned int baudrate);
int serial_release(void);	//releases port in use.
int serial_flush();
ssize_t serial_write(unsigned char * buffer, size_t byteqty);
ssize_t serial_read(unsigned char * buffer, size_t byteqty);

unsigned char serial_read1byte(void);
void serial_write1byte(unsigned char byte);

int serial_setdtr();
int serial_clrdtr();
int serial_setrts();
int serial_clrrts();
