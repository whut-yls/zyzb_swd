/**System include**/
#include "cmsis_os.h"
#include "stdbool.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
/*Private include*/
#include "main.h"
#include "common.h"
#include "parameter.h"
#include "user_tcp_client.h"
#include "cJSON.h"
#include "StringTransform.h"
#include "aht15.h"
#include "ms5637.h"
#include "pca9554.h"
#include "dac8831.h"
#include "lcdTask.h"
#include "timer.h"
#include "Wt2003hx.h"

/**
xSemaphoreTake(xSemaphore, portMAX_DELAY);//pdTRUE/pdFALSE(timeout)
xSemaphoreGive(xSemaphore);	
pdMS_TO_TICKS(10000);	ms转换成滴答型时间
mutex 型信号量不能用于中断中
**/
//static char gPressPluseCnt=0;
//static char gGasPluseCnt=0;

char gAck[JSON_ACK_MAX_LEN]={0,};

void close_Dev_Out(void)
{
	return;
}

/**数据流上传包*/
static int general_dataAck(void)
{
	unsigned short int len;
	len=gGlobalData.unitLen*2;
	gGlobalData.PlusePressDataSend[0]=0x5a;
	gGlobalData.PlusePressDataSend[1]=0xa5;
	gGlobalData.PlusePressDataSend[2]=(uint8_t)(gDeviceParam.devId>>24);
	gGlobalData.PlusePressDataSend[3]=(uint8_t)(gDeviceParam.devId>>16);
	gGlobalData.PlusePressDataSend[4]=(uint8_t)(gDeviceParam.devId>>8);
	gGlobalData.PlusePressDataSend[5]=(uint8_t)(gDeviceParam.devId);
	
	if(gGlobalData.curWorkMode==WORK_MODE_ZT)
	{
		gGlobalData.PlusePressDataSend[6]=gGlobalData.useWorkArg[0].chanelNum;//通道总数
		//gGlobalData.PlusePressDataSend[7]=gGlobalData.useWorkArg[0].outs[gGlobalData.channelPos];	//供电通道
		gGlobalData.PlusePressDataSend[7]=gGlobalData.useWorkArg[0].inputs[gGlobalData.channelPos];	//供电通道
	}else{
		gGlobalData.PlusePressDataSend[6]=gGlobalData.useWorkArg[1].chanelNum;
		//gGlobalData.PlusePressDataSend[7]=gGlobalData.useWorkArg[1].outs[gGlobalData.channelPos];
		gGlobalData.PlusePressDataSend[7]=gGlobalData.useWorkArg[1].inputs[gGlobalData.channelPos];	//供电通道
	}	
	//memcpy(&gGlobalData.PlusePressDataSend[8],&len,2);	//8 9
	gGlobalData.PlusePressDataSend[8]=len>>8;
	gGlobalData.PlusePressDataSend[9]=(uint8_t)len;
	len=len+12;//len+2+4+2+2;
	gGlobalData.PlusePressDataSend[len-2]=0xff;
	gGlobalData.PlusePressDataSend[len-1]=0xff;
	return len;
}




/*转换字符为字节*/
int HexStrToByte(const char* source, unsigned char* dest, int sourceLen)  
{  
    short i;  
    unsigned char highByte, lowByte;  

    if(sourceLen%2!=0)
    {
        //printf("device_coder num false!\n");
        return -1;
    }

    for(i=0;i<sourceLen;i++)
    {
        if((source[i]<='9'&& source[i]>='0' )|| (source[i]<='F'&&source[i]>='A' )|| (source[i]<='f'&&source[i]>='a'))
        {

        }else{
            //printf("device_coder scope false!\n");
            return -1;
        }
    }

    for (i = 0; i < sourceLen; i += 2)  
    {  
        highByte = toupper(source[i]);  
        lowByte  = toupper(source[i + 1]);  
        if (highByte > 0x39)  
            highByte -= 0x37;  
        else  
            highByte -= 0x30;  

        if (lowByte > 0x39)  
            lowByte -= 0x37;  
        else  
            lowByte -= 0x30;  

        dest[i / 2] = (highByte << 4) | lowByte;  
    }  
    return 0;  
}

