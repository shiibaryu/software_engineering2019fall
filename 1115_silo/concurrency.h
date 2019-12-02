#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <sys/time.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include "debug.h"
using namespace std;

#define MAX_ROW (1000*1000)
#define MAX_OPERATION (10)
#define MAX_TRANSACTION (1000)

typedef enum {READ, WRITE} OP_TYPE;

/* Row */
class TidWord {
public:
	union {
		uint64_t obj;
		struct {
			bool lock:1;
			bool latest:1;
			bool absent:1;
			uint64_t tid:29;
			uint64_t epoch:32;
		};
	};

	TidWord() {
		obj = 0;
	}

	bool operator==(const TidWord& right) const {
		return obj == right.obj;
	}

	bool operator!=(const TidWord& right) const {
		return !operator==(right);
	}

	bool operator<(const TidWord& right) const {
		return this->obj < right.obj;
	}
};

class Tuple {
public:
  unsigned int key;
  unsigned int value;
};

class ROW_SILO {
public:
  TidWord tidw;
  Tuple tuple;

	bool operator==(const ROW_SILO& right) const {
		return this->tuple.key == right.tuple.key;
	}

	bool operator!=(const ROW_SILO& right) const {
		return this->tuple.key != right.tuple.key;
	}

	bool operator<(const ROW_SILO& right) const {
		return this->tuple.key < right.tuple.key;
	}
};

class LogBody {
public:
  uint64_t tidw; // tidword
  Tuple tuple;  
};

class LogShell {
public:
  
  uint64_t epoch;
  LogBody *body;
  uint counter;
};

typedef struct _ROW_S2PL {
	int value;
	pthread_mutex_t mutex;
} ROW_S2PL;

typedef struct _ROW_KR {
	int value;
} ROW_KR;

typedef struct _OPERATION {
  int rowid; 
  OP_TYPE type;
} OPERATION;

typedef struct _TRANSACTION {
  OPERATION op[MAX_OPERATION];
} TRANSACTION;

typedef struct _WORK {
  TRANSACTION tx[MAX_TRANSACTION];
} WORK;

void print_result(struct timeval begin, struct timeval end, int nthread);
void task(int rowid);
void lock(int rowid);
void unlock(int rowid);
