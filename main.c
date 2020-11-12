#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "stdio.h"

static uint8_t msg[] = "Renode Alive !!\n";
static uint8_t wait_msg[] = "Ready queue is empty !!\n";
static uint8_t pressedMsg[] = "Button is pressed !!\n";
static uint8_t releasedMsg[] = "Button is released !!\n";
static char buttonPressed = 1;
static char timerFlag = 0;
static volatile uint8_t stopFlag = 0;
static uint32_t ticks = 0;
static uint32_t tempcounter = 0;
static char here[30];


void SysTick_Handler(void);
void USART2_IRQHandler(void);
void TIM2_IRQHandler(void);
void EXTI0_IRQHandler(void);
void Dispatch(void);
static void sendUART(uint8_t * data, uint32_t length);
static uint8_t receiveUART(void);

void SysTick_Handler(void)  {
	timerFlag = 1;
	//tempcounter++;
	//sprintf(here, "%d\n",tempcounter);
	//sendUART((uint8_t*)here,sizeof(here));
//	if (tempcounter%10==0)
//	{
//		sprintf(here, "%d\n",tempcounter);
//		sendUART((uint8_t*)here,sizeof(here));
//	}
		//sendUART(wait_msg,sizeof(wait_msg));
		
	/* Incrementing ticks every 100ms */
	if (ticks == INT32_MAX)
		ticks = 0;
	else
		ticks++;
	sprintf(here,"Ticks: %d\n",ticks);
	sendUART((uint8_t*)here,sizeof(here));
	
}

//void Dispatch(void)
//{
//	if (isEmpty(&readyQueue))
//	{
//		sendUART(wait_msg,sizeof(wait_msg));
//		timerFlag = 0;
//	}
//	else
//	{
//		dequeueTask(&readyQueue)();
//		timerFlag = 0;
//	}
//}

void USART2_IRQHandler(void) {
	/* pause/resume UART messages */
	stopFlag = !stopFlag;
	
	/* dummy read */
	(void)receiveUART();
}

void TIM2_IRQHandler(void)
{
	sendUART(wait_msg,sizeof(wait_msg));
	
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

static void tim2Init()
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
	TIM2->PSC = 1600000;
	TIM2->SMCR = RESET;
	TIM2->SMCR &= ~TIM_SMCR_SMS;
	TIM2->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);
	//TIM2->CR1 |= TIM_COUNTERMODE_UP;
	TIM2->CR1 &= ~TIM_CR1_CKD;
	//TIM2->CR1 |= TIM_CLOCKDIVISION_DIV1;
	TIM2->EGR = TIM_EGR_UG;
	TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;	
	TIM2->SR = 0; /* Clear the update event flag */
	
	TIM2->ARR = 65535;
	TIM2->CR1 |= TIM_CR1_CEN;
}

///* TIM2 Configuration */
///* TIM2 clock enable */
//RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
///* Set the Timer prescaler to get 8MHz as counter clock */
//Prescaler = (uint16_t) (SystemCoreClock / 8000000) - 1; 
//TIM2->SMCR = RESET;
///* Reset the SMCR register */
//#ifdef USE_ETR
///* Configure the ETR prescaler = 4 */
//TIM2->SMCR |= TIM_ETRPRESCALER_DIV4 |
///* Configure the polarity = Rising Edge */
//TIM_ETRPOLARITY_NONINVERTED |
///* Configure the ETR Clock source */
//TIM_SMCR_ECE;
//#else /* Internal clock source */
///* Configure the Internal Clock source */
//TIM2->SMCR &= ~TIM_SMCR_SMS;
//#endif /* USE_ETR */
//TIM2->CR1 &= ~(TIM_CR1_DIR | TIM_CR1_CMS);
///* Select the up counter mode */
//TIM2Clk 3 fETRP = �
//f
//ETR
//1
//3 = -- � 8Mhz = 2.66MHz
//f
//ETRP = 2MHz 1
//3
//=-- � 8MHz
//Timer clocking using external clock-source AN4776
//30/72 AN4776 Rev 3
//TIM2->CR1 |= TIM_COUNTERMODE_UP;
//TIM2->CR1 &= ~TIM_CR1_CKD;
///* Set the clock division to 1 */
//TIM2->CR1 |= TIM_CLOCKDIVISION_DIV1;
///* Set the Autoreload value */
//TIM2->ARR = PERIOD;
///* Set the Prescaler value */
//TIM2->PSC = (SystemCoreClock / 8000000)-1;
///* Generate an update event to reload the Prescaler value immediately */
//TIM2->EGR = TIM_EGR_UG;
//TIM2->CCMR1 &= ~TIM_CCMR1_CC2S;
///* Connect the Timer input to IC2 */
//TIM2->CCMR1 |= TIM_CCMR1_CC2S_0;


//TIM6->SR = 0
/// Set the required delay /
/// The timer presclaer reset value is 0. If a longer delay is required the
//presacler register may be configured to /
///TIM6->PSC = 0 /
//TIM6->ARR = ANY_DELAY_RQUIRED
/// Start the timer counter /
//TIM6->CR1 |= TIM_CR1_CEN
/// Loop until the update event flag is set /
//while (!(TIM6->SR & TIM_SR_UIF));
/// The required time delay has been elapsed /
/// User code can be executed */


int main()
{	
	  /* startup code initialization */
	  SystemInit();
		uint32_t tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
	  SystemCoreClockUpdate();
		tim2Init();
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
		SysTick->LOAD = (6100 * 1000) -1; /* reload with number of clocks per second, 100ms */
		//SysTick->LOAD = (16000 * 100) - 1;
		//SysTick->LOAD = SystemCoreClock -1;
    SysTick->CTRL = 7; /* enable SysTick interrupt, use system clock */

		uint32_t tick_copy;
		//sprintf(here, "%d\n",tmp);
		//sendUART((uint8_t*)here,sizeof(here));
	
	  while(1)
		{
				//sendUART(wait_msg,sizeof(wait_msg));
				//while (!(TIM2->SR & TIM_SR_UIF));
				
//				if(timerFlag && !stopFlag)
//				{
//						//Dispatch();
//						tick_copy = ticks;
//						//ticks = 0;
//						//tick(tick_copy);
//					sprintf(here,"Ticks: %d\n",ticks);
//					sendUART((uint8_t*)here,sizeof(here));
//					timerFlag = 0;
//				}
		}
}
