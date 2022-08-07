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
LDCONFIG = ldconfig
LIBS = /usr/lib
INCS = /usr/include
BIN = /usr/bin

ifdef LIBDIR
LIBS = $(LIBDIR)
endif

ifdef INCDIR
INCS = $(INCDIR)
endif

ifdef BINDIR
BIN = $(BINDIR)
endif

ifeq ($(NOCHK),1)
LDCONFIG = 
endif

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
	$(U_CCP) $(U_CFLAGSP) ifcon.c libcworthy.a -Wall -o ifcon -lncursesw -lpthread -ltinfo

cw: cw.c libcworthy.so libcworthy.a $(INCLUDES)
	$(U_CCP) $(U_CFLAGSP) cw.c libcworthy.a -Wall -o cw -lncursesw -lpthread -ltinfo

clean:
	rm -rf *.o $(UTILFILES)

utilities: $(UTILFILES)

install: utilities
	install -m 0755 ifcon $(DESTDIR)$(BIN)
	install -m 0755 libcworthy.so $(DESTDIR)$(LIBS)
	install -m 644 libcworthy.a $(DESTDIR)$(LIBS)
	install -m 644 cworthy.h $(DESTDIR)$(INCS)
	$(LDCONFIG)
	$(LDCONFIG)

uninstall: 
	rm -vf $(DESTDIR)$(BIN)/ifcon
	rm -vf $(DESTDIR)$(LIBS)/libcworthy.so
	rm -vf $(DESTDIR)$(LIBS)/libcworthy.a
	rm -vf $(DESTDIR)$(INCS)/cworthy.h
	$(LDCONFIG)
	$(LDCONFIG)


