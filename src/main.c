#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_i2c.h"
#include "delay.h"
#include "USART.h" 
#include "I2C.h"
#include "LiquidCrystal_I2C.h"

void USART1_Init(void); //Объявление функции инициализации периферии
void Usart1_Send_symbol(uint8_t); //Объявление функции передачи символа
void Usart1_Send_String(char* str); //Объявление функции передачи строки


u8 USART1_RX_CNT=0;
uint16_t  USART_RX_BUF_2[]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
uint16_t  CO2TxBuffer[9]={0xFE,0x04,0x00,0x03,0x00,0x01,0xD5,0xC5};
int  CO2Data;


void i2s(int num,char* str,int radix);

void CO2_Tx()
{
        int i;
  for(i = 0; i < 8; i++)
   {
      USART_ClearFlag(USART1, USART_FLAG_TC);
      USART_SendData(USART1, CO2TxBuffer[i]);  
      while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);  
   }
}

void CheckSum()
{
      char i,checksum=0;
	    char str[20]={0};
	    char strrec[20]={0};
			
      for( i = 1; i < 9; i++)  
      {  
          checksum += USART_RX_BUF_2[i];  
      }
       //checksum = 0xff - checksum;  
       //checksum += 1;                                         
       //if(checksum == USART_RX_BUF_2[6])        
           CO2Data = USART_RX_BUF_2[3] * 256 + USART_RX_BUF_2[4];
       //if(CO2Data > 9999)
           //CO2Data=0;                                                           
       //printf("CO2浓度= %d ppm   \n\a", CO2Data);
			 i2s(CO2Data, str, 10);
			 LCDI2C_setCursor(4,0);
			 LCDI2C_write_String(str);
			 LCDI2C_setCursor(8,0);
			 LCDI2C_write_String("ppm");
}

void i2s(int num,char* str,int radix)
{
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
	unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
	int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
	//获取要转换的整数的绝对值
	if(radix==10&&num<0)//要转换成十进制数并且是负数
	{
		unum=(unsigned)-num;//将num的绝对值赋给unum
		str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
	}
	else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
	//转换部分，注意转换后是逆序的
	do
	{
		str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
		unum/=radix;//unum去掉最后一位
 
	}while(unum);//直至unum为0退出循环
 
	str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
	//将顺序调整过来
	if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
	else k=0;//不是负数，全部都要调整
 
	char temp;//临时变量，交换两个值时用到
	for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
	{
		temp=str[j];//头部赋值给临时变量
		str[j]=str[i-1+k-j];//尾部赋值给头部
		str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
	}
 
	return ;//返回转换后的字符串
}

int main()
{
	char cnts[10]={0};
  USART1_Init();
  uint8_t data;

  LCDI2C_init(0x3F,16,2);

  LCDI2C_backlight();
  Delay(250);
  LCDI2C_noBacklight();
  Delay(250);

  LCDI2C_backlight();

  LCDI2C_clear();
  	
	LCDI2C_setCursor(0,0);
	LCDI2C_write_String("CO2:");
	
	while(1)
  {
    CO2_Tx();
		Delay(300);
		USART1_RX_CNT=0;
		USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
    CO2_Tx();
    Delay(1000);//        间隔一秒进行读取
    CheckSum();
		LCDI2C_setCursor(13,0);
		LCDI2C_write_String(".");
    Delay(3000);//        等待后下一次读取	
		LCDI2C_setCursor(13,0);
		LCDI2C_write_String(" ");
  }
}


void USART1_IRQHandler(void)                    //串口1中断服务程序
{
    u8 Res;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) 
    {
       Res =USART_ReceiveData(USART1);    //读取接收到的数据     
       if(USART1_RX_CNT<21)//对于接收指定长度的字符串
       {
           USART_RX_BUF_2[USART1_RX_CNT++]=Res;        //记录接收到的值    
       }
     }
    //溢出-如果发生溢出需要先读SR,再读DR寄存器则可清除不断入中断的问题
    if(USART_GetFlagStatus(USART1,USART_FLAG_ORE) == SET)
    {
        USART_ReceiveData(USART1);
        USART_ClearFlag(USART1,USART_FLAG_ORE);
    }
     USART_ClearFlag(USART1,USART_IT_RXNE); //一定要清除接收中断
     USART_ITConfig(USART1, USART_IT_TXE, DISABLE);

}

