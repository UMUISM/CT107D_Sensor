/*
  ����˵��: IIC������������
  ��������: Keil uVision 4.10
  Ӳ������: CT107��Ƭ���ۺ�ʵѵƽ̨ 8051��12MHz
  ��    ��: 2011-8-9
*/

#include "intrins.h"
#include "reg52.h"

#define DELAY_TIME 5

#define SlaveAddrW 0xA0
#define SlaveAddrR 0xA1

//�������Ŷ���
sbit SDA = P2 ^ 1; /* ������ */
sbit SCL = P2 ^ 0; /* ʱ���� */

void IIC_Delay(unsigned char i) {
  do {
    _nop_();
  } while (i--);
}
//������������
void IIC_Start(void) {
  SDA = 1;
  SCL = 1;
  IIC_Delay(DELAY_TIME);
  SDA = 0;
  IIC_Delay(DELAY_TIME);
  SCL = 0;
}

//����ֹͣ����
void IIC_Stop(void) {
  SDA = 0;
  SCL = 1;
  IIC_Delay(DELAY_TIME);
  SDA = 1;
  IIC_Delay(DELAY_TIME);
}

//����Ӧ��
void IIC_SendAck(bit ackbit) {
  SCL = 0;
  SDA = ackbit; // 0��Ӧ��1����Ӧ��
  IIC_Delay(DELAY_TIME);
  SCL = 1;
  IIC_Delay(DELAY_TIME);
  SCL = 0;
  SDA = 1;
  IIC_Delay(DELAY_TIME);
}

//�ȴ�Ӧ��
bit IIC_WaitAck(void) {
  bit ackbit;

  SCL = 1;
  IIC_Delay(DELAY_TIME);
  ackbit = SDA;
  SCL = 0;
  IIC_Delay(DELAY_TIME);
  return ackbit;
}

//ͨ��I2C���߷�������
void IIC_SendByte(unsigned char byt) {
  unsigned char i;

  for (i = 0; i < 8; i++) {
    SCL = 0;
    IIC_Delay(DELAY_TIME);
    if (byt & 0x80)
      SDA = 1;
    else
      SDA = 0;
    IIC_Delay(DELAY_TIME);
    SCL = 1;
    byt <<= 1;
    IIC_Delay(DELAY_TIME);
  }
  SCL = 0;
}

//��I2C�����Ͻ�������
unsigned char IIC_RecByte(void) {
  unsigned char i, da;
  for (i = 0; i < 8; i++) {
    SCL = 1;
    IIC_Delay(DELAY_TIME);
    da <<= 1;
    if (SDA)
      da |= 1;
    SCL = 0;
    IIC_Delay(DELAY_TIME);
  }
  return da;
}

//��AT24C02д��һ���ֽڵ�����
void Write_AT24C02(unsigned char add, unsigned char dat) {
  IIC_Start();
  IIC_SendByte(0XA0);
  IIC_WaitAck();
  IIC_SendByte(add);
  IIC_WaitAck();
  IIC_SendByte(dat);
  IIC_WaitAck();
  IIC_Stop();
}

//��AT24C02����һ���ֽڵ�����
unsigned char Read_AT24C02(unsigned char add) {
  unsigned char dat;
  IIC_Start();
  IIC_SendByte(0XA0);
  IIC_WaitAck();
  IIC_SendByte(add);
  IIC_WaitAck();
  IIC_Start();
  IIC_SendByte(0XA1);
  IIC_WaitAck();
  dat = IIC_RecByte();
  IIC_WaitAck();
  IIC_Stop();
  return dat;
}

// PCF8591��DAת��
void Read_DAC(unsigned char dat) {
  IIC_Start();
  IIC_SendByte(0X90);
  IIC_WaitAck();
  IIC_SendByte(0x40);
  IIC_WaitAck();
  IIC_SendByte(dat);
  IIC_WaitAck();
  IIC_Stop();
}

// PCF8591��ADת��
unsigned char Read_ADC(unsigned char add) {
  unsigned char dat;
  IIC_Start();
  IIC_SendByte(0X90);
  IIC_WaitAck();
  IIC_SendByte(add);
  IIC_WaitAck();
  IIC_Start();
  IIC_SendByte(0X91);
  IIC_WaitAck();
  dat = IIC_RecByte();
  IIC_WaitAck();
  IIC_Stop();
  return dat;
}