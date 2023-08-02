/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "ethernetif.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "user_tcp_server.h"
#include "user_tcp_client.h"
#include "console.h"

#include "stm32h743i_eval_qspi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#include "myAdc.h"
#include "startLoopTask.h"
#include "pca9554.h"
#include "common.h"
#include "com.h"
#include "ad9833.h"
#include "lcdTask.h"
#include "dac8831.h"
#include "crc16.h"
#include "wifi.h"
#include "EC20_4G.h"
#include "timer.h"
#include "Wt2003hx.h"
#include "dac8831.h"
SemaphoreHandle_t					ConsoleReceive_Semaphore=NULL; 
SemaphoreHandle_t  	lcdSemaphore = NULL;
unsigned char RecCom7[COM7_REC_MAX+1];	//pc
//uint16_t RecCom7[COM7_REC_MAX+1];	//pc
uint16_t 			RecCom7Num=0;
unsigned char RecCom3[COM3_REC_MAX+1];	//pc
uint16_t 			RecCom3Num=0;
unsigned char RecCom6[COM6_REC_MAX+1];	//pc
uint16_t 			RecCom6Num=0;
uint8_t       RecSen[30];   //接收屏幕返回数据
uint32_t channelTime = 0;
bool gUartPcInit=false,gUartPcTc=false;
bool gUartLcdInit=false,gUartLcdTc=false;
bool yd_4gfalg;
uint32_t countTime = 0;//计数器，用于倒计时计算
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool gEthRecvStatus=false;  //控制端口状态
bool gEthStatus=false;      //网络状态
bool gRecvEthChange=false; 	//状态改变标识
bool gMqttLinkStatus=false;      //mq网络状态
void StartDefaultTask(void const * argument);
int cnt_heartbag = 0;//心跳

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc3;

//ETH_HandleTypeDef heth;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;
SPI_HandleTypeDef hspi5;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart4;
UART_HandleTypeDef huart7;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_uart4_rx;
DMA_HandleTypeDef hdma_uart7_rx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_usart6_tx;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_spi3_tx;


osThreadId defaultTaskHandle;
osThreadId startLoopTaskHandle;
osThreadId consoleTaskHandle;
osThreadId wifiTaskHandle;
osThreadId ec4gTaskHandle;

osThreadId mqttClientTaskHandle;
osThreadId mqttSendTaskHandle;
osThreadId ethernetTaskTaskHandle;
osThreadId sendheartTaskHandle;


/* USER CODE BEGIN PV */
uint16_t gREFVOL_VAL=3300;  //3300
//int16_t gAdcVol[33];
uint16_t gADC1_DMA_BUF[AD1_NUM*N_NUM];
uint16_t gADC1_VALUE[AD1_NUM];
uint16_t gADC1_VALUE_F[AD1_NUM];

uint16_t gADC3_DMA_BUF[AD3_NUM*N_NUM];
uint16_t gADC3_VALUE[AD3_NUM];
uint16_t gADC3_VALUE_F[AD3_NUM];


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void MPU_Config(void);
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void Console_Task(void const * pvParameters);
void StartDefaultTask(void const * argument);
void StartLoopTask(void const * argument);

IWDG_HandleTypeDef IWDG_Handler; //独立看门狗句柄
void StartIwdgTask(void const *arg);
EventGroupHandle_t xCreatedEventGroup;
void IWDG_Init(uint8_t prer, uint16_t rlr);
void IWDG_Feed(void);
/* USER CODE BEGIN PFP */





/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
bool Console_TaskSemaphore_Init(void)
{
	ConsoleReceive_Semaphore = xSemaphoreCreateBinary();

	if (ConsoleReceive_Semaphore == NULL )
	{
		return false;
	}
	 return true;
}

/*软件定时器*/
uint16_t time_ = 1; //ms
//static uint16_t s_WavePos  = 0;
//static volatile uint8_t flagAutoLoadTimerRun = 0;
static void vAutoLoadTimerFunc( TimerHandle_t xTimer );

#define mainAUTO_RELOAD_TIMER_PERIOD pdMS_TO_TICKS( time_ )
uint16_t ch1buf[100];
static int cnt_led = 0;			//运行指示灯
static int cnt_down = 0;		//屏幕倒计时
 static int Resetcnt = 0;
bool clear_time = false;

uint8_t level=0;//档位强度控制变量

//软件定时器回调函数
static void vAutoLoadTimerFunc( TimerHandle_t xTimer )
{
	//xEventGroupSetBits(xCreatedEventGroup, TASK_BIT_1);//喂狗
	if(cnt_led == 0){
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	}
	else if(cnt_led == 500)
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
	Resetcnt++;
	
	cnt_led++;
	if(gGlobalData.curWorkState == WORK_START && channelTime < 0xFFFFFFF0)
	{channelTime++;
		cnt_down++;}
	if(Resetcnt==1000){
		Resetcnt=0;
		if(gGlobalData.ResetDevidcnt>=3)
		{	
			Restore_Default_Parameter();
		
			Save_Parameter();
		}
		gGlobalData.ResetDevidcnt=0;
		
	}
		
	if(cnt_led == 1000)
		cnt_led = 0;
	if(cnt_down == 1000)
	{
		if(gGlobalData.curWorkState == WORK_START)
		{
			if((gGlobalData.Alltime != 0 && gGlobalData.curWorkState == WORK_START))
			{
				gGlobalData.Alltime = gGlobalData.Alltime - 1;
			}
		}
		cnt_down = 0;
	}
	
//处理心跳包   2023.04.07 
	if(cnt_heartbag > gDeviceParam.heartRate *1000)//到时间 
	{
		cnt_heartbag = 0;
		gGlobalData.heartbag_flage = 1;
	}
	cnt_heartbag++;
	if(gGlobalData.heart_count < 5* gDeviceParam.heartRate *1000)
		gGlobalData.heart_count++;
	else
	{
			if(gGlobalData.netKind==1){              //说明时网口断开了
				gGlobalData.isConnect=0;
				gGlobalData.conFlage=2;//让wifi去连接       //网口的线程不解析心跳应答     
				gGlobalData.Wifi_set = true;														//重新设置一下wifi参数
			}
			else if(gGlobalData.netKind==2){           //说明时wifi断开了  
				gGlobalData.isConnect=0;//说明网络断开 
				gGlobalData.conFlage=3;//让4g去连接        
				gGlobalData.yd4gStatus=false;
			}
			else if(gGlobalData.netKind==3){      //说明时4g断开了
				gGlobalData.isConnect=0;						//说明网络断开 
				gGlobalData.conFlage=2;							//让wifi去连接				
				gGlobalData.yd4gStatus=false;
				gGlobalData.Wifi_set = true;					
			}
		 gGlobalData.heart_count = 0;//超时计数器置为0 重新开始计数
	}


	
	
}

