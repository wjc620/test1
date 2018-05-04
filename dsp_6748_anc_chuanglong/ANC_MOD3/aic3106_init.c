/****************************************************************************/
/*                                                                          */
/*              ���ݴ������ӿƼ����޹�˾                                    */
/*                                                                          */
/*              Copyright 2015 Tronlong All rights reserved                 */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/*                                                                          */
/*              AIC3106��ʼ��                                               */
/*                                                                          */
/*              2015��07��20��                                              */
/*                                                                          */
/****************************************************************************/
#include "codecif.h"
#include "aic3106_init.h"

/****************************************************************************/
/*                                                                          */
/*              �궨��                                                      */
/*                                                                          */
/****************************************************************************/
#define AIC31_RESET                 (0x80)

#define AIC31_SLOT_WIDTH_16         (0u << 4u)
#define AIC31_SLOT_WIDTH_20         (1u << 4u)
#define AIC31_SLOT_WIDTH_24         (2u << 4u)
#define AIC31_SLOT_WIDTH_32         (3u << 4u)

/****************************************************************************/
/*                                                                          */
/*              ��������                                                    */
/*                                                                          */
/****************************************************************************/
/**
 *  ����	��λAIC3106
 *
 *  ����	baseAddr :���� AIC3106 �ӿڵĻ���ַ��I2C�Ļ���ַ��
 *
 *  ����	��
 *
 **/
void AIC31Reset(unsigned int baseAddr)
{
	// ѡ�� Page 0
    I2CRegWrite(baseAddr, AIC31_P0_REG0, 0);

    // ��λ
    I2CRegWrite(baseAddr, AIC31_P0_REG1, AIC31_RESET);
}

/**
 *  ����	��ʼ�� AIC3106 ����ģʽ�� slot λ��
 *
 *  ����	baseAddr : ���� AIC3106 �ӿڵĻ���ַ��I2C�Ļ���ַ��
 *  		dataType : ������������ģʽ
 *  		slotWidth: ���� Slot ��λ��
 *  		dataOff  : ����ʱ�ӵ������ص�������Ч���ݵ�ʱ�Ӹ���
 *
 *  			dataType ��ֵ��������Ϊ��
 *  				AIC31_DATATYPE_I2S - I2S ģʽ \n
 *              	AIC31_DATATYPE_DSP - DSP ģʽ \n
 *              	AIC31_DATATYPE_RIGHTJ - �Ҷ���ģʽ \n
 *              	AIC31_DATATYPE_LEFTJ -  �����ģʽ \n
 *
 *  ����	��
 *
 **/
void AIC31DataConfig(unsigned int baseAddr, unsigned char dataType, 
                     unsigned char slotWidth, unsigned char dataOff)
{
    unsigned char slot;

    switch(slotWidth)
    {
        case 16:
            slot = AIC31_SLOT_WIDTH_16;
        break;

        case 20:
            slot = AIC31_SLOT_WIDTH_20;
        break;

        case 24:
            slot = AIC31_SLOT_WIDTH_24;
        break;

        case 32:
            slot = AIC31_SLOT_WIDTH_32;
        break;

        default:
            slot = AIC31_SLOT_WIDTH_16;
        break;
    }

    // ��������ģʽ�� slot λ��
    I2CRegWrite(baseAddr, AIC31_P0_REG9, (dataType | slot));
  
    // ��ʱ�ӵ������ص�������Ч���ݵ�ʱ�Ӹ���
    I2CRegWrite(baseAddr, AIC31_P0_REG10, dataOff);
}

