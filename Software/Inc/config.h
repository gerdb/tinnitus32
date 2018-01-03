/**
 *  Project     tinnitus32
 *  @file		config.h
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Header file for config.c
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
#ifndef CONFIG_H_
#define CONFIG_H_

/* Constants ------------------------------------------------------------ */
#define VERSION_MAJOR	1
#define VERSION_MINOR	2
#define VERSION_BUILD	3


/* Types ---------------------------------------------------------------- */
typedef struct
{
  uint32_t   Version;
  uint32_t   cfg01;
  uint32_t   cfg02;
  uint32_t   cfg03;
  uint32_t   cfg04;
  uint32_t   cfg05;
  uint32_t   cfg06;
  uint32_t   cfg07;
  uint32_t   cfg08;
  uint32_t   cfg09;
  uint32_t   cfg10;
  uint32_t   cfg11;
  uint32_t   cfg12;
  uint32_t   cfg13;
  uint32_t   cfg14;
  uint32_t   cfg15;

  uint32_t   cfg16;
  uint32_t   cfg17;
  uint32_t   cfg18;
  uint32_t   cfg19;
  uint32_t   cfg20;
  uint32_t   cfg21;
  uint32_t   cfg22;
  uint32_t   cfg23;
  uint32_t   cfg24;
  uint32_t   cfg25;
  uint32_t   cfg26;
  uint32_t   cfg27;
  uint32_t   cfg28;
  uint32_t   cfg29;
  uint32_t   cfg30;
  uint32_t   cfg31;

  uint32_t   cfg32;
  uint32_t   cfg33;
  uint32_t   cfg34;
  uint32_t   cfg35;
  uint32_t   cfg36;
  uint32_t   cfg37;
  uint32_t   cfg38;
  uint32_t   cfg39;
  uint32_t   cfg40;
  uint32_t   cfg41;
  uint32_t   cfg42;
  uint32_t   cfg43;
  uint32_t   cfg44;
  uint32_t   cfg45;
  uint32_t   cfg46;
  uint32_t   cfg47;

  uint32_t   cfg48;
  uint32_t   cfg49;
  uint32_t   cfg50;
  uint32_t   cfg51;
  uint32_t   cfg52;
  uint32_t   cfg53;
  uint32_t   cfg54;
  uint32_t   cfg55;
  uint32_t   cfg56;
  uint32_t   cfg57;
  uint32_t   cfg58;
  uint32_t   cfg59;
  uint32_t   cfg60;
  uint32_t   cfg61;
  uint32_t   cfg62;
  uint32_t   cfg63;
}CONFIG_TypeDef;

/* Function prototypes ----------------------------------------------------- */
void CONFIG_Init(void);

#endif /* CONFIG_H_ */
