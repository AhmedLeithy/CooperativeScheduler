#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "PriorityQueue.h"

static uint8_t msg[] = "Idle !!\n";
static uint8_t pressedMsg[] = "Button is pressed !!\n";
static uint8_t releasedMsg[] = "Button is released !!\n";
static char buttonPressed = 1;
static char timerFlag = 0;
static volatile uint8_t stopFlag = 0;

void printHello(void);
void printTask1(void);
void printTask2(void);
void printTask3(void);
void printTask4(void);
void ReRunMe(int, int);
static PriorityQueue readyQueue, delayedQueue;

void SysTick_Handler(void);
void USART2_IRQHandler(void);
void EXTI0_IRQHandler(void);
static void sendUART(uint8_t * data, uint32_t length);
static uint8_t receiveUART(void);

void SysTick_Handler(void)  {
	timerFlag = 1;
	//sendUART(pressedMsg, sizeof(pressedMsg));
}

void USART2_IRQHandler(void) {
	/* pause/resume UART messages */
	stopFlag = !stopFlag;
	
	/* dummy read */
	(void)receiveUART();
}

void EXTI0_IRQHandler(void) {
		/* Clear interrupt request */
		EXTI->PR |= 0x01;
		/* send msg indicating button state */
		if(buttonPressed)
		{
				sendUART(pressedMsg, sizeof(pressedMsg));
				buttonPressed = 0;
		}
		else
		{
				sendUART(releasedMsg, sizeof(releasedMsg));
				buttonPressed = 1;
		}
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

void printHello(){
	sendUART(msg, sizeof(msg));
}

void printTask1(){
	uint8_t task1[] = "I am Task 1!!\n";
	sendUART(task1, sizeof(task1));
	
	addDelayedTask(&delayedQueue, printTask1, 10,5);
}
void printTask2(){
	uint8_t task2[] = "I am Task 2!!\n";
	sendUART(task2, sizeof(task2));
	
	addDelayedTask(&delayedQueue, printTask2, 10,6);
}
void printTask3(){
	uint8_t task3[] = "I am Task 3!!\n";
	sendUART(task3, sizeof(task3));
	
	addDelayedTask(&delayedQueue, printTask3, 10,0);

}
void printTask4(){
	uint8_t task4[] = "I am Task 4!!\n";
	sendUART(task4, sizeof(task4));
	addDelayedTask(&delayedQueue, printTask4, 10,3);
}

int main()
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
	
		readyQueue =  newPriorityQueue();
		delayedQueue =  newPriorityQueue();

		addTask(&readyQueue, printTask1,5);
		addTask(&readyQueue, printTask2,6);
		addTask(&readyQueue, printTask3,0);
		addTask(&readyQueue, printTask4,3);
		
	  while(1)
		{
				if(timerFlag && !stopFlag)
				{
					if(isEmpty(&readyQueue)){
						sendUART(msg,sizeof(msg));
					  timerFlag = 0;
					}
					else{
						dequeueTask(&readyQueue)();
						timerFlag = 0;
					}
					tick(&delayedQueue,&readyQueue,1);
					
				}
		}
}

//Dequing task from the beginning