/**
 *  ����	���� AIC3106 �Ĳ�����
 *
 *  ����	baseAddr  : ���� AIC3106 �ӿڵĻ���ַ��I2C�Ļ���ַ��
 *  		mode      : ѡ�� AIC3106 �ڲ��� DAC ���� ADC
 *  		sampleRate: ������
 *
 *  			mode ��ֵ��������Ϊ��
 *  				AIC31_MODE_ADC -  ѡ�� ADC \n
 *                	AIC31_MODE_DAC -  ѡ�� DAC \n
 *                	AIC31_MODE_BOTH -  ѡ�� ADC �� DAC \n
 *  			sampleRate ��ֵ��������Ϊ��
 *  				FS_8000_HZ ,FS_11025_HZ ,FS_16000_HZ ,FS_22050_HZ ,FS_24000_HZ
 *  				FS_32000_HZ ,S_44100_HZ ,FS_48000_HZ ,FS_96000_HZ
 *  			������ fs �����µ�ʽ�Ƶ���
 *  				fs = (PLL_IN * [pllJval.pllDval] * pllRval) /(2048 * pllPval).
 *  			���� PLL_IN ���ⲿ�������� 	PLL_IN = 24576 kHz
 *
 *  ����	��
 *
 **/
void AIC31SampleRateConfig(unsigned int baseAddr, unsigned int mode, 
                           unsigned int sampleRate)
{
    unsigned char fs;
    unsigned char ref = 0x0Au;
    unsigned char temp;
    unsigned char pllPval = 4u;
    unsigned char pllRval = 1u;
    unsigned char pllJval = 16u; 
    unsigned short pllDval = 0u;

    // �����ʲ���ѡ��
    switch(sampleRate)
    {
        case 8000:
            fs = 0xAAu;
        break;

        case 11025:
            fs = 0x66u;
            ref = 0x8Au;
            pllJval = 14u;
            pllDval = 7000u;
        break;

        case 16000:
            fs = 0x44u;
        break;

        case 22050:
            fs = 0x22u;
            ref = 0x8Au;
            pllJval = 14u;
            pllDval = 7000u;
        break;

        case 24000:
            fs = 0x22u;
        break;
    
        case 32000:
            fs = 0x11u;
        break;

        case 44100:
            ref = 0x8Au;
            fs = 0x00u;
            pllJval = 14u;
            pllDval = 7000u;
        break;

        case 48000:
            fs = 0x00u;
        break;

        case 96000:
            ref = 0x6Au;
            fs = 0x00u;
        break;

        default:
            fs = 0x00u;
        break;
    }
    
    temp = (mode & fs);
   
    // ���ò�����
    I2CRegWrite(baseAddr, AIC31_P0_REG2, temp);
  
    I2CRegWrite(baseAddr, AIC31_P0_REG3, 0x80 | pllPval);

    // ʹ�� PLLCLK_IN ��Ϊ MCLK
    I2CRegWrite(baseAddr, AIC31_P0_REG102, 0x08);

    // ʹ�� PLLDIV_OUT ��Ϊ CODEC_CLKIN
    I2CRegBitClr(baseAddr, AIC31_P0_REG101, 0x01);

    // GPIO1 ѡ�������Ƶ���PLL IN
    I2CRegWrite(baseAddr, AIC31_P0_REG98, 0x20);

    temp = (pllJval << 2);
    I2CRegWrite(baseAddr, AIC31_P0_REG4, temp);

    // ��ʼ�� PLL ��Ƶ�Ĵ���
    I2CRegWrite(baseAddr, AIC31_P0_REG5, (pllDval >> 6) & 0xFF);
    I2CRegWrite(baseAddr, AIC31_P0_REG6, (pllDval & 0x3F) << 2);

    temp = pllRval;
    I2CRegWrite(baseAddr, AIC31_P0_REG11, temp);

    // ʹ�ܱ��������Ϊ�����������fs �� bclk
    I2CRegWrite(baseAddr, AIC31_P0_REG8, 0xD0);

    I2CRegWrite(baseAddr, AIC31_P0_REG7, ref);
}

/**
 *  ����	��ʼ�� AIC3106 �� ADC �����������
 *
 *  ����	baseAddr  : ���� AIC3106 �ӿڵĻ���ַ��I2C�Ļ���ַ��
 *  		adcGain   : ADC ���������
 *  		inSource  : ģ���ź�����Դ
 *
 *  			adcGain ��ֵ��������Ϊ�� 0~59.5����ֵΪ0.5�ı���
 *  			inSource��ֵ��������Ϊ��
 *  				AIC31_LINE_IN -  ѡ�� LINE IN \n
 *  				AIC31_MIC_IN -  ѡ�� MIC IN \n
 *
 *  ����	��
 *
 **/
