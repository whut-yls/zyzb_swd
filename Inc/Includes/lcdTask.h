/**
  ******************************************************************************
  * @file           : myTimer.h
  * @brief          : Header for myTimer.c file.
  *                   This file contains the common defines of the application
  *
  ******************************************************************************
  */
 
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LCDTASK_H
#define __LCDTASK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdbool.h"
#include "cmsis_os.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

#define SYNC_OK	1	//ͬ������
#define SYNC_ERR	0	//ͬ���쳣

#define NAME_LEN_MAX	8		//4*2
#define NAME_QR_LEN_MAX		100	//32*1 32���ַ�����   2023 8-24  yls ��ά�볤�ȼӳ�
#define DIS_QRCODE_VAL	0x0000	//��ʾ��ά��ֵ	��λ��Ӧ
#define DIS_FRONT_VAL	0x0001	//��ʾ��ά��ֵ		������Ӧ
#define DIS_BACK_VAL	0x0002	//��ʾ��ά��ֵ		ֹͣ��Ӧ

#define WORK_START_VAL	1
#define WORK_STOP_VAL		2
#define WORK_RESET_VAL	3

#define TIME_SET_ADD   0x009c	     //��Ļ��ʾʱ������
#define COUNT_DOWN_ADD	 0x1500   //0x8500	//��Ļ����ʱ�޸�	
#define TREAT_SEL_ADD	0x1400	//�޸���Ļ���Ʒ���
#define VISIT_NUMBER_ADD	0x1000	//�޸ľ����߱��
#define VISIT_NAME_ADD	0x1100	//����������
#define VISIT_AGE_ADD	0x1200	//����������
#define VISIT_SEX_ADD	0x1300	//�������Ա�
#define QR_CODE_ADD	0x2000	//��Ļ����ά��
#define QR_CODE_ADD_RIGHT	0x3000	//��Ļ�Ҳ��ά��
#define LCD_SYNC_ADD	0x1600	//�豸ͬ��״̬
#define NET_SYNC_ADD	0x1610	//����ͬ��״̬
#define MERIDIAN_ADD	0x1700  //���徭����Ƭ
#define duan_wang_ADD  0x1900   //������ʾ��
#define LCD_Text_box   0x1910   //�ı���ʾ�� by yls 2023/6/17    
#define LCD_Text_content 0x1920  //��ʾ������  by yls 2023/6/17
#define LCD_Text_buttonR  0x00B0     //ģʽ by yls 2023/6/17       
#define LCD_Text_buttonN  0x5aa5  //ģʽ by yls 2023/6/17
#define LCD_DISPLAY_ADD	0x1800	//��Ļ����״̬
#define LCD_WORKSTATUS_ADD	0x1810	//��Ļ����״̬��kardos 2023.02.03
#define WORK_CTL_ADD	0x1800	//�豸����״̬���� 01��ʼ 02ֹͣ 03��λ
#define LCD_STATUS_ADD 0x1620
#define LCD_VOLTAGE_ADD	    0x1410	//��Ļ��ѹ��ʾ
#define LCD_DEVIDE_ID     0x1510     //��ʾ�豸id
#define LCD_SYS_VS        0x1520      //��ʾ�汾��


void send_lcdPage(uint16_t pageNum);    //lcd��Ļ�л�
void send_rtcTime(uint8_t year,uint8_t month,uint8_t day,uint8_t hour,uint8_t minute,uint8_t sec);  //��Ļ��ʾʱ��
void send_countDown(uint8_t hour,uint8_t minute,uint8_t sec);        //��Ļ����ʱ
void send_treatSel(uint16_t freq,uint16_t geat,uint16_t time);    //�޸���Ļ���Ʒ���
void send_visitNumber(uint8_t* data);        //�޸���Ļ�����˱��
void send_visitName(uint8_t* data,uint8_t dataLen);       //�޸���Ļ����������
void send_visitAge(uint8_t age);      //�޸���Ļ����������
void send_visitSex(uint16_t sex);   //�޸���Ļ�������Ա�
void send_QRInfo(uint8_t* data,uint8_t dataLen,uint16_t QR);      //�޸���Ļ��ά����Ϣ
void send_LcdSync(uint8_t status);      //�޸���Ļ����״̬
void send_NetSync(uint8_t status);        //�޸���Ļ��������״̬
void send_StartPic(uint8_t status);       //�޸���Ļ����״̬ͼƬ
void page_ctl_process(uint8_t ctlNum);      //��Ļ���ƴ��� 
void send_duan_wang(uint8_t duan);     //������ʾ��
void send_treatSel_Xwtt(uint16_t wave,uint16_t Apower,uint16_t time,uint16_t Bpower);   //�޸���ĻѨλ��ʹ���Ʒ���
void send_LcdWorkStatus(uint8_t status); //�޸���Ļ����״̬ 
void send_LcdOutStatus(uint8_t status); //�����̵� ���ƽ������
void Send_LcdVoltage(uint16_t voltage);
void LcdData_Handle(uint8_t *src,uint16_t len);    //lcd�������ݴ���

void Send_LcdDevid_id(int32_t Devid);  //�����豸id����Ļ��ʾ
void Send_LcdVersion(uint8_t* version,uint8_t versionLen);	 //���Ͱ汾�ŵ���Ļ��ʾ	 

void Send_Text_Box(uint8_t state);
void Send_Text_Content(uint8_t* data,uint8_t dataLen);
void Send_Text_SetButton(uint8_t page,uint8_t state);

void Lcd_Task(void const * argument);
bool lcdTask_Semaphore_Init(void);
/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
