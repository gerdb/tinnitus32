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

extern ADC_HandleTypeDef hadc1;

uint16_t usADCResult[ADC_CHANNELS];


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
			(uint32_t)((uint32_t*)&usADCResult[0]), ADC_CHANNELS);

    __HAL_UNLOCK(hadc1.DMA_Handle);
	hadc1.DMA_Handle->State = HAL_DMA_STATE_READY;
}


/**
 * @brief 1ms Task
 */
void POTS_1msTask(void)
{
	hadc1.Instance->CR2 |= (uint32_t) ADC_CR2_SWSTART;
}
