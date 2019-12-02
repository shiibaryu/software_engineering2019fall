#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <iostream>
#include "debug.h"

uint Counter = 0;
pthread_mutex_t Lock;
static constexpr uint NbThread = 10;
static constexpr uint NbLoop = 1000*1000*10;
static constexpr uint LockBit = 0x01;
static constexpr uint UnlockBit = 0x00;
std::atomic<uint> Mutex;

bool my_lock_core() {
	auto lock = Mutex.load();
	if(lock == LockBit){
		usleep(1);
		return false;
	}
	return Mutex.compare_exchange_strong(lock,LockBit);
}

void my_unlock() 
{ 
	Mutex.store(UnlockBit);
}

void my_lock(void) {
	bool ret;
	while(1){
		ret = my_lock_core();
		if(ret == true){
			break;
		}
	}
}

void *worker(void *arg) {
  for (uint i = 0; i < NbLoop; i++) {
    my_lock();
    Counter++;
    my_unlock();    
  }
  return NULL;
}

int
main(void)
{
  pthread_t thread[NbThread];
  if ((pthread_mutex_init(&Lock, NULL)) == -1) ERR;

  for (uint i = 0; i < NbThread; i++) {
    pthread_create(&thread[i], NULL, worker, NULL);
  }
  for (uint i = 0; i < NbThread; i++) {
    pthread_join(thread[i], NULL);
  }
  printf("Counter: %u (Ref. %u)\n", Counter, NbThread * NbLoop);

  return 0;
}
