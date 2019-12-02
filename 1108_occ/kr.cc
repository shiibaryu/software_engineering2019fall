#include "concurrency.h"
#include <list>

typedef struct _RW_SET {
 	int rowid;
 	int value;
} RW_SET;

typedef struct _DONE_XACT {
 	int tx_id;
  	RW_SET write_set[MAX_OPERATION];
  	int nwrite;
} DONE_XACT;

int Tx_id = 0;
ROW_KR Row[MAX_ROW];
pthread_t *Thread;
WORK *Work;
//kore
list<DONE_XACT> DoneXact;
pthread_mutex_t GiantLock;
int *WaterMark;
int Nthread = 0;

bool
validation(const int nread, const RW_SET read_set[], const int tx_begin, const int tx_end)
{
  	// Find the start point
  	// Do Validation
	int j,k;
	list<DONE_XACT>::iterator i = DoneXact.begin();

	//get start point
	for(;i!= DoneXact.cend();i++){
		if(i->tx_id == tx_begin+1){
			break;
		}
	}
	//check point
	for(;i!=DoneXact.cend();i++){
		for(j=0;j<i->nwrite;j++){
			for(k=0;k<nread;k++){
				if(i->write_set[j].rowid==read_set[k].rowid){
					return false;
				}
			}
		}
	}
	
  	return true; // Success
}

void
transaction(int wid, int tid)
{
  // Just decralation
  RW_SET read_set[MAX_OPERATION];
  RW_SET write_set[MAX_OPERATION];

  // Set operations
  OPERATION target[MAX_OPERATION];
  memcpy(target, Work[wid].tx[tid].op, sizeof(OPERATION) * MAX_OPERATION);  

 RETRY:
  // Init
  int tx_begin = Tx_id; 
  int nread = 0;
  int nwrite = 0;
  bzero(read_set, MAX_OPERATION * sizeof(RW_SET));
  bzero(write_set, MAX_OPERATION * sizeof(RW_SET));
  WaterMark[wid] = tx_begin; // For GC

  // Read Phase
  for (int oid = 0; oid < MAX_OPERATION; oid++) {
    if (target[oid].type == READ) {
      int rowid = target[oid].rowid;
      read_set[nread].rowid = rowid;
      read_set[nread].value = Row[rowid].value;
      nread++;
    }
  }

  // Modify
  for (int oid = 0; oid < MAX_OPERATION; oid++) {
    if (target[oid].type == WRITE) {
      int rowid = target[oid].rowid;
      write_set[nwrite].rowid = rowid;
      write_set[nwrite].value = 0; // No meaning
      nwrite++;
    }
  }

  // Validate and Write phase
  if (pthread_mutex_lock(&GiantLock) != 0) ERR;
  int tx_end = Tx_id;  
  if (Tx_id != 0) {
    bool valid = validation(nread, read_set, tx_begin, tx_end);
    if (valid == false){
      if (!pthread_mutex_unlock(&GiantLock)) ERR;
      goto RETRY;
    }
  }

  // Write phase
  for (int i = 0; i < nwrite; i++){
    int rowid = write_set[i].rowid;
    int value = write_set[i].value;
    Row[rowid].value = value;
  }

  // Register myself
  Tx_id = Tx_id + 1;
  DONE_XACT me;
  me.nwrite = nwrite;
  me.tx_id = Tx_id;
  memcpy(me.write_set, write_set, sizeof(RW_SET) * MAX_OPERATION);
  DoneXact.push_back(me);
  WaterMark[wid] = me.tx_id;

  // Garbage Collection
  int min_tx_id = WaterMark[0];
  for (int i = 1; i < Nthread; i++) {
    if (WaterMark[i] < min_tx_id) min_tx_id = WaterMark[i];
  }

  list<DONE_XACT>::iterator itr = DoneXact.begin(); 
  list<DONE_XACT>::iterator end = DoneXact.end()--;
  while (itr->tx_id < min_tx_id) {
    itr = DoneXact.erase(itr); 
  }

  // Good Bye!
  if (pthread_mutex_unlock(&GiantLock) != 0) ERR;
}

static void *
worker(void *arg)
{
  int myid = *(int *)arg; free(arg);
  for (int i = 0; i < MAX_TRANSACTION; i++) {
    transaction(myid, i);
  }

  return NULL;
}

void
init_thread(void)
{
	Thread = (pthread_t *)calloc(Nthread, sizeof(pthread_t));
	if (!Thread) ERR;
}

void
init_water_mark(void)
{
	WaterMark = (int *)calloc(Nthread, sizeof(int));
	if (!WaterMark) ERR;
}

void
init_mutex(void)
{
  pthread_mutex_init(&GiantLock, NULL);
}

void 
init_trans(void)
{
  unsigned int now = (unsigned int)time(0);
  srand(now);

  Work = (WORK *)calloc(Nthread, sizeof(WORK)); if (!Work) ERR;
  for (int i = 0; i < Nthread; i++) {
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

void init(int argc, char *argv[])
{
	if (argc == 2) 
    Nthread = atoi(argv[1]); 
  else 
    Nthread = 4;

  init_thread();
  init_mutex();
  init_trans();
  init_water_mark();
}

extern int
main(int argc, char *argv[])
{
  int i;
  struct timeval begin, end;

  init(argc, argv);

	gettimeofday(&begin, NULL);
  for (i = 0; i < Nthread; i++) {
    int *myid = (int *)calloc(1, sizeof(int)); if (!myid) ERR;
    *myid = i;
    int ret = pthread_create(&Thread[i], NULL, worker, myid);
		if (ret < 0) ERR;
	}
  for (i = 0; i < Nthread; i++) {
		int ret = pthread_join(Thread[i], NULL);
		if (ret < 0) ERR;
	}
  gettimeofday(&end, NULL);
  print_result(begin, end, Nthread);
	free(Thread);
	
  return 0;
}
