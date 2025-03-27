#include "queue.h"
#include "list.h"
#include <stdlib.h>
#include <assert.h>

struct queue
{
	list head;
	list tail;
};

queue queue_create(void)
{
	queue q = malloc(sizeof(struct queue));
	q->head = q->tail = NULL;
	return q;
}

int queue_empty(queue q)
{
	return q->head == NULL;
}

void queue_enq(queue q, void *element)
{
	if(queue_empty(q)) {
		q->head = q->tail = cons(element, NULL);
	} else {
		q->tail->next = cons(element, NULL);
		q->tail = q->tail->next;
	}
}

void *queue_deq(queue q)
{
	assert(!queue_empty(q));
	{
		void *temp = q->head->element;
		q->head = cdr_and_free(q->head);
		return temp;
	}
}

int isQueueEmpty(Queue q)
{
	if (q == NULL || q->front == NULL) {
		return 1;
	} else {
		return 0;
	}
}

Queue createQueue()
{
    Queue q = (Queue)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

struct QNode* createNode(char *packet, int len, uint32_t ip_dest)
{
    QNode node = (QNode)malloc(sizeof(struct QNode));
    node->packet = packet;
    node->len = len;
    node->ip_dest = ip_dest;
    node->next = NULL;
    return node;
}

void enQueue(Queue q, char *packet, int len, uint32_t ip_dest)
{
    // Create a new LL node
    QNode node = createNode(packet, len, ip_dest);
 
    // If queue is empty, then new node is front and rear
    // both
    if (q->rear == NULL) {
        q->front = q->rear = node;
        return;
    }
 
    // Add the new node at the end of queue and change rear
    q->rear->next = node;
    q->rear = node;
}

void deQueue(Queue q, QNode node)
{
	if (!isQueueEmpty(q)) return;

	if (node == q->front) {
		if (q->front == q->rear) {
			q->front = NULL;
			q->rear = NULL;
		} else {
		q->front = node->next;
		}
		free(node);
		return;
	}

	for (QNode iter = q->front; iter != NULL; iter = iter->next) {
		if (iter->next == node) {
			iter->next = node->next;
			free(node);
			return;
		}
	}
}
