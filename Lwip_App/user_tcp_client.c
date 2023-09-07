#include "lwip/api.h"
#include "string.h"
#include "stdbool.h"
#include "user_tcp_client.h"
#include "cJSON.h"
#include "parameter.h"
#include "common.h"
#include "main.h"
#include "console.h"
#include "mqttTask.h"
#include "pca9554.h"
#include "lcdTask.h"
#include "stdio.h"
#include "Wt2003hx.h"
#include "wifi.h"

/*mqtt def about*/
MQTT_TOPIC_TYPE gTopicInfo;

/*end*/
struct netconn*  gNetSock[NET_SOCK_NUM];
unsigned char gNetSockStatus[NET_SOCK_NUM];
//static char gRecvNetData[RECV_BUF_MAX-1];
//char gSendNetData[RECV_BUF_MAX-1];
static unsigned char gRecvCheckData[100];


int netData_process(char *payload,int payloadLen);

	

/**������ź���������ⷢ���¼�**/
SemaphoreHandle_t					TcpSend_Semaphore; 
SemaphoreHandle_t					COM1_Semaphore; 	//�źŴ����ͨ��uart1
SemaphoreHandle_t	NetSendMutexeSemaphore;


/*mqtt fun about*/
/*�����ʼ��
type:��Ʒ��ʶ id:�豸id
*/
void Topic_Init(MQTT_TOPIC_TYPE* topic,char* type,char*id)
{
	memset(topic,0,sizeof(MQTT_TOPIC_TYPE));
	//ָ���
	strcat(topic->cmdReply,"command/reply/");
	strcat(topic->cmdReply,type);
	strcat(topic->cmdReply,"/");
	strcat(topic->cmdReply,id);    //��֮ǰ�� strcat(topic->cmdReply,id);���Ƕ��ĵ����������#��id��ֵ������client id�ϡ������������ֱ�Ӹ�Ϊ"#"��

	//ָ���
	strcat(topic->cmdPost,"command/post/");
	strcat(topic->cmdPost,type);
	strcat(topic->cmdPost,"/");
	strcat(topic->cmdPost,id);

	//�㲥����
	strcat(topic->boardReply,"broadcast/reply/");
	strcat(topic->boardReply,type);
	//�㲥����
	strcat(topic->boardPost,"broatcast/post/");
	strcat(topic->boardPost,type);
	//�ɼ�����
	strcat(topic->streamPost,"stream/post/");  //stream/post/
	strcat(topic->streamPost,type);
	strcat(topic->streamPost,"/");
	strcat(topic->streamPost,id);
}

void mqtt_topic_sub(MQTT_TOPIC_TYPE*topic,MQTTMessage*messageRecv,MQTTMessage*messageSend)
{

	//��������

	MQTTSubscribe(topic->cmdReply,messageRecv);
	
//	MQTTSubscribe(topic->boardReply,messageRecv);

	//��������
//	MQTTSubscribe(topic->cmdPost,messageSend);
	

//	MQTTSubscribe(topic->boardPost,messageSend);

//	MQTTSubscribe(topic->streamPost,messageSend);


	return ;
}

/*end*/
bool  TcpSend_TaskSemaphore_Init(void)
{
	TcpSend_Semaphore = xSemaphoreCreateBinary();

	if (TcpSend_Semaphore == NULL )
	{
//		printf("TcpSend_Semaphore create fail\r\n");
//		NVIC_SystemReset();
		return false;
	}
	 return true;
}

bool NetSend_MutexeSemaphore_Init(void)
{
	NetSendMutexeSemaphore		= xSemaphoreCreateMutex();
	if (NetSendMutexeSemaphore == NULL )
	{
//		NVIC_SystemReset();
		return false;
	}
	 return true;
}


bool COM1_Semaphore_Init(void)
{
	COM1_Semaphore = xSemaphoreCreateBinary();

	if (COM1_Semaphore == NULL )
	{
//		printf("TcpSend_Semaphore create fail\r\n");
//		NVIC_SystemReset();
		return false;
	}
	 return true;
}

/**
���ܣ���һ��tcp�����׽��� ��������
���أ�1�ɹ����� , ʧ�ܷ���-1(false)
**/
int transport_open(unsigned char netId,unsigned char* MQ_Address,unsigned short remotePort)
{	
	ip_addr_t ipaddr;
	err_t connect_err;
	/**���id�Ϸ���**/
	if(netId>NET_SOCK_NUM-1)
	{
		return -1;
	}
	
	/**����Ѿ������ͷ�**/
	if(NET_LINK_OPEN==gNetSockStatus[netId])
	{
		gNetSockStatus[netId]=NET_LINK_CLOSE;
		netconn_close(gNetSock[netId]);
		netconn_delete(gNetSock[netId]);
	}
	gNetSockStatus[netId]=NET_LINK_CLOSE;
	
	/**���ݳ�ʼ��  **/
	IP4_ADDR(&ipaddr, MQ_Address[0], MQ_Address[1],MQ_Address[2],MQ_Address[3]);
	/* Create a new connection identifier. */
	gNetSock[netId]= netconn_new(NETCONN_TCP);
	if (gNetSock[netId]!=NULL)
	{
		//err = netconn_bind(conn, NULL, 5001);	//����Ҫ��
		connect_err = netconn_connect(gNetSock[netId], &ipaddr, remotePort);
		/* Process the new connection. */
		if (connect_err == ERR_OK)
		{
			gNetSockStatus[netId]=NET_LINK_OPEN;
			return 1;
		}else{
			netconn_delete(gNetSock[netId]);
			return -1;
		}
			
	}else
	{
		return -1;
	}
}

/**
���ܣ���һ���򿪵��׽��־���Ϸ�������NetSendMutexeSemaphore ����Ҫ�ź�������Ϊֻ��һ���߳���ʹ�÷���
���أ��ɹ�����ʵ��д����ֽ��� ʧ�ܷ���-1
**/
int transport_sendPacketBuffer(unsigned char netId,unsigned char* send,short int sendLen)
{
	if(netId==1)	//data port
	{
			unsigned int w_len;

			if(netId>NET_SOCK_NUM-1||NET_LINK_OPEN!=gNetSockStatus[netId])
			{
				return -1;
			}
			/*����һ֡����**/			
			gNetSock[netId]->send_timeout=100;
//			if(gEthStatus==false||gEthSendStatus==false)
//			{
//				return -1;
//			}
			if(gEthStatus==false)
			{
				return -1;
			}
			if(ERR_OK==netconn_write_partly(gNetSock[netId], send, sendLen,NETCONN_COPY, &w_len))
			{
				return w_len;
			}else{
				return -1;
			}

	}else{	//ctl port
			unsigned int w_len;

			if(netId>NET_SOCK_NUM-1||NET_LINK_OPEN!=gNetSockStatus[netId])
			{
				return -1;
			}
			/*����һ֡����**/			
			gNetSock[netId]->send_timeout=100;
//			if(gEthStatus==false||gEthRecvStatus==false)
//			{
//				return -1;
//			}
			if(gEthStatus==false)
			{
				return -1;
			}
			if(ERR_OK==netconn_write_partly(gNetSock[netId], send, sendLen,NETCONN_COPY, &w_len))
			{
				return w_len;
			}else{
				return -1;
			}
		}
}