/*判断ip合法性*/
bool isVaildIp(const char *ip,unsigned char*intIp)
{
    int dots = 0; /*字符.的个数*/
		int cnt=0;
    int setions = 0; /*ip每一部分总和（0-255）*/ 
 
    if (NULL == ip || *ip == '.') { /*排除输入参数为NULL, 或者一个字符为'.'的字符串*/ 
         return false;
    }   
 
    while (*ip) {
				cnt++;
				if(cnt>=15)
					break;
 
        if (*ip == '.') {		
          dots ++; 
					intIp[dots-1]=setions;
           if (setions >= 0 && setions <= 255) { /*检查ip是否合法*/
                 setions = 0;
                 ip++;
				 
                 continue;
             }   
             return false;
         }   
         else if (*ip >= '0' && *ip <= '9') { /*判断是不是数字*/
             setions = setions * 10 + (*ip - '0'); /*求每一段总和*/
         } else 
				 {
             //return false;
					 break;
				 }
         ip++;   
    }   
 	/*判断IP最后一段是否合法*/ 
     if (setions >= 0 && setions <= 255) {
         if (dots == 3) {
			  intIp[dots]=setions;
             return true;	
         }   
     }
     	return false;
}


/*检测合法的mac*/
bool isVaildMac(char *mac,unsigned char*intMac)
{
	 if(6==AscToHex(mac, intMac,12))
	 {
		 return true;
	 }else{
		 return false;
	 }
}

/*会话json生成*/
void general_sessionAck(int fun,int status,char*msg)
{
	cJSON *root;
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,"ACK",NULL);
	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	cJSON_AddNumberToObject(root,KEY_CODE,status);
	cJSON_AddStringToObject(root,KEY_MSG,(char*)buf);

	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);

	return;
}

/*发送系统检测应答*/
void general_SysCheck_Ack(int fun,int status,char*msg)
{
		cJSON *root;
	uint8_t rj45S,wifiS,yd4gS;
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	//rj45
	if(gGlobalData.rj45Status==1)	//正常
	{
		rj45S=0;
	}else if(gGlobalData.rj45Status==0)	//端开
	{
		rj45S=1;
	}else{	//异常
		rj45S=2;
	}
	//wifi
		if(gGlobalData.wifiStatus==1)	//正常
	{
		wifiS=0;
	}else if(gGlobalData.wifiStatus==0)	//端开
	{
		wifiS=1;
	}else{	//异常
		wifiS=2;
	}
	//yd4g
		if(gGlobalData.yd4gStatus==1)	//正常
	{
		yd4gS=0;
	}else if(gGlobalData.yd4gStatus==0)	//端开
	{
		yd4gS=1;
	}else{	//异常
		yd4gS=2;
	}
	
	cJSON_AddNumberToObject(root,KEY_RJ45,rj45S);
	cJSON_AddNumberToObject(root,KEY_WIFI,wifiS);
	cJSON_AddNumberToObject(root,KEY_4G,yd4gS);

	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}

