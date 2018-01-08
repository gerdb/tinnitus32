/**
 *  Project     tinnitus32
 *  @file		pots.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for pots.c
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

#ifndef POTS_H_
#define POTS_H_

#define ADC_CHANNELS 9
#define POT_STAB_THERESHOLD 40
#define POT_STAB_TIME 10


/* Constants ------------------------------------------------------------ */

#define POT_VOLUME_OUT	0
#define POT_PITCH_SCALE	8
#define POT_PITCH_SHIFT	16
#define POT_VOL_SCALE	24

/* Types ---------------------------------------------------------------- */
typedef struct
{
  uint16_t	usRawVal;
  uint16_t	usStabilized;
  int   	iStabilizeCnt;
  int		bChanged;
  int   	iMaxValue;
}POTS_PotTypeDef;


/* global variables ------------------------------------------------------ */
extern POTS_PotTypeDef strPots[];


/* Function prototypes ----------------------------------------------------- */
void POTS_Init(void);
void POTS_1msTask(void);
int POTS_GetScaledValue(int channel);
int POTS_HasChanged(int channel);

#endif /* POTS_H_ */