/**
���ܣ���һ���Ѿ��򿪵������׼����Ͻ�������,�����������ջ������������ݶ���
�ɹ������mq_Send ,�������ճ��ȸ�ֵ��l ,�ɹ�����1 ��ʱ����0 ʶ�𷵻�-1
**/
int transport_recvPacketBuffer(unsigned char netId,unsigned char* recv,unsigned short *recvLen,int timeout)
{
	err_t recv_err;
	void *data;
	unsigned short readLen;
	struct netbuf *buf;
	if(netId>NET_SOCK_NUM-1||NET_LINK_OPEN!=gNetSockStatus[netId])
	{
		return -1;
	}
	*recvLen=0;
	
	gNetSock[netId]->recv_timeout=timeout;	//���Ҫʹ�ܽ��ճ�ʱ ����Ҫ�޸� opt.h  LWIP_SO_RCVTIMEO			1
		/*��ʱ�ȴ�����һ֡��recv_timeout��**/
	recv_err=netconn_recv(gNetSock[netId], &buf);
	if (ERR_OK==recv_err)
	{
		do
		{
			netbuf_data(buf, &data, &readLen);
			/**�������ݵ�recv,����recvLen**/
			memcpy(recv,data,readLen);
			*recvLen+=readLen;
			if(*recvLen>RECV_BUF_MAX)
			{
				break;
			}
			/**��д����*/
			//netconn_write(gNetSock[netId], data,readLen, NETCONN_COPY);
		}while (netbuf_next(buf) >= 0);
		netbuf_delete(buf);	
		return 1;
	}else if(ERR_TIMEOUT==recv_err)	//�����˳�ʱ����
	{
		*recvLen=0;
		return 0;
	}else{		//��Ҫ���³�ʼ��������
		*recvLen=0;
		return -1;
	}
	
}

/**
���ܣ��ͷ�һ���Ѿ��򿪵��׽���
**/
void transport_release(int netId)
{
	if(netId>NET_SOCK_NUM-1)
	{
		return;
	}
	gNetSockStatus[netId]=NET_LINK_CLOSE;
	netconn_close(gNetSock[netId]); //��������������ȴ��źţ�����Է�û�л�Ӧ���ܻ�ȴ��ܳ�ʱ��
	netconn_delete(gNetSock[netId]);
}

#if 0
/**
���ܣ���һ���Ѿ��򿪵�tcp�����׽��� ���ӵ�Զ�˵�ַ ������Դ
���أ��ɹ����� ����״̬NET_LINK_LINKED/NET_LINK_AGAIN, ʧ�ܷ���-1(false)
ע�� ���Է��� ���Ӳ��ɹ�ʱ ϵͳ�Զ��ͷ���������Դ ��Ŀǰ�޷������رպ����� �����ͷ�
**/
int transport_connect(unsigned char netId,unsigned char* MQ_Address,unsigned short remotePort)
{	
	return -1;
#if 0
	ip_addr_t ipaddr;
//	err_t err,recv_err;
	err_t connect_err;
	/**���id�Ϸ���**/
	if(netId>NET_SOCK_NUM-1)
	{
		return -1;
	}

	/**���ݳ�ʼ��  **/
	IP4_ADDR(&ipaddr, MQ_Address[0], MQ_Address[1],MQ_Address[2],MQ_Address[3]);
//	IP4_ADDR(&ipaddr, 192, 168,2,209);
	/* Create a new connection identifier. */
	if (gNetSock[netId]!=NULL)
	{
		//err = netconn_bind(conn, NULL, 5001);	//����Ҫ��
		connect_err = netconn_connect(gNetSock[netId], &ipaddr, remotePort);
//		connect_err = netconn_connect(gNetSock[netId], &ipaddr, 5014);
		/* Process the new connection. */
		if (connect_err == ERR_OK)
		{
			gNetSockStatus[netId]=NET_LINK_LINKED;
			return NET_LINK_LINKED;
		}else if(connect_err == ERR_ABRT||connect_err == ERR_RST){
			return NET_LINK_AGAIN;
		}else{	//������� ���³�ʼ��
			return -1;
		}
			
	}else
	{
		return -1;
	}
	#endif
}


/**
���ܣ��Ͽ�һ���Ѿ��򿪵�����,ֻ�رղ��ͷ�
ע��δ����
**/
void transport_deconnect(int netId)
{
	#if 0	
	if(netId>NET_SOCK_NUM-1)
	{
		return;
	}
	gNetSockStatus[netId]=NET_LINK_CLOSE;
	netconn_close(gNetSock[netId]);
	#endif
}
#endif

unsigned portBASE_TYPE StartMqttClientTask_stack;
/**�̣߳�Tcp ����ָ��ͻ���
ע��������˵����Ӧ��ʹ�÷����������ǿ��ǵ��Ժ��ڹ�����ʹ�� ����������Ϊ�ͻ���**/
void StartMqttClientTask(void const *arg)		//�ǵ�����ջ�ռ�ߴ� ������ܵ������
{

	char rc;
	unsigned short payloadLen=0;

	unsigned char addr[4]={0,};
	int ret;	//char ret �޷���ʾ-1=0xff
	int len;
	unsigned char buf[60]={0,};
	
	MQTTMessage messageSend;
	MQTTMessage messageRecv;
	
	MQTTString	messageTopic;
	//�����ʼ��
	sprintf((char*)buf,"%d",gDeviceParam.devId);
	Topic_Init(&gTopicInfo,(char*)gDeviceParam.productKey,(char*)buf);
	memset(buf,0,sizeof(buf));
	//���Ӳ�����ʼ��
	MQTT_ConnectData_Init(&MQTTPacket_ConnectData_Initializer);
	//
	if(MQTTConnect(&MQTTPacket_ConnectData_Initializer)<0)
	{
		MQTTDisConnect(&MQTTPacket_ConnectData_Initializer);
		gEthRecvStatus=false;	
	}else{
		gEthRecvStatus=true;
	}
	//������Ϣ��ʼ��
	messageSend.dup = 0;//��Ϣ�����ظ�
	messageSend.qos = QOS0;//��Ϣ����QoS
	messageSend.retained = 0;//����
	messageSend.payload =NULL;//gSendNetData;
	messageSend.payloadlen = 0;//strlen(gSendNetData);

	//������Ϣ��ʼ��
	messageRecv.dup = 0;
	messageRecv.qos = QOS2;
	messageRecv.retained = 0;
	messageRecv.payload =NULL;//gRecvNetData;
	
	
	//��Ϣ����
	if(gEthRecvStatus==true)
	{
		mqtt_topic_sub(&gTopicInfo,&messageRecv,&messageSend);

		if(gGlobalData.ResetStatus==true)
		{
			gGlobalData.ResetStatus=false;
			do_work_ctl(3);
			osDelay(100);//kardos 2023.03.30
			send_QRInfo(gDeviceParam.qrbuf_SWD,strlen(gDeviceParam.qrbuf_SWD),QR_CODE_ADD);   				//����������ά�뵽��Ļ�����ʾ
			osDelay(100);//kardos 2023.03.30
			send_QRInfo(gDeviceParam.qrbuf_XWTT,strlen(gDeviceParam.qrbuf_XWTT),QR_CODE_ADD_RIGHT);   //����Ѩλ��ʹ��ά�뵽��Ļ�Ҳ�����ʾ
	}
		cnt_heartbag = 0;												//���������������������		
		gGlobalData.heartbag_flage=1;			      //�����Ϸ��������������������ս���   
  }
		
	while(1)
	{
		osDelay (1);
		if(gEthRecvStatus==false)
			{
				MQTTDisConnect(&MQTTPacket_ConnectData_Initializer);
				if(MQTTConnect(&MQTTPacket_ConnectData_Initializer)<0)
				{
					gEthRecvStatus=false;
					if(gGlobalData.rj45Status ==true&&gGlobalData.conFlage==1){
							osDelay(500);
							send_NetSync(0);			//**//
							osDelay (100);
							send_duan_wang(1);    //**//
					}  
					continue;
				}else{
					gEthRecvStatus=true;											 
					wifi_deinit(); 														//ǿ�������Ͽ�wifi								 	//wifi����

					HAL_UART_Transmit_DMA(&huart6,"AT+QMTCLOSE=0\r\n",sizeof("AT+QMTCLOSE=0\r\n"));				  //4g����
					gGlobalData.yd4gStatus=false;
                  
					gGlobalData.isConnect=1;																																//��������
					gGlobalData.conFlage=1;
					gGlobalData.netKind = 1;																																//����
					mqtt_topic_sub(&gTopicInfo,&messageRecv,&messageSend);
					
																																			//ȥ��������ʾ 1900  //**//
					if(gGlobalData.ResetStatus==true)
					{
						gGlobalData.ResetStatus=false;
						do_work_ctl(3);
						osDelay(100);//kardos 2023.03.30
						send_QRInfo(gDeviceParam.qrbuf_SWD,strlen(gDeviceParam.qrbuf_SWD),QR_CODE_ADD);   				//����������ά�뵽��Ļ�����ʾ
						osDelay(100);//kardos 2023.03.30
						send_QRInfo(gDeviceParam.qrbuf_XWTT,strlen(gDeviceParam.qrbuf_XWTT),QR_CODE_ADD_RIGHT);   //����Ѩλ��ʹ��ά�뵽��Ļ�Ҳ�����ʾ
						osDelay(100);//kardos 2023.03.30
					}
					send_NetSync(1);  																																		  //����� 1601  //**//
					osDelay (100);
					send_duan_wang(0);
					cnt_heartbag = 0;																																			 //���������������������	
					gGlobalData.heartbag_flage=1;			      																						   //�����Ϸ��������������������ս���				
				}
			}
				
			//������Ϣ����		
			rc=MQTTSubscribe_RecvMessage(&messageRecv,&messageTopic);

//			printf("rc=%d\r\n",rc);

			if(rc==MQ_SUCCESS)	//MQ_SUCCSS 
			{
//				printf("Subscribe_RecvMessage Success\n");
				//��Ϣ����
				if(messageRecv.payloadlen>0)
				{
					printf("RJ45_Working\r\n");
//        printf("%s\r\n",messageRecv.payload);
					mqttMessageProcess(messageRecv.payload,messageRecv.payloadlen,messageTopic.lenstring.data,messageTopic.lenstring.len);

                }
			}else if(rc==MQ_PINGRESP){
				gGlobalData.heart_count = 0;
			}else{
	//			gMqttLinkStatus=false;
				gEthRecvStatus=false;
			}
			
			
		}

	
	
}

