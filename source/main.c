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

#include "mmustub.h"

bool __debug = true;
static char* path = (char*)0x81200000;
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

#define PATCH(I,X)	DEBUG( "%08x:%08x -> ", (I), *(vu32*)(I) ); \
					*(vu32*)(I) = (X); \
					DEBUG( "%08x\n", *(vu32*)(I) ); \
					(I)+=4;
/*
void CheckArguments(int argc, char **argv) {
	int i;
	char*pathToSet = 0;
	char*newPath = redirectedGecko->buf;
	if(argv[0][0] == 's' || argv[0][0] == 'S') // Make sure you're using an SD card
	{	pathToSet = strndup(argv[0] + 3, strrchr(argv[0], '/') - argv[0] - 3);
		snprintf(newPath, sizeof(redirectedGecko->buf), "%s/nand.bin", pathToSet);
	}
	for (i = 1; i < argc; i++)
	{	if (CHECK_ARG("debug="))
			__debug = atoi(CHECK_ARG_VAL("debug="));
		else if (CHECK_ARG("path="))
			pathToSet = strcpy(newPath, CHECK_ARG_VAL("path="));
	}
	if(pathToSet)
	{	redirectedGecko->path_magic = 0x016AE570;
		DCFlushRange(redirectedGecko, 288);
		free(pathToSet);
		printf("Will dump NAND to %s .\n", newPath);
	}
	else printf("Will dump NAND to sd:/bootmii/nand.bin .\n");
}
*/
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

#define REQUEST(X) \
			memset( buffer, 0, 0x100 ); \
			*(u32*)0x81330100 = (X); \
			*(u32*)0x81330104 = 0; \
			DCFlushRange((void*)0x81330100, 32); \
			IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL ); \
			while(*(u32*)0x81330104 == 0) DCInvalidateRange( (void*)0x81330100, 0x20 );

int main(int argc, char **argv) {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	printf("\n\n\n");
	u32 i, reentry = (u32)stub_1800_1_512;
	strcpy(path, "/title/00000001/00000200/00000003.app");
//	CheckArguments(argc, argv);
	
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
				// to avoid having to make too many adjustments to the ARM ASM below, I'll jump to a stub here.
				// 0x80000100 is overwritten by the bootrom with an infinite loop so we'll skip that address.
				*(u32*)0x80000104 = 0x3c600000 | reentry >> 16; // lis r3, entry@h
				*(u32*)0x80000108 = 0x60630000 | (reentry & 0xffff); // ori r3, r3, entry@l
				*(u32*)0x8000010C = 0x7c7a03a6; // mtsrr0 r3
				*(u32*)0x80000100 = 0x38600000; // li r3, 0
				*(u32*)0x80000100 = 0x7c7b03a6; // mtsrr1 r3
				*(u32*)0x80000100 = 0x4c000064; // rfi
				__debug = false;
				reentry = i;
				*(u32*)0x81330120 = i;
				DCFlushRange((void*)0x81330120, 32);
				
				DCInvalidateRange( (void*)i, 64 );
				PATCH(i, 0x477846C0)
				PATCH(i, 0xE3A01020)
				PATCH(i, 0xE59F0020)
				PATCH(i, 0xE60007F0)
				PATCH(i, 0xE3A01020)
				PATCH(i, 0xE59F0014)
				PATCH(i, 0xE5902000)
				PATCH(i, 0xE5922000)
				PATCH(i, 0xE5802000)
				PATCH(i, 0xE5801004)
				PATCH(i, 0xE6000810)
				PATCH(i, 0xE12FFF1E)
				PATCH(i, 0x01330100)
				DCFlushRange( (void*)reentry, 64 );
				break;
			}
				
			s32 fd = IOS_Open( "/dev/es", 0 );
			
			u8 *buffer = (u8*)memalign( 32, 0x100 );
			
			REQUEST(0xd800010)
			printf("HW_TIMER : 0x%08x\n",*(u32*)0x81330100);
			REQUEST(0xd800194)
			printf("HW_RESETS : 0x%08x\n",*(u32*)0x81330100);
			REQUEST(0xFFFF9438)
			printf("KERNEL1 : 0x%08x\n",*(u32*)0x81330100);
			REQUEST(0xFFFF9250)
			printf("Kernel2 : 0x%08x\n",*(u32*)0x81330100);
			
			while(i-reentry < 0x300000)	i++;
		}printf("exiting...\n");
	}else printf("No AHB Access (needs AHBPROT disabled) exiting.\n");
	return 0;
}
