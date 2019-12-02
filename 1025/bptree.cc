#include "bptree.h"
#include <vector>
#include <sys/time.h>
int Size_data;
vector<uint> Key_vector;

void
print_performance(struct timeval begin, struct timeval end)
{
	long diff = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
	printf("%10.0f req/sec (lat:%7ld usec)\n", ((double)Size_data) / ((double)diff/1000.0/1000.0), diff);
}

struct timeval
cur_time(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

void
init_vector(void)
{
  unsigned int now = (unsigned int)time(0);
  srand(now);
	for (int i = 0; i < Size_data; i++) {
    int key = rand() % (Size_data * 10);		
    printf("%d ", key);
		Key_vector.push_back(key);
	}
  puts("");
}

void
print_tree_core(NODE *n)
{
	printf("["); 
	for (int i = 0; i < n->nkey; i++) {
		if (!n->isLeaf) print_tree_core(n->chi[i]); 
		printf("%d", n->key[i]); 
		if (i != n->nkey-1 && n->isLeaf) putchar(' ');
	}
	if (!n->isLeaf) print_tree_core(n->chi[n->nkey]);
	printf("]");
}

void
print_tree(NODE *node)
{
	print_tree_core(node);
	printf("\n"); fflush(stdout);
}

NODE *
alloc_leaf(NODE *parent)
{
	NODE *node;
	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->isLeaf = true;
	node->parent = parent;
	node->nkey = 0;

	return node;
}

NODE *
alloc_internal(NODE *parent)
{
	NODE *node;
	if (!(node = (NODE *)calloc(1, sizeof(NODE))))
		ERR;
	node->isLeaf = false;
	node->parent = parent;
	node->nkey = 0;

	return node;
}

NODE *
alloc_root(NODE *left, int rs_key, NODE *right)
{
	NODE *node;

	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->parent = NULL;
	node->isLeaf = false;
	node->key[0] = rs_key;
	node->chi[0] = left;
	node->chi[1] = right;
	node->nkey = 1;

	return node;
}

NODE *
find_leaf(NODE *node, int key)
{
	int kid;

	if (node->isLeaf) return node;
	for (kid = 0; kid < node->nkey; kid++) {
		if (key < node->key[kid]) break;
	}

	return find_leaf(node->chi[kid], key);
}

NODE *
insert_in_leaf(NODE *leaf, int key, DATA *data)
{
	int i;
	if (key < leaf->key[0]) {
		for (i = leaf->nkey; i > 0; i--) {
			leaf->chi[i] = leaf->chi[i-1] ;
			leaf->key[i] = leaf->key[i-1] ;
		} 
		leaf->key[0] = key;
		leaf->chi[0] = (NODE *)data;
	}
	else {
		for (i = 0; i < leaf->nkey; i++) {
			if (key < leaf->key[i]) break;
		}
		for (int j = leaf->nkey; j > i; j--) {		
			leaf->chi[j] = leaf->chi[j-1] ;
			leaf->key[j] = leaf->key[j-1] ;
		} 
		leaf->key[i] = key;
		leaf->chi[i] = (NODE *)data;
	}
	leaf->nkey++;

	return leaf;
}

/*void
insert_in_temp(TEMP *temp, int key, void *ptr)
{
	int i;
	if (key < temp->key[0]) {
		for (i = temp->nkey; i > 0; i--) {
			temp->chi[i] = temp->chi[i-1] ;
			temp->key[i] = temp->key[i-1] ;
		} 
		temp->key[0] = key;
		temp->chi[0] = (NODE *)ptr;
	}
	else {
		for (i = 0; i < temp->nkey; i++) {
			if (key < temp->key[i]) break;
		}
		for (int j = temp->nkey; j > i; j--) {		
			temp->chi[j] = temp->chi[j-1] ;
			temp->key[j] = temp->key[j-1] ;
		} 
		temp->key[i] = key;
		temp->chi[i] = (NODE *)ptr;
	}

	temp->nkey++;
}*/


void
insert_in_temp(TEMP *temp, int key, void *ptr)
{
	int ki = 0;
	int pi = 0;

	for (ki = 0; ki < temp->nkey; ki++)
		if (key < temp->key[ki])
			break;
	pi = ki + 1;
	for (int i = temp->nkey; i > ki; i--)
		temp->key[i] = temp->key[i - 1];
	for (int i = temp->nkey + 1; i > pi; i--)
		temp->chi[i] = temp->chi[i - 1];

	temp->key[ki] = key;
	temp->chi[pi] = (NODE *)ptr;
	temp->nkey++;
}


void
erase_entries(NODE *node)
{
	for (int i = 0; i < N-1; i++) node->key[i] = 0;
	for (int i = 0; i < N; i++) node->chi[i] = NULL;
	node->nkey = 0;
}

void
copy_from_temp_to_left(TEMP temp, NODE *left)
{
	int i=0;
	for (i = 0; i < (int)ceil(N/2); i++) {
		left->key[i] = temp.key[i];
		left->chi[i] = temp.chi[i];
		left->nkey++;
	}
}

void
copy_from_temp_to_right(TEMP temp, NODE *right)
{
	int i;
	for (i = (int)ceil(N/2); i < N; i++) {
		right->key[i - (int)ceil(N/2)] = temp.key[i];
		right->chi[i - (int)ceil(N/2)] = temp.chi[i];
		right->nkey++;
	}
}

void
copy_from_temp_to_left_parent(TEMP *temp, NODE *left)
{
    int i;
    int ret = (N+1)/2;
    for (i = 0; i < ret; i++) {
        left->key[i] = temp->key[i];
        left->chi[i] = temp->chi[i];
    }
    left->nkey = ret;
    left->chi[i] = temp->chi[i];
}

void
copy_from_temp_to_right_parent(TEMP *temp, NODE *right)
{
   int i, j;
   int ret = (N+1)/2+1;	
   for (i = ret, j = 0; i < temp->nkey; i++, j++) {
       right->key[j] = temp->key[i];
       right->chi[j] = temp->chi[i];
   }
   right->chi[j] = temp->chi[i];
   right->nkey = temp->nkey - ret;
   
	for(i=0;i<right->nkey+1;i++){
		if(right->chi[i]->parent != right){
			right->chi[i]->parent = right;
		}
	}
}

void
copy_from_left_to_temp(TEMP *temp, NODE *left)
{
	int i;
	for (i = 0; i < (N-1); i++) {
		temp->chi[i] = left->chi[i];
		temp->key[i] = left->key[i];
	} temp->nkey = N-1;
	temp->chi[i] = left->chi[i];	
}

void
insert_after_left_child(NODE *parent, NODE *left_child, int rs_key, NODE *right_child)
{
	int keyid = 0;
	int ptrid = 0; // right_child_id
	int i;

  // Find the place to insert
	for (i = 0; i < parent->nkey+1; i++) {
		if (parent->chi[i] == left_child) {
			keyid = i; // key_id
			ptrid = keyid+1; break; 
		}
	} 

  // Shift
	for (i = parent->nkey+1; i > ptrid; i--) parent->chi[i] = parent->chi[i-1];		
	for (i = parent->nkey; i > keyid; i--) parent->key[i] = parent->key[i-1];

  // Insert
	parent->key[keyid] = rs_key;
	parent->chi[ptrid] = right_child;
	parent->nkey++;
}

/*void
insert_after_left_child(NODE *parent, NODE *left_child, int rs_key, NODE *right_child)*/
/*void insert_after_left_child(int key,NODE *parent,NODE *left_child,NODE *right_child)
{
	int i;
	int left_id=0;
	int right_id=0;

	for(i=0;i<parent->nkey+1;i++){
		if(parent->chi[i] == left_child){
			left_id = i;
			right_id = left_id+1;
			break;
		}
	}
	
	for(i=parent->nkey;i>left_id;i--){
		parent->key[i] = parent->key[i-1];
	}
	for(i=parent->nkey++;i>right_id;i--){
		parent->chi[i] = parent->chi[i-1];
	}
	
	parent->key[left_id] = key;
	parent->chi[right_id] = right_child;
	parent->nkey++;
	return;
}*/

void
print_temp(TEMP t)
{
	int i;

	for (i = 0; i < t.nkey; i++) {
		printf("[%p]", t.chi[i]);		
		printf("%d", t.key[i]);
	}
	printf("[%p]\n", t.chi[i]);		
}



void
insert_in_parent(NODE *left_child, int rs_key, NODE *right_child)
{
	NODE *left_parent;
	NODE *right_parent;
	
	if (left_child == Root) {
		Root = alloc_root(left_child, rs_key, right_child);
		left_child->parent = right_child->parent = Root;
		return;
	}
	left_parent = left_child->parent;
  if (left_parent->nkey < N-1) {
		insert_after_left_child(left_parent, left_child, rs_key, right_child);
	}
	else {// split
		TEMP temp;
		copy_from_left_to_temp(&temp, left_parent);
		insert_in_temp(&temp,rs_key,right_child);
	
		erase_entries(left_parent);	
		right_parent = alloc_internal(left_parent->parent);
		copy_from_temp_to_left_parent(&temp, left_parent);
		int rs_key_parent = temp.key[(int)ceil(N/2)]; 
		copy_from_temp_to_right_parent(&temp, right_parent);
		insert_in_parent(left_parent, rs_key_parent, right_parent);
	}
}

void 
insert(int key, DATA *data)
{
	NODE *leaf;

	if (Root == NULL) {
		leaf = alloc_leaf(NULL);
		Root = leaf;
	}
	else {
		leaf = find_leaf(Root, key);
	}

	if (leaf->nkey < (N-1)) {
		insert_in_leaf(leaf, key, data);
	}
	else { // split
		NODE *left = leaf;
		NODE *right = alloc_leaf(leaf->parent);
		TEMP temp;

		copy_from_left_to_temp(&temp, left);
		insert_in_temp(&temp, key, data);

		right->chi[N-1] = left->chi[N-1];	
		left->chi[N-1] = right;
		erase_entries(left);
		copy_from_temp_to_left(temp, left);
		copy_from_temp_to_right(temp, right);
		int rs_key = right->key[0]; // right smallest key
		insert_in_parent(left, rs_key, right);
	}
}

void
init_root(void)
{
	Root = NULL;
}

void
search_core(const int key)
{
  NODE *n = find_leaf(Root, key);
	for (int i = 0; i < n->nkey+1; i++) {
		if (n->key[i] == key) return;
	}
  cout << "Key not found: " << key << endl;
	ERR;
}

void 
search_single(void)
{
	for (int i = 0; i < (int)Key_vector.size(); i++) {
		search_core(Key_vector[i]);
  }
}

int 
interactive()
{
  int key;

  std::cout << "Key: ";
  std::cin >> key;

  return key;
}

int
main(int argc, char *argv[])
{
  while (true) {
		insert(interactive(), NULL);
    print_tree(Root);
  }

	return 0;
}
