T1 := bltree
T2 := gendata
T3 := read_data

CC := g++
CFLAGS := -g -c -Wall -O2 -MMD -MP -std=c++11
LDFLAGS := -lm -pthread 

SRCS1 := bltree.cc lib.cc
OBJS1 := $(SRCS1:%.cc=%.o)
DEPS1 := $(SRCS1:%.cc=%.d)

SRCS2 := gendata.cc
OBJS2 := $(SRCS2:%.cc=%.o)
DEPS2 := $(SRCS2:%.cc=%.d)

SRCS3 := read_data.cc
OBJS3 := $(SRCS3:%.cc=%.o)
DEPS3 := $(SRCS3:%.cc=%.d)

all: $(T1) $(T2) $(T3) 

-include $(DEPS1)

$(T1): $(OBJS1)
	$(CC) $(LDFLAGS) -o $@ $^
$(T2): $(OBJS2)
	$(CC) $(LDFLAGS) -o $@ $^
$(T3): $(OBJS3)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $<

clean:
	rm -f *~ *.exe *.stackdump $(T1) $(T2) $(T3) $(OBJS1) $(DEPS1) $(OBJS2) $(DEPS2) $(OBJS3) $(DEPS3) 
