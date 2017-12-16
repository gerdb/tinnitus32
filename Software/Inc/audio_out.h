/*
 * audio.h
 *
 *  Created on: Nov 27, 2017
 *      Author: gerd
 */

#ifndef AUDIO_OUT_H_
#define AUDIO_OUT_H_


/* Audio status definition */
#define AUDIO_OK                        0
#define AUDIO_ERROR                     1
#define AUDIO_TIMEOUT                   2

/* Function prototypes */
void AUDIO_OUT_Init();
void AUDIO_OUT_I2S_IRQHandler(void);


#endif /* AUDIO_OUT_H_ */