bool Act_music=false;
/*���ݴ���*/
int netData_process(char *payload,int payloadLen)
{
	
	int ret,i,arryNum,j;
	char strBuf[200]={0,};
	long reet;
	char *pend;

	uint8_t intIp[4]={0,},intMask[4]={0,},intGate[4]={0,},intMac[12]={0,};
	bool ipTrue=false,maskTrue=false,gateTrue=false,macTrue=false;
	int tCollectFreq,tCollectUptime;
	int tDataStartFlag,tDataStartFlag2;
	int  functionFlag,directionFlag,valueFlag,unityFlag,motorFlag,motorMode;
	cJSON *root = NULL, *item = NULL,*itemSub=NULL;
	cJSON *cjsonArr=NULL,*cjsonArr2=NULL,*arryItem=NULL;
	/*���ս��� ���лỰid�͹��ܾͻظ�*/
	memset(strBuf,0,sizeof(strBuf));
	root=cJSON_Parse(payload);
	if(Act_music==false){
			Act_music=true;
			Send_Output_Change(1);
			osDelay(20);
	}
	if (root == NULL || !cJSON_IsObject(root)) {
		Send_Fix_Ack(0,STATUS_FAIL,"json body fail");	//tmp
			cJSON_Delete(root);
			return -1;
  }
	cnt_heartbag = 0;
	/*ȡ�豸id*/
	item=cJSON_GetObjectItem(root, KEY_DEV_ID);
	if (item == NULL || !cJSON_IsNumber(item)) {    //item->type=16
		item=NULL;   //item=604131128
		cJSON_Delete(root);
		return -1;
	}
	valueFlag=item->valueint;
	if(gDeviceParam.devId!=valueFlag)
	{
		return -1;
	}

	/*ȡ�����ֶ�*/
	item=cJSON_GetObjectItem(root, KEY_FUN);
	if (item == NULL || !cJSON_IsNumber(item)) {
		item=NULL;
		cJSON_Delete(root);
		Send_Fix_Ack(0,STATUS_FAIL,"key-fun fail");
		return -1;
	}
	functionFlag=item->valueint;   //debug
	
	
	/*ȡ�Ựid�ֶ�*/
	item=cJSON_GetObjectItem(root, KEY_SESSION);
	if (item == NULL || !cJSON_IsNumber(item)) {
		item=NULL;
		Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-session fail");
		cJSON_Delete(root);
		return -1;
	}

//	strcpy((char*)gGlobalData.sessionId,item->valuestring);
	gGlobalData.sessionId=item->valueint;
	
	
	
		//ȡʱ��
	
	item=cJSON_GetObjectItem(root, KEY_TIME);
	if (item == NULL || !cJSON_IsString(item)) {
	Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-time");
	cJSON_Delete(root);
		return -1;
	}
	strcpy(gGlobalData.ack_time ,item->valuestring);
	GBK_Timeprocess(gGlobalData.ack_time );		
	
	/*���ݹ��ܽ�����������*/
	switch(functionFlag)
	{
		//�ϴ����
		case FUN_COLLECT_ARG:      //debug  1 ���òɼ�����
		#if 1
//		item=cJSON_GetObjectItem(root, KEY_DATA_UPTIME);
//		if (item == NULL || !cJSON_IsNumber(item)) {
//			Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_DATA_UPTIME);
//			cJSON_Delete(root);
//			return -1;
//		}
//		tCollectFreq=item->valueint;
//		gGlobalData.useWorkArg[0].upTime=item->valueint;
//		

		item=cJSON_GetObjectItem(root, KEY_DATA_UPTIME);
		if (item == NULL || !cJSON_IsNumber(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-uptime object format err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		gGlobalData.useWorkArg[0].upTime=item->valueint;
		gGlobalData.useWorkArg[1].upTime=item->valueint;
		
		//ȡ����ģʽ 1���������  2���ֲ�����
		item=cJSON_GetObjectItem(root, KEY_TYPE);
		if (item == NULL || !cJSON_IsNumber(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-type object err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		//gGlobalData.curWorkModeType=item->valueint;
		gGlobalData.curWorkMode=item->valueint;
		
		//ȡ��һ���������
		item=cJSON_GetObjectItem(root,  KEY_WORK);
		if (item == NULL || !cJSON_IsObject(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-work object format err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		
			//ȡwork��������ݶ���  interval
		itemSub=cJSON_GetObjectItem(item, KEY_DATA_UPTIME);
		if (item == NULL || !cJSON_IsNumber(itemSub)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key_interval err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}

		gGlobalData.useWorkArg[1].upTime=itemSub->valueint;

			//level
//		itemSub=cJSON_GetObjectItem(item,KEY_LEVEL);
//		if (item == NULL || !cJSON_IsNumber(itemSub)) {
//					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-level err");
//					cJSON_Delete(root);
//					cjsonArr=NULL;
//					item=NULL;
//					return -1;
//			}
//			gGlobalData.useWorkArg[j].level=itemSub->valueint;
			
			//waitTime
//			itemSub=cJSON_GetObjectItem(item,KEY_TIME_WAIT);
//			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
//					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-waitTime err");
//					cJSON_Delete(root);
//					cjsonArr=NULL;
//					itemSub=NULL;
//					item=NULL;
//					return -1;
//			}
//			gGlobalData.useWorkArg[j].waitTime=itemSub->valueint;
//			gGlobalData.Alltime+=gGlobalData.useWorkArg[j].waitTime;
				
			//rate
			itemSub=cJSON_GetObjectItem(item,KEY_DATA_RATE);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-rate err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
			 //gGlobalData.freqUnitTime=itemSub->valueint;  //��������֮��Ĳ������ʱ�� ms
			gGlobalData.useWorkArg[0].rateN=itemSub->valueint;	//�������Ƶ��   jly�ɼ����õ�
			
			/**��Ա�ڵ�inputs����**/
			//inputs
			cjsonArr2=cJSON_GetObjectItem(item,KEY_INPUTS);
			if (cjsonArr2 == NULL || !cJSON_IsArray(cjsonArr2)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-inputs err");
				cJSON_Delete(root);
				cjsonArr=NULL;
				cjsonArr2=NULL;
				item=NULL;
				return -1;
			}
			
			//inputs����ߴ�
			arryNum=cJSON_GetArraySize(cjsonArr2);
			if(arryNum<1||arryNum>32)
			{
				Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-inputs num err");
				cJSON_Delete(root);
				cjsonArr=NULL;
				cjsonArr2=NULL;
				item=NULL;
				return -1;
			}
			//inputs��Ա
			gGlobalData.useWorkArg[0].chanelNum=arryNum;
			for(i=0;i<arryNum;i++)
			{
				arryItem=cJSON_GetArrayItem(cjsonArr2,i);
				if (arryItem == NULL || !cJSON_IsNumber(arryItem)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-inputs obj err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					cjsonArr2=NULL;
					return -1;
				}
				ret=arryItem->valueint;
				if(ret>33||ret<1)
				{
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-inputs val err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					cjsonArr2=NULL;
					return -1;
				}
				gGlobalData.useWorkArg[0].inputs[i]=ret;
			}
			
			/**��Ա�ڵ�outs�����Ա**/
				//outs
			cjsonArr2=cJSON_GetObjectItem(item,KEY_OUTS);
			if (cjsonArr2 == NULL || !cJSON_IsArray(cjsonArr2)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-outs err");
				cJSON_Delete(root);
				cjsonArr=NULL;
				cjsonArr2=NULL;
				item=NULL;
				return -1;
			}
			
			//outs����ߴ�
			arryNum=cJSON_GetArraySize(cjsonArr2);
			if(arryNum<1||arryNum>32)
			{
				Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-outs num err");
				cJSON_Delete(root);
				cjsonArr=NULL;
				cjsonArr2=NULL;
				item=NULL;
				return -1;
			}
			//outs��Ա
			gGlobalData.useWorkArg[j].chanelNum=arryNum;
			for(i=0;i<arryNum;i++)
			{
				arryItem=cJSON_GetArrayItem(cjsonArr2,i);
				if (arryItem == NULL || !cJSON_IsNumber(arryItem)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-inputs obj err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					cjsonArr2=NULL;
					return -1;
				}
				ret=arryItem->valueint;
				if(ret>33||ret<1)
				{
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-inputs val err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					cjsonArr2=NULL;
					return -1;
				}
				gGlobalData.useWorkArg[0].outs[i]=ret;
			}	

			//wave
			itemSub=cJSON_GetObjectItem(item,KEY_WAVE_TREAT);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wave err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
//			gGlobalData.useWorkArg[j].waveTreat=itemSub->valueint;
			//freqTreat
			itemSub=cJSON_GetObjectItem(item,KEY_FREQ_TREAT);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-freq err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
//			gGlobalData.useWorkArg[j].freqTreat=itemSub->valueint;
			//time
			itemSub=cJSON_GetObjectItem(item,KEY_TIME_ZL);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
			  Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-time err");
			  cJSON_Delete(root);
			  cjsonArr=NULL;
			  itemSub=NULL;
			  item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[0].timeCheck=itemSub->valueint;
//			gGlobalData.useWorkArg[j].timeTreat=itemSub->valueint;	
//			gGlobalData.Alltime+=gGlobalData.useWorkArg[j].timeTreat;	
		//waveCheck
//			itemSub=cJSON_GetObjectItem(item,KEY_WAVE_CHECK);
//			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
//					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-waveCheck err");
//					cJSON_Delete(root);
//					cjsonArr=NULL;
//					itemSub=NULL;
//					item=NULL;
//					return -1;
//			}
//			gGlobalData.useWorkArg[j].waveCheck=itemSub->valueint;
			//freqCheck
//			itemSub=cJSON_GetObjectItem(item,KEY_FREQ_CHECK);
//			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
//					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-freqCheck err");
//					cJSON_Delete(root);
//					cjsonArr=NULL;
//					itemSub=NULL;
//					item=NULL;
//					return -1;
//			}
//			gGlobalData.useWorkArg[j].freqCheck=itemSub->valueint;
			//timeCheck
//			itemSub=cJSON_GetObjectItem(item,KEY_TIME_CHECK);
//			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
//					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-timeCheck err");
//					cJSON_Delete(root);
//					cjsonArr=NULL;
//					itemSub=NULL;
//					item=NULL;
//					return -1;
//			}
//			gGlobalData.useWorkArg[j].timeCheck=itemSub->valueint;
	
		
		
		Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
//		gGlobalData.Send_Client_Over= true;	  //debug  Ӧ��
	send_LcdWorkStatus(2);//kardos 2023.02.03 �޸��豸����״̬Ϊ���ȴ�Ѩλ����
		break;
		
	//�·��������Ʒ���
		case FUN_COLLECT_TREAT:
			gGlobalData.cur_heart_state=WAITING;    
			gGlobalData.Alltime=0;   //��ֹ�ظ�ɨ�뵼�µ���ʱ�ۼ�
			item=cJSON_GetObjectItem(root, KEY_DATA_UPTIME);
			if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_DATA_UPTIME);
				cJSON_Delete(root);
				cjsonArr=NULL;
				item=NULL;
				return -1;
			}
			tCollectFreq=item->valueint;
			gGlobalData.useWorkArg[0].upTime=item->valueint;
			gGlobalData.useWorkArg[1].upTime=item->valueint;
		
		
		//��ά�빦�ܱ�ʶ
		item=cJSON_GetObjectItem(root, KEY_Flage);
		if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key_flage err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
	  

		
		
		//ȡ��һ���������
		cjsonArr=cJSON_GetObjectItem(root,  KEY_WORK);
		if (cjsonArr == NULL || !cJSON_IsArray(cjsonArr)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL, "key-work array format err");
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
			
		gGlobalData.useWorkArg[0].chanelNum=cJSON_GetArraySize(cjsonArr);  //��ȡwork����ߴ�
		
		
		if(strcmp(strBuf,"XWTT")==0){
		gGlobalData.curWorkMode = 2;
		send_lcdPage(3);
		for(j=0;j<gGlobalData.useWorkArg[0].chanelNum;j++)
		{
			//��ȡwork��һ�������Ա
			item=cJSON_GetArrayItem(cjsonArr,j);
			if (item == NULL || !cJSON_IsObject(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-work obj err");
				cJSON_Delete(root);
				cjsonArr=NULL;
				itemSub=NULL;
				item=NULL;
				return -1;
			}
			/**��Ա�ڵ���ͨ����**/

			//wave
			itemSub=cJSON_GetObjectItem(item,KEY_WAVE_TREAT);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wave err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[j].waveTreat=itemSub->valueint;
			
			//aPower
			itemSub=cJSON_GetObjectItem(item,KEY_APOWER);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-aPower err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[j].aPower=itemSub->valueint;
			// 
			itemSub=cJSON_GetObjectItem(item,KEY_BPOWER);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-bPower err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[j].bPower=itemSub->valueint;
			//time
			itemSub=cJSON_GetObjectItem(item,KEY_TIME_ZL);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
			  Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-time err");
			  cJSON_Delete(root);
			  cjsonArr=NULL;
			  itemSub=NULL;
			  item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[j].timeTreat=itemSub->valueint;	
			gGlobalData.Alltime+=gGlobalData.useWorkArg[j].timeTreat;	
			
			//waitTime
			itemSub=cJSON_GetObjectItem(item,KEY_TIME_WAIT);
			if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-waitTime err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
			}
			gGlobalData.useWorkArg[j].waitTime=itemSub->valueint;
			gGlobalData.Alltime+=gGlobalData.useWorkArg[j].waitTime;
		}

			send_lcdPage(3);
			osDelay(50);
			send_treatSel_Xwtt(gGlobalData.useWorkArg[0].waveTreat,
								gGlobalData.useWorkArg[0].aPower,
							(gGlobalData.useWorkArg[0].timeTreat)/60,
								gGlobalData.useWorkArg[0].bPower);						//�޸���Ļ��ʾ�����Ʒ���		
			osDelay(50);
			send_LcdWorkStatus(2);//yls 2023.04.07 �޸��豸����״̬Ϊ���ȴ�Ѩλ����
			osDelay(50);	
			Countdown_Treat(gGlobalData.Alltime);
			osDelay(50);
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		}
		else if(strcmp(strBuf,"SWD")==0){
			gGlobalData.curWorkMode = 1;
			for(j=0;j<gGlobalData.useWorkArg[0].chanelNum;j++)
			{
				//��ȡwork��һ�������Ա
				item=cJSON_GetArrayItem(cjsonArr,j);
				if (item == NULL || !cJSON_IsObject(item)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-work obj err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
				}
				/**��Ա�ڵ���ͨ����**/

				
				//level
				itemSub=cJSON_GetObjectItem(item,KEY_LEVEL);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-level err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}
				gGlobalData.useWorkArg[j].level=itemSub->valueint;
				
				//inputs    debug
				itemSub=cJSON_GetObjectItem(item,KEY_INPUTS);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-inputs err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}
				
				gGlobalData.useWorkArg[0].inputs[j]=itemSub->valueint;

				//outs    debug
				itemSub=cJSON_GetObjectItem(item,KEY_OUTS);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-outs err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}			
				
				gGlobalData.useWorkArg[0].outs[j]=itemSub->valueint;
	 
				//wave
				itemSub=cJSON_GetObjectItem(item,KEY_WAVE_TREAT);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wave err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}
				gGlobalData.useWorkArg[j].waveTreat=itemSub->valueint;
				//freqTreat
				itemSub=cJSON_GetObjectItem(item,KEY_FREQ_TREAT);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-freq err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}
				if(itemSub->valueint<=10000) gGlobalData.useWorkArg[j].freqTreat=itemSub->valueint;   //by yls 2023/7/2 ĿǰƵ������2000hz          7/22�������ڴﵽ10khzƵ��
				//time
				itemSub=cJSON_GetObjectItem(item,KEY_TIME_ZL);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-time err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
						return -1;
				}
				gGlobalData.useWorkArg[j].timeTreat=itemSub->valueint;	
				gGlobalData.Alltime+=gGlobalData.useWorkArg[j].timeTreat;	
				
				//waitTime
				itemSub=cJSON_GetObjectItem(item,KEY_TIME_WAIT);
				if (itemSub == NULL || !cJSON_IsNumber(itemSub)) {
						Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-waitTime err");
						cJSON_Delete(root);
						cjsonArr=NULL;
						itemSub=NULL;
						item=NULL;
						return -1;
				}
				gGlobalData.useWorkArg[j].waitTime=itemSub->valueint;
				gGlobalData.Alltime+=gGlobalData.useWorkArg[j].waitTime;
			}
			level= gGlobalData.useWorkArg[0].level;//��ʼĬ�ϵ�λ��ֵ 
			send_lcdPage(1);
			osDelay(50);
			//���������͸���Ļ��ʾ  2023.02.05
			send_treatSel(gGlobalData.useWorkArg[0].freqTreat,
										gGlobalData.useWorkArg[0].level,
									 (gGlobalData.useWorkArg[0].timeTreat)/60);	
			osDelay(50);
			send_LcdWorkStatus(2);//kardos 2023.02.03 �޸��豸����״̬Ϊ���ȴ�Ѩλ����
			osDelay(50);
			Countdown_Treat(gGlobalData.Alltime);
			osDelay(50);					
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		}
		else {
					Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-flage swd and xwtt err");
					cJSON_Delete(root);
					cjsonArr=NULL;
					itemSub=NULL;
					item=NULL;
					return -1;
		
		
		}
		break;
		
		//�ɼ�����
		case FUN_COLLECT_CTL:	
		//workmode
