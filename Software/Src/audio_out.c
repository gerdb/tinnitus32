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

volatile int a=0, b=0;

uint16_t		wav = 0;
__IO uint32_t I2S_DR;
static AUDIO_DrvTypeDef           *pAudioDrv;
//static I2C_HandleTypeDef    I2cHandle;

/* Initial Volume level (from 0 (Mute) to 100 (Max)) */
static uint8_t Volume = 55;

//uint32_t I2cxTimeout = I2Cx_TIMEOUT_MAX;    /*<! Value of Timeout when I2C communication fails */

extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim1;

uint16_t cc[8];
uint16_t lastcc = 0;
uint16_t nowcc = 0;
int16_t wavetable[4*1024];
uint16_t pitch_period;
// Divider = 2exp(20)
// Wavetable: 4096
// DAC: 48kHz
// Audiofreq = 48000Hz * wavetablefrq / (2exp(20)) / 1024 tableentries * 2(channel left & right);
// wavetablefrq = Audiofreq / 48000Hz * 2exp(20) * 1024 /2
// wavetablefrq = Audiofreq * 11184.81066 ...
uint32_t wavetablefrq = 1000 * 11184.810666667;
uint32_t wavetableptr = 0;
uint16_t wavetableindex = 0;
volatile int cnt = 0;

int32_t pitch_periodeFiltL = 0;
int32_t pitch_periodeFilt = 0;

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

	int16_t spitch_period;
	cnt ++;
	if (cnt == 96000)
		cnt = 0;
	if (cnt < 48000)
		BSP_LED_On(LED4);
	else
		BSP_LED_Off(LED4);

	wavetableptr += wavetablefrq;
//	wav += 128;
//	hi2s3.Instance->DR = wav;
	BSP_LED_On(LED3);
	hi2s3.Instance->DR = wavetable[wavetableptr >> 20 /* use only the 12MSB of the 32bit counter*/];
	BSP_LED_Off(LED3);

	nowcc = htim1.Instance->CCR2;






	pitch_period = nowcc-lastcc;

	if (pitch_period < 2000)
		pitch_period *= 2;

	spitch_period = pitch_period - 2376;

	for (int i=0;i<7;i++)
	{
		cc[i] = cc[i+1];
	}
	cc[7] = pitch_period;

	a++;

	if (a>1000)
	{
		if (pitch_period == 0)
		{

		}
		else if (spitch_period > 100 || spitch_period < -100)
		{
			b++;
			if (b>10) {
				b+=100;
			}
		}
		else
		{
			//                          4          12      10      6
			pitch_periodeFilt += (spitch_period *  1024 * 1024  - pitch_periodeFilt) / 1024;
			//wavetablefrq = spitch_period * 5120000;
		}
	}

	//pitch_periodeFiltL += spitch_period - pitch_periodeFilt;
	//pitch_periodeFilt = pitch_periodeFiltL / 1;

	//wavetablefrq = spitch_period * 512000;
	wavetablefrq = pitch_periodeFilt * (4096 / 1024) ;

	lastcc = nowcc;

}


