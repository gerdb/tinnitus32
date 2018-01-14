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
#include "audio_out.h"
#include "theremin.h"
#include "pots.h"
#include "config.h"
#include "usb_stick.h"
#include <math.h>

uint16_t usCC[8];
int16_t ssWaveTable[4096];

// Linearization tables for pitch an volume
uint32_t ulVolLinTable[1024];
uint32_t ulPitchLinTable[2048];

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

float fVol = 0.0;			// volume value
uint16_t usVolCC;			// value of capture compare register
uint16_t usVolLastCC;		// last value (last task)
uint16_t usVolPeriod;		// period of oscillator
int32_t slVolOffset;		// offset value (result of auto-tune)
int32_t slVolPeriodeFilt;	// low pass filtered period
int32_t slVol;				// volume value
int32_t slVolFiltL;			// volume value, filtered (internal filter value)
int32_t slVolFilt;			// volume value, filtered

float fVolScale = 1.0f;
float fVolShift = 0.0f;

//int32_t slVol2;				// volume value

int32_t test;

uint32_t ulWaveTableIndex = 0;

// Auto-tune
int siAutotune = 0;			// Auto-tune down counter
uint32_t ulLedCircleSpeed;	// LED indicator speed
uint32_t ulLedCirclePos;	// LED indicator position
int32_t slMinPitchPeriode;	// minimum pitch value during auto-tune
int32_t slMinVolPeriode = 0;	// minimum volume value during auto-tune

uint16_t usDACValue;		// wave table output for audio DAC

e_waveform eWaveform = SINE;

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

	// Get the VolumeShift value from the flash configuration
	fVolShift = ((float) (CONFIG.VolumeShift)) * 0.1f + 11.5f;

	// 8 Waveforms
	strPots[POT_WAVEFORM].iMaxValue = 8;

	// Calculate the LUT for volume and pitch
	THEREMIN_Calc_VolumeTable();
	THEREMIN_Calc_PitchTable();
	THEREMIN_Calc_WavTable();

}

/**
 * @brief Recalculates the pitch LUT
 *
 */
void THEREMIN_Calc_PitchTable(void)
{
	floatint_ut u;
	uint32_t val;
	float f;

	for (int32_t i = 0; i < 2048; i++)
	{
		// Calculate back the x values of the table
		u.ui = (i << 17) + 1065353216;
		// And now calculate the precise log2 value instead of only the approximation
		// used for x values in THEREMIN_96kHzDACTask(void) when using the table.
		//	u.f = fPitch * 0.0000001f * fPitchShift;
		//	u.i = (int) (fPitchScale * (u.i - 1064866805) + 1064866805);
		//	slWavStep = (int32_t) (u.f*10000000.0f);

		f = expf(logf(u.f * 0.0000001f * fPitchShift) * fPitchScale)
				* 10000000.0f;

		// Convert it to integer
		val = (uint32_t) (f);
		// Limit it to maximum
		if (val > 500000000)
		{
			val = 500000000;
		}
		// Fill the pitch table
		ulPitchLinTable[i] = val;
	}
}

/**
 * @brief Recalculates the volume LUT
 *
 */
void THEREMIN_Calc_VolumeTable(void)
{
	floatint_ut u;
	uint32_t val;
	float f;

	for (int32_t i = 0; i < 1024; i++)
	{
		// Calculate back the x values of the table
		u.ui = (i << 17) + 1065353216;
		// And now calculate the precise log2 value instead of only the approximation
		// used for x values in THEREMIN_96kHzDACTask(void) when using the table.
		f = (fVolShift - log2f(u.f)) * 300.0f * fVolScale;

		// Limit the float value before we square it;
		if (f > 1024.0)
			f = 1024.0;
		if (f < 0.0)
			f = 0.0;

		// Square the volume value
		val = (uint32_t) ((f * f) * 0.000976562f); /* =1/1024 */
		;
		// Limit it to maximum
		if (val > 1023)
		{
			val = 1023;
		}

		// Fill the volume table
		ulVolLinTable[i] = val;
	}
}


/**
 * @brief Recalculates the wave LUT
 *
 */