/*发送系统检测应答*/
void general_JianChe_Ack(int fun,int status,char*msg)
{
	cJSON *root=NULL;
	cJSON *item_obj = NULL,*item_work=NULL,*item_env=NULL;//数组对象
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	
	cJSON_AddNumber2ToObject(root ,KEY_LONGITUDE, gSimInfo.longitude, 4);
	cJSON_AddNumber2ToObject(root ,KEY_LATITUDE, gSimInfo.latitude, 4);
	
	//sim
	item_obj= cJSON_AddObjectToObject(root,KEY_SIGNAL);
	cJSON_AddNumberToObject(item_obj,KEY_SIM,gSimInfo.signal);
	if(gSimInfo.sim==SIM_YD)
	{
		cJSON_AddStringToObject(item_obj,KEY_SIM,"zgyd");
	}else if(gSimInfo.sim==SIM_LT)
	{	
		cJSON_AddStringToObject(item_obj,KEY_SIM,"zglt");
	}else if(gSimInfo.sim==SIM_DX)
	{
		cJSON_AddStringToObject(item_obj,KEY_SIM,"zgdx");
	}else{
		cJSON_AddStringToObject(item_obj,KEY_SIM,"error");
	}
	
	cJSON_AddStringToObject(item_obj,KEY_RUN_TIME,(char*)gSimInfo.runTime);
	cJSON_AddNumberToObject(item_obj,KEY_UP_RATE,gSimInfo.upLinkRate);
	cJSON_AddNumberToObject(item_obj,KEY_DOWN_RATE,gSimInfo.downLinkRate);
	cJSON_AddNumberToObject(item_obj,KEY_PING,gSimInfo.ping);
	//work
	item_work= cJSON_AddObjectToObject(root,KEY_WORK);
	if(gGlobalData.rj45Status==1||gGlobalData.wifiStatus==1||gGlobalData.yd4gStatus==1)
	{
		cJSON_AddNumberToObject(item_obj,KEY_NET_STATE,0);
	}else{
		cJSON_AddNumberToObject(item_obj,KEY_NET_STATE,1);
	}
	
	cJSON_AddNumberToObject(item_obj, KEY_CUR_NET,gGlobalData.currentNet);
	
	if(gGlobalData.curWorkMode==WORK_MODE_WT)
	{
			cJSON_AddNumberToObject(item_obj, KEY_WORK_STATE,1);
	}else if(gGlobalData.curWorkMode==WORK_MODE_ZT||gGlobalData.curWorkMode==WORK_MODE_JB)
	{
			cJSON_AddNumberToObject(item_obj, KEY_WORK_STATE,2);
	}else if(gGlobalData.curWorkMode==WORK_MODE_ZL)
	{
			cJSON_AddNumberToObject(item_obj, KEY_WORK_STATE,3);
	}
	//environment
	item_work= cJSON_AddObjectToObject(root,KEY_ENV);
	cJSON_AddNumber2ToObject(root ,KEY_TEMP, gSimInfo.longitude, gSensorData1.RHt);
	cJSON_AddNumberToObject(item_obj,KEY_HUMID,gSensorData1.RH);
	cJSON_AddNumber2ToObject(item_obj,KEY_PRESSURE,gSensorData2.InnerPressure,1);
	
	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}

/*rj45网口信息数据*/
void general_Rj45_Ack(int fun,int status,char*msg)
{
	cJSON *root;
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	cJSON_AddNumberToObject(root,KEY_CODE,status);
	cJSON_AddStringToObject(root,KEY_MSG,(char*)buf);
	
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%02x%02x%02x%02x%02x%02x",gDeviceParam.rj45Arg.MacAddress[0],gDeviceParam.rj45Arg.MacAddress[1],gDeviceParam.rj45Arg.MacAddress[2],gDeviceParam.rj45Arg.MacAddress[3],gDeviceParam.rj45Arg.MacAddress[4],gDeviceParam.rj45Arg.MacAddress[5]);
	cJSON_AddStringToObject(root,KEY_MAC,(char*)buf);

//	memset(buf,0,sizeof(buf));
//	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.ServerIpAddress[0],gDeviceParam.ServerIpAddress[1],gDeviceParam.ServerIpAddress[2],gDeviceParam.ServerIpAddress[3]);
//	cJSON_AddStringToObject(root,KEY_SERVER_IP,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.rj45Arg.IpAddress[0],gDeviceParam.rj45Arg.IpAddress[1],gDeviceParam.rj45Arg.IpAddress[2],gDeviceParam.rj45Arg.IpAddress[3]);
	cJSON_AddStringToObject(root,KEY_IP,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.rj45Arg.MaskAddress[0],gDeviceParam.rj45Arg.MaskAddress[1],gDeviceParam.rj45Arg.MaskAddress[2],gDeviceParam.rj45Arg.MaskAddress[3]);
	cJSON_AddStringToObject(root,KEY_MASK,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.rj45Arg.GateAddress[0],gDeviceParam.rj45Arg.GateAddress[1],gDeviceParam.rj45Arg.GateAddress[2],gDeviceParam.rj45Arg.GateAddress[3]);
	cJSON_AddStringToObject(root,KEY_GATE,(char*)buf);
	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}

