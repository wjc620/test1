/****************************************************************************/
/*                                                                          */
/*              ���ݴ������ӿƼ����޹�˾                                    */
/*                                                                          */
/*              Copyright 2015 Tronlong All rights reserved                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              I2C�ӿ�API                                                  */
/*                                                                          */
/*              2015��03��27��                                              */
/*                                                                          */
/****************************************************************************/
#include <stdio.h>
#include <stdint.h>

#include "TL6748.h"
#include "hw_types.h"
#include "i2c.h"
#include "codecif.h"
#include "interrupt.h"
#include "soc_C6748.h"
#include "hw_syscfg0_C6748.h"

#include "delay.h"

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/
void I2CISR(void);
static void I2CSendBlocking(unsigned int baseAddr, unsigned int dataCnt);
static void I2CRcvBlocking(unsigned int baseAddr, unsigned int dataCnt);
void I2CIntRegister(unsigned int cpuINT, unsigned int sysIntNum);

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/
volatile unsigned int dataIdx = 0;
volatile unsigned int txCompFlag = 1;
volatile unsigned int slaveData[3];
unsigned int savedBase;

/****************************************************************************/
/*                                                                          */
/*              ��̬����I2CӲ���ж�                                         */
/*                                                                          */
/****************************************************************************/
void I2CIntRegister(unsigned int cpuINT, unsigned int sysIntNum)
{
	IntRegister(cpuINT, I2CISR);
	IntEventMap(cpuINT, sysIntNum);
	IntEnable(cpuINT);
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ��I2C�ӿ�                                               */
/*                                                                          */
/****************************************************************************/
// baseAddr: I2Cģ�����ַ
// slaveAddr: �����ӵ�ַ
void I2CSetup(unsigned int baseAddr, unsigned int slaveAddr)
{
    // ��ʼ��I2C0����PINMUX
	I2CPinMuxSetup(0);

    // IIC ��λ / ����
    I2CMasterDisable(baseAddr);

    // ���������ٶ�Ϊ 100KHz
    I2CMasterInitExpClk(baseAddr, 24000000, 8000000, 100000);

    // ���ô��豸��ַ
    I2CMasterSlaveAddrSet(baseAddr, slaveAddr);

    // IIC ʹ��
    I2CMasterEnable(baseAddr);
}

/****************************************************************************/
/*                                                                          */
/*              I2C��������                                                 */
/*                                                                          */
/****************************************************************************/
// iic IIC ģ���ַ
// dataCnt ���ݴ�С
// ��������
static void I2CSendBlocking(unsigned int baseAddr, unsigned int dataCnt)
{
    txCompFlag = 1;
    dataIdx = 0;    
    savedBase = baseAddr;
 
    I2CSetDataCount(baseAddr, dataCnt);

    I2CMasterControl(baseAddr, I2C_CFG_MST_TX | I2C_CFG_STOP);

    I2CMasterIntEnableEx(baseAddr, I2C_INT_TRANSMIT_READY | I2C_INT_STOP_CONDITION);

    I2CMasterStart(baseAddr);
   
    // �ȴ����ݷ������
    while(txCompFlag);
}

//static void I2CCodecSendBlockingNstop(unsigned int baseAddr, unsigned int dataCnt)
//{
//	txCompFlag = 1;
//    dataIdx = 0;
//    savedBase = baseAddr;
//
//    I2CSetDataCount(baseAddr, dataCnt);
//
//    I2CMasterControl(baseAddr, I2C_CFG_MST_TX );//| I2C_CFG_STOP
//
//    I2CMasterIntEnableEx(baseAddr, I2C_INT_TRANSMIT_READY | I2C_INT_STOP_CONDITION);
//
//    I2CMasterStart(baseAddr);
//
//    // �ȴ����ݷ������
//    while(dataIdx == 0);
////    Task_sleep(19);
//    delay_us(190);
//}

/****************************************************************************/
/*                                                                          */
/*              IIC ��������                                                */
/*                                                                          */
/****************************************************************************/
// iic IIC ģ���ַ
// dataCnt ���ݴ�С
// ��������
static void I2CRcvBlocking(unsigned int baseAddr, unsigned int dataCnt)
{
    txCompFlag = 1;
    dataIdx = 0;
    savedBase = baseAddr;
    
    I2CSetDataCount(baseAddr, dataCnt);

    I2CMasterControl(baseAddr, I2C_CFG_MST_RX | I2C_CFG_STOP);

    I2CMasterIntEnableEx(baseAddr, I2C_INT_DATA_READY | I2C_INT_STOP_CONDITION);

    I2CMasterStart(baseAddr);

    // �ȴ����ݽ������
    while(txCompFlag);
}

/****************************************************************************/
/*                                                                          */
/*              IIC �жϷ�����                                            */
/*                                                                          */
/****************************************************************************/
void I2CISR(void)
{
	volatile unsigned int intCode = 0;

	// ȡ���жϴ���
	intCode = I2CInterruptVectorGet(savedBase);

	if (intCode == I2C_INTCODE_TX_READY)
	{
	  I2CMasterDataPut(savedBase, slaveData[dataIdx]);
	  dataIdx++;
	}

	if(intCode == I2C_INTCODE_RX_READY)
	{
	  slaveData[dataIdx] = I2CMasterDataGet(savedBase);
	  dataIdx++;
	}

	if (intCode == I2C_INTCODE_STOP)
	{
	  // ʧ���ж�
	  I2CMasterIntDisableEx(savedBase, I2C_INT_TRANSMIT_READY
									   | I2C_INT_DATA_READY);
	  txCompFlag = 0;
	}

	if (intCode == I2C_INTCODE_NACK)
	{
	 I2CMasterIntDisableEx(SOC_I2C_0_REGS, I2C_INT_TRANSMIT_READY |
										   I2C_INT_DATA_READY |
										   I2C_INT_NO_ACK |
										   I2C_INT_STOP_CONDITION);
	 // ����ֹͣλ
	 I2CMasterStop(SOC_I2C_0_REGS);

	 I2CStatusClear(SOC_I2C_0_REGS, I2C_CLEAR_STOP_CONDITION);

	 // ����ж�
	 IntEventClear(SYS_INT_I2C0_INT);
	 txCompFlag = 0;
	}

	intCode = I2CInterruptVectorGet(savedBase);
}

/****************************************************************************/
/*                                                                          */
/*              д���ݵ��Ĵ���                                              */
/*                                                                          */
/****************************************************************************/
void I2CRegWrite(unsigned int baseAddr, unsigned char regAddr,
                   unsigned char regData)
{
    // ���ͼĴ�����ַ������
    slaveData[0] = regAddr;
    slaveData[1] = regData;

    I2CSendBlocking(baseAddr, 2);
}

/****************************************************************************/
/*                                                                          */
/*              IIC д�������ݵ��Ĵ���                                            */
/*                                                                          */
/****************************************************************************/
void I2CRegWrite3(unsigned int baseAddr, unsigned char regAddr,
                    unsigned char regData, unsigned char regData2)
{
	// ���ͼĴ�����ַ������
    slaveData[0] = regAddr;
    slaveData[1] = regData;
    slaveData[2] = regData2;

    I2CSendBlocking(baseAddr, 3);
}

/****************************************************************************/
/*                                                                          */
/*              ���Ĵ���                                                    */
/*                                                                          */
/****************************************************************************/
unsigned char I2CRegRead(unsigned int baseAddr, unsigned char regAddr)
{
	// ���ͼĴ�����ַ
    slaveData[0] = regAddr;
    I2CSendBlocking(baseAddr, 1);
//    I2CCodecSendBlockingNstop(baseAddr, 1);

    // ���ռĴ�����������
    I2CRcvBlocking(baseAddr, 1);

    return (slaveData[0]);
}

/****************************************************************************/
/*                                                                          */
/*              IICд�Ĵ�����ĳһbit                                        */
/*                                                                          */
/****************************************************************************/
void I2CRegBitSet(unsigned int baseAddr, unsigned char regAddr,
                    unsigned char bitMask)
{
	// ���ͼĴ�����ַ
    slaveData[0] = regAddr;
    I2CSendBlocking(baseAddr, 1);
//    I2CCodecSendBlockingNstop(baseAddr, 1);
  
    // ���ռĴ�����������
    I2CRcvBlocking(baseAddr, 1);

    slaveData[1] =  slaveData[0] | bitMask;
    slaveData[0] = regAddr;

    I2CSendBlocking(baseAddr, 2);
}

/****************************************************************************/
/*                                                                          */
/*              IIC ����Ĵ�����ĳһbit                                     */
/*                                                                          */
/****************************************************************************/
void I2CRegBitClr(unsigned int baseAddr, unsigned char regAddr,
                    unsigned char bitMask)
{
	// ���ͼĴ�����ַ
    slaveData[0] = regAddr;
    I2CSendBlocking(baseAddr, 1);
//    I2CCodecSendBlockingNstop(baseAddr, 1);

    // ���ռĴ�����������
    I2CRcvBlocking(baseAddr, 1);

    slaveData[1] =  slaveData[0] & ~bitMask;
    slaveData[0] = regAddr;
   
    I2CSendBlocking(baseAddr, 2);
}

/***************************** End Of File ***********************************/
