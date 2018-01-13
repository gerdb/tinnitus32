/**
 *  Project     tinnitus32
 *  @file		config.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Project configuration
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
#include "config.h"
#include "eeprom.h"

volatile const CONFIG_TypeDef __attribute__((section (".myConfigSection"))) CONFIG =
{ (VERSION_MAJOR << 16 | VERSION_MINOR << 8 | VERSION_BUILD), // Version
		7,		// cfg01
		8,		// cfg02
		9,		// cfg03
		10		// cfg04
		};

uint16_t VirtAddVarTab[NB_OF_VAR] = {
		EEPROM_ADDR_PITCH_AUTOTUNE_H,
		EEPROM_ADDR_PITCH_AUTOTUNE_L,
		EEPROM_ADDR_VOL_AUTOTUNE_H,
		EEPROM_ADDR_VOL_AUTOTUNE_L
};

/**
 * @brief initialize the module
 */
void CONFIG_Init(void)
{

	/* Unlock the Flash Program Erase controller */
	HAL_FLASH_Unlock();

	/* EEPROM Init */
	if (EE_Init() != EE_OK)
	{
		printf("EEProm error \n");
	}
}

/**
 * @brief Writes a 32 bit value to a virtual EEProm address
 *
 * @param addr: the virtual eeprom address
 * @value the 32bit value to write
 */
void CONFIG_Write_SLong(int addr, int32_t value)
{
	if ((EE_WriteVariable((uint16_t)addr, (uint16_t)( (uint32_t)value >> 16 ) ) ) != HAL_OK)
	{
		return;
	}

	if ((EE_WriteVariable((uint16_t)(addr+1), (uint16_t)value )) != HAL_OK)
	{
		return;
	}
}

/**
 * @brief Reads a 32 bit value from a virtual EEProm address
 *
 * @param addr: the virtual eeprom address
 * @return the 32bit value to write
 */
int32_t CONFIG_Read_SLong(int addr)
{
	uint16_t VarDataTmp = 0;
	int32_t value = 0;

	if (EE_ReadVariable((uint16_t)addr, &VarDataTmp) != HAL_OK)
	{
		return 0;
	}
	value = VarDataTmp;
	value <<= 16;

	if (EE_ReadVariable((uint16_t)(addr+1), &VarDataTmp) != HAL_OK)
	{
		return 0;
	}
	value |= VarDataTmp;
	return value;
}
