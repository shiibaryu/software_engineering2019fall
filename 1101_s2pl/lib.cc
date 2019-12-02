#include "concurrency.h"

extern DATA DataObj[];

void
print_result(struct timeval begin, struct timeval end, int nthread)
{
  long usec;
  double sec;

  usec = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
  sec = (double)usec / 1000.0 / 1000.0;
  printf("Throughput: %f (trans/sec)\n", (double)nthread / sec);
}

int
comp(int data, int ndive)
{
	if (ndive == MAX_RECURSION)	return data;
	data += rand() % 3;
	return comp(data, ++ndive);
}

/* Read modify write */
void
task_alpha(int data_id)
{
	int val;

	val = DataObj[data_id].val; // read
	val = comp(val, 0);         // modify
	DataObj[data_id].val = val; // write
}

void
task_beta(int data_id)
{
	usleep(100*1000);	
}

void
lock(int id)
{
	if (pthread_mutex_lock(&DataObj[id].mutex)) ERR;
}

void
unlock(int id)
{
	if (pthread_mutex_unlock(&DataObj[id].mutex)) ERR;
}