//		item=cJSON_GetObjectItem(root, KEY_WORK_MODE);
//		if (item == NULL || !cJSON_IsNumber(item)) {
//			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-workmode fail");
//			cJSON_Delete(root);
//			return -1;
//		}
//		tDataStartFlag=item->valueint;
//		if(tDataStartFlag!=WORK_MODE_ZT&&tDataStartFlag!=WORK_MODE_JB&&tDataStartFlag!=WORK_MODE_ZL)
//		{
//			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-workmode value err");
//			break;
//		}
//		gGlobalData.curWorkMode=tDataStartFlag;
		
		item=cJSON_GetObjectItem(root, KEY_VALUE);
		if (item == NULL || !cJSON_IsNumber(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-value err");
			cJSON_Delete(root);
			return -1;
		}
		tDataStartFlag2=item->valueint;
//		gGlobalData.curWorkState=tDataStartFlag2;
		
		if(tDataStartFlag2!=WORK_START&&tDataStartFlag2!=WORK_PAUSE&&tDataStartFlag2!=WORK_STOP)
		{	
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-value err");
		}else if(tDataStartFlag2 == WORK_STOP){              //��λ�Լ���ack
			
			do_work_ctl(tDataStartFlag2);
		}
		else {
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
			do_work_ctl(tDataStartFlag2);
		}
		break;
//		case FUN_DEV_OFF:	//���ػ�����
		case FUN_DEV_RESET:	//�豸����
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
			set_sampleMode(MODE_CLOSE);
			HAL_PCA9554_outputAll(0);
			osDelay(100);
			NVIC_SystemReset();
		break;
		case  FUN_DEV_LOCK:	//�豸��
		item=cJSON_GetObjectItem(root, KEY_VALUE);
		if (item == NULL || !cJSON_IsNumber(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-value fail");
			cJSON_Delete(root);
			return -1;
		}
		tDataStartFlag=item->valueint;
		
		if(tDataStartFlag==LOCK_YES)
		{
			gDeviceParam.devLock=LOCK_YES;
		}else if(tDataStartFlag==LOCK_NO){
			gDeviceParam.devLock=LOCK_NO;
		}else{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-value err");
			break;
		}
		Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
			
		case  FUN_SYS_CHECK://ϵͳ�豸���
			Send_SysCheck_Ack(functionFlag,STATUS_OK,"ok");
		break;
		case FUN_JIANCHE_DATA:	//�������
//				Send_JianChe_Ack(functionFlag,STATUS_OK,"ok");

			gGlobalData.isConnect=1;//����״̬ok
			gGlobalData.heart_count = 0;            //������������
			gGlobalData.heart_sendStatus=true;		  //wifi���Է�������
		break;
		case FUN_RJ45_SET:
		//ip
				item=cJSON_GetObjectItem(root, KEY_IP);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-ip");
			cJSON_Delete(root);
			return -1;
		}
		strcpy(strBuf,item->valuestring);
		ipTrue=isVaildIp(strBuf,intIp);
		//mask
		item=cJSON_GetObjectItem(root, KEY_MASK);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-mask");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		maskTrue=isVaildIp(strBuf,intMask);
		//gate
		item=cJSON_GetObjectItem(root, KEY_GATE);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-gate");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		gateTrue=isVaildIp(strBuf,intGate);
		//mac
		item=cJSON_GetObjectItem(root, KEY_MAC);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-mac");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		macTrue=isVaildMac(strBuf,intMac);

		if(ipTrue==true&&gateTrue==true&&maskTrue==true&&macTrue==true)
		{
			for(i=0;i<4;i++)
			{
				gDeviceParam.rj45Arg.IpAddress[i]=intIp[i];
				gDeviceParam.rj45Arg.MaskAddress[i]=intMask[i];
				gDeviceParam.rj45Arg.GateAddress[i]=intGate[i];
			}
			for(i=0;i<6;i++)
			{
				gDeviceParam.rj45Arg.MacAddress[i]=intMac[i];
			}
			
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
			Save_Parameter();
		}else{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"errno ip");
		}
		break;
		
		case FUN_RJ45_GET:
			Send_Rj45_Ack(functionFlag,STATUS_OK,"ok");
			break;
		case FUN_WIFI_SET:
			//ip
				item=cJSON_GetObjectItem(root, KEY_IP);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-ip");
			cJSON_Delete(root);
			return -1;
		}
		strcpy(strBuf,item->valuestring);
		ipTrue=isVaildIp(strBuf,intIp);
		//mask
		item=cJSON_GetObjectItem(root, KEY_MASK);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-mask");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		maskTrue=isVaildIp(strBuf,intMask);
		//gate
		item=cJSON_GetObjectItem(root, KEY_GATE);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-gate");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		gateTrue=isVaildIp(strBuf,intGate);
		//mac
		item=cJSON_GetObjectItem(root, KEY_MAC);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-mac");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		macTrue=isVaildMac(strBuf,intMac);
		
		//wifi name
			item=cJSON_GetObjectItem(root, KEY_WIFI_NAME);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wifiName");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		if(strlen(strBuf)>32)
		{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wifiPwd len>32");
			break;
		}
		strcpy((char*)gDeviceParam.wifiArg.WifiName,strBuf);
		//wifi pwd
		item=cJSON_GetObjectItem(root, KEY_WIFI_PWD);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wifiPwd");
			cJSON_Delete(root);
			return -1;
		}
		memset(strBuf,0,sizeof(strBuf));
		strcpy(strBuf,item->valuestring);
		if(strlen(strBuf)>32)
		{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wifiPwd len>32");
			break;
		}
		strcpy((char*)gDeviceParam.wifiArg.WifiPwd,strBuf);
		
		if(ipTrue==true&&gateTrue==true&&maskTrue==true&&macTrue==true)
		{
			for(i=0;i<4;i++)
			{
				gDeviceParam.wifiArg.IpAddress[i]=intIp[i];
				gDeviceParam.wifiArg.MaskAddress[i]=intMask[i];
				gDeviceParam.wifiArg.GateAddress[i]=intGate[i];
			}
			for(i=0;i<6;i++)
			{
				gDeviceParam.wifiArg.MacAddress[i]=intMac[i];
			}
			
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
			Save_Parameter();
		}else{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-wifi arg err");
		}
			break;
		case FUN_WIFI_GET:
			Send_Wifi_Ack(functionFlag,STATUS_OK,"ok");
			break;
		case FUN_USER_INFO:     //������ҲҪ��
		//number
		item=cJSON_GetObjectItem(root,KEY_NUMBER);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_NUMBER);
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		if(strlen(strBuf)>32)
		{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-number len>32");
			break;
		}
		strcpy(gUserInfo.number,item->valuestring);
	
		//name
			item=cJSON_GetObjectItem(root,KEY_NAME);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_NAME);
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		if(strlen(strBuf)>32)
		{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-name len>32");
			break;
		}
		strcpy(gUserInfo.name,item->valuestring);
	
		//sex
		item=cJSON_GetObjectItem(root,KEY_SEX);
		if (item == NULL || !cJSON_IsString(item)) {
		Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_SEX);
		cJSON_Delete(root);
		cjsonArr=NULL;
		item=NULL;
		return -1;
		}
		if(strlen(strBuf)>32)
		{
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-sex len>32");
			break;
		}
		strcpy(gUserInfo.sex,item->valuestring);
		reet=strtol(gUserInfo.sex,&pend,16);

		
		//age
			item=cJSON_GetObjectItem(root,KEY_AGE);
			if (item == NULL || !cJSON_IsNumber(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,KEY_AGE);
			cJSON_Delete(root);
			cjsonArr=NULL;
			item=NULL;
			return -1;
		}
		gUserInfo.age=item->valueint;
		
		gUserInfo.infoEn=1;
		gUserInfo.infoUpdateEn=true;
