// Source: https://www3.cs.stonybrook.edu/~skiena/392/programs/queue.c

#include <stdbool.h>

#define QUEUESIZE 18

struct Queue {
	// Queue body
        int q[QUEUESIZE+2];
	// Position of first element		        
        int first;
	// Position of last element
        int last;      
	// Number of queue elements
        int count;
};

void init_queue(struct Queue *q);
void enqueue(struct Queue *q, int x);
int dequeue(struct Queue *q);
bool empty(struct Queue *q);
void print_queue(struct Queue *q);
int peek(struct Queue *q);
char* get_queue_string(struct Queue *q);
int count(struct Queue *q);