unsigned portBASE_TYPE defaultTask_stack;

unsigned portBASE_TYPE Console_Task_stack;


/**
  * @brief  The application entry point.
  * @retval int
  */

int main(void)
{
	uint8_t version[5];
  SCB->VTOR = FLASH_BANK1_BASE | 0x80000;
	gGlobalData.current_time = 0;
	gGlobalData.current_treatNums = 0;
	gGlobalData.useWorkArg[0].timeTreat = 0;
	gGlobalData.useWorkArg[0].waitTime = 0;

  MakeSinTable(ch1buf, 100, 0, 65535);          	//初始化波形
  Dac8831_Set_Amp(10, ch1buf);

//new
/* USER CODE BEGIN 1 */
	 MPU_Config();
	
	 SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();
  SCB_DisableDCache();
	
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */
	MX_DMA_Init();
	
	/* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
	MX_ADC3_Init();
//  MX_ETH_Init();
  MX_I2C2_Init();
  MX_SPI5_Init();
	MX_USART3_UART_Init();
  MX_UART4_Init();
  MX_UART7_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_USART2_UART_Init();
//  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
	Console_TaskSemaphore_Init();
	
	lcdTask_Semaphore_Init();
  /* USER CODE END 2 */
	MX_I2C1_Init();

  /* USER CODE END SysInit */
	//new end
	Init_All_Parameter(); 

	gDeviceParam.heartRate=15;    //心跳间隔30s
	gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat=0;
	gGlobalData.useWorkArg[gGlobalData.current_treatNums].level=0;
	gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat=0;    										//初始化频率档位治疗时长值
	__enable_irq();
  /* USER CODE BEGIN 2 */
	MX_LWIP_Init();
	send_lcdPage(0);
	osDelay(100);//kardos 2023.03.30
	send_NetSync(0);
	osDelay(100);//kardos 2023.03.30
	send_duan_wang(1);	//断网提示框打开 1900 	

	osDelay(20);
	Send_LcdDevid_id(gDeviceParam.devId);  //发送设备id到屏幕显示
	osDelay(20);
	sprintf(version,"%s",gDeviceParam.version);
	Send_LcdVersion(version,strlen((const char*)version));	 //发送版本号到屏幕显示 
	osDelay(20);
	Send_Text_SetButton(0,0);	//文本框按键默认不可用
	osDelay(20);
	Send_Text_SetButton(1,0);
	osDelay(20);
	Send_Text_SetButton(3,0);


///////////DMASPI发送测试////////////////////
	DAC8831_Set_Data(0x7fff);
///////////测试音乐////////////////////

//    osDelay(200);

//    Send_ComVolume(30);	
    																
	//HAL_GPIO_WritePin(GPIOC,GPIO_PIN_12,GPIO_PIN_SET); 																			//外壳网络灯
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_11,GPIO_PIN_RESET); 																			//电源灯绿色
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET); 																			//运行红灯 set灭 reset亮
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET); 																				//运行绿灯 set灭 reset亮
	
	TimerHandle_t xAutoReloadTimer;  
	xAutoReloadTimer = xTimerCreate(	
		"AutoReload",                 
		mainAUTO_RELOAD_TIMER_PERIOD, 
		pdTRUE,                       
		0,                            
		vAutoLoadTimerFunc         
	);	
	if (xAutoReloadTimer){
		xTimerStart(xAutoReloadTimer, 0);
	}
   
	
	//wifi连接，连接MQTT 接收wifi传递过来的消息
 	osThreadDef(wifiTask, WifiMqttClientTask, osPriorityNormal, 0, 128*5);    //WifiMqttClientTask 线程大概需要占用356字节缓存    之前15
  wifiTaskHandle = osThreadCreate(osThread(wifiTask), NULL);
//	//4G连接，连接MQTT 接收4G传递过来的消息	
	osThreadDef(EC4gTask, EC200MqttClientTask, osPriorityNormal, 0, 128*5); 	//EC200MqttClientTask 线程大概需要占用328字节缓存    之前20
  ec4gTaskHandle = osThreadCreate(osThread(EC4gTask), NULL);
	//业务核心线程，调度处理采集 理疗方案切换等
	osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128*4);   //StartDefaultTask 线程大概需要占用190字节缓存       之前8                   
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);
	
//	/*传感器数据采集 温湿度 电池电量等*/
	osThreadDef(startLoopTask, StartLoopTask, osPriorityNormal, 0, 128*4);      //StartLoopTask 线程大概需要占用216字节缓存            之前4
  startLoopTaskHandle = osThreadCreate(osThread(startLoopTask), NULL);
	
	/*串口屏幕数据，串口数据交互*/
  osThreadDef(consoleTask, Console_Task, osPriorityNormal, 0, 128*3);       //Console_Task 线程大概需要占用85字节缓存            之前4  
  consoleTaskHandle = osThreadCreate(osThread(consoleTask), NULL);
	
	/**RJ45网络时 用于接收MQTT传输过来的数据**/
	osThreadDef(mqttRecvClientTask, StartMqttClientTask, osPriorityNormal, 0, 128*15);  //StartMqttClientTask 线程大概需要占用1228字节缓存  之前15
  mqttClientTaskHandle = osThreadCreate(osThread(mqttRecvClientTask), NULL);
	
 	/**RJ45时监测网卡连接状态**/
	osThreadDef(ethernetTask, ethernet_link_thread, osPriorityNormal, 0, 128*3);  //ethernet_link_thread 线程大概需要占用79字节缓存  之前2
  ethernetTaskTaskHandle = osThreadCreate(osThread(ethernetTask), NULL);
 
  /**mqtt 发送数据 发送消息确认任务*/
	osThreadDef(mqttSendTask, StartMqttSendTask, osPriorityNormal, 0, 128*16);  //StartMqttSendTask 线程大概需要占用1270字节缓存   之前10
  mqttSendTaskHandle = osThreadCreate(osThread(mqttSendTask), NULL);
 	/*心跳包发送任务*/
