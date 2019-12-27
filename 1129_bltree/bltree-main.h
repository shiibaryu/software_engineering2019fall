void print_tree(NODE *node);
void fprint_tree(FILE *fp, NODE *node);
void print_node(NODE *n);
void fprint_node(FILE *fp, NODE *node);
void lock(NODE *node);
void unlock(NODE *node);
void init_data(void);
NODE *alloc_leaf(NODE *parent);
NODE *alloc_internal(NODE *parent);
NODE *alloc_root(NODE *left, int lm_key, NODE *right);
void print_temp(TEMP *t);


LKOBJ **LockArray;
FUNC **Func;
int *CountFunc;
HIST **Hist;
int *CountHist;
vector<uint> Key_vector;
NODE *Root;
DATA Head;
DATA *Tail;
DATA *Data;
pthread_mutex_t GiantLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t MutexData = PTHREAD_MUTEX_INITIALIZER;
int Size_data;
int Size_thread;
FILE *Fp_log = NULL;

