#include "bltree.h"

extern LKOBJ **LockArray;
extern FUNC **Func;
extern int *CountFunc;
extern HIST **Hist;
extern int *CountHist;
extern vector<uint> Key_vector;
extern NODE *Root;
extern DATA Head;
extern DATA *Tail;
extern DATA *Data;
extern pthread_mutex_t GiantLock;
extern pthread_mutex_t MutexData;
extern int Size_data;
extern int Size_thread;
extern FILE *Fp_log;

void
lock(NODE *node)
{
	if (pthread_mutex_lock(&(node->lock)) != 0) ERR;
}

void
unlock(NODE *node)
{
	int ret = pthread_mutex_unlock(&node->lock); if (ret != 0) ERR;
}

void
init_data(void)
{
	struct stat sb;
	if (stat(DATA_FILE, &sb) == -1) ERR;
	if (sb.st_size < (uint)(Size_data * sizeof(int))) {
		printf("Too many data. file[%ld] < data[%ld]\n", sb.st_size, Size_data * sizeof(int));
		exit(0);
	}

	int fd = open(DATA_FILE, O_RDONLY);	if (fd == -1) ERR;	
	uint *buf = (uint *)calloc(Size_data, sizeof(uint)); if (!buf) ERR;
	int ret = read(fd, buf, Size_data * sizeof(uint)); if (ret == -1) ERR;
	for (int i = 0; i < Size_data; i++) {
		Key_vector.push_back(buf[i]);
	}
	free(buf);
	if ((close(fd)) == -1) ERR;
}

NODE *
alloc_leaf(NODE *parent)
{
	NODE *node;

	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->isLeaf = true;
	node->parent = parent;
	node->nkey = 0;
  if (pthread_mutex_init(&node->lock, NULL) != 0) ERR;

	return node;
}

NODE *
alloc_internal(NODE *parent)
{
	NODE *node;

	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->parent = parent;
	node->isLeaf = false;
	node->right_link = NULL;
	node->nkey = 0;
  if (pthread_mutex_init(&node->lock, NULL) != 0) ERR;

	return node;
}

NODE *
alloc_root(NODE *left, int lm_key, NODE *right)
{
	NODE *node;
	
	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->parent = NULL;
	node->isLeaf = false;
	node->right_link = NULL;
	node->key[0] = lm_key;
	node->chi[0] = left;
	node->chi[1] = right;
	node->nkey = 1;

  if (pthread_mutex_init(&node->lock, NULL) != 0) ERR;

	return node;
}

void
input_func_log(const int myid, const char *func, const uint line)
{
	return;
	assert(CountFunc[myid] < MAX_FUNCS);
	Func[myid][CountFunc[myid]].func = func;
	Func[myid][CountFunc[myid]].line = line;
	CountFunc[myid]++;
}

void
input_hist_log(const int myid, const uint line, NODE *n, NODE *right, bool isLock, const char *func)
{
	return;
	assert(CountHist[myid] < MAX_HISTS);

	Hist[myid][CountHist[myid]].line = line;
	Hist[myid][CountHist[myid]].node = n;
	Hist[myid][CountHist[myid]].right = right;
	Hist[myid][CountHist[myid]].isLock = isLock;
	Hist[myid][CountHist[myid]].func = func;
	CountHist[myid]++;
}

void
print_tree_core(NODE *n)
{
	printf("["); fflush(stdout);
	for (int i = 0; i < n->nkey; i++) {
		if (!n->isLeaf) {
      print_tree_core(n->chi[i]);
    }    printf("%d", n->key[i]); fflush(stdout);
		if (i != n->nkey-1 && n->isLeaf) {
      putchar(' '); fflush(stdout);
    }
	}
	if (!n->isLeaf) print_tree_core(n->chi[n->nkey]);
  printf("<%d>", n->high_key);
	printf("]"); fflush(stdout);
}

void
fprint_tree_core(FILE *fp, NODE *n)
{
	fprintf(fp, "["); 
	for (int i = 0; i < n->nkey; i++) {
		if (!n->isLeaf) {
      fprint_tree_core(fp, n->chi[i]);
    } fprintf(fp, "%d", n->key[i]); 
		if (i != n->nkey-1 && n->isLeaf) {
      fprintf(fp, " "); 
    }
	}
	if (!n->isLeaf) fprint_tree_core(fp, n->chi[n->nkey]);
  fprintf(fp, "<%d>", n->high_key);
	fprintf(fp, "]");
}

void
print_tree(NODE *node)
{
	print_tree_core(node);
	printf("\n"); fflush(stdout);
}

void
fprint_tree(FILE *fp, NODE *node)
{
	fprint_tree_core(fp, node);
	fprintf(fp, "\n"); 
}

void
print_node(NODE *n)
{
	int i;

	//printf("%p: ", n);
	for (i = 0; i < n->nkey; i++) {
		//printf("[%p]", n->chi[i]);		
		printf("[%d]", n->key[i]);
	}
	//printf("[%p]", n->chi[i]);
	printf("<%d>\n", n->high_key);		
}

void
fprint_node(FILE *fp, NODE *node)
{
	int i;
	NODE *n = node;
	
	fprintf(fp, "%p: ", n);
	for (i = 0; i < n->nkey; i++) {
		fprintf(fp, "[%p]", n->chi[i]);		
		fprintf(fp, "%d", n->key[i]);
	}
	fprintf(fp, "[%p]", n->chi[i]);
	fprintf(fp, "[%d]\n", n->high_key);
}

void
print_temp(TEMP *t)
{
	int i;

	for (i = 0; i < t->nkey; i++) {
		printf("[%p]", t->chi[i]);		
		printf("%d", t->key[i]);
	}
	printf("[%p]\n", t->chi[i]);		
}
