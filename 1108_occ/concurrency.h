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
#define MAX_TRANSACTION (10)

typedef enum {READ, WRITE} OP_TYPE;

/* Row */
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
