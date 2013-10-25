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
static char path[38] = "/title/00000001/00000200/00000003.app";
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

int main(int argc, char **argv) {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	printf("\n\n\n");
	u32 i;
//	CheckArguments(argc, argv);
	
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
				DCInvalidateRange( (void*)i, 0x80 );
				
				*(vu32*)(i+0x00)	= 0x477846C0;	// BX PC, NOP (get out of Thumb mode)
				*(vu32*)(i+0x04)	= 0xE59F005C;	// r0 = &path (PC+92)
				*(vu32*)(i+0x08)	= 0xE6000830;	// boot_PPC(path) (syscall 0x41)
				*(vu32*)(i+0x0c)	= 0xE59F0044;	// r0 = ARG1 (0x1330100 ... PC+68)
				*(vu32*)(i+0x10)	= 0xE5902000;	// r2 = read32(0x1330100)
				*(vu32*)(i+0x14)	= 0xE59F003C;	// r0 = ARG1 (PC+60)	<===--- LOOPSTART
				*(vu32*)(i+0x18)	= 0xE3A01020;	// MOV r1 0x20 (ARG2)
				*(vu32*)(i+0x1c)	= 0xE60007F0;	// sync_before_read(0x1330100,32) (syscall 0x3F)
				*(vu32*)(i+0x20)	= 0xE59F0030;	// r0 = ARG1 (PC+48)
				*(vu32*)(i+0x24)	= 0xE5903000;	// r3 = read32(0x1330100)
				*(vu32*)(i+0x28)	= 0xE1520003;	// compare r2 r3
				*(vu32*)(i+0x2c)	= 0x0AFFFFF8;	// BEQ LOOPSTART		<===---
				*(vu32*)(i+0x30)	= 0xE59F0020;	// r0 = ARG1 (PC+32)
				*(vu32*)(i+0x34)	= 0xE59F1020;	// r1 = PPC1 (PC+32)
				*(vu32*)(i+0x38)	= 0xE5801000;	// write32(r0, r1)
				*(vu32*)(i+0x3c)	= 0xE59F101C;	// r1 = PPC2 (PC+28)
				*(vu32*)(i+0x40)	= 0xE5801004;	// write32(r0+4, r1)
				*(vu32*)(i+0x44)	= 0xE59F1018;	// r1 = PPC3 (PC+24)
				*(vu32*)(i+0x48)	= 0xE5801008;	// write32(r0+8, r1)
				*(vu32*)(i+0x4c)	= 0xE3A01020;	// MOV r1 0x20 (ARG2)
				*(vu32*)(i+0x50)	= 0xE6000810;	// sync_after_write(0x1330100,32)
				*(vu32*)(i+0x54)	= 0xE12FFF1E;	// BLR
				*(vu32*)(i+0x58)	= 0x01330100;	//				<===--- ARG1
				*(vu32*)(i+0x5c)	= 0x38802000;	// li r4, 0x2000<===--- PPC1
				*(vu32*)(i+0x60)	= 0x7c800124;	// mtmsr r4		<===--- PPC2
				*(vu32*)(i+0x64)	= 0x48001802;	// b 0x1800		<===--- PPC3 (assuming init stub is at 0x1800)
				*(vu32*)(i+0x64)	= (vu32)MEM_VIRTUAL_TO_PHYSICAL(&path);

				DCFlushRange( (void*)i, 0x80 );
				
				s32 fd = IOS_Open( "/dev/es", 0 );
				
				u8 *buffer = (u8*)memalign( 32, 0x100 );
				memset( buffer, 0, 0x100 );
				
				/*if(__debug){
					printf("ES_ImportBoot():%d\n", IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL ) );
				}else{*/
					IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );
				//}
				printf("0x%08x\n",*(u32*)0x81330100);
				while(1)
				{	DCInvalidateRange( (void*)0x81330100, 0x20 );
					printf("0x%08x 0x%08x\r",*(u32*)0x81330100, i);
					i++;
				}
				break;
			}
		}
	}//else
	printf("No AHB Access (needs AHBPROT disabled) exiting.\n");
	return 0;
}
