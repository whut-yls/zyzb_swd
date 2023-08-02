#include "wifi.h"
#include "user_tcp_client.h"
#include "ethernetif.h"
#include "parameter.h"
#include "mqttTask.h"
#include "lcdTask.h"

//ATK-ESP8266 WIFI STA����
//���ڲ���TCP/UDP����
//����ֵ:0,����
//    ����,�������


char rec_cmd[255]={0,};     //���շ���ָ���
char *cjdata,*cjdata_f,*cjdata_p,_wifi_Apcode,_wifi_busy_Num=0;
char _cjdata_BK_Num=0,_cjdata_i;
bool _wifi_AtMqtt_conn=false,_wifi_AtMqtt_sub=false;

void WifiMqttClientTask(void const * argument)        //void StartDefaultTask(void const * argument)
{

	gGlobalData.isSendMq=false;
	gGlobalData.heart_sendStatus=true;
//	strcpy((char*)gDeviceParam.wifiArg.WifiName,"ZKM");			//wifi����
//	strcpy((char*)gDeviceParam.wifiArg.WifiPwd,"123456ming");
//	strcpy((char*)gDeviceParam.wifiArg.WifiName,"iphonexr");			//wifi����
//	strcpy((char*)gDeviceParam.wifiArg.WifiPwd,"12345678");
	//bool gGlobal_WifiChange=true;

	uint8_t buf_MQ_User[50],buf_MQ_pw[50],bufsub[50],bufssid[50],bufServer[50],bufport[50],bufway[50],bufMqid[50],bufCon[50],buf_Uncon[50];

	while(1)
	{

		osDelay(10);		
		if(gGlobalData.Wifi_set)																																									//��ʼ������WiFi
		{	
			cnt_heartbag=0;                                                                                         //��ռ�����������������error
			wifi_init();
			sprintf((char*)bufssid,"AT+WJAP=%s,%s",gDeviceParam.wifiArg.WifiName,gDeviceParam.wifiArg.WifiPwd);  		//�������߲���:ssid,����   ��������
			sprintf((char*)bufServer,"AT+MQTT=1,\"%d.%d.%d.%d\"",gDeviceParam.ServerIpAddress[0]
																													 ,gDeviceParam.ServerIpAddress[1]
																													 ,gDeviceParam.ServerIpAddress[2]
																													 ,gDeviceParam.ServerIpAddress[3]);    						  //����MQ������
			sprintf((char*)bufport,"AT+MQTT=2,\"%d\"",gDeviceParam.mqArg.mqttPort);		
			sprintf((char*)bufway,"AT+MQTT=3,\"%d\"",1);    																												//���ӷ�ʽ
			sprintf((char*)bufMqid,"AT+MQTT=4,\"%d\"",gDeviceParam.devId);   																				//����MQid
			sprintf((char*)buf_MQ_User,"AT+MQTT=5,\"%s\"",gDeviceParam.mqArg.MQ_Username);
			sprintf((char*)buf_MQ_pw,"AT+MQTT=6,\"%s\"",gDeviceParam.mqArg.MQ_Password);	
			sprintf((char*)bufCon,"AT+MQTT");			
			sprintf((char*)bufsub,"AT+MQTTSUB=\"%s\",0",gTopicInfo.cmdReply);																				//����MQ������Ϣ
			printf("wifi-go-to-con-debug\r\n");
			osDelay (500);
			atk_8266_send_cmd(bufssid,0xFFFF);	
			osDelay (200);
			HAL_UART_Receive_DMA(&huart3, RecCom3,COM3_REC_MAX);																										//��ȡ��������  
			gGlobalData.Wifi_set = false;
		}
		if(gUartPcTc == false)
			HAL_UART_Receive_DMA(&huart3, RecCom3,COM3_REC_MAX);																										//��ȡ��������
		
		//�����紦�ڶϿ� �ҵ�ǰ��־�ø�wifiʱ  ��ʼ����wifi
		if(gGlobalData.isConnect==0&&gGlobalData.conFlage==2){
			
				//	wifi��ʼ����		�����ǰwifi���ӳɹ��ˣ���ʼ����ִ�� �л�wifi
				if(gGlobalData.wifiStatus ==true && !gGlobalData.isSendMq)
				{
					printf("wifi-set-usermsg\r\n");
					gGlobalData.isSendMq=true;
					atk_8266_send_cmd(bufServer,0xFFFF);
					osDelay (200);
					atk_8266_send_cmd(bufport,0xFFFF);
					osDelay (200);
					atk_8266_send_cmd(bufway,0xFFFF);
					osDelay (200);
					atk_8266_send_cmd(bufMqid,0xFFFF);
					osDelay (200);			
					atk_8266_send_cmd(buf_MQ_User,0xFFFF);
					osDelay (200);					
					atk_8266_send_cmd(buf_MQ_pw,0xFFFF);
					osDelay (200);	
					atk_8266_send_cmd(bufCon,0xFFFF);
					HAL_UART_Receive_DMA(&huart3, RecCom3,COM3_REC_MAX);
				}
				if(strstr((const char*)RecCom3,MQTT_CONNECT)){
					printf("wifi-working\r\n");
					atk_8266_send_cmd(bufsub,0xFFFF);
					osDelay (1000);
					gGlobalData.netKind = 2;

					gGlobalData.isConnect=1;																																						//˵�����ӷ������ɹ�
					send_NetSync(3);
					osDelay(100);
					send_duan_wang(0);
					memset(RecCom3,0,sizeof(RecCom3));
					osDelay(100);
					cnt_heartbag = 0;																																										//���������������������
					gGlobalData.heartbag_flage=1;			     																													    //�����Ϸ��������������������ս���
					}

		}

		
		//����״̬����  ��Ϊwifi���ӵ������ ��ȥ�������������͹���������
		if(gGlobalData.isConnect==1&&gGlobalData.wifiStatus ==true&&gGlobalData.netKind==2){
		/*���ݽ��ܽ�������*/  	
		if((cjdata=strstr((const char*)RecCom3,MQTT_DATA_REC))!=0 ){
			 osDelay(10);
				if(strstr((const char*)cjdata,"HeartBag")){
					printf("HeartBag_wifi\r\n");
					memset(RecCom3,0,sizeof(RecCom3));				
				}
				else if(strstr((const char*)cjdata,"ACK")){
					printf("ACK_wifi\r\n");
					memset(RecCom3,0,sizeof(RecCom3));
				}
				else{   																																													//��������������		
				cjdata_f=cjdata;
				cjdata_p=cjdata;
				for(;;)
					{
						if(cjdata_f=strstr((const char*)(cjdata_f+1),MQTT_DATA_REC))
							_cjdata_BK_Num++;         																																	//��������,����forѭ�������һ��������,��Ϊ�����ж��Ѿ���һ����
						else break;
					}
				for(_cjdata_i=0;_cjdata_i<=_cjdata_BK_Num;_cjdata_i++)
				{
					if(cjdata_p=strstr((const char*)(cjdata_p+1),MQTT_DATA_UNREC)){
						if(_cjdata_i==_cjdata_BK_Num){
							_cjdata_BK_Num=0;
							printf("�����ɹ�_wifi\r\n");
							gGlobalData.heart_count=0;           																											  //������յ���ʱ ����һ�����������һ��
							gGlobalData.wifiStatus=true;
							gGlobalData.heart_sendStatus=false;  																												//wifi����������
							netData_process(cjdata,sizeof(cjdata));
							memset(RecCom3,0,sizeof(RecCom3));

							if(gGlobalData.ResetStatus==true)
							{
								gGlobalData.ResetStatus=false;
								do_work_ctl(3);
								osDelay (100);                        
								osDelay(100);//kardos 2023.03.30
								send_QRInfo(gDeviceParam.qrbuf_SWD,strlen(gDeviceParam.qrbuf_SWD),QR_CODE_ADD);   				//����������ά�뵽��Ļ�����ʾ
								osDelay(100);//kardos 2023.03.30
								send_QRInfo(gDeviceParam.qrbuf_XWTT,strlen(gDeviceParam.qrbuf_XWTT),QR_CODE_ADD_RIGHT);   //����Ѩλ��ʹ��ά�뵽��Ļ�Ҳ�����ʾ
								}
							send_NetSync(3);   																																					 //����� wifi���� 

							
//							atk_8266_send_cmd("AT+OTA=1,\"aithinker111.oss-cn-beijing.aliyuncs.com\",80,\"/2.5.1.xz\"",0xFFFF);
//							osDelay(500);	
//							atk_8266_send_cmd("AT+OTA",0xFFFF);
						}
					}
					else break;
				}
				_cjdata_BK_Num=0;
				_cjdata_i=0;
				}
		}
	}
		
#if 1		
		//���Ӳ���WiFi �ظ�cwjap:_wifi_Apcode      
		if(cjdata=strstr((const char*)RecCom3,WIFI_CON_ERROR)){
			  gGlobalData.wifiStatus = false;

				if((gGlobalData.netKind !=1||gGlobalData.rj45Status!=true) && gGlobalData.yd4gStatus == false){     //2023/7/20 by yls
					gGlobalData.conFlage=3;												//��ת��4G
					gGlobalData.isConnect=0;											//����
					send_NetSync(2);   													 	//����� 1601  
					osDelay(100);
					send_duan_wang(1);														//������ʾ 1900
					printf("wifi_error_goto_4g\r\n");					
				}
				_wifi_Apcode=cjdata[5];
				printf("wifi_connect_error:%c%c\r\n",_wifi_Apcode,cjdata[6]);
				switch(_wifi_Apcode){
					case 0x31:		break;        									//_wifi_Apcode=1:���ӳ�ʱ
					case 0x32:		break;				 									//_wifi_Apcode=2:�������
					case 0x33:		break;													//_wifi_Apcode=3:δ�ҵ�wifi
					case 0x34:		break;				 									//_wifi_Apcode=4:����ʧ�� ����������WiFi����
					default  :		break;
				}
				memset(RecCom3,0,sizeof(RecCom3));		
			}
		
		//wifi��ʹ�ù����жϿ�����
		if(gGlobalData.wifiStatus == true && gGlobalData.netKind == 2 && (cjdata=strstr((const char*)RecCom3,WIFI_DIS_C ))){
			gGlobalData.wifiStatus = false;
			printf("wifi_connect_error\r\n");
      if(gGlobalData.yd4gStatus == false && gGlobalData.rj45Status!=true){
				gGlobalData.conFlage=3;																					//��ת��4G
				gGlobalData.isConnect=0;																				//����
				osDelay(50);
				send_NetSync(2);   																						 	//wifi���ϵ������ 1601  
				osDelay(100);
				send_duan_wang(1);																							//������ʾ 1900 
				printf("wifi_disconnect_goto_4g\r\n");
			}               



		}

		//��ȡWiFi ip��ַ�ɹ�����	
		if(cjdata=strstr((const char*)RecCom3,WIFI_GOT_IP)){
			printf("wifi_connect_success\r\n");
			gGlobalData.wifiStatus = true;																																//˵��wifi������
			send_duan_wang(0);																																						//˵������������ ȥ������
			memset(RecCom3,0,sizeof(RecCom3));
			gGlobalData.isSendMq=false;		 																															  //�������͵ȴ��л�	
			if(gGlobalData.rj45Status != true || gGlobalData.yd4gStatus == true ){    	 								  //һ��ʼ���ڶϿ�����û��������wifi��4g
				gGlobalData.conFlage = 2;	           																											  //�������wifi����    4g���ϻ�������û���ϵ�״̬
				gGlobalData.isConnect=0;																																		//����
				if(gGlobalData.yd4gStatus == true){
					HAL_UART_Transmit_DMA(&huart6,"AT+QMTCLOSE=0\r\n",sizeof("AT+QMTCLOSE=0\r\n"));
					gGlobalData.yd4gStatus=false;
				}				
			}					
		}
		if(strstr((const char*)RecCom3,"+MQTTPUBRAW:") || strstr((const char*)RecCom3,"ERROR")){      //wifiָ��error����������wifi
			gGlobalData.Wifi_set=true;
			cnt_heartbag=0;  																																						//����������0
			gGlobalData.isConnect=0;
			printf("MQ_Pub_error_WIFI\r\n");
		}
		
		

#endif
	}


}




