//Queue Item struct for each element in the queue
struct QueueItem {
	unsigned int priority;
	unsigned int priorityForReEnquing;
	void (*task)(void);
	struct QueueItem* next;
};

typedef struct QueueItem QueueItem;

QueueItem* newQueueItem(void (*)(void), unsigned int);
QueueItem* newQueueItemDelayed(void (*)(void), unsigned int, unsigned int);
void(*getTask(QueueItem*)) (void);

//---------------------------------------------------------
//Priority Queue Class
struct PriorityQueue
{
	QueueItem * head;
	int size;
};

typedef struct PriorityQueue PriorityQueue;
PriorityQueue newPriorityQueue(void);

void addTask(PriorityQueue*, void(*)(void), unsigned int);
void addDelayedTask(PriorityQueue*, void(*)(void), unsigned int, unsigned int);

void innerAddTask(PriorityQueue*,QueueItem *);

void (*dequeueTask(PriorityQueue*))(void);
void runQueue(PriorityQueue*);


void tick(PriorityQueue*, PriorityQueue*, unsigned int);
int isEmpty(PriorityQueue*);
