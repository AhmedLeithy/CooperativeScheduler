#include "PriorityQueue.h"
#include <stdlib.h> 

//debugging
//#include "stm32f4xx.h"
//static void sendUART(uint8_t * data, uint32_t length)
//{
//	 for (uint32_t i=0; i<length; ++i){
//      // add new data without messing up DR register
//      uint32_t value = (USART2->DR & 0x00) | data[i];
//		  // send data
//			USART2->DR = value;
//      // busy wait for transmit complete
//      while(!(USART2->SR & (1 << 6)));
//		  // delay
//      for(uint32_t j=0; j<1000; ++j);
//      }
//}
//static uint8_t msg[] = "  Debugging \n";

//Queue Item Function Implementations

//Constructor
QueueItem* newQueueItem(void (*task)(void), unsigned int priority){
	QueueItem *q = (QueueItem*)malloc(sizeof(QueueItem));
	q->priority =priority;
	q->task = task;
	q->next = 0;
	q->priorityForReEnquing =0;
	return q;
}

QueueItem* newQueueItemDelayed(void (*task)(void), unsigned int priority, unsigned int delayedPriority){
	QueueItem *q = (QueueItem*)malloc(sizeof(QueueItem));
	q->priority =priority;
	q->task = task;
	q->next = 0;
	q->priorityForReEnquing =delayedPriority;
	return q;
}

//Getter for the task
void(*getTask(QueueItem* qi)) (void){
	return qi->task;
}

//---------------------------------------------------------
//Priority Queue Function Implementations

PriorityQueue newPriorityQueue(void){
	
	PriorityQueue *pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
	pq->size = 0;
	pq->head = 0;
	return *pq;
}
void addDelayedTask(PriorityQueue* queue, void(*task)(void), unsigned int delay, unsigned int delayedPriority){
	queue->size++;
	QueueItem * newItem = newQueueItemDelayed(task,delay,delayedPriority);
	innerAddTask(queue,newItem);
}


void addTask(PriorityQueue* queue, void(*task)(void), unsigned int priority){
	//add to queue
	//low value of priority is at the beginning of the queue
	queue->size ++;
	QueueItem * newItem = newQueueItem(task,priority);
	innerAddTask(queue, newItem);
}
void innerAddTask(PriorityQueue* queue, QueueItem * qi){
	if(queue->head == 0){
		
		queue->head = qi;
	} 
	else{
		
		if(queue->head->priority > qi->priority){
			qi->next = queue->head;
			queue->head = qi;
		}
		else{
			QueueItem * temp = queue->head;
			
			while(temp->next != 0 && temp->next->priority < qi->priority){
				temp = temp->next;
			}
			
			qi->next = temp->next;
			temp->next = qi;

		}
	}
}

//Dequing task from the beginning
void(*dequeueTask(PriorityQueue* queue)) (void){
	QueueItem * temp = queue->head;
	queue->size--;
	
	queue->head = queue->head->next;
	void (*task)(void) = temp->task;
	
	free(temp);
	
	return task;
}

//Returns whether Queue is empty
int isEmpty(PriorityQueue*queue){
	return queue->head == 0;
}

void runQueue(PriorityQueue* q){
	QueueItem *temp = q->head;
	while(temp!=0){
		temp->task();
		temp = temp->next;
	}
}

void tick(PriorityQueue* delayedQueue, PriorityQueue* readyQueue, unsigned int ticks){
	QueueItem *temp = delayedQueue->head;
	while(temp!=0){
		temp->priority -= ticks;
		temp = temp->next;
	}
	temp = delayedQueue->head;
	while(temp!=0){
		if(temp->priority <= 0){
			QueueItem * readyQueueItem = newQueueItem(temp->task,temp->priorityForReEnquing); 
			innerAddTask(readyQueue, readyQueueItem);
			dequeueTask(delayedQueue);
			temp = delayedQueue->head;
		}
		else
			return;
	}
}

