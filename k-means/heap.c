#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "heap.h"

typedef struct Heap {
	// neighboring nodes in doubly linked list
	struct Heap *heap__left;
	struct Heap *heap__right;

	struct Heap *parent; // similar to neighbors: need to move up and
						 // down levels
	int rank;
	struct Heap *child; // maintain a single pointer to children
						// since doubly linked, only need access to one

	int marked;

	// specific node information:
	void *payload;
	// weight used for deciding the min heap pointer:
	void *weight;
} heap_node;

struct MinPointer {
	int nodeSum;
	int maxRank; // starts at 0
	heap_node *min;

	int (*compare)(void *, void *);
};

heap_t *heap_create(int (*compare)(void *, void *)) {
	heap_t *emptyHeap = malloc(sizeof(heap_t));

	emptyHeap->nodeSum = 0;
	emptyHeap->maxRank = 0;

	emptyHeap->compare = compare;
	emptyHeap->min = NULL;

	return emptyHeap;
}

void *heap_push(heap_t *head, void *payload, void *weight) {
	heap_node *heapNode = malloc(sizeof(heap_node));

	// initialize all parameters in heapNode:
	heapNode->payload = payload;
	heapNode->weight = weight;

	heapNode->marked = 0;

	heapNode->parent = NULL;
	heapNode->rank = 0;
	heapNode->child = NULL;
	// circularly link this heapNode (when head->min == NULL):
	heapNode->heap__left = heapNode;
	heapNode->heap__right = heapNode;

	// new heap case: head->min == NULL:
	//    set this node to the new min
	//    then fix neighbors

	if (!head->min)
		head->min = heapNode;

	// connect heapNode as neighbor of min (trivial for head->min == NULL):
	heapNode->heap__right = head->min->heap__right;
	heapNode->heap__right->heap__left = heapNode;
	head->min->heap__right = heapNode;
	heapNode->heap__left = head->min;

	// check weight compared to min, reset min in new heapNode has a lower weight:
	if (head->compare(weight, head->min->weight))
		head->min = heapNode;

	head->nodeSum++;
	return heapNode;
}

void *heap_peek(heap_t *head) {
	return head->min ? head->min->payload : NULL;
}

heap_node *fixConflictNode(heap_node *conflictNode, heap_node *someNode, int *rankSize) {

	// define which pointer will be which
	heap_node *makeChild = conflictNode->weight < someNode->weight ? someNode : conflictNode;
	heap_node *makeParent = conflictNode->weight < someNode->weight ? conflictNode : someNode;

	// have to splice makeChild from root list
	makeChild->heap__left->heap__right = makeChild->heap__right;
	makeChild->heap__right->heap__left = makeChild->heap__left;

	// insert as child into makeParent
	// and makeChild parent's pointer makeParent
	if (makeParent->child) { // splice
		makeParent->child->heap__right->heap__left = makeChild;
		makeChild->heap__right = makeParent->child->heap__right;
		makeParent->child->heap__right = makeChild;
		makeChild->heap__left = makeParent->child;
	} else {// make makeChild the makeParent child pointer
		makeChild->heap__left = makeChild;
		makeChild->heap__right = makeChild;
		makeParent->child = makeChild;
	}

	makeChild->parent = makeParent;

	return makeParent;
}

