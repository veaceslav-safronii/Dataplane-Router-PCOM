#ifndef QUEUE_H
#define QUEUE_H

#include <unistd.h>
#include <stdint.h>

struct queue;
typedef struct queue *queue;

/* create an empty queue */
extern queue queue_create(void);

/* insert an element at the end of the queue */
extern void queue_enq(queue q, void *element);

/* delete the front element on the queue and return it */
extern void *queue_deq(queue q);

/* return a true value if and only if the queue is empty */
extern int queue_empty(queue q);



typedef struct QNode {
	char *packet;
	int len;
	uint32_t ip_dest;
	struct QNode *next;
} *QNode;

typedef struct Queue {
	QNode front, rear;
} *Queue;

int isQueueEmpty(Queue q);

Queue createQueue();

struct QNode* createNode(char *packet, int len, uint32_t ip_dest);

void enQueue(Queue q, char *packet, int len, uint32_t ip_dest);

void deQueue(Queue q, QNode node);


#endif
