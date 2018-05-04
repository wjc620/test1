/****************************************************************************/
/*                                                                          */
/*              ���ݴ������ӿƼ����޹�˾                                    */
/*                                                                          */
/*              Copyright 2015 Tronlong All rights reserved                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              ��Ƶ���ԣ�LINE_IN��ȡ��Ƶ���ݣ�LINE_OUT������ݣ��жϷ�ʽ�� */
/*                                                                          */
/*              2015��07��13��                  							*/
/*                                                                          */
/****************************************************************************/
#include <c6x.h>

#include "TL6748.h"                 // ���� DSP6748 �������������
#include "hw_types.h"
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

#include "AncProc.h"

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
// AIC3106 I2C�ӵ�ַ
#define I2C_SLAVE_CODEC_AIC31                 (0x18u)

// I2S ʹ��2�� slot
#define I2S_SLOTS                             (2u)

// ����/���� slot ��С
#define SLOT_SIZE                             (16u)

// ����/�������� word ��С. Word size <= Slot size
#define WORD_SIZE                             (16u)

/****************************************************************************/
/*                                                                          */
/*              ȫ�ֱ���                                                    */
/*                                                                          */
/****************************************************************************/
extern const signed short gadWgnTbl[];

#ifdef DEBUG
    float save_yn[76526];

    extern float siso_xx_data[];
    extern float siso_ee_data[];
    extern float siso_Ws_data[];

    extern float spmod_xx_data[];
    extern float spmod_dd_data[];

    // ���������������ܲ��ԣ�
    long long t_start, t_stop, t_overhead;
    int t_test[76526];
#endif

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/
static void InitAIC31I2S(void);
static void InitMcaspIntr(void);
// DSP �жϳ�ʼ��
static void InterruptInit(void);

/****************************************************************************/
/*                                                                          */
/*              McASP �жϺ���                                              */
/*                                                                          */
/****************************************************************************/
static void McASPIsr(void)
{
	unsigned int status;
	static unsigned int spkOut = 0, nearSpkIn = 0, nearChairIn = 0;
	static unsigned int buildModeSamCnt = 0, wgnPlaySamCnt = 0;
	float tmp1, tmp2, tmp3;

	IntEventClear(SYS_INT_MCASP0_INT);

	status = McASPTxStatusGet(SOC_MCASP_0_CTRL_REGS);

	if(status & MCASP_TX_STAT_CURRSLOT_EVEN)
	{
        // ������������� ����������
        OutputSample(spkOut);
        // ��ȡ��ͨ������ ��Դ�����������ɼ�
        nearSpkIn = InputSample();

        if(buildModeSamCnt < 80000)
        {
            tmp1 = (float)nearSpkIn;
            tmp2 = (float)gadWgnTbl[wgnPlaySamCnt++];
            if(wgnPlaySamCnt == 16000)
            {
                wgnPlaySamCnt = 0;
            }
            BuildMode(tmp1, tmp2, gafWs, LS_LEN);
        }
        else
        {
            tmp1 = (float)nearSpkIn;
            tmp2 = (float)nearChairIn;
            NoiceReduce(tmp2, tmp1, gafWs, LS_LEN, &tmp3);
            if(tmp3 > 65535)
            {
                tmp3 = 65535;
            }
            else if(tmp3 < 0)
            {
                tmp3 = 0;
            }
            spkOut = (unsigned int)tmp3;
        }

#ifdef DEBUG
        static int j = 0;
//        // ������ʼֵ
//        t_start = _itoll(TSCH, TSCL);
//        NoiceReduce(siso_ee_data[j], siso_xx_data[j], siso_Ws_data, LS_LEN, &save_yn[j]);
//        // ��������ֵ
//        t_stop = _itoll(TSCH, TSCL);
//        t_test[j] = (t_stop - t_start) - t_overhead;

        // ������ʼֵ
        t_start = _itoll(TSCH, TSCL);
        BuildMode(spmod_dd_data[j], spmod_xx_data[j], gafWs, LS_LEN);
        // ��������ֵ
        t_stop = _itoll(TSCH, TSCL);
        t_test[j] = (t_stop - t_start) - t_overhead;
        j++;
        if(j >= 76526)
        {
            asm(" SWBP 0 ");
            j = 0;
        }
#endif
	}
	else
	{
		// ��ȡ��ͨ������
	    nearChairIn = InputSample();
		// �������������
		OutputSample(0);
	}
}

