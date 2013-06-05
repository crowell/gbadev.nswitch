/**************************************************************************************
***																					***
*** nswitch - Simple neek/realnand switcher to embed in a channel					***
***																					***
*** Copyright (C) 2011	OverjoY														***
*** 																				***
*** This program is free software; you can redistribute it and/or					***
*** modify it under the terms of the GNU General Public License						***
*** as published by the Free Software Foundation version 2.							***
***																					***
*** This program is distributed in the hope that it will be useful,					***
*** but WITHOUT ANY WARRANTY; without even the implied warranty of					***
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the					***
*** GNU General Public License for more details.									***
***																					***
*** You should have received a copy of the GNU General Public License				***
*** along with this program; if not, write to the Free Software						***
*** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. ***
***																					***
**************************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#define le32(i) (((((u32) i) & 0xFF) << 24) | ((((u32) i) & 0xFF00) << 8) | \
                ((((u32) i) & 0xFF0000) >> 8) | ((((u32) i) & 0xFF000000) >> 24))

static void initialize(GXRModeObj *rmode)
{
	static void *xfb = NULL;

	if (xfb)
		free(MEM_K1_TO_K0(xfb));

	xfb = SYS_AllocateFramebuffer(rmode);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	xfb = MEM_K0_TO_K1(xfb);
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
}

int main() {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	printf("\nTrying to load.\nSetting memory.\n");
	char*redirectedGecko = (char*)0x81200000;
	*redirectedGecko = (char)(0);
	printf("Terminating string.\n");
	*(redirectedGecko+1) = (char)(0);
	printf("Setting magic word.\n");
	*((u16*)(redirectedGecko+2)) = 0xDEB6;
	DCFlushRange(redirectedGecko, 32);
	tikview views[4] ATTRIBUTE_ALIGN(32);
	printf("Shutting down IOS subsystems.\n");
	__IOS_ShutdownSubsystems();
	printf("Loading IOS 254.\n");
	__ES_Init();
	u32 numviews;

	ES_GetNumTicketViews(0x00000001000000FEULL, &numviews);
	ES_GetTicketViews(0x00000001000000FEULL, views, numviews);
	ES_LaunchTitleBackground(0x00000001000000FEULL, &views[0]);

	printf("Waiting for gecko output from mini...\n");
	while(true)
	{ do DCInvalidateRange(redirectedGecko, 32);
	  while(!*redirectedGecko);
	  VIDEO_WaitVSync();
	  printf(redirectedGecko);
	  *redirectedGecko = (char)(0);
	  DCFlushRange(redirectedGecko, 32);
	}

	return 0;
}
