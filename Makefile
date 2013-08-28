
#sloadhost:  interacts with microcontroller boot load protocols
#Copyright 2009, 2011 Alex Faveluke

#This is free software under the terms of the GNU GPL v3 or later.
#See the file COPYING for full details.


CC      = gcc
CFLAGS  = -Wall -ICommon 

COMMONSOURCES	= Common/serial.c
COMMONINCLUDES	= Common/serial.h

STM32SOURCES	= Protocols/STM32/actions.c Protocols/STM32/control.c 

STM32INCS	= Protocols/STM32/actions.h Protocols/STM32/globaldecs.h \
                  Protocols/STM32/stm32commanddefs.h

ADUC8051SOURCES	= Protocols/ADUC8051/actions.c Protocols/ADUC8051/control.c

ADUC8051INCLUDES = Protocols/ADUC8051/actions.h Protocols/ADUC8051/globaldecs.h \
                   Protocols/ADUC8051/aduc8051commanddefs.h

TMS320X28SOURCES = Protocols/TMS320X28/actions.c Protocols/TMS320X28/control.c

TMS320X28INCLUDES = Protocols/TMS320X28/actions.h \
                    Protocols/TMS320X28/globaldecs.h \
                    Protocols/TMS320X28/tms320x28commanddefs.h

all: Bin/sloadhost-stm32 Bin/sloadhost-aduc8051 Bin/sloadhost-tms320x28

clean:
	-rm Bin/*

Bin/sloadhost-stm32: $(COMMONSOURCES) $(COMMONINCLUDES) $(STM32SOURCES) $(STM32INCS)
	$(CC) $(CFLAGS) $(COMMONSOURCES) $(STM32SOURCES) -o Bin/sloadhost-stm32

Bin/sloadhost-aduc8051: $(COMMSOURCES) $(COMMONINCLUDES) $(ADUC8051SOURCES) $(ADUC8051INCS)
	$(CC) $(CFLAGS) $(COMMONSOURCES) $(ADUC8051SOURCES) -o Bin/sloadhost-aduc8051

Bin/sloadhost-tms320x28: $(COMMONSOURCES) $(COMMONINCLUDES) $(TMS320X28SOURCES) \
                         $(TMS320X28INCLUDES)
	$(CC) $(CFLAGS) $(COMMONSOURCES) $(TMS320X28SOURCES) -o Bin/sloadhost-tms320x28

testutils: Testutils/byterx Testutils/bytesend Testutils/testserialoutfifo \
           Testutils/testserialinfifo Testutils/serialincopyfifo \
           Testutils/serialoutcopyfifo

Testutils/bytesend: Testutils/binarybytesender.c
	$(CC) Testutils/binarybytesender.c -o Testutils/bytesend

Testutils/byterx: Testutils/binarybytereceiver.c
	$(CC) Testutils/binarybytereceiver.c -o Testutils/byterx 

Testutils/testserialinfifo: 
	mkfifo Testutils/testserialinfifo

Testutils/testserialoutfifo: 
	mkfifo Testutils/testserialoutfifo

Testutils/serialincopyfifo:
	mkfifo Testutils/serialincopyfifo

Testutils/serialoutcopyfifo:
	mkfifo Testutils/serialoutcopyfifo