void AIC31ADCInit(unsigned int baseAddr, float adcGain, unsigned char inSource)
{
    unsigned char adc_gain = adcGain/0.5;

	// �������������� ADC ����
    I2CRegWrite(baseAddr, AIC31_P0_REG15, adc_gain);
    I2CRegWrite(baseAddr, AIC31_P0_REG16, adc_gain);

    // �����ʲ���ѡ��
	switch(inSource)
	{
		case AIC31_LINE_IN:
			// MIC3L/R �����ӵ��� ADC PGA
			I2CRegWrite(baseAddr, AIC31_P0_REG17, 0xFF);

			// MIC3L/R �����ӵ��� ADC PGA
			I2CRegWrite(baseAddr, AIC31_P0_REG18, 0xFF);

			// Line L1R �ϵ�
			I2CRegWrite(baseAddr, AIC31_P0_REG19, 0x04);

			// Line L1L �ϵ�
			I2CRegWrite(baseAddr, AIC31_P0_REG22, 0x04);
		break;

		case AIC31_MIC_IN:
			// ʧ�� LINE IN ���ӵ� ADC
			I2CRegWrite(baseAddr, AIC31_P0_REG19, 0x7C);
			I2CRegWrite(baseAddr, AIC31_P0_REG22, 0x7C);

			// ���� MIC IN ���ӵ� ADC
			I2CRegWrite(baseAddr, AIC31_P0_REG17, 0x0F);
			I2CRegWrite(baseAddr, AIC31_P0_REG18, 0xF0);

			// MIC IN �ϵ�
			I2CRegWrite(baseAddr, AIC31_P0_REG25, 0xc0);
		break;
	}


}

/**
 *  ����	��ʼ�� AIC3106 �� DAC ��������˥��
 *
 *  ����	baseAddr  : ���� AIC3106 �ӿڵĻ���ַ��I2C�Ļ���ַ��
 *  		dacAtten  : DAC ������˥��
 *
 *  			dacAtten ��ֵ��������Ϊ�� 0~63.5����ֵΪ0.5�ı���
 *
 *  ����	��
 *
 **/
void AIC31DACInit(unsigned int baseAddr, float dacAtten)
{
    unsigned char dac_atten = dacAtten/0.5;

	// �������� DACs �ϵ�
    I2CRegWrite(baseAddr, AIC31_P0_REG37, 0xE0);

    // ѡ�� DAC L1 R1 ·��
    I2CRegWrite(baseAddr, AIC31_P0_REG41, 0x02);
    I2CRegWrite(baseAddr, AIC31_P0_REG42, 0x6C);


    // DAC L ���ӵ� HPLOUT
    I2CRegWrite(baseAddr, AIC31_P0_REG47, 0x80);
    I2CRegWrite(baseAddr, AIC31_P0_REG51, 0x09);

    // DAC R ���ӵ� HPROUT
    I2CRegWrite(baseAddr, AIC31_P0_REG64, 0x80);
    I2CRegWrite(baseAddr, AIC31_P0_REG65, 0x09);

    // DACL1 ���ӵ� LINE1 LOUT
    I2CRegWrite(baseAddr, AIC31_P0_REG82, 0x80);
    I2CRegWrite(baseAddr, AIC31_P0_REG86, 0x09);

    // DACR1 ���ӵ� LINE1 ROUT
    I2CRegWrite(baseAddr, AIC31_P0_REG92, 0x80);
    I2CRegWrite(baseAddr, AIC31_P0_REG93, 0x09);

    // ���� DAC ˥��
    I2CRegWrite(baseAddr, AIC31_P0_REG43, dac_atten);
    I2CRegWrite(baseAddr, AIC31_P0_REG44, dac_atten);
}

/***************************** End Of File ***********************************/