/****************************************************************************/
/*                                                                          */
/*              ������                                                      */
/*                                                                          */
/****************************************************************************/
int main(void)
{
    // ʹ�ܻ���
	CacheEnableMAR((unsigned int)0xC0000000, (unsigned int)0x10000000);
	CacheEnable(L1PCFG_L1PMODE_32K | L1DCFG_L1DMODE_32K | L2CFG_L2MODE_256K);

#ifdef DEBUG
    // ���������������ܲ��ԣ�
    TSCL = 0;
    TSCH = 0;
    t_start = _itoll(TSCH, TSCL);
    t_stop = _itoll(TSCH, TSCL);
    t_overhead = t_stop - t_start;
#endif

	// �����ն˳�ʼ�� ʹ�ô���2
    UARTStdioInit();

    UARTPuts("\r\n ============Test Start===========.\r\n", -1);
	UARTPuts("Welcome to StarterWare Audio_Line_In_Intr Demo application.\r\n\r\n", -1);
	UARTPuts("This application loops back the input at LINE_IN of the EVM to the LINE_OUT of the EVM\r\n\r\n", -1);

    // I2C ģ����������
    I2CPinMuxSetup(0);

    // McASP ��������
    McASPPinMuxSetup();

    // DSP �жϳ�ʼ��
    InterruptInit();

    // ��ʼ�� I2C �ӿڵ�ַΪ AIC31 �ĵ�ַ
    I2CSetup(SOC_I2C_0_REGS, I2C_SLAVE_CODEC_AIC31);
    I2CIntRegister(C674X_MASK_INT5, SYS_INT_I2C0_INT);

	// ��ʼ�� AIC31 ��ƵоƬ
    InitAIC31I2S();

	// ��ʼ�� McASP Ϊ�жϷ�ʽ
    InitMcaspIntr();

    while(1);
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� AIC31 ��ƵоƬ                                       */
/*                                                                          */
/****************************************************************************/
static void InitAIC31I2S(void)
{
    volatile unsigned int delay = 0xFFF;

    // ��λ
    AIC31Reset(SOC_I2C_0_REGS);
    while(delay--);

    // ��ʼ�� AIC31 Ϊ I2S ģʽ��slot �Ĵ�С
    AIC31DataConfig(SOC_I2C_0_REGS, AIC31_DATATYPE_I2S, SLOT_SIZE, 0);

    // ��ʼ��������Ϊ 8000Hz
    AIC31SampleRateConfig(SOC_I2C_0_REGS, AIC31_MODE_BOTH, FS_8000_HZ);

    // ��ʼ�� ADC 0�ֱ�����,���� LINE IN
    AIC31ADCInit(SOC_I2C_0_REGS, ADC_GAIN_0DB, AIC31_LINE_IN);

    // ��ʼ�� DAC 0�ֱ�˥��
    AIC31DACInit(SOC_I2C_0_REGS, DAC_ATTEN_0DB);
}

/****************************************************************************/
/*                                                                          */
/*              ��ʼ�� McASP Ϊ�жϷ�ʽ                                     */
/*                                                                          */
/****************************************************************************/
static void InitMcaspIntr(void)
{
	// ��ʼ�� McASP Ϊ I2S ģʽ
	McASPI2SConfigure(MCASP_BOTH_MODE, WORD_SIZE, SLOT_SIZE, I2S_SLOTS, MCASP_MODE_NON_DMA);

	// ʹ�ܷ����ж�
	McASPTxIntEnable(SOC_MCASP_0_CTRL_REGS, MCASP_TX_DATAREADY);

	// ���� McASP �ж�
	McASPIntSetup(C674X_MASK_INT6, McASPIsr);

	// ���� McASP ���ͺͽ���
	I2SDataTxRxActivate(MCASP_BOTH_MODE);
}

/****************************************************************************/
/*                                                                          */
/*              DSP �жϳ�ʼ��                                              */
/*                                                                          */
/****************************************************************************/
static void InterruptInit(void)
{
	// ��ʼ�� DSP �жϿ�����
	IntDSPINTCInit();

	// ʹ�� DSP ȫ���ж�
	IntGlobalEnable();
}




