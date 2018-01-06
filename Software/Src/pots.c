/**
 *  Project     tinnitus32
 *  @file		pots.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Process the analog potentiometer values
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

#include "stm32f4xx_hal.h"
#include "pots.h"
#include <stdlib.h>

extern ADC_HandleTypeDef hadc1;

uint16_t usADCResult[ADC_CHANNELS];
uint32_t ulPORT_MUX_MASK[8];
int	iPotMux = 0;
int iPotTask = 0;
const uint32_t POT_MULTIPLEXERS = 4;

// Index 0..7  : AD Channel 0 - MUX 0..7
// Index 8..15 : AD Channel 1 - MUX 0..7
//  ...
// Index 64..71: AD Channel 8 - MUX 0..7
POTS_PotTypeDef strPots[ADC_CHANNELS * 8];

/**
 * @brief initialize the module
 */
void POTS_Init(void)
{
	__HAL_ADC_ENABLE(&hadc1);
	/* Set ADC state                                                          */
	/* - Clear state bitfield related to regular group conversion results     */
	/* - Set state bitfield related to regular group operation                */
	ADC_STATE_CLR_SET(hadc1.State,
			HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR,
			HAL_ADC_STATE_REG_BUSY);
	/* Reset ADC all error code fields */
	ADC_CLEAR_ERRORCODE(&hadc1);

	/* Clear regular group conversion flag and overrun flag */
	/* (To ensure of no unknown state from potential previous ADC operations) */
	__HAL_ADC_CLEAR_FLAG(&hadc1, ADC_FLAG_EOC | ADC_FLAG_OVR);

	/* Enable ADC overrun interrupt */
	__HAL_ADC_ENABLE_IT(&hadc1, ADC_IT_OVR);

	/* Enable ADC DMA mode */
	hadc1.Instance->CR2 |= ADC_CR2_DMA;

	/* Start the DMA channel */
	HAL_DMA_Start(hadc1.DMA_Handle, (uint32_t) &hadc1.Instance->DR,
			(uint32_t)((uint32_t*)usADCResult), ADC_CHANNELS);

    __HAL_UNLOCK(hadc1.DMA_Handle);
	hadc1.DMA_Handle->State = HAL_DMA_STATE_READY;

	//Fill the multiplexer mask for faster access
	for (int i=0; i<8; i++)
	{
		ulPORT_MUX_MASK[i] = 0;
		if (i & 0x01)
		{
			ulPORT_MUX_MASK[i] |= POT_MUX_A_Pin;
		}
		else
		{
			ulPORT_MUX_MASK[i] |= (uint32_t)POT_MUX_A_Pin << 16U;;
		}

		if (i & 0x02)
		{
			ulPORT_MUX_MASK[i] |= POT_MUX_B_Pin;
		}
		else
		{
			ulPORT_MUX_MASK[i] |= (uint32_t)POT_MUX_B_Pin << 16U;;
		}
		if (i & 0x04)
		{
			ulPORT_MUX_MASK[i] |= POT_MUX_C_Pin;
		}
		else
		{
			ulPORT_MUX_MASK[i] |= (uint32_t)POT_MUX_C_Pin << 16U;;
		}

	}

	// Initialize the pots structure
	for (int i=0; i< (ADC_CHANNELS * 8); i++)
	{
		strPots[i].usRawVal = 0;
		strPots[i].usStabilized = 0;
		strPots[i].iStabilizeCnt = 0;
		strPots[i].bChanged = 0;
	}

	// Start with -1, so in the next task, the first value will be 0
	iPotTask = -1;
	iPotMux = -1;
}


/**
 * @brief 1ms Task
 */
void POTS_1msTask(void)
{
	int iPot;

	// Generate a task counter
	iPotTask ++;


	switch (iPotTask)
	{
	case 0:
		// Select next mux channel
		iPotMux++;
		if (iPotMux >=8)
		{
			iPotMux = 0;
		}
		// Set 3 output ports
		GPIOC->BSRR = ulPORT_MUX_MASK[iPotMux];
		break;

	case 2:
		// Start ADC conversion for 9 AD channels
		hadc1.Instance->CR2 |= (uint32_t) ADC_CR2_SWSTART;
		break;

	case 3:

		for (int i=0;i<ADC_CHANNELS ; i++)
		{
			// Index 0..7  : AD Channel 0 - MUX 0..7
			// Index 8..15 : AD Channel 1 - MUX 0..7
			//  ...
			// Index 64..71: AD Channel 8 - MUX 0..7
			if (i < (ADC_CHANNELS-POT_MULTIPLEXERS)) {
				iPot = i*8;
			} else {
				iPot = i*8 + iPotMux;
			}

			// Use the ADC value
			strPots[iPot].usRawVal = usADCResult[i];

			// Change in pot value detected?
			if (abs((int)(strPots[iPot].usRawVal) - (int)(strPots[iPot].usStabilized)) > POT_STAB_THERESHOLD )
			{
				strPots[iPot].iStabilizeCnt = POT_STAB_TIME;
			}

			// Update the stabilzed value a certain time after change detection
			if (strPots[iPot].iStabilizeCnt != 0)
			{
				strPots[iPot].usStabilized = strPots[iPot].usRawVal;
				strPots[iPot].iStabilizeCnt --;
				strPots[iPot].bChanged = 1;
			} else {
				strPots[iPot].bChanged = 0;
			}
		}
		// Next iPotTask will be 0
		iPotTask = -1;
		break;
	}
}


/**
 * @brief Get the scaled pot value
 *
 * @channel Potentiometer
 * @return the scaled pot value
 */
int POTS_GetScaledValue(int channel)
{
	return (strPots[channel].usStabilized * strPots[channel].iMaxValue) / 4096;
}

/**
 * @brief returns the bChanged flag
 *
 * @channel Potentiometer
 * @return 1, if the pot value has changed
 */
int POTS_HasChanged(int channel)
{
	return strPots[channel].bChanged;
}
