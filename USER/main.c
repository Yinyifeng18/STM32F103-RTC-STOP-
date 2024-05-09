#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	 
#include "wkup.h"
 
#define SLEEP_TIME  2

u8  sleepwork = 0;


static void RTC_NVIC_Config(void)
{	
  EXTI_InitTypeDef EXTI_InitStructure;  
  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;		//RTC闹钟中断
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//先占优先级1位
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	//先占优先级0位
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//使能该通道中断
  NVIC_Init(&NVIC_InitStructure);		//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
//  
  //闹钟中断接到第17线外部中断
  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure); 

}
	
u8 RTC_Init(void)
{
  //检查是不是第一次配置时钟
  u8 temp=0;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟   
  PWR_BackupAccessCmd(ENABLE);	//使能后备寄存器访问  
  if (BKP_ReadBackupRegister(BKP_DR1) != 0x5050)		//从指定的后备寄存器中读出数据:读出了与写入的指定数据不相乎
  {
    BKP_DeInit();	//复位备份区域 	
    //RCC_LSEConfig(RCC_LSE_ON);	//设置外部低速晶振(LSE),使用外设低速晶振
    RCC_LSICmd(ENABLE);//开启内部低速晶振LSI
    //while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	//检查指定的RCC标志位设置与否,等待外部低速晶振就绪
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)	//检查指定的RCC标志位设置与否,等待内部低速晶振就绪
    {
      temp++;
      delay_ms(10);
    }
    if(temp>=250)return 0;//初始化时钟失败,晶振有问题	    
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);   //设置RTC时钟(RTCCLK),选择LSI作为RTC时钟
    //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//设置RTC时钟(RTCCLK),选择LSE作为RTC时钟    
    RCC_RTCCLKCmd(ENABLE);	//使能RTC时钟  
    RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
    RTC_WaitForSynchro();		//等待RTC寄存器同步  
    
    RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
    RTC_EnterConfigMode();/// 允许配置	
    RTC_SetPrescaler(32767); //设置RTC预分频的值
    RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成

    RTC_SetCounter(0);    //初始值设定为0s
    RTC_WaitForLastTask();
    
    RTC_ExitConfigMode(); //退出配置模式  
    BKP_WriteBackupRegister(BKP_DR1, 0X5050);	//向指定的后备寄存器中写入用户程序数据
  }
  else//系统继续计时
  {
    PWR_BackupAccessCmd(DISABLE);
    RTC_WaitForSynchro();	//等待最近一次对RTC寄存器的写操作完成
    RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
  }
  RTC_NVIC_Config();//RCT中断分组设置		    				     
  return 1; //ok
}		 			


//RTC定时闹钟时间设置以及开启
void RTC_Set_Alarm_Time(uint32_t AlarmValue)
{
  if(AlarmValue == 0)
  {
     return;
  }
  EXTI_ClearITPendingBit(EXTI_Line17);
  RTC_ClearFlag(RTC_FLAG_ALR);
  PWR_BackupAccessCmd(ENABLE);
  RTC_EnterConfigMode();	
  RTC_SetAlarm(AlarmValue+RTC_GetCounter());    //闹钟值设定为AlarmValue秒
  RTC_WaitForLastTask();
  RTC_ExitConfigMode();   
  PWR_BackupAccessCmd(DISABLE);
  RTC_NVIC_Config();
  RTC_ITConfig(RTC_IT_ALR, ENABLE);             //使能RTC闹钟中断
}

//RTC闹钟中断
void RTCAlarm_IRQHandler(void)
{
  EXTI_ClearITPendingBit(EXTI_Line17);
  RTC_ClearFlag(RTC_FLAG_ALR);
  SystemInit();//重要，由于停机下对所有时钟关闭，所以唤醒需要重新配置时钟！！
  if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
  {
    PWR_ClearFlag(PWR_FLAG_WU);// 清除唤醒标志
  }
  RTC_ITConfig(RTC_IT_ALR, DISABLE);            //失能RTC闹钟中断
}



 int main(void)
 {	 
  GPIO_InitTypeDef  GPIO_InitStructure;  
	 delay_init();	    	 //延时函数初始化	  
	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
		
		RTC_Init();
		RTC_Set_Alarm_Time(2);      //设置唤醒时间

	 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,DISABLE);	
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,DISABLE);
		
  /* NVIC configuration */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
		
		
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz; 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  GPIO_Init(GPIOE, &GPIO_InitStructure);
		GPIO_Init(GPIOF, &GPIO_InitStructure);


	 RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR|RCC_APB1Periph_BKP, ENABLE);
  PWR_BackupAccessCmd(ENABLE);
  
  //PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI); 
		//PWR_EnterSTANDBYMode();

	
	 while(1)
	 {
			 if(sleepwork == 1)
				{			
					 delay_ms(1);
					 sleepwork = 0;
				}
				else 
				{
					RTC_Set_Alarm_Time(2);      //设置唤醒时间
     PWR_EnterSTOPMode(PWR_Regulator_LowPower,PWR_STOPEntry_WFI); 
	   	//PWR_EnterSTANDBYMode();
					
					 sleepwork = 1;
				}
	 }
}


void RTC_IRQHandler(void)
{

 
    if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
 
        RTC_ClearITPendingBit(RTC_IT_SEC);
 
    }
 
    if(RTC_GetITStatus(RTC_IT_ALR)!= RESET) {
 
        RTC_ClearITPendingBit(RTC_IT_ALR);
 
        PWR_BackupAccessCmd(ENABLE);
 
        RTC_WaitForLastTask();
 
        RTC_SetAlarm(RTC_GetCounter() + SLEEP_TIME);
 
        RTC_WaitForLastTask();
 
        PWR_BackupAccessCmd(DISABLE);
 
    }
 
    RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);
 
 
 
    if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET) {
 
        PWR_ClearFlag(PWR_FLAG_WU);
 
    }
 
    if(PWR_GetFlagStatus(PWR_FLAG_SB) != RESET) {
 
        PWR_ClearFlag(PWR_FLAG_SB);
 
    }
 
}
