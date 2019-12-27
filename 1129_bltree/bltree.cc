#include "bltree.h"
#include "bltree-main.h"
using namespace std;

NODE *
find_leaf(NODE *node, const int key)
{
	int kid;
	
	while (node->isLeaf == false) {
		int nkey = node->nkey;
		for (kid = 0; kid < nkey; kid++) {
			if (key <= node->key[kid]) { 
				break;
			}
		}
		
		node = node->chi[kid];
	}

	return node;
}

NODE *
insert_in_leaf(NODE *leaf, int key, DATA *data)
{
	int i;
	if (key < leaf->key[0]) {
		for (i = leaf->nkey; i > 0; i--) {
			leaf->chi[i] = leaf->chi[i-1];
			leaf->key[i] = leaf->key[i-1] ;
		} 
		leaf->chi[0] = (NODE *)data;
		leaf->key[0] = key;
	}
	else {
		for (i = 0; i < leaf->nkey; i++) {
			if (key < leaf->key[i]) break;
		}
		for (int j = leaf->nkey; j > i; j--) {		
			leaf->chi[j] = leaf->chi[j-1];
			leaf->key[j] = leaf->key[j-1] ;
		} 
		leaf->chi[i] = (NODE *)data;
		leaf->key[i] = key;
	}

	leaf->nkey = leaf->nkey + 1;

	// necessary for search?
	if (leaf->high_key < key) {
		leaf->high_key = key;
	}
	
	return leaf;
}

void
insert_in_temp(TEMP *temp, int key, void *ptr)
{
	int i;
	if (key < temp->key[0]) {
		for (i = temp->nkey; i > 0; i--) {
			temp->chi[i] = temp->chi[i-1] ;
			temp->key[i] = temp->key[i-1] ;
		} 
		temp->chi[0] = (NODE *)ptr;
		temp->key[0] = key;
	}
	else {
		for (i = 0; i < temp->nkey; i++) {
			if (key < temp->key[i]) break;
		}
		for (int j = temp->nkey; j > i; j--) {		
			temp->chi[j] = temp->chi[j-1] ;
			temp->key[j] = temp->key[j-1] ;
		} 
		temp->chi[i] = (NODE *)ptr;
		temp->key[i] = key;
	}

	temp->nkey++;
}

void
copy_from_temp_to_left(TEMP temp, NODE *left)
{
	left->nkey = (int)ceil(N/2);
	for (int i = (int)ceil(N/2) - 1; i >= 0; i--) {
		left->chi[i] = temp.chi[i];
		left->key[i] = temp.key[i];
	}
}

void
copy_from_temp_to_right(TEMP temp, NODE *right)
{
	//int nkey = 0;

	for (int i = (int)ceil(N/2); i < N; i++) {
		right->chi[i - (int)ceil(N/2)] = temp.chi[i];
		right->key[i - (int)ceil(N/2)] = temp.key[i];
		right->nkey++;
	}
	//DDD((int)right->nkey);
	//print_node(right);
	//right->nkey = (N - (int)ceil(N/2));


	//assert(nkey == right->nkey);
}

void
copy_from_temp_to_left_parent(TEMP *temp, NODE *left)
{
	left->nkey = ((int)ceil((N+1)/2) - 1);
	for (int i = ((int)ceil((N+1)/2) - 2); i >=0 ; i--) {
		left->chi[i+1] = temp->chi[i+1];
		left->key[i]   = temp->key[i];
	}
	left->chi[0] = temp->chi[0];

	// parent
	for (int i = 0; i < left->nkey+1; i++) {
		left->chi[i]->parent = left;
	}
}

void
copy_from_temp_to_right_parent(TEMP *temp, NODE *right)
{
	int id;

	right->nkey = 0;
	for (id = (int)ceil((N+1)/2); id < N; id++) {
		right->chi[id - (int)ceil((N+1)/2)] = temp->chi[id];
		right->key[id - (int)ceil((N+1)/2)] = temp->key[id];
		right->nkey++;
	}
  right->chi[id - ((int)ceil((N+1)/2))] = temp->chi[id];
	
	//assert(right->nkey == (int)ceil((N+1)/2));

	for (int i = 0; i < right->nkey+1; i++) {
		right->chi[i]->parent = right;
	}
}