//		gGlobalData.Send_Client_Over= true;	   //debug Ӧ��

		osDelay(50);
		GBK_Msgprocess(gUserInfo.name);    //����ָ������Ļ��ʾ�·����ݵ�����
		osDelay(50);
		send_visitNumber(gUserInfo.number);          //����ָ������Ļ��ʾ���
		osDelay(50);
		send_visitSex(reet);       //����ָ������Ļ��ʾ�·����ݵ��Ա�
		osDelay(50);
		send_visitAge(gUserInfo.age);   //debug  �������䵽��Ļ��ʾ
		osDelay(50);
		Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
		case FUN_DEV_SET:
			//serverIp
			item=cJSON_GetObjectItem(root, KEY_SERVER_IP);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-serverIp");
			cJSON_Delete(root);
			return -1;
			}
			memset(strBuf,0,sizeof(strBuf));
			strcpy(strBuf,item->valuestring);
			
			//server port
			item=cJSON_GetObjectItem(root,KEY_SERVER_PORT);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-port");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
	
			if(true==isVaildIp(strBuf,intIp)&&valueFlag>0&&valueFlag<65535)
			{
				for(i=0;i<4;i++)
				{
					gDeviceParam.ServerIpAddress[i]=intIp[i];
				}
				gDeviceParam.mqArg.mqttPort=valueFlag;
			}else{
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"arg-mac fail");
			}
			//heart
			item=cJSON_GetObjectItem(root, KEY_HEAR_RATE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-port");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			gDeviceParam.heartRate=valueFlag;
			//product key
				item=cJSON_GetObjectItem(root,KEY_PRODUCT);
				if (item == NULL || !cJSON_IsString(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-productKey err");
				cJSON_Delete(root);
				return -1;
			}
			memset(strBuf,0,sizeof(strBuf));
			strcpy(strBuf,item->valuestring);
			if(strlen(strBuf)>32)
			{
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"arg-productKey err");
				break;
			}
			strcpy(gDeviceParam.productKey,strBuf);
		
			//newDevid
			item=cJSON_GetObjectItem(root, KEY_NEW_ID);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-newDevid");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
//			if(valueFlag>=100000&&valueFlag<=99999999)
//			{
//				
//				Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
//			}else{
//				Send_Fix_Ack(functionFlag,STATUS_FAIL,"range:100000000~99999999");
//				break;
//			}
			if(valueFlag<100000||valueFlag>999999999)
			{
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"range:10000~999999999");
				break;
			}
				gDeviceParam.devId=valueFlag;
		
			//����ά��swd

	    item=cJSON_GetObjectItem(root, KEY_LEFT_Code);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-leftCode err");
			cJSON_Delete(root);
			return -1;
			}
			if(strBuf!=0)
			memset(gDeviceParam.qrbuf_SWD,0,sizeof(gDeviceParam.qrbuf_SWD)); //���swd��ά��
			memset(strBuf,0,sizeof(strBuf));
		  strcpy(strBuf,item->valuestring);
			SendleftQR(strBuf,0);
			//�Ҳ��ά��xwtt
	    item=cJSON_GetObjectItem(root, KEY_RIGHT_Code);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-rightCode err");
			cJSON_Delete(root);
			return -1;
			}
			if(strBuf!=0)
			memset(gDeviceParam.qrbuf_XWTT,0,sizeof(gDeviceParam.qrbuf_XWTT));	//���xwtt��ά��
			memset(strBuf,0,sizeof(strBuf));
		  strcpy(strBuf,item->valuestring);
			SendleftQR(strBuf,1);
	
			//MQ_Username
			item=cJSON_GetObjectItem(root, KEY_Mquser);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-MQ_Username err");
			cJSON_Delete(root);
			return -1;
			}
			strcpy(gDeviceParam.mqArg.MQ_Username,item->valuestring);
			
			//MQ_Password
			item=cJSON_GetObjectItem(root, KEY_Mqpassword);
			if (item == NULL || !cJSON_IsString(item)) {
			Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-MQ_Password err");
			cJSON_Delete(root);
			return -1;
			}
			strcpy(gDeviceParam.mqArg.MQ_Password,item->valuestring);
			
			Save_Parameter();
			Send_Fix_Ack(functionFlag,STATUS_OK,"OK");
		break;
			
		case FUN_DEV_GET:
				Send_BaseArg_Ack(functionFlag,STATUS_OK,"OK");
		break;
		
		#endif
		case FUN_SAVE_ARG://���浱ǰ��������ΪĬ�Ϲ�������
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
			memcpy(gDeviceParam.workArg,gGlobalData.useWorkArg,sizeof(Parameter_Work));
			Save_Parameter();
		break;
		
		case FUN_MESSAGE_BOX:            //�·���ʾ��Ϣ��ʾ��
			item=cJSON_GetObjectItem(root, KEY_MSG);
				if (item == NULL || !cJSON_IsString(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-message err");
				cJSON_Delete(root);
				return -1;
			}
			memset(strBuf,0,sizeof(strBuf));
			strcpy(strBuf,item->valuestring);
			SendMessageDialog(strBuf,strlen(strBuf));                    //������ʾ��
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break; 
			
 		case FUN_PLAY_CONTENT:         //�·����������ļ�
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsString(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-voice-content err");
				cJSON_Delete(root);
				return -1;
			}
			memset(strBuf,0,sizeof(strBuf));
			strcpy(strBuf,item->valuestring);
			Send_PlayMusic(strBuf);
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;  
			
		case FUN_PLAY_CTL:            //�·����ſ���
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-voice-ctl err");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			Send_ComMusic(valueFlag);     
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
			
		case FUN_VOLUME_CTL:          //�·���������
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-volume-ctl err");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			Send_ComVolume(valueFlag);
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
			
		case FUN_LIGHT_CTL:              //�ƹ����
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-voice-content err");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			if(valueFlag){
					WriteOUT13(0);
			}
			else{
					WriteOUT13(1);    // ����
			}  
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
			
		case FUN_AUDIO_CTL:              //��Ƶ����
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-audio-ctrl err");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			if(valueFlag==1){   //��������Ҳ���Ƕ���
				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_SET);
			}
			else if(valueFlag==2){    //��������Ҳ��������
				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET);	
			}  
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break; 

		case FUN_LEVEL_CTL:              //�Ӽ�������
			item=cJSON_GetObjectItem(root, KEY_CHANNEL);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-channel-ctrl err");
				cJSON_Delete(root);
				return -1;
			}		
			gGlobalData.channel=item->valueint;	
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-level-ctrl err");
				cJSON_Delete(root);
				return -1;
			}
			valueFlag=item->valueint;
			if(valueFlag==1){     //�ӵ�
				if(gGlobalData.channel==1){
					if(gGlobalData.curWorkMode==1){
						do_work_ctl(4);        //swd�ӵ�
					}
					else if(gGlobalData.curWorkMode==2){
						do_work_ctl(6);        //xwtt aͨ���ӵ�
					}
				}
				else if(gGlobalData.channel==2 && gGlobalData.curWorkMode==2){
					do_work_ctl(8);        //xwtt bͨ���ӵ�
				}
				else if(gGlobalData.channel==3 && gGlobalData.curWorkMode==2){
					do_work_ctl(6);        //xwtt aͨ���ӵ�
					osDelay(10);
					do_work_ctl(8);        //xwtt bͨ���ӵ�
				}
			}
			else if(valueFlag==2){    //����
				if(gGlobalData.channel==1){
					if(gGlobalData.curWorkMode==1){
						do_work_ctl(5);        //swd����
					}
					else if(gGlobalData.curWorkMode==2){
						do_work_ctl(7);        //xwtt aͨ������
					}
				}
				else if(gGlobalData.channel==2 && gGlobalData.curWorkMode==2){
					do_work_ctl(9);        //xwtt bͨ������
				}
				else if(gGlobalData.channel==3 && gGlobalData.curWorkMode==2){
					do_work_ctl(7);        //xwtt aͨ���ӵ�
					osDelay(10);
					do_work_ctl(9);        //xwtt bͨ���ӵ�
				}				
			}
			osDelay(10);
			cnt_heartbag = 0;												//���������������������			
			gGlobalData.heartbag_flage = 1;	         //����������ϴ�һ������
		break; 			
		
		case FUN_AUTO_LEVEL_CTL:                   //�Զ��Ӽ�������
			item=cJSON_GetObjectItem(root, KEY_CHANNEL);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-channel-ctrl err");
				cJSON_Delete(root);
				return -1;
			}
			if(gGlobalData.Auto_Level_Ctl == 0){	       //Ҫ�Ӽ�����������½���			
				gGlobalData.channel=item->valueint;
			}
			else{
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-control-repeatedly err");
				break;
			}
			item=cJSON_GetObjectItem(root, KEY_VALUE);
				if (item == NULL || !cJSON_IsNumber(item)) {
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-level-ctrl err");
				cJSON_Delete(root);
				return -1;
			}
			if(gGlobalData.curWorkMode == 1 && item->valueint >= 5 && item->valueint <= 60)
				gGlobalData.Auto_Level_Ctl = item->valueint;  
			else if(gGlobalData.curWorkMode == 2 && item->valueint >= 6 && item->valueint <= 250)
				gGlobalData.Auto_Level_Ctl = item->valueint/2*2;
			else{
				Send_Fix_Ack(functionFlag,STATUS_FAIL,"key-control-value err");
				break;			
			}
			Send_Fix_Ack(functionFlag,STATUS_OK,"ok");
		break;
			
			
		default:
		break;
	}

	
	
	 cJSON_Delete(root);
	 root=NULL;
	/*������*/
	//transport_sendPacketBuffer(NET_LINK_ID0,payload,payloadLen);

	return 0;
}