/*wifi网口信息数据*/
void general_Wifi_Ack(int fun,int status,char*msg)
{
	cJSON *root;
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	cJSON_AddNumberToObject(root,KEY_CODE,status);
	cJSON_AddStringToObject(root,KEY_MSG,(char*)buf);
	
	cJSON_AddStringToObject(root,KEY_WIFI,(char*)gDeviceParam.wifiArg.WifiName);
	cJSON_AddStringToObject(root,KEY_WIFI,(char*)gDeviceParam.wifiArg.WifiPwd);
	
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%02x%02x%02x%02x%02x%02x",gDeviceParam.wifiArg.MacAddress[0],gDeviceParam.wifiArg.MacAddress[1],gDeviceParam.wifiArg.MacAddress[2],gDeviceParam.wifiArg.MacAddress[3],gDeviceParam.wifiArg.MacAddress[4],gDeviceParam.wifiArg.MacAddress[5]);
	cJSON_AddStringToObject(root,KEY_MAC,(char*)buf);

//	memset(buf,0,sizeof(buf));
//	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.ServerIpAddress[0],gDeviceParam.ServerIpAddress[1],gDeviceParam.ServerIpAddress[2],gDeviceParam.ServerIpAddress[3]);
//	cJSON_AddStringToObject(root,KEY_SERVER_IP,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.wifiArg.IpAddress[0],gDeviceParam.wifiArg.IpAddress[1],gDeviceParam.wifiArg.IpAddress[2],gDeviceParam.wifiArg.IpAddress[3]);
	cJSON_AddStringToObject(root,KEY_IP,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.wifiArg.MaskAddress[0],gDeviceParam.wifiArg.MaskAddress[1],gDeviceParam.wifiArg.MaskAddress[2],gDeviceParam.wifiArg.MaskAddress[3]);
	cJSON_AddStringToObject(root,KEY_MASK,(char*)buf);

	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.wifiArg.GateAddress[0],gDeviceParam.wifiArg.GateAddress[1],gDeviceParam.wifiArg.GateAddress[2],gDeviceParam.wifiArg.GateAddress[3]);
	cJSON_AddStringToObject(root,KEY_GATE,(char*)buf);
	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}

/*wifi网口信息数据*/
void general_BaseArg_Ack(int fun,int status,char*msg)
{
	cJSON *root;
	static char *jp;
	int jsonLen;
	char buf[36];
	memset(buf,0,sizeof(buf));
	strncpy(buf,msg,35);	//防止msg越界
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();

	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	cJSON_AddNumberToObject(root,KEY_CODE,status);
	cJSON_AddStringToObject(root,KEY_MSG,(char*)buf);
	//serverIp
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%d.%d.%d.%d",gDeviceParam.ServerIpAddress[0],gDeviceParam.ServerIpAddress[1],gDeviceParam.ServerIpAddress[2],gDeviceParam.ServerIpAddress[3]);
	cJSON_AddStringToObject(root,KEY_SERVER_IP,(char*)buf);
	//serverPort
	cJSON_AddNumberToObject(root,KEY_SERVER_PORT,gDeviceParam.mqArg.mqttPort);
	//new devid
	cJSON_AddNumberToObject(root,KEY_HEAR_RATE,gDeviceParam.heartRate);
	//prodectKey
	cJSON_AddStringToObject(root,KEY_PRODUCT,gDeviceParam.productKey);
		//hardversion
	cJSON_AddStringToObject(root,KEY_HARD_VER,HardVersion );
		//softverson
	cJSON_AddStringToObject(root,KEY_SOFT_VER,SoftVersion);
	
	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}

