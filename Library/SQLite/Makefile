#Makefile for Sqlite
#CC = gcc
INCLUDE = .
CFLAGS = -Wall -fPIC

Sqlite: sqlite3.c sqlite3.h
	$(CC) -I$(INCLUDE) $(CFLAGS) -c $<
	ar  rcs libsqlite3.a      sqlite3.o
clean:
	rm *.o *.a
