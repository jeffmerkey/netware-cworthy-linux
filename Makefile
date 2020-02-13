#***************************************************************************
#
#	Copyright (c) 1997-2019 Jeffrey Vernon Merkey
#	All Rights Reserved.
#
#**************************************************************************

INCLUDES=cworthy.h netware-screensaver.h
UTILFILES=libcworthy.so libcworthy.a ifcon cw

# user utility build flags
U_CC = gcc
U_CCP = g++
U_CFLAGSP = -g -O3
U_CFLAGS_LIBP = -g -c -O3 
LD = ld
AR = ar

all : utilities

libcworthy.so: cworthy.o netware-screensaver.o
	$(LD) -shared -lc -o libcworthy.so cworthy.o netware-screensaver.o

libcworthy.a: cworthy.o netware-screensaver.o
	$(AR) r libcworthy.a cworthy.o netware-screensaver.o

cworthy.o: cworthy.c $(INCLUDES)
	$(U_CCP) $(U_CFLAGS_LIBP) -fPIC -Wall cworthy.c 

netware-screensaver.o: netware-screensaver.c $(INCLUDES)
	$(U_CCP) $(U_CFLAGS_LIBP) -fPIC -Wall netware-screensaver.c 

ifcon: ifcon.c libcworthy.so libcworthy.a $(INCLUDES)
	$(U_CCP) $(U_CFLAGSP) ifcon.c -Wall -o ifcon -lncursesw -lpthread libcworthy.a

cw: cw.c libcworthy.so libcworthy.a $(INCLUDES)
	$(U_CCP) $(U_CFLAGSP) cw.c -Wall -o cw -lncursesw -lpthread libcworthy.a

clean:
	rm -rf *.o $(UTILFILES)

utilities: $(UTILFILES)

install: utilities
	install -m 0755 ifcon /usr/bin
	install -m 0755 libcworthy.so /usr/lib64
	install -m 644 libcworthy.a /usr/lib64
	install -m 644 cworthy.h /usr/include
	ldconfig
	ldconfig

uninstall: 
	rm -vf /usr/bin/ifcon
	rm -vf /usr/lib64/libcworthy.so
	rm -vf /usr/lib64/libcworthy.a
	rm -vf /usr/include/cworthy.h
	ldconfig
	ldconfig


