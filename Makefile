CC =gcc
CFLAGS = -o
RM =rm
CFLAG = -c

all: writeonceFS.o
	$(CC) $(CFLAGS) test testwriteonceFS.c writeonceFS.o

writeonceFS.o: writeonceFS.c
	$(CC) $(CFLAGS) writeonceFS.o $(CFLAG) writeonceFS.c

clean:
	@echo "Clean Success"
	$(RM) *.o ./test *.txt