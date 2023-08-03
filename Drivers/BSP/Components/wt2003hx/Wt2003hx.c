#include "Wt2003hx.h"



const uint8_t header_Song[1]={0x7E};

//����ָ��ָ����������
/*
�ļ����̶�4 ���ַ�����֧������
*/
void Send_PlayMusic(uint8_t *file)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=7;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;

	tmp=Play_by_name;
	buf[2]=tmp;

	for(i=0;i<len-3;i++)
	{
		buf[i+3]=file[i];
	
	}	
	for(i=1;i<len;i++)
	{
		crc16+=buf[i];
	}	
	buf[7]=crc16;    
	buf[8]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);		
}	


//����ָ���������
/*

0xAA     //��ͣ�������AA��  1 2
0xAB     //ֹͣ���AB��   3
0xAC     //��һ������(AC)   5
0xAD     //��һ������(AD)  4

*/
void Send_ComMusic(uint8_t com)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	switch(com)
	{
		case 1: tmp=Pause_Start;
			break;
		case 2: tmp=Pause_Start;
			break;
		case 3: tmp=Stop_Com;
			break;
		case 4: tmp=Last_Step_Com;
			break;
		case 5: tmp=Next_Step_Com;
			break;
		default:
			break;
		
	
	}
	buf[2]=tmp;
	crc16 =tmp+len;
	buf[3]=crc16;
	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	

}

//����ָ�����������С  ӵ�е������
void Send_ComVolume(uint8_t size)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0;
	len=4;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Volume_Control;
	buf[2]=tmp;
	buf[3]=size;	
	crc16=size+tmp+len;
	buf[4]=crc16;
	buf[5]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	

}

//����ָ����Ʋ���ģʽ
/*
00:������ѭ��
01:����ѭ������
02:������Ŀѭ������
03:�������
*/
void Send_ComMode(uint8_t mode)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0;
	len=4;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Assigned_Play;
	buf[2]=tmp;
	buf[3]=mode;	
	crc16=mode+tmp+len;
	buf[4]=crc16;

	buf[5]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	

}

//����ָ�������ϲ���
void Send_Combination_play(uint8_t length,uint16_t *mode)
{
	uint8_t buf[40]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i,j=4;
	len=4+length*2;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Combination_play;
	buf[2]=tmp;
	buf[3]=length;

	for(i=0;i<length;i++)
	{
		utmp16=mode[i];
		buf[j++]=utmp16>>8;
		buf[j++]=(uint8_t)utmp16;

	}
	crc16=0;
	for(i=1;i<len;i++)
	{
	crc16+=buf[i++];

	}
	
	buf[j++]=crc16;
	buf[j++]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	

}

//���Ͳ岥ָ��
/*
��һ�β岥����δ������ʱ�����ڶ��β岥����ʱ��������Ч��Ҫ�ȵ�һ�β岥���ֲ������ſ�����һ�ν��в岥��
*/
void Send_Insert_PMusic(uint16_t position)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=5;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Inter_cut;
	buf[2]=tmp;
	
	utmp16=position;
	buf[3]=utmp16>>8;
	buf[4]=utmp16;

	crc16=0;
	for(i=1;i<len;i++)
	{
		crc16+=buf[i];
	}
	buf[5]=crc16;
	buf[6]=0xEF;
	
	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}
	
//������Ƶ�����ʽ�л�
/*
������00 ��ʾSPK �����01 ��ʾDAC ���  Ĭ��spk
*/
void Send_Output_Change(uint8_t mode)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=4;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Audio_Mode_Change;
	buf[2]=tmp;
	buf[3]=mode;

	crc16=len+tmp+mode;
	buf[4]=crc16;

	buf[5]=0xEF;

	
	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}


//����ָ�����͹���
/*
�ϵ�󲻻�������������ģʽ������ָ��Ż����
ָ��"00"����С��2uA��ָ���ʱ��Ϊ100 ����
ָ��"01"����С��30uA��ָ���ʱ��Ϊ2 ����

ע��оƬ��RXD ���½����л��ѣ����鷢��0x00 0x00 ���л��ѣ�Ҳ����������ͬ��ָ��ѣ���ʹ�����Flash ����ʱ�����߹��������FLash �йء�
���Ѻ󲻻���������������ģʽ���ٴη��ͽ�������ָ��󣬲���Ч��
*/
void Send_Goto_Low_Power(uint8_t mode)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=4;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Low_Power;
	buf[2]=tmp;
	buf[3]=mode;

	crc16=len+tmp+mode;
	buf[4]=crc16;

	buf[5]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}


//��ѯ��ǰ����汾
/*
7E 19 C0 57 54 43 32 32 30 33 32 36 2D 31 35 30 2D 41 32 30 56 34 2E 30 30 A1 EF
�� 57 54 43 32 32 30 33 32 36 2D 31 35 30 2D 41 32 30 56 34 2E 30 30 �� 16 ����ת�ַ�����ʾΪ��
WTC220326-150-A20V4.00 ����WTC������˾��ָ����220326����2022 ��03 ��26 ���з���һ����򣬡�150����
��˾�ڲ����룬��A20������Ӧѡ�ͱ��ܴ��룬��V4.00������Ӧ�汾��
*/
void Send_Current_Version(void)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Current_Version;
	buf[2]=tmp;

	crc16=len+tmp;
	buf[3]=crc16;

	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}


//��ѯ��ǰ��������
void Send_Current_Volume(void)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Current_Volume;
	buf[2]=tmp;

	crc16=len+tmp;
	buf[3]=crc16;

	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}


//��ѯ��ǰ����״̬
/*
��?: 01 ��ʾ�����ţ�
?: 02 ��ʾ��ֹͣ��
?: 03 ��ʾ����ͣ��

*/

void Send_Current_Status(void)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Read_Current_Status;
	buf[2]=tmp;

	crc16=len+tmp;
	buf[3]=crc16;

	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}

//��ѯ��ǰ�����ļ�����
void Send_Number_Song(void)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Query_file_total;
	buf[2]=tmp;

	crc16=len+tmp;
	buf[3]=crc16;

	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}


//��ѯ��ǰ���������ļ���ַ
void Send_Number_Address(void)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=3;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Query_current_play;
	buf[2]=tmp;

	crc16=len+tmp;
	buf[3]=crc16;

	buf[4]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}

//�������л�
/*
�������е�����䣨�ϵ�Ĭ�ϲ�����9600��
�����ʲ�������Ϊʮ��������ʾ�����ϡ�������ֵ��01 C2 00�������ֽڱ�ʾ����Ӧ115200��
*/
void Send_Change_Baud(uint8_t *parameter)
{
	uint8_t buf[20]={0x00},tmp;
	uint16_t utmp16=0,len,crc16=0,i;
	len=6;
	memset(buf,0,sizeof(buf));
	buf[0]=header_Song[0];
	buf[1]=len;
	
	tmp=Change_Baud;
	buf[2]=tmp;
	
	for(i=0;i<len-3;i++)
	{
		buf[i+3]=parameter[i];
	
	}
	for(i=1;i<len;i++)
	{
		crc16+=buf[i];
	}
	buf[6]=crc16;
	buf[7]=0xEF;

	HAL_UART_Transmit(&huart2,buf,strlen(buf),0xFFFF);	
}



