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

float fNonLinTable[1024+1];

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
int iWavMask = 0x0FFF;
int iWavLength = 4096;
int bUseNonLinTab = 0;
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
 * @brief Sets the length ov the wave LUT
 * @length of the LUT, only 2expN values <= 4096 are valid
 *
 */
void THEREMIN_SetWavelength(int length)
{
	iWavLength = length;
	iWavMask = length - 1;
}

/**
 * @brief Recalculates the wave LUT
 *
 */
void THEREMIN_Calc_WavTable(void)
{
	int bLpFilt = 0;
	float a0=0.0f;
	float a1=0.0f;
	float a2=0.0f;
	float b1=0.0f;
	float b2=0.0f;

	// Mute as long as new waveform is beeing calculated
	bMute = 1;
	THEREMIN_SetWavelength(4096);
	bUseNonLinTab = 0;

	if (eWaveform != USBSTICK)
	{
		bWavLoaded = 0;
	}

	switch (eWaveform)
	{
	case SINE:
		for (int i = 0; i < 1024; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024);
		}
		THEREMIN_SetWavelength(1024);
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
		// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
		// 48kHz, Fc=60, Q=0.7071, A=0dB

		a0 = 0.00001533600755608856f;
		a1 = 0.00003067201511217712f;
		a2 = 0.00001533600755608856f;
		b1 = -1.9888928005576803f;
		b2 = 0.9889541445879048f;
		bLpFilt = 1;
		break;

	case COSPULSE:
		for (int i = 0; i < 4096; i++)
		{
			if (i < 512)
			{
				ssWaveTable[i] = - 32767 * cos((i * 2 * M_PI) / 512);
			}
			else
			{
				ssWaveTable[i] = -32767;
			}
		}
		break;


	case HARMON:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 16384 * (0.8f*sin((i * 2.0f * M_PI) / 1024) + 1.0f * sin((i * 6.0f * M_PI) / 1024)) ;
		}
		break;

	case RECTIFIED:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 65535 * sin((i * 2 * M_PI) / 2048) - 32768;
		}
		THEREMIN_SetWavelength(1024);

		break;

	case GLOTTAL:

		// Based on:
		// http://www.fon.hum.uva.nl/praat/manual/PointProcess__To_Sound__phonation____.html
		for (int i = 0; i < 768; i++)
		{
			ssWaveTable[i] = (int32_t)(621368.0 * (powf((float)i / 768.0f, 3) - powf((float)i / 768.0f, 4))) - 32768 ;
		}
		for (int i = 768; i < 1024; i++)
		{
			ssWaveTable[i] = -32768 ;
		}
		THEREMIN_SetWavelength(1024);

		// http://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
		// 48kHz, Fc=400, Q=0.7071, A=0dB

		a0 = 0.0006607788720867079f;
		a1 = 0.0013215577441734157f;
		a2 = 0.0006607788720867079f;
		b1 = -1.9259833105871227f;
		b2 = 0.9286264260754695f;
		bLpFilt = 1;

		break;

	case THEREMIN:
		for (int i = 0; i < 1024; i++)
		{
			ssWaveTable[i] = 32767 * sin((i * 2 * M_PI) / 1024);
		}
		for (int i = 0; i < 1024; i++)
		{
			fNonLinTable[i] = 32767.0f-(65536.0f*((expf((((float)i/1024.0f)*4.5f)))-1)/(expf(4.5f)-1.0f));
		}
		fNonLinTable[1024] = -32768;
		bUseNonLinTab = 1;
		THEREMIN_SetWavelength(1024);

		break;


	/*
	case SAWTOOTH:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = (i & 0x03FF)*64-32768;
		}
		a0 = 0.00001533600755608856f;
		a1 = 0.00003067201511217712f;
		a2 = 0.00001533600755608856f;
		b1 = -1.9888928005576803f;
		b2 = 0.9889541445879048f;

		bLpFilt = 1;

		break;
	 */


	case USBSTICK:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
		USB_STICK_ReadFiles();
		break;

	default:
		for (int i = 0; i < 4096; i++)
		{
			ssWaveTable[i] = 0;
		}
	}

	// Additional low pass filter;
	if (bLpFilt)
	{
		float result = 0.0f;
		float sample = 0.0f;
		float x1=0.0f;
		float x2=0.0f;
		float y1=0.0f;
		float y2=0.0f;

		for (int run = 0; run < 2; run ++)
		{
			for (int i = 0; i < iWavLength; i++)
			{
				sample = ((float)ssWaveTable[i]) * 0.8f;

			    // the biquad filter
			    result = a0 * sample + a1 * x1 + a2 * x2 -b1 * y1 - b2 * y2;

			    // shift x1 to x2, sample to x1
			    x2 = x1;
			    x1 = sample;

			    //shift y1 to y2, result to y1
			    y2 = y1;
			    y1 = result;

			    if (run == 1)
			    {
			    	if (result > 32767.0)
			    	{
			    		ssWaveTable[i] = 32767;
			    	}
			    	else if (result < -32768.0)
			    	{
			    		ssWaveTable[i] = -32768;
			    	}
			    	else
			    	{
						ssWaveTable[i] = (int16_t)result;
			    	}
			    }
			}
		}
	}

	// Mute as long as new waveform is beeing calculated
	bMute = 0;

}

