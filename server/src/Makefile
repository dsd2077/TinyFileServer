#将当前文件下的所有.c文件生成一个可执行文件
SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
ELF:=server
CC:=gcc
CFLAGS:=-g -Wall 
$(ELF):$(SRCS)
		gcc -o $@ $^ -pthread -lmysqlclient -lcrypt
clean:
		rm -rf $(OBJS) $(ELF)

