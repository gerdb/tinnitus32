/**
 *  Project     tinnitus32
 *  @file		theremin.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Theremin functionality
 *
 *  @copyright	GPL3
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "../Drivers/BSP/STM32F4-Discovery/stm32f4_discovery.h"
#include "stm32f4xx_hal.h"
#include "theremin.h"
#include "pots.h"
#include "config.h"
#include <math.h>

uint16_t usCC[8];
int16_t ssWaveTable[4096];

// Linearization tables for pitch an volume
uint32_t ulVolLinTable[1024];

// Pitch
uint16_t usPitchCC;			// value of capture compare register
uint16_t usPitchLastCC; 	// last value (last task)
uint16_t usPitchPeriod;		// period of oscillator
int32_t slPitchOffset; 		// offset value (result of auto-tune)
int32_t slPitchPeriodeFilt;	// low pass filtered period
// Divider = 2exp(20)
// Wavetable: 4096
// DAC: 48kHz
// Audiofreq = 48000Hz * wavetablefrq / (2exp(20)) / 1024 tableentries * 2(channel left & right);
// wavetablefrq = Audiofreq / 48000Hz * 2exp(20) * 1024 /2
// wavetablefrq = Audiofreq * 11184.81066 ...
int32_t slPitch;			// pitch value
float fPitch;			// pitch value

float fPitchScale = 1.0f;
float fPitchShift = 1.0f;
float fVolScale = 1.0f;
union
{
	float f;
	int i;
} u;

float fVol = 0.0;			// volume value


uint16_t usVolCC;			// value of capture compare register
uint16_t usVolLastCC;		// last value (last task)
uint16_t usVolPeriod;		// period of oscillator
int32_t slVolOffset;		// offset value (result of auto-tune)
int32_t slVolPeriodeFilt;	// low pass filtered period
int32_t slVol;				// volume value
int32_t slVol2;				// volume value

uint32_t ulWaveTableIndex = 0;

// Auto-tune
int siAutotune = 0;			// Auto-tune down counter
uint32_t ulLedCircleSpeed;	// LED indicator speed
uint32_t ulLedCirclePos;	// LED indicator position
int32_t slMinPitchPeriode;	// minimum pitch value during auto-tune
int32_t slMinVolPeriode = 0;	// minimum volume value during auto-tune

uint16_t usDACValue;		// wave table output for audio DAC

extern TIM_HandleTypeDef htim1;	// Handle of timer for input capture

#ifdef DEBUG
	uint32_t ulStopwatch = 0;
	#define STOPWATCH_START() DWT->CYCCNT = 0;
	#define STOPWATCH_STOP() ulStopwatch = DWT->CYCCNT;
#else
	#define STOPWATCH_START() ;
	#define STOPWATCH_STOP() ;
#endif


/**
 * @brief Initialize the module
 *
 */
void THEREMIN_Init(void)
{
#ifdef DEBUG
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#endif

	// Read auto-tune values from virtual EEPRom
	slPitchOffset = CONFIG_Read_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H);
	slVolOffset = CONFIG_Read_SLong(EEPROM_ADDR_VOL_AUTOTUNE_H);


	for (int i = 0; i < 1024; i++)
	{
		if (i < 32)
		{
			ulVolLinTable[31 - i] = 596 + i * 4;

		}
		else
		{
			ulVolLinTable[i] = 1023
					- (pow(((double) i) / 1023.0, 0.25) * 1023.0);
		}

	}

	for (int i = 0; i < (4096); i++)
	{
		ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 4096);
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
}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask(void)
{
	int32_t slWavStep;


	// cycles: 71
	// fast pow approximation:
	// powf_fast() from https://github.com/ekmett/approximate/blob/master/cbits/fast.c
	u.f = fPitch * 0.0000001f * fPitchShift;
	u.i = (int) (fPitchScale * (u.i - 1064866805) + 1064866805);
	slWavStep = (int32_t) (u.f*10000000.0f);

	// cycles:12
	if (slWavStep > 0)
	{
		ulWaveTableIndex += slWavStep;
	}

	// cycles: 44
	// logarithmic scale of the volume raw value
	u.f = (float)(slVol);
	fVol =  (u.i - 1064866805) * 8.262958405176314e-8f; /* 1 / 12102203.0; */
	slVol = (int32_t)(((6.5f - fVol) * fVolScale)*1024.0f);

	// cycles: 18
	// Limit volume and pitch values
	if (slVol < 0)
	{
		slVol = 0;
	}
	if (slVol > 1023)
	{
		slVol = 1023;
	}

//	if (fVol < 0.0f)
//	{
//		fVol = 0.0f;
//	}
//	if (fVol > 0.99f)
//	{
//		fVol = 0.99f;
//	}
//	slVol2 = slVol;

	// cycles: 29
	// WAV output to audio DAC
	usDACValue =
			(ssWaveTable[ulWaveTableIndex >> 20 /* use only the 12MSB of the 32bit counter*/])
//					* fVol;
					* slVol / 1024;

	// cycles: 29
	// Get the input capture timestamps
	usPitchCC = htim1.Instance->CCR1;
	usVolCC = htim1.Instance->CCR2;

	// cycles: 26
	// Calculate the period by the capture compare value and
	// the last capture compare value
	usPitchPeriod = usPitchCC - usPitchLastCC;
	usVolPeriod = usVolCC - usVolLastCC;

	// cycles: 9
	if (usPitchPeriod < 2000)
		usPitchPeriod *= 2;
	if (usVolPeriod < 2000)
		usVolPeriod *= 2;

//	for (int i=0;i<7;i++)
//	{
//		usCC[i] = usCC[i+1];
//	}
//	usCC[7] = usPitchPeriod;

	// cycles: 29
	// Low pass filter period values
	// factor *1024 is necessary, because of the /1024 integer division
	// factor *1024 is necessary, because we want to oversample the input signal
	if (usPitchPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		slPitchPeriodeFilt += ((int16_t) (usPitchPeriod - 2048) * 1024 * 1024
				- slPitchPeriodeFilt) / 1024;
	}

	// cycles: 21
	// Low pass filter volume values
	if (usVolPeriod != 0)
	{
		//                                   11bit                10bit  10bit
		slVolPeriodeFilt += ((int16_t) (usVolPeriod - 2048) * 1024 * 1024
				- slVolPeriodeFilt) / 1024;
	}

	// cycles: 34
	fPitch = (float) ((slPitchPeriodeFilt - slPitchOffset) * 8);
	slVol = ((slVolPeriodeFilt - slVolOffset) / 16384);

	// cycles: 9
	// Store values for next task
	usPitchLastCC = usPitchCC;
	usVolLastCC = usVolCC;
}

