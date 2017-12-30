/**
 *  Project     tinnitus32
 *  @file		usb_stick.c
 *  @author		Gerd Bartelt - www.sebulli.com
 *  @brief		Load WAV table from file
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

#include <stdio.h>
#include "fatfs.h"
#include "usb_host.h"

/* Variable used by FatFs*/
DIR Directory;
char path[] = "0:/";
extern ApplicationTypeDef Appli_state;


void USB_STICK_Connected(void)
{
	printf("USB stick connected\n");
	/* Initializes the File System */
	if (f_mount(&USBHFatFS, (TCHAR const*) USBHPath, 0) == FR_OK)
	{
		/* FatFs initialisation fails */
		printf("mounted\n");
	}
	else
	{
		printf("not mounted\n");
	}

	MX_USB_HOST_Process();

	if (f_open(&USBHFile, "TEST.TXT", FA_READ) == FR_OK)
	{
		printf("f_open!\n");
	}
	else
	{
		printf("no f_open!\n");
	}

	MX_USB_HOST_Process();

	if (f_opendir(&Directory, path) == FR_OK)
	{
		printf("f_opendir!\n");
	}
	else
	{
		printf("no f_opendir!\n");
	}


}

void USB_STICK_Disconnected(void)
{
	printf("USB stick disconnected\n");
}
