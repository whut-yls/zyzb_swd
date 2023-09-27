#include "EC20_4G.h"
#include "main.h"
//#include "usart.h"
#include "stdio.h"
#include "string.h"
//#include "gpio.h"
//#include "delay.h"
#include "common.h"
#include "user_tcp_client.h"
#include "ethernetif.h"
#include "mqttTask.h"
#include "wifi.h"
#include "lcdTask.h"

uint8_t aTxBuffer[]="���ڷ�������\r\n";
uint8_t aRxBuffer1[1024];		// �������մ���1���͵�����
uint8_t aRxBuffer2[2048];		// �������մ���4���͵�����		AT+QICLOSE=0
extern	bool yd_4gfalg;
char *cjdata_f_4g,*cjdata_p_4g;
char _cjdata_BK_Num_4g=0,_cjdata_i_4g;

int netData_process(char *payload,int payloadLen);
uint8_t aTxBuffer0[]="AT+QICLOSE=0\r\n";		//�ر��ϴε�socket����
uint8_t aTxBuffer1[]="AT\r\n";							//����ָ�������OK����ģ�������ͨ��
//uint8_t aTxBuffer2[]="AT+QICSGP=1,1,\"CMNET\"\r\n";
//uint8_t aTxBuffer3[]="AT+QIDEACT=1\r\n";
//uint8_t aTxBuffer4[]="AT+QIACT=1\r\n";
uint8_t aTxBuffer2[]="AT+CPIN?\r\n";				//���SIM���Ƿ���λ
uint8_t aTxBuffer3[]="AT+CSQ\r\n";					//��ѯ�ź�����
uint8_t aTxBuffer4[]="AT+CREG?\r\n";				//��ѯ����״̬
uint8_t aTxBuffer5[]="AT+CGATT=1\r\n";			//��������
uint8_t aTxBuffer6[]="AT+CGATT?\r\n";				//��ѯ����״̬
uint8_t aTxBuffer7[]="AT+CGPADDR\r\n";			//��ȡPDP��ַ
uint8_t aTxBuffer8[]="AT+QMTCFG=\"recv/mode\",0,0,1\r\n";				//����Ϊ����ģʽ
uint8_t aTxBuffer9[]="AT+QMTOPEN=0,\"124.239.10.57\",9300\r\n";				//	���ð������ϵ��豸��Ϣ
uint8_t aTxBuffer10[]="AT+QMTCONN=0,\"100006\",\"100001\",\"123456\"\r\n";				//����Ϊ����ģʽ
uint8_t aTxBuffer12[]="AT+QMTPUBEX=0,0,0,0,\"command/post/JLY/100006\",77\r\n";				//����Ϊ����ģʽ      {devid:100006function:2}
uint8_t aTxBuffer11[]="AT+QMTSUB=0,1,\"command/reply/JLY/100006\",2\r\n";				//����Ϊ����ģʽ
uint8_t aTxBuffer14[]="{\"devId\":100006,\"sessionId\":1542957,\"fucntion\":2,\"statusCode\":200,\"msg\":\"ok\"}\r\n";   //���͵�JSON����
uint8_t aTxBuffer13[]="AT+QMTRECV: 0,0\r\n";				//����Ϊ����ģʽ