/**
 * @brief 96kHz DAC task called in interrupt
 *
 * At 168MHz the absolute maximum cycle count would be 168MHz/96kHz = 1750
 */
inline void THEREMIN_96kHzDACTask(void)
{
	int32_t p1, p2, tabix, tabsub;
	float p1f, p2f;
	floatint_ut u;
	static volatile float fWavStepFilt = 0.0f;
	int task48 = 0;
	float result = 0.0f;
	int iWavOut;



	task48 = 1-task48;

	if (task48)
	{

		if (fPitch >= 1.0f)
		{
			// cycles: 59..62
			// fast pow approximation by LUT with interpolation
			// float bias: 127, so 127 << 23bit mantissa is: 1065353216
			// We use (5 bit of) the exponent and 6 bit of the mantissa
			u.f = fPitch;
			tabix = ((u.ui - 1065353216) >> 17);
			tabsub = (u.ui & 0x0001FFFF) >> 2;
			p1f = (float)ulPitchLinTable[tabix];
			p2f = (float)ulPitchLinTable[tabix + 1];
			fWavStepFilt = (p1f + (((p2f - p1f) * tabsub) * 0.000030518f /*1/32768*/));
			//fWavStepFilt += ((p1f + (((p2f - p1f) * tabsub) * 0.000007629394531f))- fWavStepFilt) * 0.0001f;
			//fWavStepFilt = 81460152.0f;
			ulWaveTableIndex += (uint32_t)fWavStepFilt;
		}


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
		p1 = ssWaveTable[tabix & iWavMask];
		p2 = ssWaveTable[(tabix + 1) & iWavMask];
		iWavOut = ((p1 + (((p2 - p1) * tabsub) / 256)) * slVolFilt / 1024);

		if (bUseNonLinTab)
		{
			tabix = (iWavOut+32768) / 64;
			tabsub = iWavOut & 0x003F;
			p1f = fNonLinTable[tabix];
			p2f = fNonLinTable[tabix + 1];
			result = (p1f + (((p2f - p1f) * tabsub) * 0.015625f));
		}
		else
		{
			result = (float)iWavOut;
		}

		// Limit the output to 16bit
    	if (result > 32767.0)
    	{
    		usDACValue = 32767;
    	}
    	else if (result < -32768.0)
    	{
    		usDACValue = -32768;
    	}
    	else
    	{
    		usDACValue = (int16_t)result;
    	}
	}


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
	int bReqCalcPitchTable = 0;
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
#ifdef DEBUG
		printf("%d %d\n", usPitchPeriod-2048, slPitchOffset/65536/16);
#endif
			CONFIG_Write_SLong(EEPROM_ADDR_PITCH_AUTOTUNE_H, slPitchOffset);
			CONFIG_Write_SLong(EEPROM_ADDR_VOL_AUTOTUNE_H, slVolOffset);
		}
	}

	// pitch scale pot
	if (POTS_HasChanged(POT_PITCH_SCALE))
	{
		// from 2^0.5 ... 2^2.0
		// 2^((POT-2048)/1024)
		fPitchScale = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SCALE) - 2048))
						* 0.000976562f /* 1/1024 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
	}

	// pitch shift pot
	if (POTS_HasChanged(POT_PITCH_SHIFT))
	{
		// from 2^0.25 ... 2^4.0
		// 2^((POT-2048)/1024)
		fPitchShift = powf(2,
				((float) (POTS_GetScaledValue(POT_PITCH_SHIFT) - 2048))
						* 0.001953125f /* 1/512 */);

		// Request the calculation of a new pitch table
		bReqCalcPitchTable = 1;
	}

	// Is it necessary to recalculate the pitch table?
	if (bReqCalcPitchTable)
	{
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
		//printf("%d\n", (uint32_t)slPitchPeriodeFilt);
		//printf("Stopwatch %d\n", ulStopwatch);
	}
#endif
}