unsigned portBASE_TYPE StartMqttSendTask_stack;
/*�̣߳�tcp�������˿ڼ��*/
void StartMqttSendTask(void const *arg)
{
	int len;
	int rb=0;
	unsigned char buf[20]={0,};
	char buf4g_ack_head[128]={0,},bufwifi_ack_head[128]={0,};			//wifi&4gӦ��ͷ
	uint16_t mqCount=0;//mq����������
	MQTTMessage messageSend;

	//������Ϣ��ʼ��
	messageSend.dup = 0;
	messageSend.qos = QOS0;
	messageSend.retained = 0;
	messageSend.payload =NULL;//gSendNetData;
	messageSend.payloadlen = 0;//strlen(gSendNetData);
	
	while(1)
	{
//		StartMqttSendTask_stack=uxTaskGetStackHighWaterMark(mqttSendTaskHandle); 
		//hyadd
		osDelay(10);
		if(gGlobalData.heartbag_flage == 1)
		{
			gGlobalData.heartbag_flage =0;
			Send_heartBag(FUN_JIANCHE_DATA,STATUS_OK,gGlobalData.netKind,gGlobalData.cur_heart_state,gGlobalData.Alltime);
			
		}	
					//hyadd ����������
		if(gGlobalData.Send_Heart_Bag==true)
		{
			gGlobalData.Send_Heart_Bag=false;
			messageSend.payloadlen = strlen(gAck);
			messageSend.payload=gAck;
			if(gGlobalData.rj45Status==true)
			{
				MQTTPublish(gTopicInfo.cmdPost, &messageSend);
			}
				
		}
		//wifi���������������
		else if(gGlobalData.Send_Heart_Bag_wifi){
			gGlobalData.Send_Heart_Bag_wifi = false;
			taskENTER_CRITICAL();
			messageSend.payloadlen = strlen(gAck);
			messageSend.payload=gAck;
			taskEXIT_CRITICAL();
			if(gGlobalData.wifiStatus)        													//wifiӦ��
			{
				
				sprintf(bufwifi_ack_head,"AT+MQTTPUBRAW=\"%s\",1,0,%d",gTopicInfo.cmdPost,messageSend.payloadlen);  //wifiӦ��ͷ
				osDelay(100);
				atk_8266_send_cmd((char*)bufwifi_ack_head,strlen(bufwifi_ack_head));
				osDelay(50);
				taskENTER_CRITICAL();				
				HAL_UART_Transmit_DMA(&huart3,(char*)messageSend.payload,messageSend.payloadlen);
				taskEXIT_CRITICAL();
//				HAL_UART_Transmit_DMA(&huart3,(char*)gAck,strlen(gAck));			
				osDelay(10);						
				
			}
		}

		//���Ͳɼ��������� �磺����������
		if(gGlobalData.Send_Data_Task==true)
		{

			messageSend.payload =gGlobalData.PlusePressDataSend;                                 //gGlobalData.PlusePressDataSend;
			messageSend.payloadlen =gGlobalData.PlusePressDataLen;                                     //gGlobalData.PlusePressDataLen;
			MQTTPublish(gTopicInfo.streamPost, &messageSend);
			gGlobalData.Send_Data_Task=false;
		}

		//д���ݹ̶�Ӧ��   by YlsAdd 2023/3/8
		if(gGlobalData.Send_Ack_Task==true&&gGlobalData.Send_Heart_Bag==false)   //jlyû��
		{
 			messageSend.payloadlen = strlen(gAck);
			messageSend.payload=gAck;
			if(gGlobalData.rj45Status==true&&gGlobalData.netKind==1)
			{
				MQTTPublish(gTopicInfo.cmdPost, &messageSend);
				printf("ACK  to MQTT!\r\n");
			}
			else if(gGlobalData.wifiStatus==true&&gGlobalData.netKind==2)        													//wifiӦ��
			{	
				sprintf(bufwifi_ack_head,"AT+MQTTPUBRAW=\"%s\",1,0,%d",gTopicInfo.cmdPost,messageSend.payloadlen);  //wifiӦ��ͷ				
				osDelay(50);
				atk_8266_send_cmd((char*)bufwifi_ack_head,strlen(bufwifi_ack_head));
				osDelay(50);
				HAL_UART_Transmit_DMA(&huart3,(char*)gAck,strlen(gAck));
				osDelay(10);

			}
			else if(gGlobalData.yd4gStatus==true&&gGlobalData.netKind==3)																																		//4gӦ��
			{

				sprintf(buf4g_ack_head,"AT+QMTPUBEX=0,0,0,0,\"%s\",%d\r\n",gTopicInfo.cmdPost,messageSend.payloadlen);//4gӦ��ͷ				
				osDelay(10);
				HAL_UART_Transmit_DMA(&huart6,(char*)buf4g_ack_head,strlen(buf4g_ack_head));
				osDelay (50);
				HAL_UART_Transmit_DMA(&huart6,(char*)gAck,strlen(gAck));
				osDelay(10);



			}	
			gGlobalData.Send_Ack_Task=false;

		}
		
		if(gGlobalData.Send_Update_Task==true)
		{

			gGlobalData.Send_Update_Task=false;
//			memset(gSendNetData,0,sizeof(gSendNetData));	
		}

		//��ʪ�Ȳɼ���Ϊ����ʱ ���淽����Ч
 		if(gGlobalData.Send_Ping_Task==true)
		{
			len=MQTTSerialize_pingreq(buf,sizeof(buf));
			if(transport_sendPacketBuffer(MQ_NET_ID,buf,len)<=0)
			{
//				printf("PINGREQ Send errno\r\n");
			}
			gGlobalData.Send_Ping_Task=false;
		}

		
	}
}