//	osThreadDef(sendheartTask, StartSendheartTask, osPriorityNormal, 0, 128*4); //StartSendheartTask 线程大概需要占用236字节缓存   之前4
//  sendheartTaskHandle = osThreadCreate(osThread(sendheartTask), NULL); 

	

  osKernelStart();
  while (1)
  {

  }

}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_CKPER;
  PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}





/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */

/* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();
  /** Initializes and configures the Region and the memory to be protected 
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x30040000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /** Initializes and configures the Region and the memory to be protected 
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30044000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV64;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;//ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;//ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_16CYCLES_5 ;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}


/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */
  /** Common config
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
  hadc3.Init.Resolution = ADC_RESOLUTION_16B;
  hadc3.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.ContinuousConvMode = ENABLE;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_ONESHOT;//ADC_CONVERSIONDATA_DR;
  hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc3.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_16CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}
#if 0
/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}
#endif
/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x307075B1;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x307075B1;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x307075B1;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;//SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI2_Init(void)
{
	hspi2.Instance               = SPI2;                   
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;    
	hspi2.Init.Direction         = SPI_DIRECTION_2LINES;   
	hspi2.Init.CLKPhase          = SPI_PHASE_1EDGE;            
	hspi2.Init.CLKPolarity       = SPI_POLARITY_LOW;          
	hspi2.Init.DataSize          = SPI_DATASIZE_8BIT;     
	hspi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;      
	hspi2.Init.TIMode            = SPI_TIMODE_DISABLE;     
	hspi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE; 
	hspi2.Init.CRCPolynomial     = 7;                     
	hspi2.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;   
	hspi2.Init.NSS               = SPI_NSS_SOFT;             
	hspi2.Init.FifoThreshold     = SPI_FIFO_THRESHOLD_16DATA; 
	hspi2.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;   
	hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
	hspi2.Init.Mode 			 	= SPI_MODE_MASTER;       
	if (HAL_SPI_DeInit(&hspi2) != HAL_OK)
	{
		Error_Handler();
	}
  
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }

}


/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_HARD_OUTPUT;      								//硬件控制
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4 ;  //速度160M/4=40M速度      4分频刚好控制  50M的dac8831
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 0x0;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;  					/* 使能脉冲输出 */
  hspi3.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;    			/* 低电平有效 */
  hspi3.Init.FifoThreshold = SPI_FIFO_THRESHOLD_07DATA;   /* 设置FIFO大小是一个数据项 */
  hspi3.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi3.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi3.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;       							  /* MSS, 插入到NSS有效边沿和第一个数据开始之间的额外延迟，单位SPI时钟周期个数 */
  hspi3.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;  			  /* MIDI, 两个连续数据帧之间插入的最小时间延迟，单位SPI时钟周期个数 */
  hspi3.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi3.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  									  /* 禁止SPI后，SPI相关引脚保持当前状态 */ 
  hspi3.Init.IOSwap = SPI_IO_SWAP_DISABLE;
    
    if (HAL_SPI_DeInit(&hspi3) != HAL_OK)
	{
        Error_Handler();
	}
     if (HAL_SPI_Init(&hspi3) != HAL_OK)
    {
       Error_Handler();
    }	 


  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}
/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 0x0;
  hspi5.Init.NSSPMode =SPI_NSS_PULSE_DISABLE;// SPI_NSS_PULSE_ENABLE;
  hspi5.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi5.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi5.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi5.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi5.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi5.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi5.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi5.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * @brief UART7 Initialization Function
  * @param None
  * @retval None
  */
void MX_UART7_Init(void)
{

  /* USER CODE BEGIN UART7_Init 0 */

  /* USER CODE END UART7_Init 0 */

  /* USER CODE BEGIN UART7_Init 1 */

  /* USER CODE END UART7_Init 1 */
  huart7.Instance = UART7;
  huart7.Init.BaudRate = 115200;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  huart7.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart7.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart7.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart7, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart7, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART7_Init 2 */

  /* USER CODE END UART7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart6, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart6, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}



/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
void MX_DMA_Init(void)
{
   /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
	
	 /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
   
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0, 0);   //WIFI
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);   //4G
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);   //SPI3
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);



}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
	
	/*spi5 cs PF6*/
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOF, IO7_Pin, GPIO_PIN_RESET);
	
  /*Configure GPIO pin Output Level */
//  HAL_GPIO_WritePin(GPIOH, RMII_INT_Pin|RMII_RES_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, WIFI_RESET_Pin|WIFI_EN_Pin|LED1_Pin
                          |LED2_Pin|LED3_Pin|LED4_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, IO1_Pin|IO2_Pin|IO3_Pin|IO4_Pin|IO5_Pin|IO6_Pin, GPIO_PIN_RESET);
  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, IO8_Pin|IO9_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOH, IO10_Pin|IO11_Pin|IO12_Pin, GPIO_PIN_RESET);
//  HAL_GPIO_WritePin(GPIOI, IO13_Pin|IO14_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RS485_EN2_GPIO_Port, RS485_EN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET);
  
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);



  /*Configure GPIO pins : RMII_INT_Pin RMII_RES_Pin */
