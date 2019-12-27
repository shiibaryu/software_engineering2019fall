#define MAX_THREAD (24)
#define N (600)

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <vector>
#include <algorithm>
#include <thread>
#include "debug.h"
using namespace std;
#define DATA_FILE "/tmp/DATA_FILE"
#define MAX_DATA (1000 * 1000 * 10)
#define MAX_LOCKS (3)
#define MAX_FUNCS (1000000)
#define MAX_HISTS (1000000)
#define ERR_LOGFILE "err_logfile"

typedef enum {INSERT, SEARCH} OPTYPE;

typedef struct _DATA {
	int key;
	int val;
  struct _DATA *next;
} DATA;

typedef struct _NODE {
	bool isLeaf;
	struct _NODE *chi[N];
	int key[N-1]; 
	int nkey;
  int high_key; // high-key
  struct _NODE *right_link; // right-link
  struct _NODE *parent;
  pthread_mutex_t lock;
} NODE;

typedef struct _TEMP {
	bool isLeaf;
	NODE *chi[N+1]; // for internal split (for leaf, only N is enough)
	int key[N]; // for leaf split
	int nkey;
} TEMP;

typedef struct _LKOBJ {
	NODE *node;
	uint line;
}	LKOBJ;

typedef struct _FUNC {
	const char *func;
	uint line;
}	FUNC;

typedef struct _HIST {
	uint line;
	NODE *node;
	NODE *right;
	bool isLock;
	const char *func;
} HIST;