// takes in a node and finds the smallest root node:
heap_node *fixHeap(heap_t *head, heap_node *startNode, int *rankSize, int nodeNumber) {
	heap_node *someNode = startNode ? startNode->heap__right : NULL;
	heap_node *min = someNode ? someNode : NULL;

	// make array of size max rank:
	int max_deg = ceil(log2(nodeNumber));
	heap_node *rankarray[max_deg];
	memset(rankarray, 0, sizeof(rankarray));

	while (someNode != startNode) {
		// check someNode against rankarray table
		heap_node *conflictNode = rankarray[someNode->rank];
		// if there is a value at conflictNode need to combine one to the other
		// (based on which has a smaller weight):

		// used for checking conflicts after altering tree
		heap_node *finalParent = someNode;
		heap_node *next_someNode = someNode->heap__right; // temp store value right of someNode
		while (conflictNode != NULL) {

			// clear conflictnode from rankarray
			rankarray[conflictNode->rank] = NULL;

			finalParent = fixConflictNode(conflictNode, finalParent, rankSize);

			// increase parent rank
			finalParent->rank++;

			conflictNode = rankarray[finalParent->rank];
		}

		// set whatever finalParent->rank equals position in array:
		rankarray[finalParent->rank] = finalParent;

		if (head->compare(finalParent->weight, min->weight))
			min = finalParent;

		someNode = next_someNode;
	}

	return min;
}

void *heap_pop(heap_t *head) {
	if (!head->min) // uhhh
		return NULL;

	// get children
	heap_node *child = head->min->child;
	head->min->child = NULL;
	// insert children as a root node:
	if (child) {
		child->parent = NULL;
		// grab the the another heap child, so we grab each side of the link, then can pull it up:
		/*
			child 		otherNodes				endConnector
			|			|			|			|
			50			30			45			10
		*/
		heap_node *endConnector = child->heap__left;

		// doesn't matter if endConnector equals child or not:
		endConnector->heap__right = head->min->heap__right;
		head->min->heap__right->heap__left = endConnector;
		head->min->heap__right = child;
		child->heap__left = head->min;

		head->min->child = NULL;
	}

	// get the old min:
	heap_node *prevMin = head->min;

	// update min
	if (head->min != head->min->heap__right) {
		head->min = fixHeap(head, prevMin, &head->maxRank, head->nodeSum); // also finds minimum
	} else
		head->min = NULL;

	head->nodeSum--;

	// splice complete,
	// extract previous min
	heap_node *neighbor = prevMin->heap__right;
	neighbor->heap__left = prevMin->heap__left;
	neighbor->heap__left->heap__right = neighbor;

	// extract payload to return
	void *prevMinPayload = prevMin->payload;
	// delete the previous min
	free(prevMin->weight);
	free(prevMin);
	return prevMinPayload;
}

int heap_decrease_key(heap_t *head, void *key, void *newWeight) {
	heap_node *node = (heap_node *) key;
	// decrease key of node:
	node->weight = newWeight;

	// move to root if newWeight is less than parent weight
	if (!node->parent) {// check min then finished
		head->min = head->compare(newWeight, head->min->weight) ? node : head->min;

		return 0;
	}

	if (head->compare(newWeight, node->parent->weight)) {
		// splice node into root level (to the right of min)
		node->heap__left = head->min;
		node->heap__right = head->min->heap__right;
		head->min->heap__right->heap__left = node;
		head->min->heap__right = node;

		// at this point update head->min IF node is less than min
		if (head->compare(newWeight, head->min->weight))
			head->min = node;

		// extract node previous parent
		heap_node *prevParent = node->parent;
		prevParent->rank--;
		prevParent->child = NULL;

		node->parent = NULL;

		// check if parent is already marked
		// if it is, need to splice parent into root list as well:
		if (prevParent->marked) {
			prevParent->marked = 0;
			heap_decrease_key(head, (void *) prevParent, prevParent->weight);
		} else if (prevParent->parent)
			// mark parent as per norm (UNLESS parent is a root
			// meaning parent->parent should exist for marking to occur)
			prevParent->marked = 1;
	}

	return 0;
}

int METAheap_destroy(heap_node *start) {
	heap_node *initialNode = start;
	heap_node *buffer;

	do {
		// delete all children
		if (start->child)
			METAheap_destroy(start->child);

		buffer = start->heap__right;
		free(start->payload);
		free(start->weight);
		free(start);
		start = buffer;
	} while (start != initialNode);

	return 0;
}

int heap_destroy(heap_t **head) {
	if ((*head)->min) METAheap_destroy((*head)->min);

	free(*head);
	*head = NULL;

	return 0;
}