//  GPIO_InitStruct.Pin = RMII_INT_Pin|RMII_RES_Pin;
//  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
//  GPIO_InitStruct.Pull = GPIO_PULLUP;
//  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
//  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : WIFI_RESET_Pin WIFI_EN_Pin LED1_Pin LED2_Pin
                           LED3_Pin LED4_Pin */
  GPIO_InitStruct.Pin = WIFI_RESET_Pin|WIFI_EN_Pin|LED1_Pin|LED2_Pin
                          |LED3_Pin|LED4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : SOCKET_LINK_Pin WIFI_LINK_Pin */
  GPIO_InitStruct.Pin = SOCKET_LINK_Pin|WIFI_LINK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = IO1_Pin|IO2_Pin|IO4_Pin|IO3_Pin|IO5_Pin|IO6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = IO7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = IO8_Pin|IO9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = IO10_Pin|IO11_Pin|IO12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = IO13_Pin|IO14_Pin; 
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);         // 初始化默认拉低 拉高亮灯
   

  
  /*Configure GPIO pin : RS485_EN2_Pin */
  GPIO_InitStruct.Pin = RS485_EN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RS485_EN2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IN2_Pin */
  GPIO_InitStruct.Pin = IN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IN2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IN1_Pin */
  GPIO_InitStruct.Pin = IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IN1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : IN3_Pin */
  GPIO_InitStruct.Pin = IN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IN3_GPIO_Port, &GPIO_InitStruct);	 
	
	/*Configure GPIO pin : IN4_Pin */
  GPIO_InitStruct.Pin = IN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(IN4_GPIO_Port, &GPIO_InitStruct);	

  /*Configure GPIO pin : RS485_EN_Pin */
  GPIO_InitStruct.Pin = RS485_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(RS485_EN_GPIO_Port, &GPIO_InitStruct);
	
	/*spi5 cs PF6*/
	  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct); 
  //2023/3/14 by yls add	
	GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
//音频切换    默认
	GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
//12v风扇
	GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOI, GPIO_PIN_5, GPIO_PIN_SET);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
		uint8_t gbk0[8]={0,};
	uint8_t gbkNum0[13]={0,};
  /* USER CODE BEGIN 5 */

	/*ADC校准**/
	if(HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED)!=HAL_OK)
	{
		Error_Handler();
	}
	
	if(HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED)!=HAL_OK)
	{
		Error_Handler();
	}

	osDelay(10);	
	
	/*开始DMA接收ADC，DMA满产生中断*/
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)gADC1_DMA_BUF,N_NUM* AD1_NUM);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t *)gADC3_DMA_BUF,N_NUM* AD3_NUM);
	
	/*获取内部参考电压*/
	osDelay(50);	
	gREFVOL_VAL=__LL_ADC_CALC_VREFANALOG_VOLTAGE(gADC3_VALUE[0],LL_ADC_RESOLUTION_16B);
	if(gREFVOL_VAL>3500||gREFVOL_VAL<3000)
	{
		gREFVOL_VAL=3300;
	}
	

	
  /* Infinite loop */
	for(;;)
  {
		defaultTask_stack=uxTaskGetStackHighWaterMark(defaultTaskHandle);
		osDelay(10);
		if(gDeviceParam.devLock==true)
		{
			continue;
		}
		if(gGlobalData.curWorkMode != gGlobalData.oldWorkMode)
		{
			if(gGlobalData.curWorkState == WORK_START)
			{
				gGlobalData.oldWorkMode = gGlobalData.curWorkMode;
				gGlobalData.current_treatNums = 0;                               
				if(gGlobalData.curWorkMode == 1)														//生物电理疗
				{
					gGlobalData.oldWorkMode = gGlobalData.curWorkMode;
					gGlobalData.currentUnitNum=0;
					gGlobalData.channelPos = 0;
					set_sampleMode(MODE_ZL);
					//-----------------------波形发生-----------------//
					Wave_select(gGlobalData.useWorkArg[0].waveTreat, ch1buf);//波形选择
					Dac8831_Set_Amp(gGlobalData.useWorkArg[0].level, ch1buf);//幅值改变
					DAC8831_Set_Data_Dma(ch1buf,sizeof(ch1buf)/2,gGlobalData.useWorkArg[0].freqTreat);					//定时器开启产生波形
					
					osDelay(100);
					send_LcdOutStatus(1);                                    //理疗绿灯 理疗结束红灯
					osDelay(100);
					

					osDelay(100);
					if(level <= 60)
					//	Send_LcdVoltage(5.513*level);	//发送显示电压 适用于高功率版本
							Send_LcdVoltage(5.84*level);	
				}
				else																											 //穴位疼痛理疗
				{
				
					send_wave_xwtt(gGlobalData.useWorkArg[0].waveTreat);     //发送波形
					osDelay(200);
					send_ecA_xwtt(gGlobalData.useWorkArg[0].aPower);         //发送通道A电流
					osDelay(200);
					send_ecB_xwtt(gGlobalData.useWorkArg[0].bPower);         //发送通道B电流
					osDelay(200);
//					send_startORstop_xwtt(1);
					send_LcdOutStatus(1);                                    //理疗开始绿灯

			}
		}

		}
		if(gGlobalData.curWorkMode == 1)
		{
			//SWD
			switch(gGlobalData.curWorkState)
			{
				case WORK_START:
							if(gGlobalData.channelPos>=gGlobalData.useWorkArg[0].chanelNum-1)																			//是否是最后一个方案
							{
							  if((uint32_t)((gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat + 
									gGlobalData.useWorkArg[gGlobalData.current_treatNums].waitTime)*1000-channelTime)<=120000)				//说明是最后120秒 开始降低档位
								{
									if(channelTime-countTime>=4000&&level>10){
									 level-=1;
								  	Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);						//波形选择
										Dac8831_Set_Amp(level, ch1buf);						//赋值改变
										
										DAC8831_Set_Data_Dma(ch1buf,sizeof(ch1buf)/2,gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat);					//定时器开启产生波形
										
										countTime=channelTime;
										send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
										level,
										(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);												//修改屏幕显示的治疗方案
										osDelay(100);
										if(level <= 60)
//											Send_LcdVoltage(5.513*level);	//发送显示电压 适用于高功率版本
											Send_LcdVoltage(5.84*level);	
									}
								}
							}

							//判断治疗时间是否到，且不是最后一个通道，切换下一通道,并更新通道位置、重置包位置计数、时间计数
							if(channelTime>=(uint32_t)(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat+
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].waitTime)*1000)
							{
									if(gGlobalData.channelPos>=gGlobalData.useWorkArg[0].chanelNum-1)
								{
									level=0;//设置成默认档位为1 2023.04.04 kardos
									HAL_TIM_Base_DeInit(&htim12);   //不产生波形
									send_LcdWorkStatus(6);																					//修改设备工作状态为：穴位理疗完成
									gGlobalData.curWorkMode=WORK_MODE_WT;
									gGlobalData.oldWorkMode=gGlobalData.curWorkMode;
									gGlobalData.curWorkState=WORK_STOP;
									gGlobalData.current_treatNums =0;
									gGlobalData.channelPos =0;
                  set_sampleMode(MODE_CLOSE);
									HAL_PCA9554_outputAll(0);
									HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET); 						//运行红灯 set灭 reset亮
									HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET); 							//运行绿灯 set灭 reset亮
									send_LcdSync(0);
                  gGlobalData.cur_heart_state=LEISURE;
									Send_Fix_Ack(24,STATUS_OK,"OK");
									osDelay(100);	
									send_LcdOutStatus(0);                                           //理疗结束红灯
									osDelay(100);	
									send_treatSel(0,0,0);     																			//SWD治疗方案全置成0
									osDelay(100);
									Send_LcdVoltage(0);
									osDelay(100);
									send_visitNumber(gbkNum0);
									osDelay(100);
									send_visitName(gbk0,8);
									osDelay(100);
									send_visitAge(0);
									osDelay(100);
									send_visitSex(0);
									osDelay(100);	
									send_LcdWorkStatus(1);																					//修改设备工作状态为：设备待机状态
									osDelay(100);	
									send_lcdPage(0);         																				//返回扫码界面					
								}
								else{
									gGlobalData.channelPos++;
									//先降低波形强度
							//	Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);
									Dac8831_Set_Amp(5, ch1buf);
									
									osDelay(200);	
								//切换治疗波形
									gGlobalData.current_treatNums++;		
									level=gGlobalData.useWorkArg[gGlobalData.current_treatNums].level;								
									send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
																level,
																(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);	
									osDelay(200);
									Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);
									Dac8831_Set_Amp(level, ch1buf);
									DAC8831_Set_Data_Dma(ch1buf,sizeof(ch1buf)/2,gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat);					//定时器开启产生波形			
							
	