/**
 * @brief 1ms task
 *
 */
void THEREMIN_1msTask(void)
{
	if (siAutotune == 0)
	{
		// Start autotune by pressing BUTTON_KEY
		if (BSP_PB_GetState(BUTTON_KEY) == GPIO_PIN_SET)
		{
			// 1.0sec auto-tune
			siAutotune = 1000;

			// Reset LED indicator and pitch and volume values
			ulLedCircleSpeed = siAutotune;
			ulLedCirclePos = 0;
			slPitchOffset = 0;
			slPitchPeriodeFilt = 0x7FFFFFFF;
			slMinPitchPeriode = 0x7FFFFFFF;
			slVolOffset = 0;
			slVolPeriodeFilt = 0x7FFFFFFF;
			slMinVolPeriode = 0x7FFFFFFF;
		}
	}
	else
	{
		// Find lowest pitch period
		if (slPitchPeriodeFilt < slMinPitchPeriode)
		{
			slMinPitchPeriode = slPitchPeriodeFilt;
		}
		// Find lowest volume period
		if (slVolPeriodeFilt < slMinVolPeriode)
		{
			slMinVolPeriode = slVolPeriodeFilt;
		}
		siAutotune--;

		// LED indicator
		ulLedCircleSpeed = siAutotune;
		ulLedCirclePos += ulLedCircleSpeed;
		BSP_LED_Off(((ulLedCirclePos / 32768) + 4 - 1) & 0x03);
		BSP_LED_On(((ulLedCirclePos / 32768) + 4) & 0x03);

		// Auto-tune is finished
		if (siAutotune == 0)
		{
			// Use minimum values for offset of pitch and volume
			slPitchOffset = slMinPitchPeriode;
			slVolOffset = slMinVolPeriode;// + 16384 * 128;

			CONFIG_Write_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H, slPitchOffset);
			CONFIG_Write_SLong(EEPROM_ADDR_VOL_AUTOTUNE_H,   slVolOffset);
		}
	}

	// pitch scale pot
	if (POTS_HasChanged(POT_PITCH_SCALE)) {
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchScale = powf(2, ((float)(POTS_GetScaledValue(POT_PITCH_SCALE)-2048))*0.000976562f /* 1/1024 */);
	}

	// pitch shift pot
	if (POTS_HasChanged(POT_PITCH_SHIFT)) {
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchShift = powf(2, ((float)(POTS_GetScaledValue(POT_PITCH_SHIFT)-2048))*0.000976562f /* 1/1024 */);
	}

	// volume scale pot
	if (POTS_HasChanged(POT_VOL_SCALE)) {
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fVolScale = powf(2, ((float)(POTS_GetScaledValue(POT_VOL_SCALE)-2048))*0.000976562f /* 1/1024 */);
	}


}

/**
 * @brief 1s task used for debugging purpose
 *
 */
void THEREMIN_1sTask(void)
{
#ifdef DEBUG
	if (siAutotune == 0)
	{
		//printf("%d %d\n", slVol2, (int32_t)(fVol*1000.0));
		//printf("Stopwatch %d\n", ulStopwatch);

	}
#endif
}
