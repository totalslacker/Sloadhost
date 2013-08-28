
/*-----------------------------------------------------------------
 * sloadhost:  interacts with microcontroller boot load protocols
 * Copyright 2009 Alex Faveluke
 *
 * This is free software under the terms of the GNU GPL v3 or later.
 * See the file COPYING for full details.
 -----------------------------------------------------------------*/
/*actions.h*/
void autobaudstart(void);
void clear_writeprotect(void);
void set_writeprotect(void);
void clear_readprotect(void);
void set_readprotect(void);
void globalerase_flash(void);
void pageserase_flash(void);
void program_memory(void);
void read_memory(void);	
void get_commands(void);
void get_version(void);
void get_id(void);
void go_jump(void);

void setdtr(void);
void clrdtr(void);
void setrts(void);
void clrrts(void);

void targopts_init(void);

void testfunction(void);

