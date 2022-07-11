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

ifeq ($(NOCHK),1)
LDCONFIG = 
else
LDCONFIG = ldconfig
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
	$(U_CCP) $(U_CFLAGSP) ifcon.c -Wall -o ifcon -lncursesw -lpthread libcworthy.a

cw: cw.c libcworthy.so libcworthy.a $(INCLUDES)
	$(U_CCP) $(U_CFLAGSP) cw.c -Wall -o cw -lncursesw -lpthread libcworthy.a

clean:
	rm -rf *.o $(UTILFILES)

utilities: $(UTILFILES)

install: utilities
	install -m 0755 ifcon $(DESTDIR)/usr/bin
	install -m 0755 libcworthy.so $(DESTDIR)/usr/lib
	install -m 644 libcworthy.a $(DESTDIR)/usr/lib
	install -m 644 cworthy.h $(DESTDIR)/usr/include
	$(LDCONFIG)
	$(LDCONFIG)

uninstall: 
	rm -vf $(DESTDIR)/usr/bin/ifcon
	rm -vf $(DESTDIR)/usr/lib/libcworthy.so
	rm -vf $(DESTDIR)/usr/lib/libcworthy.a
	rm -vf $(DESTDIR)/usr/include/cworthy.h
	$(LDCONFIG)
	$(LDCONFIG)


