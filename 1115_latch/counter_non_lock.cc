#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "debug.h"

uint Counter = 0;
pthread_mutex_t Lock;
static constexpr uint NbThread = 10;
static constexpr uint NbLoop = 1000 * 1000 * 10;

void *
worker(void *arg)
{
  for (uint i = 0; i < NbLoop; i++) {
    Counter++;
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
