#include "concurrency.h"
#include <iostream>
#include <future>
#include <cstdlib>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <algorithm>
#include "tsc.hpp"
#include "debug.h"

#include <cstdint>
#define THREAD_NUM (20)
#define EPOCH_TIME (40)
#define CLOCK_PER_US (1000)
using namespace std;

std::atomic<unsigned int> Running;
//uint64_t_64byte GlobalEpoch;

ROW_SILO Row[MAX_ROW];
pthread_t *Thread;
WORK *Work;

uint64_t GlobalEpoch;
uint64_t *ThLocalEpoch;

//if (posix_memalign((void**)&ThLocalEpoch, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;//[0]は使わない

uint64_t
loadAcquireGE()
{
  return __atomic_load_n(&(GlobalEpoch), __ATOMIC_ACQUIRE);
}

void
atomicAddGE()
{
  uint64_t expected, desired;

  expected = loadAcquireGE();
  for (;;) {
    desired = expected + 1;
    if (__atomic_compare_exchange_n(&(GlobalEpoch), &(expected), desired, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) break;
  }
}

ROW_SILO *
searchWriteSet(unsigned int key, std::vector<ROW_SILO> writeSet)
{
	for (auto itr = writeSet.begin(); itr != writeSet.end(); ++itr) {
		if ((*itr).tuple.key == key) return &(*itr);
	}

	return nullptr;
}

void unlockWriteSet(std::vector<ROW_SILO> lockList)
{
	TidWord expected, desired;

	for (auto itr = lockList.begin(); itr != lockList.end(); ++itr) {
		expected.obj = __atomic_load_n(&(Row[(*itr).tuple.key].tidw.obj), __ATOMIC_ACQUIRE);
		desired = expected;
		desired.lock = 0;
		__atomic_store_n(&(Row[(*itr).tuple.key].tidw.obj), desired.obj, __ATOMIC_RELEASE);
	}
}

void writePhase(std::vector<ROW_SILO> writeSet, TidWord max_rset, TidWord max_wset, int thid, std::vector<ROW_SILO> readSet, std::vector<ROW_SILO> lockList)
{
	//It calculates the smallest number that is 
	//(a) larger than the TID of any record read or written by the transaction,
	//(b) larger than the worker's most recently chosen TID,
	//and (C) in the current global epoch.
	
	TidWord tid_a, tid_b, tid_c;
	TidWord mrctid;

	//calculates (a)
	//about readSet
	tid_a = max(max_wset, max_rset);
	tid_a.tid++;
	
	//calculates (b)
	//larger than the worker's most recently chosen TID,
	tid_b = mrctid;
	tid_b.tid++;

	//calculates (c)
	tid_c.epoch = ThLocalEpoch[thid];

	//compare a, b, c
	TidWord maxtid = max({tid_a, tid_b, tid_c});
	maxtid.lock = 0;
	maxtid.latest = 1;
	mrctid = maxtid;

	//write(record, commit-tid)
	for (auto itr = writeSet.begin(); itr != writeSet.end(); ++itr) {
		//update and unlock
		Row[(*itr).tuple.key].tuple.value = (*itr).tuple.value;
		__atomic_store_n(&(Row[(*itr).tuple.key].tidw.obj), maxtid.obj, __ATOMIC_RELEASE);
	}

	//this->finishTransactions++;
  unlockWriteSet(lockList);
  lockList.clear();
	readSet.clear();
	writeSet.clear();
}

bool
chkClkSpan(uint64_t &start, uint64_t &stop, uint64_t threshold)
{
  uint64_t diff = 0;
  diff = stop - start;
  if (diff > threshold) return true;
  else return false;
}

bool
chkEpochLoaded()
{
  uint64_t nowEpoch = loadAcquireGE();

  for (unsigned int i = 1; i < THREAD_NUM; ++i) {
    if (__atomic_load_n(&(ThLocalEpoch[i]), __ATOMIC_ACQUIRE) != nowEpoch) return false;
  }

  return true;
}

// Epoch worker
static void *
epocher(void *arg) 
{
// 1. 40msごとに global epoch を必要に応じてインクリメントする
// 2. 十分条件
// 全ての worker が最新の epoch を読み込んでいる。
//
	const int *myid = (int *)arg;
	pid_t pid = syscall(SYS_gettid);
	cpu_set_t cpu_set;
	uint64_t EpochTimerStart, EpochTimerStop;

	CPU_ZERO(&cpu_set);
	CPU_SET(*myid % sysconf(_SC_NPROCESSORS_CONF), &cpu_set);
	
	if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
		printf("thread affinity setting is error.\n");
		exit(1);
	}

	//----------
	//Bgn = rdtsc();
	EpochTimerStart = rdtsc();
	for (;;) {
		usleep(1);
		//End = rdtsc();

		EpochTimerStop = rdtsc();
		//chkEpochLoaded は最新のグローバルエポックを
		//全てのワーカースレッドが読み込んだか確認する．
		if (chkClkSpan(EpochTimerStart, EpochTimerStop, 
                   EPOCH_TIME * CLOCK_PER_US * 1000) 
        && chkEpochLoaded()) {
			atomicAddGE();
			EpochTimerStart = EpochTimerStop;
		}
	}
	//----------

	return nullptr;
}

