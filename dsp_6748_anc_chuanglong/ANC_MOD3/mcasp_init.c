/****************************************************************************/
/*                                                                          */
/*              ���ݴ������ӿƼ����޹�˾                                    */
/*                                                                          */
/*              Copyright 2015 Tronlong All rights reserved                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              McASP ��ʼ��										        */
/*                                                                          */
/*              2015��07��13��                  							*/
/*                                                                          */
/****************************************************************************/
#include "TL6748.h"                 // ���� DSP6748 �������������

#include "edma_event.h"
#include "interrupt.h"
#include "soc_OMAPL138.h"
#include "hw_syscfg0_OMAPL138.h"

#include "codecif.h"
#include "mcasp.h"
#include "edma.h"
#include "psc.h"
#include "uartStdio.h"
#include "dspcache.h"

#include "aic3106_init.h"
#include "mcasp_init.h"

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
// McASP ����ͨ��
#define MCASP_XSER_RX                         (12u)

// McASP ����ͨ��
#define MCASP_XSER_TX                         (11u)

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/*              McASP �������                                              */
/*                                                                          */
/****************************************************************************/
void OutputSample(unsigned int outData)
{
	McASPTxBufWrite(SOC_MCASP_0_CTRL_REGS, 11, outData);
}

/****************************************************************************/
/*                                                                          */
/*              McASP �������ݰ�                                            */
/*                                                                          */
/****************************************************************************/
unsigned int InputSample(void)
{
	return (McASPRxBufRead(SOC_MCASP_0_CTRL_REGS, 12));
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� McASP ����ͨ��    	                                */
/*                                                                          */
/****************************************************************************/
void McASPI2SRxConfigure(unsigned char wordSize,unsigned char slotSize,
		unsigned int slotNum, unsigned char modeDMA)
{
	// ��λ
	McASPRxReset(SOC_MCASP_0_CTRL_REGS);

	switch(modeDMA)
	{
		case MCASP_MODE_DMA:
			// ʹ�� FIFO
			McASPReadFifoEnable(SOC_MCASP_0_FIFO_REGS, 1, 1);

			// ���ý��� word �� slot �Ĵ�С
			McASPRxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, wordSize, slotSize,
								MCASP_RX_MODE_DMA);
			break;
		case MCASP_MODE_NON_DMA:
			// ���ý��� word �� slot �Ĵ�С
			McASPRxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, wordSize, slotSize,
								MCASP_RX_MODE_NON_DMA);
			break;
	}

	// ��ʼ��֡ͬ����TDM ��ʽʹ�� slot ����������֡ͬ���źŵ�������
	McASPRxFrameSyncCfg(SOC_MCASP_0_CTRL_REGS, slotNum, MCASP_RX_FS_WIDTH_WORD,
						MCASP_RX_FS_EXT_BEGIN_ON_RIS_EDGE);

	// ��ʼ������ʱ�ӣ�ʹ���ⲿʱ�ӣ�ʱ����������Ч
	McASPRxClkCfg(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_EXTERNAL, 0, 0);
	McASPRxClkPolaritySet(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_POL_RIS_EDGE);
	McASPRxClkCheckConfig(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLKCHCK_DIV32,
						  0x00, 0xFF);

	// ʹ�ܷ��ͽ���ͬ��
	McASPTxRxClkSyncEnable(SOC_MCASP_0_CTRL_REGS);
	McASPTxClkCfg(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_EXTERNAL, 0, 0);

	// ʹ�� ���� slot
	McASPRxTimeSlotSet(SOC_MCASP_0_CTRL_REGS, (1 << slotNum)-1);

	// ���ô�����������12ͨ������
	McASPSerializerRxSet(SOC_MCASP_0_CTRL_REGS, MCASP_XSER_RX);

	// ��ʼ�� McASP ���ţ������������������
	McASPPinMcASPSet(SOC_MCASP_0_CTRL_REGS, 0xFFFFFFFF);
	McASPPinDirInputSet(SOC_MCASP_0_CTRL_REGS, MCASP_PIN_AFSX
											   | MCASP_PIN_ACLKX
											   | MCASP_PIN_AXR(MCASP_XSER_RX));
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� McASP ����ͨ��    	                                */
/*                                                                          */
/****************************************************************************/
void McASPI2STxConfigure(unsigned char wordSize,unsigned char slotSize,
		unsigned int slotNum, unsigned char modeDMA)
{
	// ��λ
	McASPTxReset(SOC_MCASP_0_CTRL_REGS);

	switch(modeDMA)
	{
		case MCASP_MODE_DMA:
			// ʹ�� FIFO
			McASPWriteFifoEnable(SOC_MCASP_0_FIFO_REGS, 1, 1);

			// ���÷��� word �� slot �Ĵ�С
			McASPTxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, wordSize, slotSize,
								MCASP_TX_MODE_DMA);
			break;
		case MCASP_MODE_NON_DMA:
			// ���÷��� word �� slot �Ĵ�С
			McASPTxFmtI2SSet(SOC_MCASP_0_CTRL_REGS, wordSize, slotSize,
								MCASP_TX_MODE_NON_DMA);
			break;
	}

	// ��ʼ��֡ͬ����TDM ��ʽʹ�� slot ����������֡ͬ���źŵ�������
	McASPTxFrameSyncCfg(SOC_MCASP_0_CTRL_REGS, slotNum, MCASP_TX_FS_WIDTH_WORD,
						MCASP_TX_FS_EXT_BEGIN_ON_RIS_EDGE);

	// ��ʼ������ʱ�ӣ�ʹ���ⲿʱ�ӣ�ʱ����������Ч
	McASPTxClkCfg(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_EXTERNAL, 0, 0);
	McASPTxClkPolaritySet(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_POL_FALL_EDGE);
	McASPTxClkCheckConfig(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLKCHCK_DIV32,
						  0x00, 0xFF);

	// ʹ�ܷ��ͽ���ͬ��
	McASPTxRxClkSyncEnable(SOC_MCASP_0_CTRL_REGS);

	// ʹ�� ���� slot
	McASPTxTimeSlotSet(SOC_MCASP_0_CTRL_REGS, (1 << slotNum)-1);

	// ���ô�����������11ͨ������
	McASPSerializerTxSet(SOC_MCASP_0_CTRL_REGS, MCASP_XSER_TX);

	// ��ʼ�� McASP ���ţ������������������
	McASPPinMcASPSet(SOC_MCASP_0_CTRL_REGS, 0xFFFFFFFF);
	McASPPinDirOutputSet(SOC_MCASP_0_CTRL_REGS,MCASP_PIN_AXR(MCASP_XSER_TX));
	McASPPinDirInputSet(SOC_MCASP_0_CTRL_REGS, MCASP_PIN_AFSX
											   | MCASP_PIN_ACLKX);
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� McASP Ϊ I2S ģʽ	                                */
/*                                                                          */
/****************************************************************************/
void McASPI2SConfigure(unsigned char transmitMode, unsigned char wordSize,
		unsigned char slotSize, unsigned int slotNum, unsigned char modeDMA)
{
	// ʹ�� McASP ģ�� PSC
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_MCASP0, PSC_POWERDOMAIN_ALWAYS_ON,
			 PSC_MDCTL_NEXT_ENABLE);

	if(transmitMode & MCASP_TX_MODE)
	{
		McASPI2STxConfigure(wordSize, slotSize, slotNum,  modeDMA);
	}

	if(transmitMode & MCASP_RX_MODE)
	{
		McASPI2SRxConfigure(wordSize, slotSize, slotNum,  modeDMA);
	}
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� McASP �ж�			                                */
/*                                                                          */
/****************************************************************************/
void McASPIntSetup(unsigned int cpuINT, void (*userISR)(void))
{
	// ��ʼ���ж�
	IntRegister(cpuINT, userISR);
	IntEventMap(cpuINT, SYS_INT_MCASP0_INT);
	IntEnable(cpuINT);
}

/****************************************************************************/
/*                                                                          */
/*              ���� McASP ���ͺͽ���			                            */
/*                                                                          */
/****************************************************************************/
void I2SDataTxRxActivate(unsigned char transmitMode)
{
	if(transmitMode & MCASP_TX_MODE)
	{
		// ����ʹ���ⲿʱ��
		McASPTxClkStart(SOC_MCASP_0_CTRL_REGS, MCASP_TX_CLK_EXTERNAL);

		// ����������
		McASPTxSerActivate(SOC_MCASP_0_CTRL_REGS);

		// ʹ��״̬��
		McASPTxEnable(SOC_MCASP_0_CTRL_REGS);

		// ��������0
		McASPTxBufWrite(SOC_MCASP_0_CTRL_REGS, MCASP_XSER_TX, 0);
	}

	if(transmitMode & MCASP_RX_MODE)
	{
		// ����ʹ���ⲿʱ��
		McASPRxClkStart(SOC_MCASP_0_CTRL_REGS, MCASP_RX_CLK_EXTERNAL);

		// ����������
		McASPRxSerActivate(SOC_MCASP_0_CTRL_REGS);

		// ʹ��״̬��
		McASPRxEnable(SOC_MCASP_0_CTRL_REGS);
	}
}













