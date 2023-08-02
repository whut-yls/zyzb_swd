#include "ad9833.h"
#include "main.h"
//#include "stm32f4xx_hal.h"
//
//#include "mygpio.h"
//#include "myspi.h"
//***************************
//***************************	
//数字电位器片选cs

/*
*********************************************************************************************************
*	函 数 名: AD9833_Write
*	功能说明: 向SPI总线发送16个bit数据
*	形    参: TxData : 数据
*	返 回 值: 无
*********************************************************************************************************
*/
void AD9833_Write(unsigned short int TxData)
{
	uint8_t byte[3]={0,};
	byte[0]=TxData>>8;
	byte[1]=(uint8_t)TxData;
	
//	WriteNSS1(GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1,byte,2,0xffff);
//	WriteNSS1(GPIO_PIN_SET);
} 

/*
*********************************************************************************************************
*	函 数 名: AD9833_AmpSet
*	功能说明: 改变输出信号幅度值
*	形    参: 1.amp ：幅度值  0- 255
*	返 回 值: 无
*********************************************************************************************************
*/ 


void AD9833_AmpSet(unsigned char amp)
{
	uint16_t temp;
	uint8_t byte[3]={0,};
	#if 0
	unsigned char i;
	unsigned int temp;
   	
	CS_0();
	temp =0x1100|amp;
	for(i=0;i<16;i++)
	{
	    SCK_0();	
	   if(temp&0x8000)
	   	DAT_1();
	   else
		DAT_0();
		temp<<=1;
	    SCK_1();
	    AD9833_Delay();
	}
	
   	CS_1();
	#endif
	
	temp =0x1100|amp;
	byte[0]=temp>>8;
	byte[1]=(uint8_t)temp;
//	WriteNSS4(GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi3,byte,2,0xffff);
//	WriteNSS4(GPIO_PIN_SET);
	
	return;
}


/*
*********************************************************************************************************
*	函 数 名: AD9833_WaveSeting
*	功能说明: 向SPI总线发送16个bit数据
*	形    参: 1.Freq: 频率值, 0.1 hz - 12Mhz
			  2.Freq_SFR: 0 或 1
			  3.WaveMode: TRI_WAVE(三角波),SIN_WAVE(正弦波),SQU_WAVE(方波)
			  4.Phase : 波形的初相位
*	返 回 值: 无
*********************************************************************************************************
*/ 
void AD9833_WaveSeting(double Freq,unsigned int Freq_SFR,unsigned int WaveMode,unsigned int Phase )
{

		int frequence_LSB,frequence_MSB,Phs_data;
		double   frequence_mid,frequence_DATA;
		long int frequence_hex;

		/*********************************计算频率的16进制值***********************************/
		frequence_mid=268435456/25;//适合25M晶振
		//如果时钟频率不为25MHZ，修改该处的频率值，单位MHz ，AD9833最大支持25MHz
		frequence_DATA=Freq;
		frequence_DATA=frequence_DATA/1000000;
		frequence_DATA=frequence_DATA*frequence_mid;
		frequence_hex=frequence_DATA;  //这个frequence_hex的值是32位的一个很大的数字，需要拆分成两个14位进行处理；
		frequence_LSB=frequence_hex; //frequence_hex低16位送给frequence_LSB
		frequence_LSB=frequence_LSB&0x3fff;//去除最高两位，16位数换去掉高位后变成了14位
		frequence_MSB=frequence_hex>>14; //frequence_hex高16位送给frequence_HSB
		frequence_MSB=frequence_MSB&0x3fff;//去除最高两位，16位数换去掉高位后变成了14位

		Phs_data=Phase|0xC000;	//相位值
		AD9833_Write(0x0100); //复位AD9833,即RESET位为1
		AD9833_Write(0x2100); //选择数据一次写入，B28位和RESET位为1

		if(Freq_SFR==0)				  //把数据设置到设置频率寄存器0
		{
		 	frequence_LSB=frequence_LSB|0x4000;
		 	frequence_MSB=frequence_MSB|0x4000;
			 //使用频率寄存器0输出波形
			AD9833_Write(frequence_LSB); //L14，选择频率寄存器0的低14位数据输入
			AD9833_Write(frequence_MSB); //H14 频率寄存器的高14位数据输入
			AD9833_Write(Phs_data);	//设置相位
			//AD9833_Write(0x2000); /**设置FSELECT位为0，芯片进入工作状态,频率寄存器0输出波形**/
	    }
		if(Freq_SFR==1)				//把数据设置到设置频率寄存器1
		{
			 frequence_LSB=frequence_LSB|0x8000;
			 frequence_MSB=frequence_MSB|0x8000;
			//使用频率寄存器1输出波形
			AD9833_Write(frequence_LSB); //L14，选择频率寄存器1的低14位输入
			AD9833_Write(frequence_MSB); //H14 频率寄存器1为
			AD9833_Write(Phs_data);	//设置相位
			//AD9833_Write(0x2800); /**设置FSELECT位为0，设置FSELECT位为1，即使用频率寄存器1的值，芯片进入工作状态,频率寄存器1输出波形**/
		}

		if(WaveMode==TRI_WAVE) //输出三角波波形
		 	AD9833_Write(0x2002); 
		if(WaveMode==SQU_WAVE)	//输出方波波形
			AD9833_Write(0x2028); 
		if(WaveMode==SIN_WAVE)	//输出正弦波形
			AD9833_Write(0x2000); 

}




