T1 = counter_fetch_and_add
T2 = counter_cas_lock
T3 = counter_non_lock
T4 = counter_pthread_lock

CC = g++
CFLAGS = -g -c -Wall -O0 -std=c++11
LDFLAGS = -lm -pthread 

OBJ_T1 = counter_fetch_and_add.o
OBJ_T2 = counter_cas_lock.o
OBJ_T3 = counter_non_lock.o
OBJ_T4 = counter_pthread_lock.o

all: $(T1) $(T2) $(T3) $(T4) 

$(T1): $(OBJ_T1)
	$(CC) $(LDFLAGS) -o $@ $^
$(T2): $(OBJ_T2)
	$(CC) $(LDFLAGS) -o $@ $^
$(T3): $(OBJ_T3)
	$(CC) $(LDFLAGS) -o $@ $^
$(T4): $(OBJ_T4)
	$(CC) $(LDFLAGS) -o $@ $^

.cc.o:
	$(CC) $(CFLAGS) $<

clean:
	rm -f *~ *.o $(T1) $(T2) $(T3) $(T4) 
