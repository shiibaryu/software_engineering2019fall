T1 = silo

CC = g++
CFLAGS = -c -Wall -g -O0 -std=c++0x -Wunused-variable -std=c++17
LDFLAGS = -lpthread -lcurses -lreadline

O1 = silo.cc lib.cc

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
