#include "CT107D.h"
#include "STC15.h"
#include "iic.h"
#include "intrins.h"
#include "onewire.h"

//-----------------------------------------
// 通讯格式
// 访问数据
// char[8], 0xFF   0x00   0x00 0x00 0x00 0x00 0x00 0xOD 0x31
//          起始位 读写位 设备 数据 数据 数据 数据 停止
//                 0 读
//                 1 写
// 返回数据
// char[8], 0xEE   0x00   0x00 0x00 0x00 0x00 0x00 0xOD 0x31
//          起始位 读写位 设备 数据 数据 数据 数据 停止
//                 0 读
//                 1 写
//
// 一种数据对应一个数据区段
// 例：
//  读温度  0xFF   0x00   0x00 0x00 0x00 0x00 0x00 0x00
//      ->  0xEE   0x01   0x00 0x00 0x00 0x01 0x17 0x01
//
//  写数码管
//          0xFF   0x01   0x02 0x0F 0x00 0x00 0x00 0x00
//      ->  0xEE   0x00   0x00 0x00 0x00 0x00 0x00 0x01
//-----------------------------------------
//串口模式配置
#define FOSC 11059200L //系统频率
#define BAUD 115200    //串口波特率

#define NONE_PARITY 0  //无校验
#define ODD_PARITY 1   //奇校验
#define EVEN_PARITY 2  //偶校验
#define MARK_PARITY 3  //标记校验
#define SPACE_PARITY 4 //空白校验

#define PARITYBIT NONE_PARITY //定义校验位

#define END_1 0x31

//-----------------------------------------
//标准定义
#define TRUE 1         // True
#define FALSE 0        // False
#define CMD_START 0xFF //命令头
#define CMD_READ 0x00  //读操作
#define CMD_WRITE 0x01 //写操作

#define DATA_START 0xEE //命令头
#define DATA_READ 0x00  //读操作
#define DATA_WRITE 0x01 //写操作

//-----------------------------------------
//功能定义
#define TEMPTURE 0x00     //温度传感器
#define ILLUMINA 0x01     //光明电阻
#define SET_TEMPTURE 0x02 //温度范围
#define SET_ILLUMINA 0x03 //温度范围

//-----------------------------------------
//上下限设置
u32 tempture_max = 27;
u32 tempture_min = 20;
u8 illumina_max = 120;
u8 illumina_min = 40;

#define tempture_max_led 0x00
#define tempture_min_led 0x01
#define illumina_max_led 0x02
#define illumina_min_led 0x03

sbit tempture_max_led_port = P0 ^ 0;
sbit tempture_min_led_port = P0 ^ 1;
sbit illumina_max_led_port = P0 ^ 2;
sbit illumina_min_led_port = P0 ^ 3;

bit tempture_max_led_data = 1;
bit tempture_min_led_data = 1;
bit illumina_max_led_data = 1;
bit illumina_min_led_data = 1;

//-----------------------------------------
//外设切换部分
#define S1_S0 0x40 // P_SW1.6
#define S1_S1 0x80 // P_SW1.7
bit busy;