void cmdReplyProcess(char* msg,int msgLen)
{
	netData_process(msg,msgLen);
	return;
}

void boarReplyProcess(char*msg,int msgLen)
{
	return;
}
//mqtt ��Ϣ����
void mqttMessageProcess(char*msg,int msgLen,char*topic,int topicLen)
{
//	printf("msg=%s\r\n",msg);   //debug
//	HAL_UART_Transmit(&huart3,msg,strlen(msg),0xFFFF);	
	/*����ָ������*/
	if(strncmp(topic,gTopicInfo.cmdReply,topicLen)==0)
	{
		cmdReplyProcess(msg,msgLen);
	}
	/*�����㲥����*/
	if(strncmp(topic,gTopicInfo.boardReply,topicLen)==0)
	{
		boarReplyProcess(msg,msgLen);
	}
	
}


// �����������������Ļ��ʾ
void GBK_Msgprocess(char*buf)
{	
	int i=0,j=0,c=0,k=0;
	char buffer[18];
	char twobuf[2];
	long ret;
	char *pend;
	uint8_t gbk0[8]={0,};
	uint8_t gbk16[8];
	strcpy(buffer,buf);
	for(c=0;c<9;c++ )	
	{
		for(i=0;i<2;i++)
		{
			twobuf[i]=buffer[j++];	
		}
			ret=strtol(twobuf,&pend,16);
			if(ret==0)
			{
					c=9;
//					gbk16[k]='\0';
				send_visitName(gbk0,8);
				osDelay(10);
				send_visitName(gbk16,k);
//				return gbk16;
			}
			else
			{
			gbk16[k++]=ret;
			memset (twobuf,0,sizeof(twobuf));
			}	
	}
	return;
}


