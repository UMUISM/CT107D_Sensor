#include "CT107D.h"
#include "STC15.h"
#include "iic.h"
#include "intrins.h"
#include "onewire.h"

//-----------------------------------------
// ͨѶ��ʽ
// ��������
// char[8], 0xFF   0x00   0x00 0x00 0x00 0x00 0x00 0xOD 0x31
//          ��ʼλ ��дλ �豸 ���� ���� ���� ���� ֹͣ
//                 0 ��
//                 1 д
// ��������
// char[8], 0xEE   0x00   0x00 0x00 0x00 0x00 0x00 0xOD 0x31
//          ��ʼλ ��дλ �豸 ���� ���� ���� ���� ֹͣ
//                 0 ��
//                 1 д
//
// һ�����ݶ�Ӧһ����������
// ����
//  ���¶�  0xFF   0x00   0x00 0x00 0x00 0x00 0x00 0x00
//      ->  0xEE   0x01   0x00 0x00 0x00 0x01 0x17 0x01
//
//  д�����
//          0xFF   0x01   0x02 0x0F 0x00 0x00 0x00 0x00
//      ->  0xEE   0x00   0x00 0x00 0x00 0x00 0x00 0x01
//-----------------------------------------
//����ģʽ����
#define FOSC 11059200L //ϵͳƵ��
#define BAUD 115200    //���ڲ�����

#define NONE_PARITY 0  //��У��
#define ODD_PARITY 1   //��У��
#define EVEN_PARITY 2  //żУ��
#define MARK_PARITY 3  //���У��
#define SPACE_PARITY 4 //�հ�У��

#define PARITYBIT NONE_PARITY //����У��λ

#define END_1 0x31

//-----------------------------------------
//��׼����
#define TRUE 1         // True
#define FALSE 0        // False
#define CMD_START 0xFF //����ͷ
#define CMD_READ 0x00  //������
#define CMD_WRITE 0x01 //д����

#define DATA_START 0xEE //����ͷ
#define DATA_READ 0x00  //������
#define DATA_WRITE 0x01 //д����

//-----------------------------------------
//���ܶ���
#define TEMPTURE 0x00 //�¶ȴ�����
#define ILLUMINA 0x01 //��������
#define SMG 0x02      //�����

//-----------------------------------------
//����ܶ�λ��
u8 const SMG_DUAN[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F,
                       0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71}; // 0~F�Ķ����
u8 const SMG_WEI[] = {
    0x01, 0x02, 0x04, 0x08,
    0x10, 0x20, 0x40, 0x80}; //��ߵ�һƬ����ܵ����ұ�����ܵ�λѡ��

//-----------------------------------------
//�����л�����
#define S1_S0 0x40 // P_SW1.6
#define S1_S1 0x80 // P_SW1.7
bit busy;

