/**
 *  Project     tinnitus32
 *  @file		audio_out.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for audio_out.c
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

#ifndef AUDIO_OUT_H_
#define AUDIO_OUT_H_


/* Audio status definition ------------------------------------------------- */
#define AUDIO_OK                        0
#define AUDIO_ERROR                     1
#define AUDIO_TIMEOUT                   2

/* Function prototypes ----------------------------------------------------- */
void AUDIO_OUT_Init();
void AUDIO_OUT_I2S_IRQHandler(void);
void AUDIO_OUT_1msTask(void);



#endif /* AUDIO_OUT_H_ */