//ʱ�䴦�������Ļ��ʾ
void GBK_Timeprocess(char *buf)
{
  int c=0,i=0,j=2,k=0;
  char buffer[18];
	char buf6[6];
  char twobuf[2];
	long ret;
	char *pend;
	strcpy(buffer,buf);
	for(c=0;c<6;c++)
	{
	   for(i=0;i<2;i++)
		{
	    twobuf[i]=buffer[j++];
    }
		ret=strtol(twobuf,&pend,10);
    buf6[k++]=ret;
		memset (twobuf,0,sizeof(twobuf));

	}
	
	send_rtcTime(buf6[0],buf6[1],buf6[2],buf6[3],buf6[4],buf6[5]);
	return;
}

// ����ʱ���������Ļ��ʾ
void Countdown_Treat(uint16_t count)
{
	int h,min,second;
	h = count / 3600;				    //�����3600�͵���Сʱ
	min = (count % 3600) / 60;		//�Ȱѿ��Ի����Сʱ�������޳����ٳ�60�õ�����
	second = (count % 3600) % 60;		//�ѿ��Ի����Сʱ�������ͷ��ӵ������޳��͵õ�����
	send_countDown(h,min,second);
	return;
}

//������Ϣ��ʾ��
void SendMessageDialog(uint8_t *Msg,uint8_t Msglen)
{
	int c,i,j=0,k=0;
	long ret;
	char *pend;
	char twobuf[2];    
	char Recbuf[200],Msgbuf[200];
	memset(Msgbuf,0,sizeof(Msgbuf));
	strcpy(Recbuf,Msg);
    
	for(c=0;c<Msglen/2;c++)
	{
	 for(i=0;i<2;i++)
		{
	    twobuf[i]=Recbuf[j++];
		}
		ret=strtol(twobuf,&pend,16);
		Msgbuf[k++]=ret;
		memset (twobuf,0,sizeof(twobuf));
	}
	Send_Text_Box(1);
	osDelay(50);
	Send_Text_Content(Msgbuf,strlen(Msgbuf));
	osDelay(50);
	Send_Text_SetButton(0,1);
	osDelay(50);
	Send_Text_SetButton(1,1);
	osDelay(50);
	Send_Text_SetButton(3,1);
	osDelay(50);
}
//���·��Ķ�ά�봢�������´��ϵ�ʹ�� Location:0����ŵ�������� 1����ŵ�Ѩλ��ʹ��
void SendleftQR(char *buf,char QR_locatoin){
	int c,i,j=0,k=0;
	char twobuf[2];
	long ret;
	char *pend;
	char Recbuf[200];
	strcpy(Recbuf,buf);

	for(c=0;c<strlen(Recbuf)/2;c++)
	{
	   for(i=0;i<2;i++)
		{
	    twobuf[i]=Recbuf[j++];
        }
		ret=strtol(twobuf,&pend,16);
		if(QR_locatoin==0){
            gDeviceParam.qrbuf_SWD[k++]=ret;
		}
		else if(QR_locatoin==1){
		gDeviceParam.qrbuf_XWTT[k++]=ret;
		}
		memset (twobuf,0,sizeof(twobuf));
	}
	return;
}


/*�̣߳��������˿ڼ��*/
void StartSendheartTask(void const *arg)
{
	char bufwifi_ack_head[128]={0,};				//����Ϊ����ģʽ
	MQTTMessage messageSend;

	//������Ϣ��ʼ��
	messageSend.dup = 0;
	messageSend.qos = QOS0;
	messageSend.retained = 0;
	messageSend.payload =NULL;//gSendNetData;
	messageSend.payloadlen = 0;//strlen(gSendNetData);
	
	while(1)
	{

	
	
	}
	



}


