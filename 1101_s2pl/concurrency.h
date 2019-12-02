#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "debug.h"
#include <iostream>
using namespace std;

#define MAX_OBJ (1000*1000)
#define MAX_OPERATION (10)
#define MAX_RECURSION (10000)
//#define MAX_RECURSION (10)

/* Data Area */
typedef struct _DATA {
	int val;
	pthread_mutex_t mutex;
} DATA;

void print_result(struct timeval begin, struct timeval end, int nthread);
int comp(int data, int ndive);
void task_alpha(int data_id);
void task_beta(int data_id);
void lock(int id);
void unlock(int id);
