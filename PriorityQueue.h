//Queue Item struct for each element in the queue
struct QueueItem {
	int priority;
	int priorityForReEnquing;
	void (*task)(void);
	struct QueueItem* next;
};

typedef struct QueueItem QueueItem;

QueueItem* newQueueItem(void (*)(void), int);
QueueItem* newQueueItemDelayed(void (*)(void), int, int);
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

void addTask(PriorityQueue*, void(*)(void), int);
void addDelayedTask(PriorityQueue*, void(*)(void), int, int);

void innerAddTask(PriorityQueue*,QueueItem *);

void (*dequeueTask(PriorityQueue*))(void);
void runQueue(PriorityQueue*);


void tick(PriorityQueue*, PriorityQueue*, int);
int isEmpty(PriorityQueue*);