void EC200MqttClientTask(void const * argument)	//EC20���ó�ʼ��������MQTT������    void const * argument
{
	char nums=0;
	yd_4gfalg = true;
	char *cjdata;
    char buf_MQ_User_4g[50];
	char bufMqid_4g[50],bufServer_4g[50];
    memset(bufServer_4g,0,sizeof (bufServer_4g));
	memset(buf_MQ_User_4g,0,sizeof (buf_MQ_User_4g));
	memset(bufMqid_4g,0,sizeof (bufMqid_4g));

		HAL_UART_Receive_DMA(&huart6, RecCom6,COM6_REC_MAX);
	__HAL_UART_CLEAR_IDLEFLAG(&huart6);
	__HAL_UART_ENABLE_IT(&huart6, UART_IT_IDLE);

	MQTTMessage messageSend;

	//������Ϣ��ʼ��
	messageSend.dup = 0;
	messageSend.qos = QOS0;
	messageSend.retained = 0;
	messageSend.payload =NULL;//gSendNetData;
	messageSend.payloadlen = 0;//strlen(gSendNetData);
	char buf4g_ack_head[128]={0,};				//����Ϊ����ģʽ
	  while(1)
		{
			
			osDelay (20);
			//˵�������� ��Ȩ�޽�����4Gȥ����
			if(gGlobalData.isConnect==0&&gGlobalData.conFlage==3){
			
				if(gGlobalData.yd4gStatus == false ){
					gGlobalData.yd4gStatus = true;              
					wifi_deinit();                                  //����4gǿ�ƶϿ�wifi
					osDelay(2000);
					sprintf(buf_MQ_User_4g,"AT+QMTCONN=0,\"%d\",\"%s\",\"%s\"\r\n",gDeviceParam.devId,gDeviceParam.mqArg.MQ_Username,gDeviceParam.mqArg.MQ_Password);
					sprintf(bufMqid_4g,"AT+QMTSUB=0,1,\"command/reply/JLY/%d\",0\r\n",gDeviceParam.devId);                    
					sprintf(bufServer_4g,"AT+QMTOPEN=0,\"%d.%d.%d.%d\",%d\r\n",gDeviceParam.ServerIpAddress[0]
																																	 ,gDeviceParam.ServerIpAddress[1]
																																	 ,gDeviceParam.ServerIpAddress[2]
																																	 ,gDeviceParam.ServerIpAddress[3]
																																	 ,gDeviceParam.mqArg.mqttPort); 
		
					send_NetSync(4);    //����� 1601  
					osDelay(100);
					send_duan_wang(1);	//������ʾ 1900 
					
					gGlobalData.netKind = 3;			//4G				
					osDelay(100);
					 
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer0,sizeof(aTxBuffer0));    //AT+QICLOSE=0
					osDelay(200);

					HAL_UART_Transmit_DMA(&huart6,aTxBuffer1,sizeof(aTxBuffer1));   //AT
					osDelay(200);
					
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer2,sizeof(aTxBuffer2));   //AT+CPIN  ���SIM���Ƿ���λ
					osDelay(200);
					
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer3,sizeof(aTxBuffer3));   //AT+CSQ   ��ѯ�ź�����
					osDelay(200);
					
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer4,sizeof(aTxBuffer4));		//AT+CEREG  ��ѯ����״̬
					osDelay(200);

					HAL_UART_Transmit_DMA(&huart6,aTxBuffer5,sizeof(aTxBuffer5));		//AT+CGATT=1  ��������
					osDelay(500);

					HAL_UART_Transmit_DMA(&huart6,aTxBuffer6,sizeof(aTxBuffer6));		//AT+CGATT?   ��ѯ����״̬
					osDelay(200); 

					HAL_UART_Transmit_DMA(&huart6,aTxBuffer7,sizeof(aTxBuffer7));		//AT+CGPADDR   ��ȡPDP��ַ
					osDelay(200);
					
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer8,sizeof(aTxBuffer8));		//AT+QMTCFG=\"recv/mode\",0,0,1  ����Ϊ����ģʽ
					osDelay(200);
					
					HAL_UART_Transmit_DMA(&huart6,(uint8_t *)bufServer_4g,sizeof(bufServer_4g));  //AT+QMTOPEN=0    124.239.10.57,9300
					osDelay(1000);
					
					HAL_UART_Transmit_DMA(&huart6,(uint8_t *)buf_MQ_User_4g,sizeof(buf_MQ_User_4g));				//QMTCONN
					osDelay(1000);
			
					HAL_UART_Transmit_DMA(&huart6,(uint8_t *)bufMqid_4g,sizeof(bufMqid_4g));					//QMTSUB
					osDelay(100);
					HAL_UART_Receive_DMA(&huart6, RecCom6,COM6_REC_MAX);
					HAL_UART_Transmit_DMA(&huart6,aTxBuffer13,sizeof(aTxBuffer13));
					osDelay(100);
					printf("4g-working\r\n");
					cnt_heartbag = 0;															//���������������������
					gGlobalData.heartbag_flage=1; 							  //��һ������
					gGlobalData.isConnect=1;											//����Ϊ��������
				}
			}
			
				//4gģ���Լ�������ά��  ��֤4gģ��һֱ����
			nums++;
			if(nums>200&&gGlobalData.yd4gStatus==true&&cjdata==NULL){      //cjdata==NULL�ڴ��������ʱ��Ͳ��ᷢ��������Ӱ�����
				nums=0;
				HAL_UART_Transmit_DMA(&huart6,aTxBuffer13,sizeof(aTxBuffer13));				//"AT+QMTRECV: 0,0\r\n" ����Ϊ����ģʽ    ������
				osDelay(200);

			}
	
		
			//�������� ��4g���� �ҵ�ǰ����Ϊ4g
			if(gGlobalData.isConnect==1&&gGlobalData.yd4gStatus==true&&gGlobalData.netKind==3){
				
				//4g�������������������
			if(gGlobalData.Send_Heart_Bag_4G==true)
			{
				gGlobalData.Send_Heart_Bag_4G=false;
				messageSend.payloadlen = strlen(gAck);
				messageSend.payload=gAck;
				if(gGlobalData.yd4gStatus==true)																																		//4gӦ��
				{
					sprintf(buf4g_ack_head,"AT+QMTPUBEX=0,0,0,0,\"%s\",%d\r\n",gTopicInfo.cmdPost,messageSend.payloadlen);//4gӦ��ͷ
					osDelay(10);
					HAL_UART_Transmit_DMA(&huart6,(uint8_t*)buf4g_ack_head,strlen(buf4g_ack_head));
					osDelay (50);
					HAL_UART_Transmit_DMA(&huart6,(uint8_t*)gAck,strlen(gAck));
					osDelay(10);	
				}	
			}
			
			if(cjdata = strstr((const char*)RecCom6,"{"))
			{
				osDelay(10);
				if(strstr((const char*)cjdata,"HeartBag")){
					printf("HeartBag_4g\r\n");
					memset(RecCom6,0,sizeof(RecCom6));				
				}
				else if(strstr((const char*)cjdata,"ACK")){
					printf("ACK_4g\r\n");
					memset(RecCom6,0,sizeof(RecCom6));
				}
				else{  //��������������
						cjdata_f_4g=cjdata;
						cjdata_p_4g=cjdata;
						while(1)
						{
							if(cjdata_f_4g=strstr((const char*)(cjdata_f_4g+1),"{"))
								_cjdata_BK_Num_4g++;         																				//��������,����forѭ�������һ��������,��Ϊ�����ж��Ѿ���һ����
							else break;
						}
						for(_cjdata_i_4g=0;_cjdata_i_4g<=_cjdata_BK_Num_4g;_cjdata_i_4g++)
						{
						if(cjdata_p_4g=strstr((const char*)(cjdata_p_4g+1),"}")){
							if(_cjdata_i_4g==_cjdata_BK_Num_4g){
								_cjdata_BK_Num_4g=0;
								printf("�����ɹ�_4g\r\n");
								gGlobalData.heart_count=0;             //������յ���ʱ
								gGlobalData.yd4gStatus=true;
								netData_process(cjdata,sizeof(cjdata));
								memset(RecCom6,0,sizeof(RecCom6));

								if(gGlobalData.ResetStatus==true)
								{
									gGlobalData.ResetStatus=false;
									do_work_ctl(3);
									osDelay(100);//kardos 2023.03.30
									send_QRInfo(gDeviceParam.qrbuf_SWD,strlen((const char*)gDeviceParam.qrbuf_SWD),QR_CODE_ADD);   				//����������ά�뵽��Ļ�����ʾ
									osDelay(100);//kardos 2023.03.30
									send_QRInfo(gDeviceParam.qrbuf_XWTT,strlen((const char*)gDeviceParam.qrbuf_XWTT),QR_CODE_ADD_RIGHT);   //����Ѩλ��ʹ��ά�뵽��Ļ�Ҳ�����ʾ
								}		
								send_NetSync(5);    //����� 1601  
								osDelay (100);	
								send_duan_wang(0);	//������ʾ 1900									
							}
						
						}
						else break;
						}
						_cjdata_BK_Num_4g=0;
						_cjdata_i_4g=0;
						}
				}
				HAL_UART_Receive_DMA(&huart6, RecCom6,COM6_REC_MAX);
				
			}

	 }//while(1)
}

 






