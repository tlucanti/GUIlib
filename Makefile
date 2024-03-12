
CFLAGS = -Wall -Wextra -I . -fPIC
CC = gcc
LD = gcc

py-build:
	$(CC) $(CFLAGS) -c py/backend.c -o py/backend.o
	$(CC) $(CFLAGS) -c py/test.c -o py/test.o
	$(LD) -shared py/backend.o py/test.o -o py/libbackend.so

py-tkinter: py-build
	cd py && python3 frontend.py

py-pil: py-build
	cd py && python3 frontend.py




