CC = gcc

OS := $(shell uname)
ifeq ($(OS),SunOS)
	CFLAG = -lpthread -lnsl -lsocket
else
	CFLAG = -lpthread
endif

all: myftpserver myftpclient

myftpserver: myftpserver.c
	$(CC) -o $@ $< $(CFLAG)

myftpclient: myftpclient.c
	$(CC) -o $@ $< $(CFLAG)

clean:
	rm myftpserver myftpclient