static void
init_worker(const int myid)
{
	pid_t pid = syscall(SYS_gettid);
	cpu_set_t cpu_set;

	CPU_ZERO(&cpu_set);
	CPU_SET(myid % sysconf(_SC_NPROCESSORS_CONF), &cpu_set);

	if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
		printf("thread affinity setting is error.\n");
		exit(1);
	}
}

static void *
worker(void *arg)
{
  int myid = *(int *)arg;
  std::vector<ROW_SILO> readSet;
  std::vector<ROW_SILO> writeSet;
  std::vector<ROW_SILO> lockList;
  TidWord max_rset, max_wset;

#if MASSTREE_USE
  MasstreeWrapper<Tuple>::thread_init(int(thid));
#endif

	//----------
  init_worker(myid);
  for (int i = 0; i < MAX_TRANSACTION; i++) {


  RETRY_TX:
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
    /*
     * Commit Protocol
     */

    // Phase 1: Sort lock list
    copy(writeSet.begin(), writeSet.end(), back_inserter(lockList) );
    std::sort(lockList.begin(), lockList.end());
    lockList.erase(std::unique(lockList.begin(), lockList.end()), lockList.end());

    // Phase 2: Lock write set;
    TidWord expected, desired;
    uint prev_key = -1;
    for (auto itr = lockList.begin(); itr != lockList.end(); itr++) {
      if ((*itr).tuple.key == prev_key) continue;
      ROW_SILO *row = &Row[(*itr).tuple.key];
      expected.obj = __atomic_load_n(&(row->tidw.obj), __ATOMIC_ACQUIRE);
      for (;;) {
        if (expected.lock) {
          expected.obj = __atomic_load_n(&(row->tidw.obj), __ATOMIC_ACQUIRE);
        } else {
          desired = expected;
          desired.lock = 1;
          if (__atomic_compare_exchange_n(&(row->tidw.obj), &(expected.obj), desired.obj, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) break;
        }
      }
    }

    // Serialization point
    asm volatile("" ::: "memory");
    __atomic_store_n(&(ThLocalEpoch[myid]), (loadAcquireGE()), __ATOMIC_RELEASE);
    asm volatile("" ::: "memory");
  
    // Phase 3: Validation
    TidWord check;
    for (auto itr = readSet.begin(); itr != readSet.end(); ++itr) {
      check.obj = __atomic_load_n(&(Row[(*itr).tuple.key].tidw.obj), __ATOMIC_ACQUIRE);

      // 1
      // different tid
      if ((*itr).tidw.epoch != check.epoch || (*itr).tidw.tid != check.tid ) {
        unlockWriteSet(lockList);
        lockList.clear();
        readSet.clear();
        writeSet.clear();
        goto RETRY_TX;
      }
      // 3 (2 is omitted since it is needless)
      // is locked
      if ((*itr).tidw.lock && !searchWriteSet((*itr).tuple.key, writeSet)) {
        unlockWriteSet(lockList);
        lockList.clear();
        readSet.clear();
        writeSet.clear();
        goto RETRY_TX;
      }
      max_rset = max(max_rset, check);
    }

    // Phase 4: Write & Unlock
    writePhase(writeSet, max_rset, max_wset, myid, readSet, lockList);
  }

  return NULL;
}

void
init_thread(int nthread)
{
	Thread = (pthread_t *)calloc(nthread, sizeof(pthread_t));
	if (!Thread) ERR;

  //uint64_t *ThLocalEpoch;
  ThLocalEpoch = (uint64_t *)calloc(nthread, sizeof(uint64_t));
	if (!ThLocalEpoch) ERR;

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
        int val = rand() % 2; 
        OP_TYPE type;
        if (val == 0) type = READ; else type = WRITE;
        /* input */
        Work[i].tx[j].op[k].type = type;
        Work[i].tx[j].op[k].rowid = rand() % MAX_ROW;
      }
    }
  }

  /*
  for (int i = 0; i < nthread; i++) {
    for (int j = 0; j < MAX_TRANSACTION; j++) {
      for (int k = 0; k < MAX_OPERATION; k++) {
        printf("Work[%d].tx[%d].op[%d].rowid: %d\n", i, j, k, Work[i].tx[j].op[k].rowid);
      }
    }
  }
  */
}

void
init_database()
{
  for (int i = 0; i < MAX_ROW; i++) {
    Row[i].tuple.key = i;
    //Row[i].tuple.key = 0;
    Row[i].tuple.value = rand();
  }
}

extern int
main(int argc, char *argv[])
{
  int nthread = THREAD_NUM;
  struct timeval begin, end;

  init_database();
  init_thread(nthread);
  init_trans(nthread);
	
	gettimeofday(&begin, NULL);
  for (int i = 0; i < nthread; i++) {
    int *myid = (int *)calloc(1, sizeof(int)); if (!myid) ERR;
    *myid = i;
    /*
    if (*myid == 0) {
      int ret = pthread_create(&Thread[i], NULL, epocher, myid);      
      if (ret < 0) ERR;
    }
    */
    int ret = pthread_create(&Thread[i], NULL, worker, myid);
    if (ret < 0) ERR;
	}

  for (int i = 1; i < nthread; i++) {
		int ret = pthread_join(Thread[i], NULL);
		if (ret < 0) ERR;
	}
  gettimeofday(&end, NULL);
  print_result(begin, end, nthread);
	free(Thread);
	
  return 0;
}