//								Send_LcdVoltage(5.513*level);	//适用于高功率版本
									Send_LcdVoltage(5.84*level);	//适用于低功率版本
									osDelay(200);	
									channelTime=0;

								}
								//
							}
				break;
			case WORK_PAUSE:	  	 
				break;
			case WORK_STOP:	     
					channelTime=0;
				break;
			default:
					break;
			}
		}
		else if(gGlobalData.curWorkMode == 2)
		{
			//XWTT
			switch(gGlobalData.curWorkState)
			{
				case WORK_START:
				        
							if(gGlobalData.channelPos>=gGlobalData.useWorkArg[0].chanelNum-1)																			//是否是最后一个方案
							{
							  if((uint32_t)((gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat + 
									gGlobalData.useWorkArg[gGlobalData.current_treatNums].waitTime)*1000-channelTime)<=120000)				//说明是最后120秒 开始降低档位
								 {
									if(channelTime-countTime>=4000){
										if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower>6){
										gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower-=2;
										send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
										osDelay(200);	
										send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
																	(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
									}
										if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower>6)
										{
											gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower-=2;
											send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
												osDelay(200);	
										send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
																	(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
										}
										countTime=channelTime;
									}
								}
							}	
				
				
							//判断治疗时间是否到，且不是最后一个通道，切换下一通道,并更新通道位置、重置包位置计数、时间计数
							if(channelTime>=(uint32_t)(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat+
							gGlobalData.useWorkArg[gGlobalData.current_treatNums].waitTime)*1000)
							{
								if(gGlobalData.channelPos>=gGlobalData.useWorkArg[0].chanelNum-1)
								{
									send_startORstop_xwtt(0);//关闭输出
									osDelay(200);	
									gGlobalData.curWorkMode=WORK_MODE_WT;
									gGlobalData.oldWorkMode=gGlobalData.curWorkMode;
									gGlobalData.curWorkState=WORK_STOP;
									gGlobalData.current_treatNums =0;
									gGlobalData.channelPos =0;
									set_sampleMode(MODE_CLOSE);
									HAL_PCA9554_outputAll(0);
									HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET); 						//运行红灯 set灭 reset亮
									HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET); 							//运行绿灯 set灭 reset亮
                  gGlobalData.cur_heart_state=LEISURE;
									send_LcdSync(0);
									Send_Fix_Ack(24,STATUS_OK,"OK");
									osDelay(100);	
									send_treatSel_Xwtt(0,0,0,0);   																	//XWTT始治疗方案全置成0
									osDelay(100);	
									send_LcdOutStatus(0);                                           //理疗结束红灯
									osDelay(100);	
									send_lcdPage(0);      																				  //返回扫码界面	
								
								}
								else{
								send_ecA_xwtt(5);
								osDelay(100);
								send_ecB_xwtt(5);														
					      osDelay(100);
								send_startORstop_xwtt(0);//先停止波形输出
								gGlobalData.channelPos++;

								//切换治疗波形
								gGlobalData.current_treatNums++;

								send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
																	(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
																		gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
								osDelay(500);
								send_startORstop_xwtt(1);//开始波形输出
                osDelay(100);
								send_wave_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat);
								osDelay(100);
								send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
								osDelay(100);
								send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
		
								channelTime=0;
								}							
							
							}
				break;
			case WORK_PAUSE:	  
				break;
			case WORK_STOP:	    
					channelTime=0;
				break;
			default:
					break;
			}	
		}
	}//for
  /* USER CODE END 5 */
}

