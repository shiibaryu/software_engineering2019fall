#include "bptree.h"
#include <vector>
#include <sys/time.h>

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
	if (!(node = (NODE *)calloc(1, sizeof(NODE)))) ERR;
	node->isLeaf = false;
	node->parent = parent;
	node->nkey = 0;

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
    // Step 1: Find the place to insert
		for (i = 0; i < leaf->nkey; i++) {
			if (key < leaf->key[i]) break;
		}

    // Step 2: Shift and insert
		for (int j = leaf->nkey; j > i; j--) {		
			leaf->chi[j] = leaf->chi[j-1] ;
			leaf->key[j] = leaf->key[j-1] ;
		} 
		leaf->key[i] = key;
		printf("first %d\n",leaf->key[i]);
		leaf->chi[i] = (NODE *)data;
	}
	leaf->nkey++;

	return leaf;
}

void
copy_from_left_to_temp(TEMP *temp, NODE *left)
{
  // Step 1
  int i;
  for(i=0;i<left->nkey;i++){
	temp->key[i] = left->key[i];
	temp->chi[i] = left->chi[i];
  }
  temp->nkey = left->nkey;
  temp->chi[i] = left->chi[i];
}

void
insert_in_temp(TEMP *temp, int key, void *ptr)
{
  // Step 2
  int nkey = temp->nkey;
  temp->key[nkey] = key;
  temp->chi[nkey] = (NODE*)ptr;
  temp->nkey++;
}

void
erase_entries(NODE *node)
{
  // Step 4
  int i;
  for(i=0;i<node->nkey;i++){
	node->key[i] = 0;
        node->chi[i] = NULL;
  }
}

void
copy_from_temp_to_left(TEMP temp, NODE *left)
{
  // Step 5
  int i;
  for(i=0;i<temp.nkey/2;i++){
	left->key[i] = temp.key[i];
	//printf("temp left %d\n",temp.key[i]);
        left->chi[i] = temp.chi[i];
  }
  left->nkey = i;
  //left->chi[i*2] = temp.chi[i];
  
}

void
copy_from_temp_to_right(TEMP temp, NODE *right)
{
  // Step 6
  int i;
  int n = temp.nkey;
  for(i=0;i<n/2;i++){
	right->key[i] = temp.key[i+2];
	right->chi[i] = temp.chi[i+2];
  }	
  right->nkey = i;
  
}

void
insert_in_parent(NODE *left_child, int rs_key, NODE *right_child)
{
  // Step 8 
  // You do not need to implement today.
  // This is for the next week.

  // printf("Left:  "); print_tree(left_child);
  // printf("Right: "); print_tree(right_child);
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

		copy_from_left_to_temp(&temp, left);   // 0
		insert_in_temp(&temp, key, data);      // 1
                print_temp(temp);
		right->chi[N-1] = left->chi[N-1];	     // 2
		left->chi[N-1] = right;                // 3
		erase_entries(left);                   // 4
		copy_from_temp_to_left(temp, left);    // 5
		copy_from_temp_to_right(temp, right);  // 6
		int rs_key = right->key[0];            // 7
		insert_in_parent(left, rs_key, right); // 8
	}
}

void
init_root(void)
{
	Root = NULL;
}

/*
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
*/

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
	init_root();

  while (true) {
	insert(interactive(), NULL);
   	print_tree(Root);
  }

	return 0;
}
