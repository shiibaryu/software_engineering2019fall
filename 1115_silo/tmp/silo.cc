#include "concurrency.h"

ROW_SILO Row[MAX_ROW];
pthread_t *Thread;
WORK *Work;

typedef struct _RW_SET {
  int rowid;
  int value;
} RW_SET;

static void *
worker(void *arg)
{
  int myid = *(int *)arg;
  std::vector<ROW_SILO> readSet;
  std::vector<ROW_SILO> writeSet;
  std::vector<ROW_SILO> lockList;

  for (int i = 0; i < MAX_TRANSACTION; i++) {
    // Read phase
    for (int j = 0; j < MAX_OPERATION; j++) {
      int rowid = Work[myid].tx[i].op[j].rowid;
      if (Work[myid].tx[i].op[j].type == READ) {
        readSet.push_back(Row[rowid]);
      }
      else {
        writeSet.push_back(Row[rowid]);
      }
    }

    // Commit Protocol
    // Phase 1: sort lock list
    lockList = writeSet;
    sort(lockList.begin(), lockList.end());
    
    
  }

  return NULL;
}

void
init_thread(int nthread)
{
	Thread = (pthread_t *)calloc(nthread, sizeof(pthread_t));
	if (!Thread) ERR;
}

void 
init_trans(int nthread)
{
  unsigned int now = (unsigned int)time(0);
  srand(now);

  Work = (WORK *)calloc(nthread, sizeof(WORK)); if (!Work) ERR;
  for (int i = 0; i < nthread; i++) {
    for (int j = 0; j < MAX_TRANSACTION; j++) {
      for (int k = 0; k < MAX_OPERATION; k++) {
        int val = rand() % 2; OP_TYPE type;
        if (val == 0) type = READ; else type = WRITE;
        /* input */
        Work[i].tx[j].op[k].type = type;
        Work[i].tx[j].op[k].rowid = rand() % MAX_ROW;
      }
    }
  }
}


extern int
main(int argc, char *argv[])
{
  int i;
  int nthread = 4;
  struct timeval begin, end;
	
	if (argc == 2) nthread = atoi(argv[1]);
  init_thread(nthread);
  init_trans(nthread);
	
	gettimeofday(&begin, NULL);
  for (i = 0; i < nthread; i++) {
    int *myid = (int *)calloc(1, sizeof(int)); if (!myid) ERR;
    *myid = i;
    int ret = pthread_create(&Thread[i], NULL, worker, myid);
		if (ret < 0) ERR;
	}
  for (i = 0; i < nthread; i++) {
		int ret = pthread_join(Thread[i], NULL);
		if (ret < 0) ERR;
	}
  gettimeofday(&end, NULL);
  print_result(begin, end, nthread);
	free(Thread);
	
  return 0;
}
