SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:= client
CC:=gcc
CFLAGS:=-g -Wall
$(ELF):$(SRCS)
	gcc $^ -o $@ -lcrypt -pthread

clean:
	rm -rf $(OBJS)  $(ELF)