void send_wave_xwtt(uint8_t num)
{
	uint8_t buf[20]={0x00};
	uint8_t buf_crc[20]={0x00};
	int i;
	uint16_t len,crc16;
	len=11;
	memset(buf,0,sizeof(buf));
	buf[0]=0x3A;
	buf[1]=0;
	buf[2]=0x05;	
	buf[3]=0x04;

	buf[4]=0x00;
	buf[5]=0x02;
	buf[6]=0x01;
	buf[7]=num;
	for(i=0;i<len-5;i++)
	buf_crc[i]=buf[i+2];
	
	crc16=crc16_Xwtt(buf_crc,len-5);
	buf[9]=(uint8_t)crc16;
	buf[8]=crc16>>8;

	buf[10]=0x0D;
	HAL_UART_Transmit_Xwtt(buf,len,0xFFFF);	
}

void send_ecA_xwtt(uint8_t num)
{
	uint8_t buf[20]={0x00};
	uint8_t buf_crc[20]={0x00};
	int i;
//	short int tmp=0;
	uint16_t len,crc16;
	len=11;
	memset(buf,0,sizeof(buf));
	buf[0]=0x3A;
	buf[1]=0;
	buf[2]=0x05;	
	buf[3]=0x04;

	buf[4]=0x00;
	buf[5]=0x04;
	buf[6]=0x01;
	buf[7]=num;
	for(i=0;i<len-5;i++)
	buf_crc[i]=buf[i+2];
	
	crc16=crc16_Xwtt(buf_crc,len-5);
	buf[9]=(uint8_t)crc16;
	buf[8]=crc16>>8;
	buf[10]=0x0D;
	HAL_UART_Transmit_Xwtt(buf,len,0xFFFF);	
}
void send_ecB_xwtt(uint8_t num)
{
	uint8_t buf[20]={0x00};
	uint8_t buf_crc[20]={0x00};
	int i;
//	short int tmp=0;
	uint16_t len,crc16;
	len=11;
	memset(buf,0,sizeof(buf));
	buf[0]=0x3A;
	buf[1]=0;
	buf[2]=0x05;	
	buf[3]=0x04;
	buf[4]=0x00;
	buf[5]=0x06;
	buf[6]=0x01;
	buf[7]=num;
	for(i=0;i<len-5;i++)
	buf_crc[i]=buf[i+2];
	
	crc16=crc16_Xwtt(buf_crc,len-5);
	buf[9]=(uint8_t)crc16;
	buf[8]=crc16>>8;
	buf[10]=0x0D;
	HAL_UART_Transmit_Xwtt(buf,len,0xFFFF);	
}

//1启动 0停止
void send_startORstop_xwtt(uint8_t num)
{
	uint8_t buf[20]={0x00};
	uint8_t buf_crc[20]={0x00};
	int i;
//	short int tmp=0;
	uint16_t len,crc16;
	len=11;
	memset(buf,0,sizeof(buf));
	buf[0]=0x3A;
	buf[1]=0;
	buf[2]=0x05;	
	buf[3]=0x04;
	buf[4]=0x00;
	buf[5]=0x00;
	buf[6]=0x01;
	buf[7]=num;
	for(i=0;i<len-5;i++)
	buf_crc[i]=buf[i+2];
	
	crc16=crc16_Xwtt(buf_crc,len-5);
	buf[9]=(uint8_t)crc16;
	buf[8]=crc16>>8;
	buf[10]=0x0D;
	HAL_UART_Transmit_Xwtt(buf,len,0xFFFF);	
}



void StartLoopTask(void const * argument)
{
  /* USER CODE BEGIN StartLoopTask */
  /* Infinite loop */
	HAL_PCA9554_outputAll(0);
	HAL_PCA9554_init();
	
	startLoopTask(argument);
  /* USER CODE END StartLoopTask */
}