//-----------------------------------------
//ͨѶ����
u8 COMMAND[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 DATA[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//-----------------------------------------
//�����ʱ���� @11.0592MHz
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
//��ʼ�������壬�رշ��������رռ̵���
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
//��ʼ��UART1����
void UART_INIT() {
#if (PARITYBIT == NONE_PARITY)
  SCON = 0x50; // 8λ�ɱ䲨����
#elif (PARITYBIT == ODD_PARITY) || (PARITYBIT == EVEN_PARITY) ||               \
    (PARITYBIT == MARK_PARITY)
  SCON = 0xda; // 9λ�ɱ䲨����,У��λ��ʼΪ1
#elif (PARITYBIT == SPACE_PARITY)
  SCON = 0xd2; // 9λ�ɱ䲨����,У��λ��ʼΪ0
#endif

  AUXR = 0x40;                       //��ʱ��1Ϊ1Tģʽ
  TMOD = 0x00;                       //��ʱ��1Ϊģʽ0(16λ�Զ�����)
  TL1 = (65536 - (FOSC / 4 / BAUD)); //���ò�������װֵ
  TH1 = (65536 - (FOSC / 4 / BAUD)) >> 8;
  TR1 = 1; //��ʱ��1��ʼ����
  ES = 1;  //ʹ�ܴ����ж�
  EA = 1;
}

//-----------------------------------------
//ѡ�񴮿ڶ˿ڣ�0->P30,P31, 1->P36,P37, 2->P16,P17
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
// ���ʹ�������
void SendUart(u8 dat) {
  while (busy)
    ;        //�ȴ�ǰ������ݷ������
  ACC = dat; //��ȡУ��λP (PSW.0)
  if (P)     //����P������У��λ
  {
#if (PARITYBIT == ODD_PARITY)
    TB8 = 0; //����У��λΪ0
#elif (PARITYBIT == EVEN_PARITY)
    TB8 = 1; //����У��λΪ1
#endif
  } else {
#if (PARITYBIT == ODD_PARITY)
    TB8 = 1; //����У��λΪ1
#elif (PARITYBIT == EVEN_PARITY)
    TB8 = 0; //����У��λΪ0
#endif
  }
  busy = 1;
  SBUF = ACC; //д���ݵ�UART���ݼĴ���
}

//-----------------------------------------
//��������
void SendDATA() {
  u8 i = 0;
  DATA[7] = END_1;
  for (i = 0; i < 8; i++) //��������
  {
    SendUart(DATA[i]); //���͵�ǰ����
  }
}

//-----------------------------------------
//��������
u8 RecvUart() {
  while (!RI)
    ;          //�ȴ��������ݽ������
  RI = 0;      //������ձ�־
  return SBUF; //���ش�������
}

//-----------------------------------------
//ˢ����������
void RefreshUART() {
  DATA[0] = 0x00;
  DATA[1] = 0x00;
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
//��������
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

//-----------------------------------------
//���¶�
void ReadTempture() {
  u32 tempture;
  tempture = (u32)rd_temperature() * 10000;

  DATA[0] = DATA_START;
  DATA[1] = DATA_WRITE;
  DATA[2] = TEMPTURE;
  DATA[3] = (u8)((tempture >> 24) & 0xFF);
  DATA[4] = (u8)((tempture >> 16) & 0xFF);
  DATA[5] = (u8)((tempture >> 8) & 0xFF);
  DATA[6] = (u8)(tempture & 0xFF);
}

//-----------------------------------------
//������
void ReadIllumination() {
  u8 adc_val;
  adc_val = Read_ADC(ILLUMINA);

  DATA[0] = DATA_START;
  DATA[1] = DATA_WRITE;
  DATA[2] = ILLUMINA;
  DATA[3] = (0x00);
  DATA[4] = (0x00);
  DATA[5] = (0x00);
  DATA[6] = (adc_val);
}

//-----------------------------------------
//������
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
}

void WriteSMG() {
  u8 duan[8], i;

  duan[0] = COMMAND[3] & 0xF0;
  duan[1] = COMMAND[3] & 0x0F;
  duan[2] = COMMAND[4] & 0x0F;
  duan[3] = COMMAND[4] & 0x0F;
  duan[4] = COMMAND[5] & 0x0F;
  duan[5] = COMMAND[5] & 0x0F;
  duan[6] = COMMAND[6] & 0x0F;
  duan[7] = COMMAND[6] & 0x0F;

  for (i = 0; i < 8; i++) {
    SelectHC573(6);
    P0 = 0x00;
    SelectHC573(0);

    SelectHC573(7);
    P0 = SMG_WEI[i];
    SelectHC573(0);

    SelectHC573(6);
    P0 = SMG_DUAN[duan[i]];
    SelectHC573(0);
  }
}

//-----------------------------------------
//д����
void SYSWriteFunc() {
  switch (COMMAND[2]) {
  case SMG:
    WriteSMG();
    break;
  default:
    _nop_();
    break;
  }
}

//-----------------------------------------
//���������
void main() {
  BOARD_INIT();
  SELECT_UART_PORT(0); //ѡ��P30��P31�ܽ���Ϊ����0
  UART_INIT();         //��ʼ������

  while (1) {
    RecvCMD();
    switch (COMMAND[1]) //����д
    {
    case CMD_READ:   //������
      SYSReadFunc(); //���ö�����
      break;
    case CMD_WRITE:
      break;
    default:
      break;
    }

    SendDATA();
    RefreshUART();
  }
}

//-----------------------------------------
// UART �жϷ������
void UART1() interrupt 4 using 1 {
  if (RI) {
    RI = 0;    //���RIλ
    P0 = SBUF; // P0��ʾ��������
    P22 = RB8; // P2.2��ʾУ��λ
  }
  if (TI) {
    TI = 0;   //���TIλ
    busy = 0; //��æ��־
  }
}