void THEREMIN_Calc_WavTable(void)
{
	switch (eWaveform)
	{
	case SINE:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024);
		}
		break;

	case CAT:
		for (int i = 0; i < 4096; i++)
		{
			if (i < 1024)
			{
				ssWaveTable[i] = 0;
			}
			else
			{
				ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024);
			}
		}
		break;

	case SAWTOOTH:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = (i & 0x03FF)*64-32768;
		}
		break;

	case USBSTICK:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
		if (bMounted)
		{
			USB_STICK_ReadWAVFile("WAV1.WAV");
			USB_STICK_ReadCFile("WAV1.C");
		}
		break;

	default:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
	}


}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask(void)
{
	int32_t p1, p2, tabix, tabsub;
	floatint_ut u;
	uint32_t ulWavStep;

	// cycles: 59..62
	// fast pow approximation by LUT with interpolation
	// float bias: 127, so 127 << 23bit mantissa is: 1065353216
	// We use (5 bit of) the exponent and 6 bit of the mantissa

	if (fPitch < 1.0f)
	{
		fPitch = 1.0f;
	}

	u.f = fPitch;
	tabix = ((u.ui - 1065353216) >> 17);
	tabsub = (u.ui & 0x0001FFFF) >> 2;
	p1 = ulPitchLinTable[tabix];
	p2 = ulPitchLinTable[tabix + 1];
	ulWavStep = p1 + ((((p2 - p1) >> 8) * tabsub) >> 7);
	ulWaveTableIndex += ulWavStep >> 2;

	//test = ulWavStep;

	// cycles: 39 .. 43
	// scale of the volume raw value by LUT with interpolation
	// float bias: 127, so 127 << 23bit mantissa is: 1065353216
	// We use (4 bit of) the exponent and 6 bit of the mantissa
	if (slVol < 1)
	{
		slVol = 1;
	}
	if (slVol > 32767)
	{
		slVol = 32767;
	}
	u.f = (float) (slVol);
	tabix = ((u.ui - 1065353216) >> 17);
	tabsub = u.ui & 0x0001FFFF;
	p1 = ulVolLinTable[tabix];
	p2 = ulVolLinTable[tabix + 1];
	slVol = p1 + (((p2 - p1) * tabsub) >> 17);

	// cycles
	// Low pass filter the output to avoid aliasing noise.
	slVolFiltL += slVol - slVolFilt;
	slVolFilt = slVolFiltL / 1024;

	// cycles: 29..38
	// WAV output to audio DAC
	tabix = ulWaveTableIndex >> 20; // use only the 12MSB of the 32bit counter
	tabsub = (ulWaveTableIndex >> 12) & 0x000000FF;
	p1 = ssWaveTable[tabix];
	p2 = ssWaveTable[(tabix + 1) & 0x0FFF];
	usDACValue = (p1 + (((p2 - p1) * tabsub) >> 8)) * slVolFilt / 1024;

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
	slVol = ((slVolPeriodeFilt - slVolOffset) / 4096);

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

			// Mute the output
			bMute = 1;
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
			// activate output
			bMute = 0;

			// Use minimum values for offset of pitch and volume
			slPitchOffset = slMinPitchPeriode;
			slVolOffset = slMinVolPeriode;	// + 16384 * 128;

			CONFIG_Write_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H, slPitchOffset);
			CONFIG_Write_SLong(EEPROM_ADDR_VOL_AUTOTUNE_H, slVolOffset);
		}
	}

	// pitch scale pot
	if (POTS_HasChanged(POT_PITCH_SCALE))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchScale = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SCALE) - 2048))
						* 0.000976562f /* 1/1024 */);
		THEREMIN_Calc_PitchTable();
	}

	// pitch shift pot
	if (POTS_HasChanged(POT_PITCH_SHIFT))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchShift = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SHIFT) - 2048))
						* 0.000976562f /* 1/1024 */);
		THEREMIN_Calc_PitchTable();
	}

	// volume scale pot
	if (POTS_HasChanged(POT_VOL_SCALE))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fVolScale = powf(2,
				((float) (POTS_GetScaledValue(POT_VOL_SCALE) - 2048))
						* 0.000976562f /* 1/1024 */);
		THEREMIN_Calc_VolumeTable();
	}

	// waveform pot
	if (POTS_HasChanged(POT_WAVEFORM))
	{
		eWaveform = POTS_GetScaledValue(POT_WAVEFORM);
		THEREMIN_Calc_WavTable();
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
		//printf("%d %d\n", (uint32_t)fPitch, test);
		//printf("Stopwatch %d\n", ulStopwatch);
	}
#endif
}