void
copy_from_left_to_temp(TEMP *temp, NODE *left)
{
	int i;

	bzero(temp, sizeof(TEMP));
	for (i = 0; i < (N-1); i++) {
		temp->chi[i] = left->chi[i];
		temp->key[i] = left->key[i];
	}
	temp->chi[i] = left->chi[i];	
	temp->nkey = N-1;
}

void
finalize_parent(NODE *parent, NODE *right_child, const int lm_key)
{
	int lcid = 0;
	int nkey = parent->nkey;
	
	for (lcid = 0; lcid < nkey; lcid++) {
		if (parent->key[lcid] > lm_key) break; 
	} 
	
	for (int i = nkey; i > lcid; i--) { 
		parent->chi[i+1] = parent->chi[i];
		parent->key[i]   = parent->key[i-1];
	}
	parent->chi[lcid+1] = right_child;
	parent->key[lcid]   = lm_key;
	parent->nkey = nkey + 1;
}

void
finalize_temp(TEMP *temp, NODE *right_child, int lm_key)
{
	int lcid = 0;

	for (lcid = 0; lcid < temp->nkey; lcid++) {
		if (temp->key[lcid] > lm_key) break; 
	} 

	for (int i = temp->nkey+1; i > lcid+1; i--) temp->chi[i] = temp->chi[i-1];
	for (int i = temp->nkey; i > lcid; i--) temp->key[i] = temp->key[i-1];

	temp->chi[lcid+1] = right_child;
	temp->key[lcid] = lm_key;
	temp->nkey++;
}

NODE *
move_right(NODE *node, int key)
{
	NODE *curr = node;
	NODE *prev = NULL;
	NODE *next = NULL;
	int prev_hk = 0;
	
	while (true) {
    if (!curr->right_link) {
      break;
    }
    else {
			// disorder check
      if (prev && prev_hk >= key) {	DDD(prev_hk);	DDD(key);	ERR;}

      next = curr->right_link; // cache
      if (curr->high_key < key) {
				prev_hk = curr->high_key;
				prev = curr;
				lock(next);
				unlock(curr);
        curr = next;
      }
      else {
        break;
      }
    }
  }
  return curr;
}

void
insert_in_parent(NODE *lc, int lm_key, NODE *rc)
{
	NODE *left_child = lc;
	NODE *right_child = rc;
	NODE *left_parent = NULL;

	if (left_child == Root) {
		NODE *new_root = alloc_root(left_child, lm_key, right_child);
		left_child->parent = right_child->parent = new_root;
		Root = new_root;
		if (left_child->isLeaf == false) {
			left_child->nkey = ((int)ceil((N+1)/2) - 1); // subtle change
		} else {
			left_child->nkey = ((int)ceil((N+1)/2)); // subtle change
		}
		unlock(left_child);
		return;
	}
	
	if (left_child->isLeaf == false) {
		left_child->nkey = ((int)ceil((N+1)/2) - 1); // subtle change
	} else {
		left_child->nkey = ((int)ceil((N+1)/2)); // subtle change
	}

	left_parent = left_child->parent;
	lock(left_parent);
  left_parent = move_right(left_parent, lm_key);

	if (left_parent->nkey < N-1) {
		finalize_parent(left_parent, right_child, lm_key);
		unlock(left_child);
		unlock(left_parent);
	}
	else {// split (insert_in_parent)
		TEMP temp;
		NODE *right_parent = alloc_internal(left_parent->parent);		

    // Make a temporal node
		copy_from_left_to_temp(&temp, left_parent);
		finalize_temp(&temp, right_child, lm_key);

		// Distribute and set right node
		copy_from_temp_to_right_parent(&temp, right_parent);
		copy_from_temp_to_left_parent(&temp, left_parent);

		right_parent->high_key = left_parent->high_key;
    right_parent->right_link = left_parent->right_link;
		// Left high-key and link
		left_parent->high_key = temp.key[((int)ceil((N+1)/2) - 1)]; // careful, important
    left_parent->right_link = right_parent;

		
		// Parent
		unlock(left_child);
		insert_in_parent(left_parent, left_parent->high_key, right_parent);
	}
}

