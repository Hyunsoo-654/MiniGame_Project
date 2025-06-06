#include "device_driver.h"

#define TIM1_TICK			(20) 					// usec
#define TIM1_FREQ			(1000000/TIM1_TICK) 	// Hz
#define TIME1_PLS_OF_1ms	(1000/TIM1_TICK)
#define TIM1_MAX			(0xffffu)

void TIM1_Delay_Int(int en, int time)
{
	if(en)
	{
		Macro_Set_Bit(RCC->APB2ENR, 11);

		TIM1->CR1 = (1<<4)+(0<<3)+(0<<0);
		TIM1->PSC = (unsigned int)(TIMXCLK/(double)TIM1_FREQ + 0.5)-1;
		TIM1->ARR = TIME1_PLS_OF_1ms * time;

		Macro_Set_Bit(TIM1->EGR,0);
		Macro_Set_Bit(TIM1->SR, 0);
		NVIC_ClearPendingIRQ((IRQn_Type)25);
		Macro_Set_Bit(TIM1->DIER, 0);
		NVIC_EnableIRQ((IRQn_Type)25);
		Macro_Set_Bit(TIM1->CR1, 0);
	}

    else
	{
		NVIC_DisableIRQ((IRQn_Type)25);
		Macro_Clear_Bit(TIM1->CR1, 0);
		Macro_Clear_Bit(TIM1->DIER, 0);
		Macro_Clear_Bit(RCC->APB2ENR, 11);
	}
}

#define TIM2_TICK			(20) 					// usec
#define TIM2_FREQ			(1000000/TIM2_TICK) 	// Hz
#define TIME2_PLS_OF_1ms	(1000/TIM2_TICK)
#define TIM2_MAX			(0xffffu)

void TIM2_Delay(int time)
{
	int i;
	unsigned int t = TIME2_PLS_OF_1ms * time;

	Macro_Set_Bit(RCC->APB1ENR, 0);

	TIM2->PSC = (unsigned int)(TIMXCLK/(double)TIM2_FREQ + 0.5)-1;
	TIM2->CR1 = (1<<4)|(1<<3);
	TIM2->ARR = 0xffff;
	Macro_Set_Bit(TIM2->EGR,0);
	Macro_Set_Bit(TIM2->DIER, 0);

	for(i=0; i<(t/0xffff); i++)
	{
		Macro_Set_Bit(TIM2->EGR,0);
		Macro_Clear_Bit(TIM2->SR, 0);
		Macro_Set_Bit(TIM2->CR1, 0);
		while(Macro_Check_Bit_Clear(TIM2->SR, 0));
	}

	TIM2->ARR = t % 0xffff;
	Macro_Set_Bit(TIM2->EGR,0);
	Macro_Clear_Bit(TIM2->SR, 0);
	Macro_Set_Bit(TIM2->CR1, 0);
	while (Macro_Check_Bit_Clear(TIM2->SR, 0));

	Macro_Clear_Bit(TIM2->CR1, 0);
	Macro_Clear_Bit(TIM2->DIER, 0);
	Macro_Clear_Bit(RCC->APB1ENR, 0);
}

void TIM2_Repeat_Interrupt_Enable(int en, int time)
{
  if(en)
  {
    Macro_Set_Bit(RCC->APB1ENR, 0);

    TIM2->CR1 = (1<<4)+(0<<3)+(0<<0);
    TIM2->PSC = (unsigned int)(TIMXCLK/(double)TIM2_FREQ + 0.5)-1;
    TIM2->ARR = TIME2_PLS_OF_1ms * time;

    Macro_Set_Bit(TIM2->EGR,0);
    Macro_Set_Bit(TIM2->SR, 0);
    NVIC_ClearPendingIRQ((IRQn_Type)28);
    Macro_Set_Bit(TIM2->DIER, 0);
    NVIC_EnableIRQ((IRQn_Type)28);
    Macro_Set_Bit(TIM2->CR1, 0);
  }

  else
  {
    NVIC_DisableIRQ((IRQn_Type)28);
    Macro_Clear_Bit(TIM2->CR1, 0);
    Macro_Clear_Bit(TIM2->DIER, 0);
    Macro_Clear_Bit(RCC->APB1ENR, 0);
  }
}

#define TIM3_FREQ 	  		(8000000) 	      	// Hz
#define TIM3_TICK	  		(1000000/TIM3_FREQ)	// usec
#define TIME3_PLS_OF_1ms  	(1000/TIM3_TICK)

void TIM3_Out_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 1);
	Macro_Set_Bit(RCC->APB2ENR, 3);
	Macro_Write_Block(GPIOB->CRL,0xf,0xb,0);
	Macro_Write_Block(TIM3->CCMR2,0x7,0x6,4);
	TIM3->CCER = (0<<9)|(1<<8);
}

void TIM3_Out_Freq_Generation(unsigned short freq)
{
	TIM3->PSC = (unsigned int)(TIMXCLK/(double)TIM3_FREQ + 0.5)-1;
	TIM3->ARR = (double)TIM3_FREQ/freq-1;
	TIM3->CCR3 = TIM3->ARR/2;

	Macro_Set_Bit(TIM3->EGR,0);
	TIM3->CR1 = (1<<4)|(0<<3)|(0<<1)|(1<<0);
}

void TIM3_Out_Stop(void)
{
	Macro_Clear_Bit(TIM3->CR1, 0);
	Macro_Clear_Bit(TIM3->DIER, 0);
}

#define TIM4_TICK	  (20) 							// usec
#define TIM4_FREQ 	  (1000000/TIM4_TICK) 			// Hz
#define TIME4_PLS_OF_1ms  (1000/TIM4_TICK)
#define TIM4_MAX	  (0xffffu)

void TIM4_Repeat_Interrupt_Enable(int en, int time)
{
  if(en)
  {
    Macro_Set_Bit(RCC->APB1ENR, 2);

    TIM4->CR1 = (1<<4)+(0<<3)+(0<<0);
    TIM4->PSC = (unsigned int)(TIMXCLK/(double)TIM4_FREQ + 0.5)-1;
    TIM4->ARR = TIME4_PLS_OF_1ms * time;

    Macro_Set_Bit(TIM4->EGR,0);
    Macro_Set_Bit(TIM4->SR, 0);
    NVIC_ClearPendingIRQ((IRQn_Type)30);
    Macro_Set_Bit(TIM4->DIER, 0);
    NVIC_EnableIRQ((IRQn_Type)30);
    Macro_Set_Bit(TIM4->CR1, 0);
  }

  else
  {
    NVIC_DisableIRQ((IRQn_Type)30);
    Macro_Clear_Bit(TIM4->CR1, 0);
    Macro_Clear_Bit(TIM4->DIER, 0);
    Macro_Clear_Bit(RCC->APB1ENR, 2);
  }
}
