T1 = kr

CC = g++
CFLAGS = -c -Wall -g -O0 -std=c++0x -Wunused-variable
LDFLAGS = -lpthread

O1 = kr.o lib.o

#
# Rules for make
#
all: $(T1) 

$(T1): $(O1) 
	$(CC) -o $@ $^ $(LDFLAGS)

.cc.o:
	$(CC) $(CFLAGS) $<

clean:
	rm -f *~ *.o *.exe *.stackdump $(T1) 
