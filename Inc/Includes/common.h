/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : myTimer.h
  * @brief          : Header for myTimer.c file.
  *                   This file contains the common defines of the application
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYCOMMON_H
#define __MYCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stdbool.h"
#include "parameter.h"
/* Private includes ----------------------------------------------------------*/
#define	HIGH		1
#define LOW			0
/* USER CODE END Includes */
/* Private defines ------------------------------*/

extern char gAck[JSON_ACK_MAX_LEN];
//float Getf_Average(float *pt,int l);
uint16_t Gets_Average(uint16_t *pt,int l);

/*电机相关*/
int SendPlusePressData(void);

void StoragePlusePressData(void);
bool isVaildIp(const char *ip,unsigned char*intIp);
bool isVaildMac(char *mac,unsigned char*intMac);

void general_sessionAck(int fun,int status,char*msg);
void Send_Fix_Ack(int functionFlag,int status,char*msg);
void Send_JianChe_Ack(int functionFlag,int status,char*msg);
void Send_SysCheck_Ack(int functionFlag,int status,char*msg);
void Send_Rj45_Ack(int functionFlag,int status,char*msg);
void Send_Wifi_Ack(int functionFlag,int status,char*msg);
void Send_BaseArg_Ack(int functionFlag,int status,char*msg);

void do_work_ctl(uint8_t workMode);
void close_Dev_Out(void);//关闭输出

int HexStrToByte(const char* source, unsigned char* dest, int sourceLen);

/*心跳相关 by hyadd 2023/3/8*/
void general_heartBag(int fun, int status, int netKind, int workState, int timeLast);
void Send_heartBag(int fun, int status, int netKind, int workState, int timeLast);
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
