#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "stdio.h"
#include "PriorityQueue.h"

static uint8_t wait_msg[] = "Ready queue is empty !!\n";


static int operand = 0; 
static uint8_t command;
static uint8_t commandList[30];
static int commandIterator =0;
static int commandReader =0;

//static char debug_msg[30];


static char timerFlag = 0;

static int ticks = 0;
static int tick_copy = 0;

static QueueItem* rerunItem;
static int rerunFlag = 0;

static PriorityQueue readyQueue, delayedQueue;

void Init(void);
void SysTick_Handler(void);
void USART2_IRQHandler(void);
void Dispatch(void);
static void sendUART(uint8_t * data, uint32_t length);
static uint8_t receiveUART(void);

void uartReader(void);
static const int uartReaderPrio = 8;

void Doubler(void);
static const int doublerPrio = 2;

void Squarer(void);
static const int squarerPrio = 3;

void queueTask(void(*task)(void), int prio);
void queueDelayedTask(QueueItem* qI);
void rerunMe(void(*task)(void), int delay, int prio);
void manageDelayedTasks(void);
void delayMs(int);

void delayMs(int n)
{
	//This delayMs function is adjusted to match the PerformanceInMips value set in the Renode config file
	//It is used to simulate cycles spent executing instructions by a "task"
	
	volatile int i,j;
	for (i = 0; i < n; i++)
		for (j = 0; j < 44167; j++)
				{}
}

void queueTask(void(*task)(void), int prio)
{
	addTask(&readyQueue, task, prio >= 0? prio:0);
}

void USART2_IRQHandler(void) {

	command = receiveUART();
	commandList[commandIterator] = command;
	commandIterator = (commandIterator+1)%30;
	
	queueTask(uartReader, uartReaderPrio);
	
}

void queueDelayedTask(QueueItem* qI)
{
	innerAddTask(&delayedQueue, qI);
}


void SysTick_Handler(void)  
{
	timerFlag = 1;
	ticks++;
}


void Dispatch(void)
{
	if (isEmpty(&readyQueue))
	{
		if(timerFlag){
			sendUART(wait_msg,sizeof(wait_msg));
			timerFlag = 0;
		}
	}
	else
	{
		dequeueTask(&readyQueue)();
		timerFlag = 0;
	}
	manageDelayedTasks(); /* Handle tasks in DelayQueue */
}



void rerunMe(void(*task)(void), int delay, int prio)
{
	if(prio >0){
		rerunItem = newQueueItemDelayed(task,delay,prio >= 0? prio:0);	
		rerunFlag = 1;
	} else {
		queueTask(task, prio >= 0? prio:0);
	}
}


void manageDelayedTasks()
{
	tick_copy = ticks;
	
	//debugging 
//	sprintf(debug_msg, "%d\n",tick_copy);
//	sendUART((uint8_t*)debug_msg,sizeof(debug_msg));
	
	ticks = 0;
	tick(&delayedQueue, &readyQueue, tick_copy);
	if(rerunFlag == 1)
	{
		queueDelayedTask(rerunItem); 
		rerunFlag = 0;
	}
}

void uartReader()
{
	int input = commandList[commandReader];
	commandReader = (commandReader+1)%30;
	
	if(input == 'u')
		operand++;
	else if (input == 'd')
		operand--;

	delayMs(20); // simulating operation

	
	queueTask(Doubler, doublerPrio);
	queueTask(Squarer,squarerPrio);
}

void Doubler()
{
	uint8_t t2msg[] = "Doubler Output:";
	sendUART(t2msg, sizeof(t2msg));
	
	char numString[16] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int sum = operand*2;
	
	delayMs(80); // simulating operation
	
	sprintf(numString, " %d\n",sum);
	sendUART((uint8_t*)numString,sizeof(numString));
}

