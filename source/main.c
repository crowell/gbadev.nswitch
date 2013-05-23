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
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <wiiuse/wpad.h>

#include "armboot.h"

#define le32(i) (((((u32) i) & 0xFF) << 24) | ((((u32) i) & 0xFF00) << 8) | \
                ((((u32) i) & 0xFF0000) >> 8) | ((((u32) i) & 0xFF000000) >> 24))
				
enum
{
    SD = 0,
    USB1,
    USB2,
    USB3,
    USB4,
    MAXDEV
};

static const char dev[MAXDEV][6] =
{
    "sd",
    "usb1",
    "usb2",
    "usb3",
    "usb4"
};

typedef struct _PR 
{
    u8 state;                            
    u8 chs_st[3];                        
    u8 type;                              
    u8 chs_e[3];                         
    u32 lba;                         
    u32 bc;                       
} __attribute__((__packed__)) _pr;

typedef struct _MBR
{
    u8 ca[446];               
    _pr part[4]; 
    u16 sig;                       
} __attribute__((__packed__)) _mbr;

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
	GXRModeObj *rmode, *vidmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	PAD_Init();
	WPAD_Init();
	u32 dpad = WPAD_BUTTON_UP | WPAD_BUTTON_DOWN | WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT,
	done = ~(WPAD_BUTTON_HOME | dpad);
	const WPADData *wd;
	printf("\n\n\nPress any button to start.");
	do
	{	WPAD_ReadPending(WPAD_CHAN_0, NULL);
		wd = WPAD_Data(WPAD_CHAN_0);
		if(wd->btns_d & WPAD_BUTTON_HOME)
		{	printf("\nExiting.");
			SYS_ResetSystem( SYS_RETURNTOMENU, 0, 0 );
		}VIDEO_WaitVSync();
	}while(!(wd->btns_d & (dpad|done) ));
	printf("\nTrying to load.");

	/*** Boot mini from mem code by giantpune ***/
	void *mini = memalign( 32, armboot_size );  
	if( !mini ) 
		return 0;    

	memcpy( mini, armboot, armboot_size );  
	DCFlushRange( mini, armboot_size );		

	*(u32*)0xc150f000 = 0x424d454d;  
	asm volatile( "eieio" );  

	*(u32*)0xc150f004 = MEM_VIRTUAL_TO_PHYSICAL( mini );  
	asm volatile( "eieio" );

	char*redirectedGecko = (char*)0xC1200000;
	*redirectedGecko = (char)(0);
	*(redirectedGecko+1) = (char)(0);

	printf("Reloading IOS 254\n");
	IOS_ReloadIOS( 0xfe );   

	printf("Waiting for gecko output from mini...\n");
	while(true)
	{ printf(redirectedGecko);
	  *redirectedGecko = (char)(0);
	}
	free( mini );

	return 0;
}
