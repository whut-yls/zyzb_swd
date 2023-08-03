#ifndef _WT2003HX_H
#define _WT2003HX_H

#include "stdio.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
/* default header*/
#include "main.h"

#define Play_Root_Index  0xA0     //ָ��Flash ��Ŀ¼��������
#define Play_by_name     0xA3     //ָ��Flash ��Ŀ¼�ļ�������  ע:�ĵ���д����A1����ʵ������A3
#define Pause_Start      0xAA     //��ͣ�������AA��
#define Stop_Com         0xAB     //ֹͣ���AB��
#define Next_Step_Com    0xAC     //��һ������(AC)
#define Last_Step_Com    0xAD     //��һ������(AD)

#define Volume_Control    0xAE     //������������
#define Assigned_Play     0xAF     //ָ������ģʽ
#define Combination_play  0xB0     //��ϲ���ģʽ
#define Inter_cut         0xB1     //�岥ָ��
#define Audio_Mode_Change 0xB6     //��Ƶ�����ʽ�л�
#define Low_Power         0xB8     //����͹���ģʽ
#define Current_Version   0xC0     //��ѯ��ǰ����汾
#define Current_Volume    0xC1     //��ѯ��ǰ��������
#define Read_Current_Status 0xC2   //��ȡ��ǰ����״̬
#define Query_file_total   0xC3    //��ѯFLASH �������ļ�����
#define Query_current_play 0xC9    //��ѯ��ǰ����
#define Change_Baud        0xFB    //�������л�

void Send_PlayMusic(uint8_t *file);
void Send_ComMusic(uint8_t com);
void Send_ComVolume(uint8_t size);
void Send_ComMode(uint8_t mode);
void Send_Combination_play(uint8_t length,uint16_t *mode);
void Send_Insert_PMusic(uint16_t position);
void Send_Output_Change(uint8_t mode);
void Send_Goto_Low_Power(uint8_t mode);
void Send_Current_Version(void);
void Send_Current_Volume(void);
void Send_Current_Status(void);
void Send_Number_Song(void);
void Send_Number_Address(void);
void Send_Change_Baud(uint8_t *parameter);



#endif


