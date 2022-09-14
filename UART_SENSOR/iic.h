#ifndef _IIC_H
#define _IIC_H

void IIC_Start(void);
void IIC_Stop(void);
bit IIC_WaitAck(void);
void IIC_SendAck(bit ackbit);
void IIC_SendByte(unsigned char byt);
unsigned char IIC_RecByte(void);
void Write_AT24C02(unsigned char add, unsigned char dat);
unsigned char Read_AT24C02(unsigned char add);
void Read_DAC(unsigned char dat);
unsigned char Read_ADC(unsigned char add);

#endif