/*发送固定应答包**/
void Send_Fix_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_sessionAck(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	//transport_sendPacketBuffer(NET_LINK_ID0,(uint8_t*)gAck,strlen(gAck));
	return;
}

/*发送系统设备检测*/
void Send_SysCheck_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_SysCheck_Ack(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	return;
}

/*发送系统监测数据*/
void Send_JianChe_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_JianChe_Ack(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	return;
}

/*发送rj45网口信息*/
void Send_Rj45_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_Rj45_Ack(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	return;
}

/*发送wifi网口信息*/
void Send_Wifi_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_Wifi_Ack(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	return;
}

/*发送基本设备信息*/
void Send_BaseArg_Ack(int functionFlag,int status,char*msg)
{
	/*回复固定应答*/
	general_BaseArg_Ack(functionFlag,status,msg);
	gGlobalData.Send_Ack_Task=true;
	return;
}

/*脉搏压力数据发送  根据是否发送标识来决定是否发送*/
int SendPlusePressData(void)
{
	int len;

	len=general_dataAck();
	gGlobalData.PlusePressDataLen=len;
//	ret=transport_sendPacketBuffer(NET_LINK_ID0,(unsigned char*)gGlobalData.PlusePressDataSend,len);	
//	memset(gGlobalData.PlusePressDataSend,0,sizeof(gGlobalData.PlusePressDataSend));
	gGlobalData.Send_Data_Task=true;
	return 1;
}

	
//工作模式控制
void do_work_ctl(uint8_t workMode)
{
	switch(workMode){
		case Lcd_Button_to_Start:
			if(gGlobalData.curWorkState != WORK_START){
				if(gGlobalData.curWorkMode == 1){										//如果是生物电理疗 再次开启的时候将电压值显示最新的值			
					set_sampleMode(MODE_ZL);
					if(level <= 60)
					{
						Send_LcdVoltage(4.20f*level);									//适用于低功率放大板子   3.125
					}
					else 
						Send_LcdVoltage(4.20*60);	
				}
				else if(gGlobalData.curWorkMode == 2){															//疼痛理疗 		//2023.02.21 kardos
					send_startORstop_xwtt(1);																				//启动指令
					osDelay(200);
					if(gGlobalData.curWorkState == WORK_PAUSE){//2023.02.24 kardos  先设置波形 再设置ab两个通道的电流值
						send_wave_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat); //发送波形
						osDelay(200);
						send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
						osDelay(200);
						send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
						osDelay(200);
					}
				}
				gGlobalData.cur_heart_state = WORKING;
				gGlobalData.curWorkState = WORK_START;
				osDelay(50);
				send_LcdSync(1); 																						//屏幕运行灯绿灯
				osDelay(50);
				send_LcdWorkStatus(3);																			//穴位理疗中   
				osDelay(50);	
				send_LcdOutStatus(1);//设置输出为绿灯 kardos 2023.03.05
			}				
			break;
		case Lcd_Button_to_Pause:
			if(gGlobalData.curWorkState == WORK_START){
				gGlobalData.cur_heart_state = PAUSE;
				gGlobalData.curWorkState = WORK_PAUSE;
				if(gGlobalData.curWorkMode == 2){		//2023.02.21 kardos
					send_startORstop_xwtt(0);
				}
				else if(gGlobalData.curWorkMode == 1){	//如果是生物电理疗 暂停时电压给0V
					Send_LcdVoltage(0);
				}
				osDelay(20);					
				send_LcdSync(0);  
				set_sampleMode(MODE_CLOSE);																	//设置模式
				osDelay(200);
				send_LcdWorkStatus(5);																			//设备暂停状态
				osDelay(200);	
				send_LcdOutStatus(0);//设置输出为红灯 kardos 2023.03.05
				gGlobalData.Auto_Level_Ctl = 0;  			  
			}
			break;
		case Lcd_Button_to_Reset: 
			Send_Fix_Ack(100,STATUS_OK,"OK");
			gGlobalData.cur_heart_state = LEISURE;
			gGlobalData.curWorkState = WORK_STOP; 
			gGlobalData.Alltime = 0;
			if(gGlobalData.oldWorkMode == 1){
					send_treatSel(0,0,0);   																  //swd治疗方案全置成0
					Send_LcdVoltage(0);//电压值设置为0
			}
			else if(gGlobalData.oldWorkMode == 2){															//疼痛理疗
					send_startORstop_xwtt(0);
					osDelay(100);
					send_treatSel_Xwtt(0,0,0,0);  
			}
			osDelay(100);	
			send_LcdOutStatus(0);//设置输出为红灯 kardos 2023.03.05
			osDelay(100);
			send_LcdSync(0);  					
			set_sampleMode(MODE_CLOSE);																	//设置模式
			gGlobalData.curWorkMode = WORK_MODE_WT;
			gGlobalData.oldWorkMode = gGlobalData.curWorkMode;
			gGlobalData.current_treatNums = 0;
			gGlobalData.channelPos = 0;
			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET); 				//运行红灯 set灭 reset亮
			HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET); 					//运行绿灯 set灭 reset亮
			osDelay(50);
			send_LcdWorkStatus(1);																			//设备待机状态       
			osDelay(50);
			Send_ComMusic(3);
			osDelay(50);
			send_lcdPage(0);    
			level=0;//复位时 给0  2023.04.04 kardos
			gGlobalData.Auto_Level_Ctl = 0;  
			HAL_TIM_Base_DeInit(&htim12);   //不产生波形
			HAL_TIM_Base_DeInit(&htim1);
			break;
		case Lcd_Button_to_Level_Up_SWD:     //swd档位加
			if(level <= 60)
			{
				level += 1;								
				Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);//波形选择
				Dac8831_Set_Amp(level, ch1buf);//幅值改变		
				osDelay(1);													
				send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
							level,
							(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);
				osDelay(500);
				Send_LcdVoltage(4.20f*level);//适用于低功率放大板子	
				if(gGlobalData.ZL_Feedback_To_Down_Level == 0)
					RecRmsl_old = Gets_Rmsl_update(RecRmsl);               //获取最新反馈值，人为加减档时重新赋值				
			}	
			break;						
		case Lcd_Button_to_Level_Down_SWD:    //SWD档位减
			if(level >= 5)
			{
				level -= 1;
				Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);//波形选择
				Dac8831_Set_Amp(level, ch1buf);//幅值改变	
				osDelay(1);			 
				send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
											level,
											(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);
				osDelay(500);
				Send_LcdVoltage(4.20f*level);//适用于低功率放大板子			
				if(gGlobalData.ZL_Feedback_To_Down_Level == 0)
					RecRmsl_old = Gets_Rmsl_update(RecRmsl);               //获取最新反馈值，人为加减档时重新赋值
			}		
			break;
		//A通道  +5
		case Lcd_Button_to_Level_Up_XWTT_A:		
			if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower <= 250)	
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower += 2;
			send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
																			gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
																			(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
																			gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
			osDelay(200);
			send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);                    
			break;
			//A通道  -5
		case Lcd_Button_to_Level_Down_XWTT_A:
			if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower > 6)
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower -= 2;			
			send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
					gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
					(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
					gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
			osDelay(200);
			send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
			break;
			//B通道  +5
		case Lcd_Button_to_Level_Up_XWTT_B:
			if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower <= 250)		
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower += 2;			
			send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
					gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
				(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
					gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
			osDelay(200);
			send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
     break;
		//B通道  -5
		case Lcd_Button_to_Level_Down_XWTT_B:
			if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower > 6)														
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower -= 2;
			send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
			(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
				gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
			osDelay(200);
			send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
			break;
		default: 
			break;
	}
	return;
}

