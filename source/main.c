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
#include <ogc/machine/processor.h>

#include "armboot.h"

typedef struct armboot_config armboot_config;
struct armboot_config
{	char str[2];		// character sent from armboot to be printed on screen
	u16 debug_magic;	// set to 0xDEB6 if we want armboot to send us it's debug
						// or set to 0xABCD if we jsut want armboot to send us it's normal output
	u32 path_magic;		// set to 0x016AE570 if se are sending a custom ppcboot path
	char buf[256]; // a buffer to put the string in where there will still be space for mini
};

bool __debug = false;
armboot_config *redirectedGecko = (armboot_config*)0x81200000;
#define MEM_REG_BASE 0xd8b4000
#define MEM_PROT (MEM_REG_BASE + 0x20a)
#define AHBPROT_DISABLED (*(vu32*)0xcd800064 == 0xFFFFFFFF)

// Check if string X is in current argument
#define CHECK_ARG(X) (!strncmp((X), argv[i], sizeof((X))-1))
#define CHECK_ARG_VAL(X) (argv[i] + sizeof((X))-1)
#define DEBUG(...) if (__debug) printf(__VA_ARGS__)
// Colors for debug output
#define	RED		"\x1b[31;1m"
#define	GREEN	"\x1b[32;1m"
#define	YELLOW	"\x1b[33;1m"
#define	WHITE	"\x1b[37;1m"

// Remeber to set it back to WHITE when you're done
#define CHANGE_COLOR(X)	(printf((X)))

void CheckArguments(int argc, char **argv) {
	int i;
	bool pathSet = false;
	char*newPath = redirectedGecko->buf;
	if(( pathSet = (argv[0][0] == 's' || argv[0][0] == 'S') )) // Make sure you're using an SD card
	{	*strrchr(argv[0], '/') = '\0';
		snprintf(newPath, sizeof(redirectedGecko->buf), "%s/nand.bin", argv[0]+3);
	}
	for (i = 1; i < argc; i++)
	{	if (CHECK_ARG("debug="))
			__debug = atoi(CHECK_ARG_VAL("debug="));
		else if ( pathSet |= (CHECK_ARG("path=")) )
			strcpy(newPath, CHECK_ARG_VAL("path="));
	}
	if(pathSet)
	{	redirectedGecko->path_magic = 0x016AE570;
		DCFlushRange(redirectedGecko, 288);
		printf("Will dump nand.bin to %s .\n", newPath);
	}
	else printf("Will dump nand.bin to sd:/bootmii/nand.bin .\n");
}

static void disable_memory_protection() {
	write32(MEM_PROT, read32(MEM_PROT) & 0x0000FFFF);
}

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

int main(int argc, char **argv) {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	printf("\n\n\n");
	u32 i;
	CheckArguments(argc, argv);
	DEBUG("Setting magic word.\n");
	redirectedGecko->str[0] = '\0';
	redirectedGecko->str[1] = '\0';
	if(__debug) redirectedGecko->debug_magic = 0xDEB6;
	else redirectedGecko->debug_magic = 0xABCD;
	DCFlushRange(redirectedGecko, 32);
	
	DEBUG("Shutting down IOS subsystems.\n");
	__IOS_ShutdownSubsystems();
	if(AHBPROT_DISABLED){
	
			/** boot mini without BootMii IOS code by Crediar. **/
	
		DEBUG("** Running boot mini without BootMii IOS code by Crediar. **\n");

		unsigned char ES_ImportBoot2[16] =
		{
			0x68, 0x4B, 0x2B, 0x06, 0xD1, 0x0C, 0x68, 0x8B, 0x2B, 0x00, 0xD1, 0x09, 0x68, 0xC8, 0x68, 0x42
		};
		DEBUG("Searching for ES_ImportBoot2.\n");
		disable_memory_protection();
		for( i = 0x939F0000; i < 0x939FE000; i+=2 )
		{	
			if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) == 0 )
			{	DEBUG("Found. Patching.\n");
				DCInvalidateRange( (void*)i, 0x20 );
				
				*(vu32*)(i+0x00)	= 0x48034904;	// LDR R0, 0x10, LDR R1, 0x14
				*(vu32*)(i+0x04)	= 0x477846C0;	// BX PC, NOP
				*(vu32*)(i+0x08)	= 0xE6000870;	// SYSCALL
				*(vu32*)(i+0x0C)	= 0xE12FFF1E;	// BLR
				*(vu32*)(i+0x10)	= 0x10100000;	// offset
				*(vu32*)(i+0x14)	= 0x0000FF01;	// version

				DCFlushRange( (void*)i, 0x20 );
				DEBUG("Copying Mini into place.\n");
				void *mini = (void*)0x90100000;
				memcpy(mini, armboot, armboot_size);
				DCFlushRange( mini, armboot_size );
				
				s32 fd = IOS_Open( "/dev/es", 0 );
				
				u8 *buffer = (u8*)memalign( 32, 0x100 );
				memset( buffer, 0, 0x100 );
				
				if(__debug){
					printf("ES_ImportBoot():%d\n", IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL ) );
				}else{
					IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );
				}
				break;
			}
		}
	}else{
	
			/** Boot mini from mem code by giantpune. **/
	
		DEBUG("** Running Boot mini from mem code by giantpune. **\n");
		
		void *mini = memalign(32, armboot_size);  
		if(!mini) 
			  return 0;    
		
		memcpy(mini, armboot, armboot_size);  
		DCFlushRange(mini, armboot_size);               

		*(u32*)0xc150f000 = 0x424d454d;  
		asm volatile("eieio");  

		*(u32*)0xc150f004 = MEM_VIRTUAL_TO_PHYSICAL(mini);  
		asm volatile("eieio");

		tikview views[4] ATTRIBUTE_ALIGN(32);
		printf("Loading IOS 254.\n");
		__ES_Init();
		u32 numviews;
		ES_GetNumTicketViews(0x00000001000000FEULL, &numviews);
		ES_GetTicketViews(0x00000001000000FEULL, views, numviews);
		ES_LaunchTitleBackground(0x00000001000000FEULL, &views[0]);

		free(mini);
	}
	DEBUG("Waiting for mini gecko output.\n");
	char* miniDebug = redirectedGecko->str;
	while(!miniDebug[1])
	{	do DCInvalidateRange(miniDebug, 32);
		while(!*miniDebug);
		printf(miniDebug);
		*miniDebug = '\0';
		DCFlushRange(miniDebug, 32);
	}
	return 0;
}
