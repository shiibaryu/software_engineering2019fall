#include "kernel.h"
#include "debug.h"
#include "tsc.hpp"

#include <readline/readline.h>
#include <readline/history.h>
using namespace std;
#define DLM " \t\0" // Delimiter
#define EPOCH_TIME (40) // ms
#define CLOCK_PER_US (1000)
#define LOG_FILE "/tmp/LogFile"

std::vector<LogShell> LogList;
ROW_SILO Row[MAX_ROW];

extern bool createTable(char buf[]);
extern int getNewQid(void);
extern void doQuery(char *t, const int sfd);
extern uint64_t GlobalEpoch;
extern uint64_t ThLocalEpoch[MAX_TRANSACTION];
extern int ThreadNum;
extern pthread_mutex_t MutexThreadNum;
extern pthread_mutex_t MutexLogList;

/***********************************************
 * 
 * Functions
 *
 ***********************************************/
/*
static void
sendMsg(string msg, const int sfd)
{
  int ack;
  char rtn[BUFSIZ];

  strcpy(rtn, msg.c_str());
  if (send(sfd, rtn, sizeof(rtn), 0) == -1) ERR;    
  if (recv(sfd, &ack, sizeof(ack), MSG_WAITALL) == -1) ERR;
  strcpy(rtn, FIN);
  if (send(sfd, rtn, sizeof(rtn), 0) == -1) ERR;    
}
*/

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
searchWriteSet(Tuple tuple, std::vector<ROW_SILO> writeSet)
{
	for (auto itr = writeSet.begin(); itr != writeSet.end(); ++itr) {
		if ((*itr).tuple.key == tuple.key) return &(*itr);
		if ((*itr).tuple.value == tuple.value) return &(*itr);
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

  for (int i = 1; i < ThreadNum; ++i) {
    if (__atomic_load_n(&ThLocalEpoch[i], __ATOMIC_ACQUIRE) != nowEpoch) return false;
  }

  return true;
}

// Logging thread
void *
logger(void *arg) 
{
  int fd = open(LOG_FILE, O_APPEND|O_CREAT, 0644);
  while (true) {
    uint64_t curEpoch = loadAcquireGE();
    pthread_mutex_lock(&MutexLogList);
    NNN;
    for (auto itr = LogList.begin(); itr != LogList.end(); itr++) {
      if (itr->epoch < curEpoch) {
        write(fd, itr->body, sizeof(LogBody) * itr->counter);
        LogList.erase(itr);
        itr--;
      }
    }
    fsync(fd);
    pthread_mutex_unlock(&MutexLogList);
    sleep(1);
  }
}


// Epoch thread
void *
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

  /*
	CPU_ZERO(&cpu_set);
	CPU_SET(*myid % sysconf(_SC_NPROCESSORS_CONF), &cpu_set);
	
	if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
		printf("thread affinity setting is error.\n");
		exit(1);
	}
  */

	//Bgn = rdtsc();
	EpochTimerStart = rdtsc();
	for (;;) {
		usleep(1);

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
exec_logging(std::vector<ROW_SILO> writeSet, const int myid)
{
  LogBody *lb = (LogBody *)calloc(writeSet.size(), sizeof(LogBody)); if (!lb) ERR;
  uint counter = 0;
  for (auto itr = writeSet.begin(); itr != writeSet.end(); itr++) {
    lb[counter].tidw = itr->tidw.obj;
    lb[counter].tuple = itr->tuple;
    ++counter;
  }
  LogShell ls;
  ls.epoch = ThLocalEpoch[myid];
  ls.body = lb;
  ls.counter = counter;

  NNN;
  pthread_mutex_lock(&MutexLogList);
  LogList.push_back(ls);
  pthread_mutex_unlock(&MutexLogList);
}

static void 
exec_trans(vector<OPERATION> op, const int myid)
{
  std::vector<ROW_SILO> readSet;
  std::vector<ROW_SILO> writeSet;
  std::vector<ROW_SILO> lockList;
  TidWord max_rset, max_wset;
  //uint64_t thLocalEpoch;

 RETRY_TX:
  // Read phase
  for (auto itr = op.begin(); itr != op.end(); ++itr) {
    int rowid = itr->rowid;
    if (itr->type == READ) {
      readSet.push_back(Row[rowid]);
    }
    else {
      writeSet.push_back(Row[rowid]);
    }
  }
  /*
   * Commit Protocol
   */

  NNN;  
  // Phase 1: Sort lock list;
  copy(writeSet.begin(), writeSet.end(), back_inserter(lockList) );
  std::sort(lockList.begin(), lockList.end());
  lockList.erase(std::unique(lockList.begin(), lockList.end()), lockList.end());

  NNN;  
  // Phase 2: Lock write set;
  TidWord expected, desired;
  //uint prev_key = -1;
  for (auto itr = lockList.begin(); itr != lockList.end(); itr++) {
    //if ((*itr).tuple.key == prev_key) continue;
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

  NNN;
  // Serialization point
  asm volatile("" ::: "memory");
  __atomic_store_n(&(ThLocalEpoch[myid]), (loadAcquireGE()), __ATOMIC_RELEASE);
  asm volatile("" ::: "memory");

  NNN;  
  // Phase 3: Validation
  TidWord check;
  for (auto itr = readSet.begin(); itr != readSet.end(); ++itr) {
    // 1
    check.obj = __atomic_load_n(&(Row[(*itr).tuple.key].tidw.obj), __ATOMIC_ACQUIRE);
    if ((*itr).tidw.epoch != check.epoch || (*itr).tidw.tid != check.tid) {
      unlockWriteSet(lockList);
      lockList.clear();
      readSet.clear();
      writeSet.clear();
      goto RETRY_TX;
    }
    // 3 (2 is omitted since it is needless)
    if (check.lock && !searchWriteSet((*itr).tuple, writeSet)) {
      unlockWriteSet(lockList);
      lockList.clear();
      readSet.clear();
      writeSet.clear();
      goto RETRY_TX;
    }
    max_rset = max(max_rset, check);
  }
  
  // Phase 4: Write & Unlock
  NNN;
  exec_logging(writeSet, myid);
  writePhase(writeSet, max_rset, max_wset, myid, readSet, lockList);
}

std::vector<std::string> 
split(std::string str, char del) {
  uint first = 0;
  uint last = str.find_first_of(del);
 
  std::vector<std::string> result;
 
  while (first < str.size()) {
    std::string subStr(str, first, last - first);
 
    result.push_back(subStr);
 
    first = last + 1;
    last = str.find_first_of(del, first);
 
    if (last == std::string::npos) {
      last = str.size();
    }
  }
 
  return result;
}

extern void *
doThreadInteractive(void *arg)
{
  int sfd = *(int *)arg;
  char buf[BUFSIZ];
  std::string str;
  int myid;

  if (ThreadNum == MAX_TRANSACTION) return NULL;
  pthread_mutex_lock(&MutexThreadNum);
  myid = ThreadNum;
  ThreadNum++;
  pthread_mutex_unlock(&MutexThreadNum);  

  while (true) {
    vector<OPERATION> vector_op;
    // receiving a buffer that includes a transaction 
    // (which has multiple operations)
    int n = recv(sfd, buf, BUFSIZ, MSG_WAITALL);
    if (n == -1) RERR(sfd);        
    str = buf;

    // r:100,w:128
    vector<string> tmp1 = split(str, ',');
    for (auto i = tmp1.begin(); i != tmp1.end(); ++i) {
      char type;
      //int rowid;
      OPERATION op;
      string tstr = *i;
      const char *cstr = tstr.c_str();
      sscanf(cstr, "%c:%d", &type, &op.rowid);
      if (type == 'r') op.type = READ; else op.type = WRITE;
      vector_op.push_back(op);
    }
    NNN;
    exec_trans(vector_op, myid);
    vector_op.clear();

    int ack;

    n = send(sfd, &ack, sizeof(int), MSG_WAITALL);
    if (n == -1) RERR(sfd);        
  }
}
