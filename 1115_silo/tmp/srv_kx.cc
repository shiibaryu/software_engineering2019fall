#include "kernel.h"
#include "debug.h"
#include "port.h"

#include <cstdint>
#define THREAD_NUM (2)
#define LISTENSIZE (10)
using namespace std;

std::atomic<unsigned int> Running;
//uint64_t_64byte GlobalEpoch;

pthread_t *Thread;
pthread_t EpochThread;
pthread_t LogThread;
WORK *Work;

uint64_t GlobalEpoch;
uint64_t *ThLocalEpoch;
int ThreadNum = 0;
pthread_mutex_t MutexThreadNum;
pthread_mutex_t MutexLogList;

extern void *doThreadInteractive(void *a);
extern void *epocher(void *arg);
extern void *logger(void *arg);
extern ROW_SILO Row[MAX_ROW];

//if (posix_memalign((void**)&ThLocalEpoch, 64, THREAD_NUM * sizeof(uint64_t_64byte)) != 0) ERR;//[0]は使わない

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

void
invoke_logger(void)
{
  int ret = pthread_create(&LogThread, NULL, logger, NULL);      
  if (ret < 0) ERR;
}

void
invoke_epocher(void)
{
  int ret = pthread_create(&EpochThread, NULL, epocher, NULL);      
  if (ret < 0) ERR;
}

extern int 
createConnection(const int port)
{ 
  int sockfd;
  int optval = 1;
  struct sockaddr_in addr;
  char hostname[BUFSIZ];

  /*
   * Set parameters
   */
  if (gethostname(hostname, BUFSIZ) == -1) ERR;
  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  /*
   * Socket, bind & listen
   */
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) ERR;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) ERR;
  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == -1) ERR;
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) ERR;
  if (listen(sockfd, LISTENSIZE) == -1) ERR;

  return sockfd;
}

static void
threadInteractive(const int afd)
{
  pthread_t t;
  pthread_attr_t a;

  if (pthread_attr_init(&a)) ERR;
  if (pthread_attr_setschedpolicy(&a, SCHED_RR)) ERR;
  if (pthread_create(&t, &a, doThreadInteractive, (void *)&afd)) ERR;
  if (pthread_detach(t)) ERR;
}

static void
srvmain(void)
{
  int acptfd;
  int ifd;  /* Interactive Client Channel */

  ifd = createConnection(DBMS_PORT);
  while (true) {
    if ((acptfd = accept(ifd, NULL, NULL)) == -1) ERR;
    threadInteractive(acptfd);
  }
}

void
init_mutex(void)
{
  pthread_mutex_init(&MutexThreadNum, NULL);
  pthread_mutex_init(&MutexLogList, NULL);
}

static void
invoke_thread(void)
{
  invoke_epocher();
  invoke_logger();
}

extern int
main(int argc, char *argv[])
{
  int nthread = THREAD_NUM;

  init_database();
  init_thread(nthread);
  init_trans(nthread);
  init_mutex();
  invoke_thread();

  srvmain();
	free(Thread);
	
  return 0;
}