void 
insert(int key, DATA *data)
{
	NODE *leaf;

	leaf = find_leaf(Root, key);
	lock(leaf);
  leaf = move_right(leaf, key);
	
	if (leaf->nkey < (N-1)) {
		insert_in_leaf(leaf, key, data);

		unlock(leaf);
		return;
	}
	else { // split (insert)
		NODE *right = alloc_leaf(leaf->parent);
		TEMP temp;
		
		// Distribute: temp
		copy_from_left_to_temp(&temp, leaf);
		insert_in_temp(&temp, key, data);

		// Distribute: right
		copy_from_temp_to_right(temp, right);
		copy_from_temp_to_left(temp, leaf);
		
		right->high_key = leaf->high_key;
    right->right_link = leaf->right_link;

		// Distribute: left
    leaf->high_key = leaf->key[(int)ceil(N/2)-1];
    leaf->right_link = right;

		// Parent
		insert_in_parent(leaf, leaf->high_key, right);
	}
}

void
init_parameter(void)
{
	Root = alloc_leaf(NULL);
  pthread_mutex_init(&GiantLock, NULL);
  pthread_mutex_init(&MutexData, NULL);
  Data = Head.next;
  Fp_log = fopen(ERR_LOGFILE, "w");
}

void *
work_core_insert(void *arg)
{
	int *myid = (int *)arg;
	int range = Size_data / Size_thread;
	int start = range * (*myid);
	int end = start + range;

	for (int i = start; i < end; i++) {
		insert(Key_vector[i], NULL);
  }
	free(myid);

  return NULL;
}

void
search_core(const int key)
{
  NODE *n = find_leaf(Root, key);
	for (int i = 0; i < n->nkey+1; i++) {
		if (n->key[i] == key) return;
	}
  cout << "Key not found: " << key << endl;

  fprint_node(Fp_log, n);
  fprint_tree(Fp_log, Root);

	ERR;
}

void *
work_core_search(void *arg)
{
	int *myid = (int *)arg;
	int range = Size_data / Size_thread;
	int start = range * (*myid);
	int end = start + range;
	
	for (int i = start; i < end; i++) {
		search_core(Key_vector[i]);
  }
	free(myid);

  return NULL;
}

void
work_main(int size, OPTYPE ot)
{
  pthread_t *thread = (pthread_t *)calloc(size, sizeof(pthread_t)); if (!thread) ERR;
	
  for (int i = 0; i < size; i++) {
		int *thread_id = (int *)calloc(1, sizeof(int)); if (!thread_id) ERR; *thread_id = i;
		if (ot == INSERT)	pthread_create(&thread[i], NULL, work_core_insert, (void *)thread_id);
		else if (ot == SEARCH)	pthread_create(&thread[i], NULL, work_core_search, (void *)thread_id);
  }
  for (int i = 0; i < size; i++) {  
    pthread_join(thread[i], NULL);
  }
	free(thread);
}

void 
search_single(void)
{
	for (int i = 0; i < (int)Key_vector.size(); i++) {
		search_core(Key_vector[i]);
  }
}

void
init_global(void)
{
	Size_data = 1000;
	Size_thread = 1;
}

void
print_performance(struct timeval begin, struct timeval end)
{
	long diff = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
	printf("%10.0f req/sec (lat:%7ld usec)\n", ((double)Size_data) / ((double)diff/1000.0/1000.0), diff);
	//printf("%10.0f\n", ((double)Size_data) / ((double)diff/1000.0/1000.0));
}

struct timeval
cur_time(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

int
main(int argc, char *argv[])
{
	struct timeval begin, end;

	init_global();
	if (argc == 2) Size_data = atoi(argv[1]);
	else if (argc == 3) {
		Size_data = atoi(argv[1]);
		Size_thread = atoi(argv[2]);
		assert(Size_thread <= MAX_THREAD);
	}
  std::cout << "Size_data: " << Size_data << std::endl;
  std::cout << "Size_thread: " << Size_thread << std::endl;
	//assert(Size_data % Size_thread == 0);
	
	init_data();
	init_parameter();
	//std::cout << "Init done" << std::endl;

	//printf("----Insert (%d)-----\n", Size_thread);
	begin = cur_time();
  work_main(Size_thread, INSERT);
	end = cur_time();
	print_performance(begin, end);

	/*
	printf("----Search (%d)-----\n", 1);
	begin = cur_time();
  search_single();
	end = cur_time();
	print_performance(begin, end);
	*/
	
	//printf("----Search (%d)-----\n", Size_thread);

	begin = cur_time();
  work_main(Size_thread, SEARCH);
	end = cur_time();
	/*
	print_performance(begin, end);
	*/
	
	return 0;
}
