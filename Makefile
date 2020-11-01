CC = gcc

all : payload inject

inject : inject.c
			  ${CC} inject.c -o $@

payload : payload.s
				nasm -f elf64 -o payload.o payload.s && ld -o payload payload.o
