CC=gcc
CXX=g++
CFLAGS=-std=c99 -O2 
CXXFLAGS=-O2
LDFLAGS=

BIN=cuckoo_hash_v2

$(BIN).o: cuckoo_hash_v2.c hashes_v2.c cuckoo_hash_v2.h
	$(CC) $(CFLAGS) -c hashes_v2.c
	$(CC) $(CFLAGS) -c $<
	ar rcs libcuckoo_hash_lock.a hashes_v2.o $@
doxygen:
	doxygen cuckoo_hash_v2.c

clean:
	rm -f *.o $(BIN)
