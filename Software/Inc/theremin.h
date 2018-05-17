/**
 *  Project     tinnitus32
 *  @file		theremin.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for theremin.c
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

#ifndef THEREMIN_H_
#define THEREMIN_H_

/* Types  ------------------------------------------------------------------ */
// Union to convert between float and int
typedef union
{
	float f;
	int i;
	uint32_t ui;
} floatint_ut;

// The different waveforms
typedef enum
{
	SINE = 0,
	CAT = 1,
	COSPULSE = 2,
	HARMON = 3,
	COMPRESSED = 4,
	GLOTTAL = 5,
	THEREMIN = 6,
	USBSTICK = 7
}e_waveform;


#define TESTPORT_ON()  GPIOD->BSRR = 0x00002000
#define TESTPORT_OFF() GPIOD->BSRR = 0x20000000

/* Global variables  ------------------------------------------------------- */
extern uint16_t usDACValue;
extern int16_t ssWaveTable[4 * 1024];
extern e_waveform eWaveform;

/* Function prototypes ----------------------------------------------------- */
void THEREMIN_Init(void);
void THEREMIN_96kHzDACTask(void);
void THEREMIN_1msTask(void);
void THEREMIN_1sTask(void);
void THEREMIN_Calc_VolumeTable(void);
void THEREMIN_Calc_PitchTable(void);
void THEREMIN_Calc_WavTable(void);
void THEREMIN_Calc_LFOTable(void);
void THEREMIN_PitchDisplay(void);
void THEREMIN_AutotuneDisplay(void);

#endif /* THEREMIN_H_ */