uint16_t Gets_Average(uint16_t *pt,int l)
{
	//uint16_t *p = pt;
	uint16_t nums = 0;
	uint32_t sum = 0;
//	uint16_t buf_temp[l];
//	memcpy(buf_temp, pt, l);
	for(int i = 0; i < l; ++i)
	{
		if(pt[i] != 0)
		{
			sum = sum + (uint32_t)(pt[i]);
			nums++;
		}
		
	}
	if(nums == 0) return 0;
	else
		return (uint16_t)(sum/nums);
}


uint16_t Gets_Rmsl(uint16_t *pt,int l)         //计算有效值
{
	int16_t Rec_RmsTemp = 0;
	static uint32_t sum = 0;
	static uint16_t rms = 0;
	
	for(uint16_t i = 0 ; i < l ;i++){
		Rec_RmsTemp = *(pt+i)*3300/65535-1650;   
		sum += Rec_RmsTemp * Rec_RmsTemp;
	}	
	rms = (uint16_t)sqrt(sum/l);
	sum=0;
	return rms;
}
uint16_t Gets_Rmsl_update(uint16_t data){
	return data;
}

//hyadd
void general_heartBag(int fun, int status, int netKind, int workState, int timeLast)
{
	cJSON *root;
	cJSON *item_work=NULL, *item_env=NULL ,*item_para;
	static char *jp;
	int jsonLen;
	memset(gAck,0,sizeof(gAck));
	root = cJSON_CreateObject();
	
	cJSON_AddNumberToObject(root,"HeartBag",NULL);			
	cJSON_AddNumberToObject(root,KEY_DEV_ID,gDeviceParam.devId);		
	cJSON_AddNumberToObject(root,KEY_SESSION,gGlobalData.sessionId);
	cJSON_AddStringToObject(root,KEY_TIME,(const char *)gGlobalData.ack_time);
	cJSON_AddNumberToObject(root,KEY_FUN,fun);
	cJSON_AddStringToObject(root,KEY_Flage,"SWD");
    
	item_work = cJSON_AddObjectToObject(root,KEY_WORK);
	cJSON_AddNumberToObject(item_work ,KEY_CUR_NET, netKind);
	cJSON_AddNumberToObject(item_work ,KEY_WORK_STATE, workState);
	cJSON_AddNumberToObject(item_work ,KEY__WORK_TIME_LAST, timeLast);
	cJSON_AddNumberToObject(item_work ,KEY__WORK_CURRENT, gGlobalData.currentNet);
	cJSON_AddNumberToObject(item_work ,KEY__ZL_CURRENT,RecRmsl*5);      //治疗人体流过的电流有效值 单位ua  2024/1/22
	
	item_env = cJSON_AddObjectToObject(root,KEY_ENV);
	cJSON_AddNumberToObject(item_env ,KEY_TEMP, (int) (gSensorData1.RHt*100+0.5f)/100.0);
	cJSON_AddNumberToObject(item_env,KEY_HUMID,gSensorData1.RH);

	item_para = cJSON_AddObjectToObject(root,KEY_PARA);
	cJSON_AddNumberToObject(item_para ,KEY_LEVEL, (char)level);
	cJSON_AddNumberToObject(item_para,KEY_APOWER,gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
    cJSON_AddNumberToObject(item_para,KEY_BPOWER,gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
	//cJSON_AddStringToObject(root,KEY_UPD_FLAGE,"SWD");
	
	jp = cJSON_PrintUnformatted(root);
	if(jp==NULL)
	{
		NVIC_SystemReset();
	}else{
		jsonLen=strlen(jp);
		if(jsonLen<=JSON_ACK_MAX_LEN)
		strncpy(gAck,jp,jsonLen);
		free(jp);
	}
	cJSON_Delete(root);
	return;
}
void Send_heartBag(int fun, int status, int netKind, int workState, int timeLast)
{
	general_heartBag(fun,status,netKind,workState,timeLast);
	if(netKind==1&&gGlobalData.rj45Status==true)
		gGlobalData.Send_Heart_Bag=true;
	if(netKind==3&&gGlobalData.yd4gStatus)
		gGlobalData.Send_Heart_Bag_4G=true;
	if(netKind==2&&gGlobalData.wifiStatus)
		gGlobalData.Send_Heart_Bag_wifi=true;
	return;
}

