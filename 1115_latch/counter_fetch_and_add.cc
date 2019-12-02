#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <iostream>
#include "debug.h"

static constexpr uint NbThread = 10;
static constexpr uint NbLoop = 1000 * 1000 * 10;
static constexpr uint LockBit = 0x01;
static constexpr uint UnlockBit = 0x00;
std::atomic<uint> Mutex;
std::atomic<int> Counter(NbThread);

void *
worker(void *arg)
{
  for (uint i = 0; i < NbLoop; i++) {
    atomic_fetch_add(&Counter, 1);
  }
  return NULL;
}

int
main(void)
{
  pthread_t thread[NbThread];

  Counter.store(0);
  for (uint i = 0; i < NbThread; i++) {
    pthread_create(&thread[i], NULL, worker, NULL);
  }
  for (uint i = 0; i < NbThread; i++) {
    pthread_join(thread[i], NULL);
  }
  printf("Counter: %u (Ref. %u)\n", Counter.load(), NbThread * NbLoop);

  return 0;
}
