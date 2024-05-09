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
  NVIC_InitStructure.NVIC_IRQChannel = RTCAlarm_IRQn;		//RTC�����ж�
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�1λ
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;	//��ռ���ȼ�0λ
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//ʹ�ܸ�ͨ���ж�
  NVIC_Init(&NVIC_InitStructure);		//����NVIC_InitStruct��ָ���Ĳ�����ʼ������NVIC�Ĵ���
//  
  //�����жϽӵ���17���ⲿ�ж�
  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure); 

}
	
u8 RTC_Init(void)
{
  //����ǲ��ǵ�һ������ʱ��
  u8 temp=0;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//ʹ��PWR��BKP����ʱ��   
  PWR_BackupAccessCmd(ENABLE);	//ʹ�ܺ󱸼Ĵ�������  
  if (BKP_ReadBackupRegister(BKP_DR1) != 0x5050)		//��ָ���ĺ󱸼Ĵ����ж�������:��������д���ָ�����ݲ����
  {
    BKP_DeInit();	//��λ�������� 	
    //RCC_LSEConfig(RCC_LSE_ON);	//�����ⲿ���پ���(LSE),ʹ��������پ���
    RCC_LSICmd(ENABLE);//�����ڲ����پ���LSI
    //while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)	//���ָ����RCC��־λ�������,�ȴ��ⲿ���پ������
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)	//���ָ����RCC��־λ�������,�ȴ��ڲ����پ������
    {
      temp++;
      delay_ms(10);
    }
    if(temp>=250)return 0;//��ʼ��ʱ��ʧ��,����������	    
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);   //����RTCʱ��(RTCCLK),ѡ��LSI��ΪRTCʱ��
    //RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//����RTCʱ��(RTCCLK),ѡ��LSE��ΪRTCʱ��    
    RCC_RTCCLKCmd(ENABLE);	//ʹ��RTCʱ��  
    RTC_WaitForLastTask();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������
    RTC_WaitForSynchro();		//�ȴ�RTC�Ĵ���ͬ��  
    
    RTC_WaitForLastTask();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������
    RTC_EnterConfigMode();/// ��������	
    RTC_SetPrescaler(32767); //����RTCԤ��Ƶ��ֵ
    RTC_WaitForLastTask();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������

    RTC_SetCounter(0);    //��ʼֵ�趨Ϊ0s
    RTC_WaitForLastTask();
    
    RTC_ExitConfigMode(); //�˳�����ģʽ  
    BKP_WriteBackupRegister(BKP_DR1, 0X5050);	//��ָ���ĺ󱸼Ĵ�����д���û���������
  }
  else//ϵͳ������ʱ
  {
    PWR_BackupAccessCmd(DISABLE);
    RTC_WaitForSynchro();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������
    RTC_WaitForLastTask();	//�ȴ����һ�ζ�RTC�Ĵ�����д�������
  }
  RTC_NVIC_Config();//RCT�жϷ�������		    				     
  return 1; //ok
}		 			


//RTC��ʱ����ʱ�������Լ�����
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
  RTC_SetAlarm(AlarmValue+RTC_GetCounter());    //����ֵ�趨ΪAlarmValue��
  RTC_WaitForLastTask();
  RTC_ExitConfigMode();   
  PWR_BackupAccessCmd(DISABLE);
  RTC_NVIC_Config();
  RTC_ITConfig(RTC_IT_ALR, ENABLE);             //ʹ��RTC�����ж�
}

//RTC�����ж�
void RTCAlarm_IRQHandler(void)
{
  EXTI_ClearITPendingBit(EXTI_Line17);
  RTC_ClearFlag(RTC_FLAG_ALR);
  SystemInit();//��Ҫ������ͣ���¶�����ʱ�ӹرգ����Ի�����Ҫ��������ʱ�ӣ���
  if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET)
  {
    PWR_ClearFlag(PWR_FLAG_WU);// ������ѱ�־
  }
  RTC_ITConfig(RTC_IT_ALR, DISABLE);            //ʧ��RTC�����ж�
}



 int main(void)
 {	 
  GPIO_InitTypeDef  GPIO_InitStructure;  
	 delay_init();	    	 //��ʱ������ʼ��	  
	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
		
		RTC_Init();
		RTC_Set_Alarm_Time(2);      //���û���ʱ��

	 
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
					RTC_Set_Alarm_Time(2);      //���û���ʱ��
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