void Console_Task(void const * pvParameters)
{
	//int i;
	gGlobalData.ResetDevidcnt=0;
//	uint8_t *RecSen;
//	RecSen = (uint8_t*)calloc(sizeof(TxTimeStamp_Trans*DWT_TIME_UNITS)+14,sizeof(uint8_t));
//	HAL_UART_Receive_DMA(&huart7, RecCom7,COM7_REC_MAX);
		HAL_UART_Receive_DMA(&huart7, RecSen,30);
	__HAL_UART_CLEAR_IDLEFLAG(&huart7);
	__HAL_UART_ENABLE_IT(&huart7, UART_IT_IDLE);

	for(;;)
	{
//		Console_Task_stack=uxTaskGetStackHighWaterMark(consoleTaskHandle);   //by 2023/5/9 yls 屏蔽
//		osDelay(50);
		xSemaphoreTake(ConsoleReceive_Semaphore,portMAX_DELAY);

		 if(RecSen[0] == 0x5A && RecSen[1] == 0xA5)
		 {
				if(RecSen[3] == 0x83) //wifi设置相关
				{
					if(RecSen[4] == 0x18 && RecSen[5] == 0x20)
					{
						//存wifi名称
						memset(gDeviceParam.wifiArg.WifiName,0,sizeof(gDeviceParam.wifiArg.WifiName));
						for(uint8_t i = 7;i < sizeof(gDeviceParam.wifiArg.WifiName); ++i)
						{
							if(RecSen[i] == 0xFF)
								break;
							gDeviceParam.wifiArg.WifiName[i-7] = RecSen[i];
						}
					}
					if(RecSen[4] == 0x18 && RecSen[5] == 0x30)
					{
						//存wifi密码
						memset(gDeviceParam.wifiArg.WifiPwd,0,sizeof(gDeviceParam.wifiArg.WifiPwd));
						for(uint8_t i = 7;i < sizeof(gDeviceParam.wifiArg.WifiPwd); ++i)
						{
							if(RecSen[i] == 0xFF)
								break;
							gDeviceParam.wifiArg.WifiPwd[i-7] = RecSen[i];
						}
					}
					if(RecSen[4] == 0x19 && RecSen[5] == 0x10)  //屏幕提示框按键后便不可用
					{
							Send_Text_SetButton(0,0);	//文本框按键默认不可用
							osDelay(50);
							Send_Text_SetButton(1,0);
							osDelay(50);
							Send_Text_SetButton(3,0); 
							osDelay(50);
					}
				}
			 if(RecSen[2] == 0x06){
         switch(RecSen[8])
					{ 
						case 0x01: 							//开始
							if(gGlobalData.curWorkMode==1){
								
								//如果是生物电理疗 再次开启的时候将电压值显示最新的值
							if(gGlobalData.curWorkState==WORK_PAUSE)
								 set_sampleMode(MODE_ZL);
								if(level <= 60)
								{
								 Send_LcdVoltage(5.84*level);//适用于低功率放大板子
								}
								else 
									Send_LcdVoltage(5.84*60);	
							}
							if(gGlobalData.curWorkMode==2){															//疼痛理疗 		//2023.02.21 kardos								
								send_startORstop_xwtt(1);																	//启动指令
								osDelay(200);
								if(gGlobalData.curWorkState==WORK_PAUSE){//2023.02.24 kardos  先设置波形 再设置ab两个通道的电流值
												send_wave_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat); //发送波形
												osDelay(200);
												send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
												osDelay(200);
												send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
												osDelay(200);
										}
								}
								gGlobalData.cur_heart_state=WORKING;
								gGlobalData.curWorkState=WORK_START;
								osDelay(20);
								send_LcdSync(1); 																						//屏幕运行灯绿灯
								osDelay(200);
								send_LcdWorkStatus(3);																			//穴位理疗中   
								osDelay(200);	
								send_LcdOutStatus(1);//设置输出为绿灯 kardos 2023.03.05		
								osDelay(200);
								cnt_heartbag = 0;																						//发送心跳清空心跳计数器
								gGlobalData.heartbag_flage = 1;                            
							break;
						case 0x02:	//停止
							gGlobalData.cur_heart_state=PAUSE;
							gGlobalData.curWorkState=WORK_PAUSE;
							if(gGlobalData.curWorkMode==2){		//2023.02.21 kardos
									send_startORstop_xwtt(0);
							}
							if(gGlobalData.curWorkMode==1){	//如果是生物电理疗 暂停时电压给0V
									Send_LcdVoltage(0);
							}
							osDelay(20);					
							send_LcdSync(0);  
							set_sampleMode(MODE_CLOSE);																	//设置模式
							osDelay(200);
							send_LcdWorkStatus(5);																			//设备暂停状态
							osDelay(200);	
							send_LcdOutStatus(0);//设置输出为红灯 kardos 2023.03.05
							cnt_heartbag = 0;																						//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;                               
							break;
						case 0x03: 																									  //复位
              Send_Fix_Ack(100,STATUS_OK,"OK");							
							gGlobalData.cur_heart_state=LEISURE;
							gGlobalData.curWorkState=WORK_STOP; 
							gGlobalData.Alltime=0;
							if(gGlobalData.oldWorkMode==1){
                                send_treatSel(0,0,0);   																  //swd治疗方案全置成0
								Send_LcdVoltage(0);//电压值设置为0
							}
							if(gGlobalData.oldWorkMode==2){															//疼痛理疗
								send_startORstop_xwtt(0);
								osDelay(100);
								send_treatSel_Xwtt(0,0,0,0);  
							}
							osDelay(100);	
							send_LcdOutStatus(0);//设置输出为红灯 kardos 2023.03.05
							osDelay(100);
							send_LcdSync(0);  					
							set_sampleMode(MODE_CLOSE);																	//设置模式
							gGlobalData.curWorkMode=WORK_MODE_WT;
							gGlobalData.oldWorkMode=gGlobalData.curWorkMode;
							gGlobalData.curWorkState=WORK_STOP;
							gGlobalData.current_treatNums =0;
							gGlobalData.channelPos =0;
							//set_sampleMode(MODE_CLOSE);
							HAL_PCA9554_outputAll(0);
							HAL_GPIO_WritePin(GPIOD,GPIO_PIN_1,GPIO_PIN_RESET); 				//运行红灯 set灭 reset亮
							HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET); 					//运行绿灯 set灭 reset亮
							osDelay(100);
							send_LcdWorkStatus(1);																			//设备待机状态       
							osDelay(100);
							send_lcdPage(0);
							osDelay(100);
							Send_ComMusic(3);
							osDelay(100);
							level=0;//复位时 给0  2023.04.04 kardos
							HAL_TIM_Base_DeInit(&htim12);   //不产生波形
							break;
						//SWD  +5
						case 0x0b:

							if(level <= 60)
							{
								level+=1;
								send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
											level,
											(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);
								osDelay(200);
								if(level <= 60)
								{
									Send_LcdVoltage(5.84*level);//适用于低功率放大板子
								}
								
								Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);//波形选择
								Dac8831_Set_Amp(level, ch1buf);//幅值改变				
							}		
							Dac_level_CTL(1);   //档位改变后波形产生
							osDelay(500);//延时 防止点击过快
							cnt_heartbag = 0;																						//发送心跳清空心跳计数器
              gGlobalData.heartbag_flage = 1;                             
							break;
						//SWD  -5
						case 0x0c:

							if(level >= 5)
							{
								level-=1;
								send_treatSel(gGlobalData.useWorkArg[gGlobalData.current_treatNums].freqTreat,
														level,
														(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60);
							  osDelay(200);
								if(level <= 60)
								{
									Send_LcdVoltage(5.84*level);//适用于低功率放大板子
								}
								Wave_select(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat, ch1buf);//波形选择
								Dac8831_Set_Amp(level, ch1buf);//幅值改变																			
							}
							Dac_level_CTL(1);   //档位改变后波形产生
							osDelay(500);//延时 防止点击过快
							cnt_heartbag = 0;															//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;                            
							break;
						//A通道  +5
						case 0x05:
							if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower <= 250)	
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower+=2;
							send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
																	gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
																	(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
																	gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							osDelay(200);
							send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
							osDelay(500);//延时 防止点击过快	
							cnt_heartbag = 0;															//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;                        
							break;
						//A通道  -5
						case 0x06:
							if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower >6)		
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower-=2;
							send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
										gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
										(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
										gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							//send_wave_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat);
							osDelay(200);
							send_ecA_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower);
							osDelay(500);//延时 防止点击过快
							cnt_heartbag = 0;															//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;
              break;
						//B通道  +5
						case 0x07:
							if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower <= 250)		
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower+=2;
							send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
									gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
								(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
									gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							osDelay(200);
							send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							osDelay(500);//延时 防止点击过快	
							cnt_heartbag = 0;															//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;
             break;
						//B通道  -5
						case 0x08:
							if(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower >6)														
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower-=2;
							send_treatSel_Xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].waveTreat,
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].aPower,
								(gGlobalData.useWorkArg[gGlobalData.current_treatNums].timeTreat)/60,
								gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							osDelay(200);
							send_ecB_xwtt(gGlobalData.useWorkArg[gGlobalData.current_treatNums].bPower);
							osDelay(500);//延时 防止点击过快	
							cnt_heartbag = 0;															//发送心跳清空心跳计数器
							gGlobalData.heartbag_flage = 1;
             break;																			
						case 0x0a:
							gGlobalData.ResetDevidcnt++;
							break;
						case 0x50://wifi更改确认指令
							gGlobalData.Wifi_set = true;
							Save_Parameter();
							printf("wifi_go_to_con\r\n");
							//if(gGlobalData.curWorkMode) swd的wifi设置指令，设置完后谁在运行就返回谁
							if(gGlobalData.curWorkMode == 1){
								send_lcdPage(1);
							}
							else if (gGlobalData.curWorkMode == 2){
								send_lcdPage(3);
							}
							else
							send_lcdPage(0);
							break;
						default: break;		
					}
				}
				//memset (RecSen,0,sizeof(RecSen));
			}
			memset (RecSen,0,sizeof(RecSen));
