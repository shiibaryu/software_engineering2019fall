#include "concurrency.h"

extern ROW_S2PL Row[];

void
print_result(struct timeval begin, struct timeval end, int nthread)
{
  long usec;
  double sec;

  usec = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
  sec = (double)usec / 1000.0 / 1000.0;
  printf("Throughput: %f (trans/sec)\n", (double)nthread * MAX_TRANSACTION / sec);
}

/* Read Modify Write */
void
task(int rowid)
{
	int value;

	value = Row[rowid].value; // read
	value = value + rand();
  Row[rowid].value = value;
}

void
lock(int id)
{
	if (pthread_mutex_lock(&Row[id].mutex)) ERR;
}

void
unlock(int id)
{
	if (pthread_mutex_unlock(&Row[id].mutex)) ERR;
}
