#Makefile for complie Get system MAC & boradcat address
#CC = gcc
LIB_NAME = libsmart
LIB_OBJ = Checkusb.o
LIBFLAG = -I. -Wall -shared
CFLAGS += -Wall -O2  -I. -fPIC

default:
	$(MAKE) libsmart

libsmart: $(LIB_OBJ)
	$(CC) $(LIBFLAG) $(LIB_OBJ) -o $(LIB_NAME).so
ifeq (risc, $(RISC_TARGET))
	$(STRIP) $(LIB_NAME).so
else
	strip $(LIB_NAME).so
endif

clean:
	rm -f *.o
	rm -f $(LIB_NAME).so