//��esp07s��������
//cmd:���͵������ַ���
//waittime:�ȴ�ʱ��(��λ:10ms)
void atk_8266_send_cmd(uint8_t *cmd,uint16_t waittime)
{
	sprintf(rec_cmd,"%s\r\n",cmd);	//��������
	HAL_UART_Transmit_Wifi(rec_cmd,strlen(rec_cmd),0xffff);

	return;
} 
//wifi��ʼ��
void wifi_init(void)
{
	osDelay(200);	
	atk_8266_send_cmd("AT",0xFFFF);
	osDelay(500);	
	atk_8266_send_cmd("AT+RST",0xFFFF);																																					//����WIFI STAģʽ
	osDelay(3000);
	atk_8266_send_cmd("AT+WMODE=1,1",0xFFFF);
	osDelay(200);
	return;
}

//wifi��ʼ���ҵ�wifi
void wifi_deinit(void)
{
	gGlobalData.wifiStatus = false;
	cnt_heartbag = 0;																																														//��wifiʱ�������������
	osDelay(100);	
	atk_8266_send_cmd("AT",0xFFFF);
	osDelay(200);	
	atk_8266_send_cmd("AT+RST",0xFFFF);																																					//����WIFI STAģʽ
	osDelay(3000);
	atk_8266_send_cmd("AT+WJAP=0,0",0xffff);
//	atk_8266_send_cmd("AT+WMODE=0,0",0xffff);                                                                  //wifi �̼������������
	printf("WIFI-Reset-powerdown");	
	return;
}