//			vTaskSuspendAll();
			
//			taskENTER_CRITICAL();
//			if(HAL_UART_Receive_DMA(&huart7, RecSen,9) != HAL_OK)
//			{
//				taskEXIT_CRITICAL();
//				osDelay(200);}
//			else
//				taskEXIT_CRITICAL();
			HAL_UART_Receive_DMA(&huart7, RecSen,30);
//			xTaskResumeAll();
//		HAL_UART_Receive_DMA(&huart7, RecCom7, COM7_REC_MAX);
	}
	
}
bool Get_ADC1_Hex(void)	
{
	//static uint16_t pt[AD1_NUM*10];
	int i,j ;
	uint16_t temp[N_NUM];
	for(i=0;i<AD1_NUM;i++)
	{
		for(j=0;j<N_NUM;j++) {temp[j] =  gADC1_DMA_BUF[j*AD1_NUM+i];}
		gGlobalData.currentNet=Gets_Average(temp,N_NUM);	
		*(gADC1_VALUE+i) = Gets_Average(temp,N_NUM);	
		*(gADC1_VALUE_F+i) = ((*(gADC1_VALUE+i))*gREFVOL_VAL)/65535;   //gREFVOL_VAL
	}
	return true;
}

/*out*/
bool Get_ADC3_Hex(void)
{
	//static uint16_t pt[AD1_NUM*10];
	int i,j ;
	uint16_t temp[N_NUM];
	
	for(i=0;i<AD3_NUM;i++)
	{
		for(j=0;j<N_NUM;j++) {temp[j] =  gADC3_DMA_BUF[j*AD3_NUM+i];} 
		
		*(gADC3_VALUE+i) = Gets_Average(temp,N_NUM);
		*(gADC3_VALUE_F+i) = ((*(gADC3_VALUE+i))*gREFVOL_VAL)/65535;
		
	}
	return true;
}


/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM11 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM17) {
    HAL_IncTick();
  }

  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
//	static uint8_t val[2];
//	static uint16_t Vol_De;
//	static int16_t Vol;
	if(GPIO_Pin==GPIO_PIN_2)	//f2
	{	
	}
	
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}


#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart4, (uint8_t *)&ch, 1, 0xFFFF);         //不能重定向串口3 和 串口6 和串口7                         
  return ch;
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


//初始化独立看门狗
//prer:分频数:0~7(只有低3位有效!)
//rlr:自动重装载值,0~0XFFF.
//分频因子=4*2^prer.但最大值只能是256!
//rlr:重装载寄存器值:低11位有效.
//时间计算(大概):Tout=((4*2^prer)*rlr)/32 (ms).
void IWDG_Init(uint8_t prer,uint16_t rlr)
{
    IWDG_Handler.Instance=IWDG1;
    IWDG_Handler.Init.Prescaler=prer;    
    IWDG_Handler.Init.Reload=rlr;        
    IWDG_Handler.Init.Window=IWDG_WINDOW_DISABLE;
    HAL_IWDG_Init(&IWDG_Handler);    
}
    
//喂独立看门狗
void IWDG_Feed(void)
{   
    HAL_IWDG_Refresh(&IWDG_Handler);
}

void StartIwdgTask(void const *arg)
{
	 EventBits_t uxBits;
	 const TickType_t xTicksToWait = 2000 / portTICK_PERIOD_MS; /* 最大延迟2000ms */
	/*
	  开始执行启动任务主函数前使能独立看门狗。
	  设置LSI是64分频，下面函数参数范围0-0xFFF，分别代表最小值2ms和最大值6552ms

	   设置LSI是128分频，下面函数参数范围0-0xFFF，分别代表最小值4ms和最大值13104ms
	   Tout = prv/40 * rlv (s)
	  下面设置的是5s，如果5s内没有喂狗，系统复位。
	*/
	IWDG_Init(0x00000004 ,625*5);
	while(1)
	{
		/* 等待所有任务发来事件标志 */
		uxBits = xEventGroupWaitBits(xCreatedEventGroup, /* 事件标志组句柄 */
									 TASK_BIT_ALL,       /* 等待TASK_BIT_ALL被设置 */
									 pdTRUE,             /* 退出前TASK_BIT_ALL被清除，这里是TASK_BIT_ALL都被设置才表示“退出”*/
									 pdTRUE,             /* 设置为pdTRUE表示等待TASK_BIT_ALL都被设置*/
									 xTicksToWait);      /* 等待延迟时间 */
		if((uxBits & TASK_BIT_ALL) == TASK_BIT_ALL)//判断各个任务是否执行
		{
			IWDG_Feed();
		}
	}
		
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