//-----------------------------------------
//通讯数组
u8 COMMAND[8] = {0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 DATA[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//-----------------------------------------
//软件延时函数 @11.0592MHz
void DelayMS(u16 ms) {
  do {
    u8 i, j;

    _nop_();
    _nop_();
    _nop_();
    i = 11;
    j = 190;
    do {
      while (--j)
        ;
    } while (--i);
  } while (--ms);
}

//-----------------------------------------
//初始化开发板，关闭蜂鸣器，关闭继电器
void BOARD_INIT() {
  SelectBUZZ(FALSE);
  SelectRELAY(FALSE);
  P2 = 0xA0;
  P0 = 0x00;
  P2 = 0x00;
  P2 = 0x80;
  P0 = 0xFF;
  P2 = 0x00;
}

//-----------------------------------------
//初始化UART1外设
void UART_INIT() {
#if (PARITYBIT == NONE_PARITY)
  SCON = 0x50; // 8位可变波特率
#elif (PARITYBIT == ODD_PARITY) || (PARITYBIT == EVEN_PARITY) ||               \
    (PARITYBIT == MARK_PARITY)
  SCON = 0xda; // 9位可变波特率,校验位初始为1
#elif (PARITYBIT == SPACE_PARITY)
  SCON = 0xd2; // 9位可变波特率,校验位初始为0
#endif

  AUXR = 0x40;                       //定时器1为1T模式
  TMOD = 0x00;                       //定时器1为模式0(16位自动重载)
  TL1 = (65536 - (FOSC / 4 / BAUD)); //设置波特率重装值
  TH1 = (65536 - (FOSC / 4 / BAUD)) >> 8;
  TR1 = 1; //定时器1开始启动
  ES = 1;  //使能串口中断
  EA = 1;
}

//-----------------------------------------
//选择串口端口，0->P30,P31, 1->P36,P37, 2->P16,P17
void SELECT_UART_PORT(u8 PORT) {
  switch (PORT) {
  case 0:
    ACC = P_SW1;
    ACC &= ~(S1_S0 | S1_S1); // S1_S0=0 S1_S1=0
    P_SW1 = ACC;             //(P3.0/RxD, P3.1/TxD)
    break;
  case 1:
    ACC = P_SW1;
    ACC &= ~(S1_S0 | S1_S1); // S1_S0=1 S1_S1=0
    ACC |= S1_S0;            //(P3.6/RxD_2, P3.7/TxD_2)
    P_SW1 = ACC;
    break;
  case 2:
    ACC = P_SW1;
    ACC &= ~(S1_S0 | S1_S1); // S1_S0=0 S1_S1=1
    ACC |= S1_S1;            //(P1.6/RxD_3, P1.7/TxD_3)
    P_SW1 = ACC;
    break;
  }
}

//-----------------------------------------
// 发送串口数据
void SendUart(u8 dat) {
  while (busy)
    ;        //等待前面的数据发送完成
  ACC = dat; //获取校验位P (PSW.0)
  if (P)     //根据P来设置校验位
  {
#if (PARITYBIT == ODD_PARITY)
    TB8 = 0; //设置校验位为0
#elif (PARITYBIT == EVEN_PARITY)
    TB8 = 1; //设置校验位为1
#endif
  } else {
#if (PARITYBIT == ODD_PARITY)
    TB8 = 1; //设置校验位为1
#elif (PARITYBIT == EVEN_PARITY)
    TB8 = 0; //设置校验位为0
#endif
  }
  busy = 1;
  SBUF = ACC; //写数据到UART数据寄存器
}

//-----------------------------------------
//发送数据
void SendDATA() {
  u8 i = 0;
  DATA[7] = END_1;
  for (i = 0; i < 8; i++) //遍历数据
  {
    SendUart(DATA[i]); //发送当前数据
  }
}

//-----------------------------------------
//接收数据
u8 RecvUart() {
  while (!RI)
    ;          //等待串口数据接收完成
  RI = 0;      //清除接收标志
  return SBUF; //返回串口数据
}

//-----------------------------------------
//刷新命令数据
void RefreshUART() {
  DATA[0] = 0x00;
  DATA[1] = 0xFF;
  DATA[2] = 0x00;
  DATA[3] = 0x00;
  DATA[4] = 0x00;
  DATA[5] = 0x00;
  DATA[6] = 0x00;
  DATA[7] = 0x00;
  COMMAND[0] = 0x00;
  COMMAND[1] = 0x00;
  COMMAND[2] = 0x00;
  COMMAND[3] = 0x00;
  COMMAND[4] = 0x00;
  COMMAND[5] = 0x00;
  COMMAND[6] = 0x00;
  COMMAND[7] = 0x00;
}

//-----------------------------------------
//接收命令
void RecvCMD() {
  if (RecvUart() == CMD_START) {
    COMMAND[0] = CMD_START;
    COMMAND[1] = RecvUart();
    COMMAND[2] = RecvUart();
    COMMAND[3] = RecvUart();
    COMMAND[4] = RecvUart();
    COMMAND[5] = RecvUart();
    COMMAND[6] = RecvUart();
    COMMAND[7] = RecvUart();
  }
}

void LightLED() {
  SelectHC573(4);
  P0 = 0xFF;
  SelectHC573(0);

  SelectHC573(4);
  tempture_max_led_port = tempture_max_led_data;
  tempture_min_led_port = tempture_min_led_data;
  illumina_max_led_port = illumina_max_led_data;
  illumina_min_led_port = illumina_min_led_data;
  SelectHC573(0);
}

//-----------------------------------------
//检查温度范围
void CheckTempture(u32 tempture) {
  tempture = tempture / 10000;

  tempture_max_led_data = 1;
  tempture_min_led_data = 1;
  if (tempture > tempture_max) {
    tempture_max_led_data = 0;
  } else if (tempture < tempture_min) {
    tempture_min_led_data = 0;
  }
}

//-----------------------------------------
//读温度
void ReadTempture() {
  u32 tempture;
  tempture = (u32)rd_temperature() * 10000;
  CheckTempture(tempture);

  DATA[0] = DATA_START;
  DATA[1] = DATA_WRITE;
  DATA[2] = TEMPTURE;
  DATA[3] = (u8)((tempture >> 24) & 0xFF);
  DATA[4] = (u8)((tempture >> 16) & 0xFF);
  DATA[5] = (u8)((tempture >> 8) & 0xFF);
  DATA[6] = (u8)(tempture & 0xFF);
}

//-----------------------------------------
//检查温度范围
void CheckIllumination(u8 illumination) {
  illumina_max_led_data = 1;
  illumina_min_led_data = 1;
  if (illumination > illumina_max) {
    illumina_max_led_data = 0;
  } else if (illumination < illumina_min) {
    illumina_min_led_data = 0;
  }
}

//-----------------------------------------
//读光照
void ReadIllumination() {
  u8 adc_val;
  adc_val = Read_ADC(ILLUMINA);
  CheckIllumination(adc_val);

  DATA[0] = DATA_START;
  DATA[1] = DATA_WRITE;
  DATA[2] = ILLUMINA;
  DATA[3] = (0x00);
  DATA[4] = (0x00);
  DATA[5] = (0x00);
  DATA[6] = (adc_val);
}

//-----------------------------------------
//读操作
void SYSReadFunc() {
  switch (COMMAND[2]) {
  case TEMPTURE:
    ReadTempture();
    break;
  case ILLUMINA:
    ReadIllumination();
    break;
  default:
    _nop_();
    break;
  }
  LightLED();
}

//-----------------------------------------
//写操作
void SYSWriteFunc() {
  switch (COMMAND[2]) {
  case SET_TEMPTURE:
    tempture_max = COMMAND[3];
    tempture_min = COMMAND[4];
    break;
  case SET_ILLUMINA:
    illumina_max = COMMAND[3];
    illumina_min = COMMAND[4];
    break;
  default:
    _nop_();
    break;
  }
  DATA[0] = DATA_START;
  DATA[1] = DATA_READ;
}

//-----------------------------------------
//主服务程序
void main() {
  BOARD_INIT();
  SELECT_UART_PORT(0); //选择P30，P31管脚作为串口0
  UART_INIT();         //初始化串口

  while (1) {
    RecvCMD();
    if (COMMAND[0] == CMD_START) {
      switch (COMMAND[1]) //检测读写
      {
      case CMD_READ:   //读操作
        SYSReadFunc(); //调用读操作
        break;
      case CMD_WRITE:
        SYSWriteFunc();
        break;
      default:
        break;
      }
      SendDATA();
      RefreshUART();
    }
  }
}

//-----------------------------------------
// UART 中断服务程序
void UART1() interrupt 4 using 1 {
  if (RI) {
    RI = 0;    //清除RI位
    P0 = SBUF; // P0显示串口数据
    P22 = RB8; // P2.2显示校验位
  }
  if (TI) {
    TI = 0;   //清除TI位
    busy = 0; //清忙标志
  }
}
