CC=g++
CFLAGS= -Wall -w -Wno-sign-compare -Wno-write-strings -O3 -I./
LDFLAGS=-lnfnetlink -lnetfilter_queue -lm -lpthread

EXE=nat
OBJ=nat.o checksum.o nat_table.o

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LDFLAGS) 

clean:
	/bin/rm -f $(OBJ) $(EXE)


