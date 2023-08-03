/**
  ******************************************************************************
  * @file    tim.c
  * @brief   This file provides code for the configuration
  *          of the TIM instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "timer.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

TIM_HandleTypeDef htim12;
TIM_MasterConfigTypeDef sMasterConfig = {0};
TIM_OC_InitTypeDef sConfig = {0};


void MX_TIM12_Init(uint32_t _ulFreq)
{
	uint16_t usPeriod;
	uint16_t usPrescaler;
	uint32_t uiTIMxCLK;


  TIM_MasterConfigTypeDef sMasterConfig = {0};

#if 0
  htim12.Instance = TIM12;
  htim12.Init.Prescaler =pre-1;              						//��ʱ��6���ֻ���䵽 240M/120/5=400kHz    һ������100�� Ƶ��=400kHz/100=4khz   ���ڣ�1/4k s = 0.25ms �ڲ������ݵ������
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;					//��ʱ��6���ֻ���䵽 240M/240/5=200kHz    һ������100�� Ƶ��=200kHz/100=2khz   ���ڣ�1/2k s = 0.5ms  �ڷ����ݵ������
  htim12.Init.Period =period-1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
#endif
	
	uiTIMxCLK = SystemCoreClock / 2;
	
	if (_ulFreq < 1000)
	{
		usPrescaler = 240 - 1;					/* ��Ƶ�� = 100 */
		usPeriod =  (uiTIMxCLK / 24000) / _ulFreq  - 1;		/* �Զ���װ��ֵ */
	}
	else	/* ����1k��Ƶ�ʣ������Ƶ */
	{
		usPrescaler = 24-1;					/* ��Ƶ�� = 1 */
		usPeriod = (uiTIMxCLK/2400) / _ulFreq - 1;	/* �Զ���װ��ֵ */
	}	
	
	
#if 1	
  htim12.Instance = TIM12;
  htim12.Init.Prescaler =usPrescaler;              						//��ʱ��6���ֻ���䵽 240M/120/5=400kHz    һ������100�� Ƶ��=400kHz/100=4khz   ���ڣ�1/4k s = 0.25ms �ڲ������ݵ������
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;					//��ʱ��6���ֻ���䵽 240M/240/5=200kHz    һ������100�� Ƶ��=200kHz/100=2khz   ���ڣ�1/2k s = 0.5ms  �ڷ����ݵ������
  htim12.Init.Period =usPeriod;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	htim12.Init.ClockDivision     = 0;	
	htim12.Init.RepetitionCounter = 0;
	
	if(HAL_TIM_Base_DeInit(&htim12) != HAL_OK)
	{
		Error_Handler();		
	}

  if (HAL_TIM_Base_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }	
	

	sConfig.OCMode     = TIM_OCMODE_PWM1;
	sConfig.OCPolarity = TIM_OCPOLARITY_LOW;
	sConfig.Pulse = usPeriod / 2;     /* ռ�ձ�50% */
	if(HAL_TIM_OC_ConfigChannel(&htim12, &sConfig, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	/* ����OC1 */
	if(HAL_TIM_OC_Start(&htim12, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}
	
	/* TIM12��TRGO���ڴ���DMAMUX���������� */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;    //TIM_TRGO_UPDATE    TIM_TRGO_OC1REF
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim12, &sMasterConfig);
	
   /* ʹ�ܶ�ʱ�� */
  HAL_TIM_Base_Start(&htim12);	
#endif
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM12)
  {

    __HAL_RCC_TIM12_CLK_ENABLE();

 
		
		
    HAL_NVIC_SetPriority(TIM8_BRK_TIM12_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(TIM8_BRK_TIM12_IRQn);

  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle)
{

  if(tim_baseHandle->Instance==TIM12)
  {

    __HAL_RCC_TIM12_CLK_DISABLE();

    HAL_NVIC_DisableIRQ(TIM8_BRK_TIM12_IRQn);

  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
