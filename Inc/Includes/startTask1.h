/**
  ******************************************************************************
  * @file           : startTASK1.h
  * @brief          : Header for startTASK1.c file.
  *                   This file contains the common defines of the application
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STARTTASK1_H
#define __STARTTASK1_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stdbool.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
#define IN1_MASK 0x0001<<0
#define IN2_MASK 0x0001<<1
#define IN3_MASK 0x0001<<2
#define IN4_MASK 0x0001<<3
#define IN5_MASK 0x0001<<4
#define IN6_MASK 0x0001<<5
#define IN7_MASK 0x0001<<6
#define IN8_MASK 0x0001<<7
#define IN9_MASK 0x0001<<8
#define IN10_MASK 0x0001<<9

#define IN1_CLEAN 	0x03FE
#define IN2_CLEAN 	0x03FD
#define IN3_CLEAN		0x03FB
#define IN4_CLEAN 	0x03F7
#define IN5_CLEAN 	0x03EF
#define IN6_CLEAN 	0x03DF
#define IN7_CLEAN 	0x03BF
#define IN8_CLEAN 	0x037F
#define IN9_CLEAN		0x02FF
#define IN10_CLEAN 	0x01FF


	
/* USER CODE END Private defines */
	
/** ��ȫ�����;����Լ� ��Ӧ�� ��Ч�Ե�ƽ���� num=7
*/
	typedef struct __SAFE_PIN
	{
		bool			safeLock1; 			//IN1	��ȫ����1
		bool			safeLock1Level; 			
		bool			safeLock2; 			//IN2
		bool			safeLock2Level; 			
		bool			safeLock3; 			//IN3
		bool			safeLock3Level; 			
		bool			safeLock4; 			//IN4
		bool			safeLock4Level; 
		bool			safeAlarm1;			//IN5	���ⱨ�� ��ѹ/��ѹ
		bool			safeAlarm1Level;	
		bool			safeAlarm2;			//IN6	ˮ�䱨��
		bool			safeAlarm2Level;	
		bool			safeAlarm3;			//IN7 ��������
		bool			safeAlarm3Level;			
	} SafeCtlpin;
	
	/**�������� �Ĵ�������,���䱣������ num=2
	**/
		typedef struct __OPEN_PIN
	{
		bool			openStart; 			//IN8	���⿪���ź� ����Ч
		bool			openLock; 			//IN9	���⿪�������ź�	����Ч
	} OpenCtlpin;
	
	/***IN10 ����
	*/
	
	/*����Ŷ���*/
	//out1  NO2
	//out2	HE
	//out3	N2
	
	
	
	
bool startTask1_Semaphore_Init(void);
void StartTask1(void const * argument);
	
extern SafeCtlpin safePin;
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