void Squarer()
{
	uint8_t t3msg[] = "Squarer Output:";
	sendUART(t3msg, sizeof(t3msg));
	
	char numString[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int square = operand*operand;
	
	delayMs(80); // simulating operation

	
	sprintf(numString, " %d\n",square);
	sendUART((uint8_t*)numString,sizeof(numString));
}


static void sendUART(uint8_t * data, uint32_t length)
{
	 for (uint32_t i=0; i<length; ++i){
      // add new data without messing up DR register
      uint32_t value = (USART2->DR & 0x00) | data[i];
		  // send data
			USART2->DR = value;
      // busy wait for transmit complete
      while(!(USART2->SR & (1 << 6)));
		  // delay
      for(uint32_t j=0; j<1000; ++j);
      }
}

static uint8_t receiveUART()
{
	  // extract data
	  uint8_t data = USART2->DR & 0xFF;
	
	  return data;
}

static void gpioInit()
{	
    // enable GPIOA clock, bit 0 on AHB1ENR
    RCC->AHB1ENR |= (1 << 0);

    // set pin modes as alternate mode 7 (pins 2 and 3)
    // USART2 TX and RX pins are PA2 and PA3 respectively
    GPIOA->MODER &= ~(0xFU << 4); // Reset bits 4:5 for PA2 and 6:7 for PA3
    GPIOA->MODER |=  (0xAU << 4); // Set   bits 4:5 for PA2 and 6:7 for PA3 to alternate mode (10)

    // set pin modes as high speed
    GPIOA->OSPEEDR |= 0x000000A0; // Set pin 2/3 to high speed mode (0b10)

    // choose AF7 for USART2 in Alternate Function registers
    GPIOA->AFR[0] |= (0x7 << 8); // for pin A2
    GPIOA->AFR[0] |= (0x7 << 12); // for pin A3
}

static void uartInit()
{
	
    // enable USART2 clock, bit 17 on APB1ENR
    RCC->APB1ENR |= (1 << 17);
	
	  // USART2 TX enable, TE bit 3
    USART2->CR1 |= (1 << 3);

    // USART2 rx enable, RE bit 2
    USART2->CR1 |= (1 << 2);
	
	  // USART2 rx interrupt, RXNEIE bit 5
    USART2->CR1 |= (1 << 5);

    // baud rate = fCK / (8 * (2 - OVER8) * USARTDIV)
    //   for fCK = 16 Mhz, baud = 115200, OVER8 = 0
    //   USARTDIV = 16Mhz / 115200 / 16 = 8.6805
    // Fraction : 16*0.6805 = 11 (multiply fraction with 16)
    // Mantissa : 8
    // 12-bit mantissa and 4-bit fraction
    USART2->BRR |= (8 << 4);
    USART2->BRR |= 11;

    // enable usart2 - UE, bit 13
    USART2->CR1 |= (1 << 13);
}

void Init(void)
{
		/* startup code initialization */
	  SystemInit();
	  SystemCoreClockUpdate();
	
	  /* intialize UART */
	  gpioInit();
	
		/* intialize UART */
	  uartInit();
	
	  /* enable SysTick timer to interrupt system every second */
	  SysTick_Config(SystemCoreClock);
	
	  /* enable interrupt controller for USART2 external interrupt */
		NVIC_EnableIRQ(USART2_IRQn);
	
		/* Unmask External interrupt 0 */
		EXTI->IMR |= 0x0001;
	
	  /* Enable rising and falling edge triggering for External interrupt 0 */
		EXTI->RTSR |= 0x0001;
		EXTI->FTSR |= 0x0001;
	
	  /* enable interrupt controller for External interrupt 0 */
		NVIC_EnableIRQ(EXTI0_IRQn);
	
	
		//SysTick->LOAD = (6100 * 1000) -1; /* reload with number of clocks per second, 100ms */
		//SysTick->LOAD = (16000 * 100) - 1;
		//SysTick->LOAD = SystemCoreClock -1;
    //SysTick->CTRL = 7; /* enable SysTick interrupt, use system clock */
		
		// Intialize Queues
		readyQueue = newPriorityQueue();
		delayedQueue = newPriorityQueue();
		
		// Add initial tasks to queue

}

int main()
{	
	  Init(); /* Initialize structures, uart, gpio etc...*/
		
	  while(1)
		{
			Dispatch(); /* Check ReadyQueue to execute tasks */
		}
}
