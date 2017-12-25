/*
 * Audio.c
 *
 *  Created on: Nov 27, 2017
 *      Author: gerd
 */


#include "../Drivers/BSP/Components/cs43l22/cs43l22.h"
#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "../Drivers/BSP/Components/Common/audio.h"
#include "stm32f4xx_hal.h"
#include "audio_out.h"
#include <math.h>



__IO uint32_t I2S_DR;
static AUDIO_DrvTypeDef           *pAudioDrv;
//static I2C_HandleTypeDef    I2cHandle;

/* Initial Volume level (from 0 (Mute) to 100 (Max)) */
static uint8_t Volume = 60;

//uint32_t I2cxTimeout = I2Cx_TIMEOUT_MAX;    /*<! Value of Timeout when I2C communication fails */

extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim1;

uint16_t cc[8];
int16_t wavetable[4*1024];

uint16_t usPitchCC = 0;
uint16_t usPitchLastCC = 0;
uint16_t usPitchPeriod;
int16_t spitch_period;

int32_t siPitchOffset = 0;
int32_t siPitchPeriodeFilt = 0;
uint16_t usVolCC = 0;
uint16_t usVolLastCC = 0;
uint16_t usVolPeriod;
int32_t siVolOffset = 0;
int32_t siVolPeriodeFilt = 0;

// Divider = 2exp(20)
// Wavetable: 4096
// DAC: 48kHz
// Audiofreq = 48000Hz * wavetablefrq / (2exp(20)) / 1024 tableentries * 2(channel left & right);
// wavetablefrq = Audiofreq / 48000Hz * 2exp(20) * 1024 /2
// wavetablefrq = Audiofreq * 11184.81066 ...
uint32_t wavetablefrq = 1000 * 11184.810666667;
uint32_t wavetableptr = 0;
uint16_t wavetableindex = 0;

int32_t siVol=0;

void AUDIO_OUT_Init(void)
{
	  uint8_t ret = AUDIO_OK;

	  for (int i = 0; i<(4*1024); i++)
	  {
		  wavetable[i] = 32767 * sin((i*2*M_PI)/1024);
	  }


//	  for (int i = 0; i<1024; i++)
//	  {
//		  wavetable[i] = 32767 * sin(((256*3+i)*2*M_PI)/1024);
//	  }
//	  for (int i = 1024; i<(4*1024); i++)
//	  {
//		  wavetable[i] = -32767;
//	  }

//	  for (int i = 0; i<(4*1024); i++)
//	  {
//		  wavetable[i] = 32767 * sin((((i*1)+100*sin((i*0.1*M_PI)/1024))*M_PI)/1024);
//	  }

	  if(ret == AUDIO_OK)
	  {
	    /* Retieve audio codec identifier */
	    if(((cs43l22_drv.ReadID(AUDIO_I2C_ADDRESS)) & CS43L22_ID_MASK) == CS43L22_ID)
	    {
	      /* Initialize the audio driver structure */
	      pAudioDrv = &cs43l22_drv;
	    }
	    else
	    {
	      ret = AUDIO_ERROR;
	    }
	  }

	  if(ret == AUDIO_OK)
	  {
	    pAudioDrv->Init(AUDIO_I2C_ADDRESS, OUTPUT_DEVICE_HEADPHONE, Volume, 48000);
	  }

	  if(ret == AUDIO_OK)
	  {
		  /* Call the audio Codec Play function */
		  if(pAudioDrv->Play(AUDIO_I2C_ADDRESS, 0, 0) != 0)
		  {
			  ret = AUDIO_ERROR;
		  }
		  else
		  {
		    /* Update the Media layer and enable it for play */
		    //HAL_I2S_Transmit_DMA(&hAudioOutI2s, pBuffer, DMA_MAX(Size/AUDIODATA_SIZE));

		    /* Return AUDIO_OK when all operations are correctly done */
		    ret = AUDIO_OK;
		  }
	  }

	  if(ret == AUDIO_OK)
	  {
		    /* Enable TXE and ERR interrupt */
		    __HAL_I2S_ENABLE_IT(&hi2s3, (I2S_IT_TXE | I2S_IT_ERR));

		    /* Check if the I2S is already enabled */
		    if((hi2s3.Instance->I2SCFGR &SPI_I2SCFGR_I2SE) != SPI_I2SCFGR_I2SE)
		    {
		      /* Enable I2S peripheral */
		      __HAL_I2S_ENABLE(&hi2s3);
		    }

		    hi2s3.Instance->DR = 0;
	  }


}

/*
 * This IRQ Handler is called every 96kHz, to send a new sample
 *  first left then right audio channel to the DAC over the I2S bus.
 *  The Audio frequency is 48kHz
 */
void AUDIO_OUT_I2S_IRQHandler(void)
{


	wavetableptr += wavetablefrq;

	// WAV output to audio DAC
	hi2s3.Instance->DR = (wavetable[wavetableptr >> 20 /* use only the 12MSB of the 32bit counter*/]) * siVol / 256;

	// Get the input capture timestamps
	usPitchCC = htim1.Instance->CCR1;
	usVolCC = htim1.Instance->CCR2;

	// Calculate the periode
	usPitchPeriod = usPitchCC-usPitchLastCC;
	usVolPeriod = usVolCC-usVolLastCC;

	if (usPitchPeriod < 2000)
		usPitchPeriod *= 2;
	if (usVolPeriod < 2000)
		usVolPeriod *= 2;


//	for (int i=0;i<7;i++)
//	{
//		cc[i] = cc[i+1];
//	}
//	cc[7] = usPitchPeriod;

	if (usPitchPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		siPitchPeriodeFilt += ((int16_t)(usPitchPeriod - 2048) *  1024 * 1024  - siPitchPeriodeFilt) / 1024;
	}

	if (usVolPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		siVolPeriodeFilt += ((int16_t)(usVolPeriod - 2048) *  1024 * 1024  - siVolPeriodeFilt) / 1024;
	}

	wavetablefrq = (siPitchPeriodeFilt - siPitchOffset) * (4096 / 1024) ;
	siVol = 256 - ((siVolPeriodeFilt - siVolOffset) / 65536);
	if (siVol<0)
	{
		siVol = 0;
	}
	if (siVol>256)
	{
		siVol = 256;
	}


	usPitchLastCC = usPitchCC;
	usVolLastCC = usVolCC;

}


