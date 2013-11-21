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

#include <ogc/machine/asm.h>
//#include <ogc/cache.h>
#include <ogc/ipc.h>
//#include <ogc/stm.h>
//#include <ogc/es.h>
//#include <ogc/ios.h>
//#include <ogc/irq.h>

#include "mmustub.h"
#include "elf.h"
#include "runtimeiospatch.h"

#define MAX_IPC_RETRIES 400

extern void udelay(int us);


bool __debug = true;
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
int loadBINfromNAND(const char *path, u32 size)
{
	int fd ATTRIBUTE_ALIGN(32);
	s32 fres;
	
	DEBUG("Loading BIN file: %s .\n", path);
	fd = ISFS_Open(path, ISFS_OPEN_READ);
	if (fd < 0)
		return fd;
	fres = ISFS_Read(fd, (void*)0x90100000, size);
	DCFlushRange((void*)0x90100000, size);
	if (fres < 0)
		return fres;
	ISFS_Close(fd);
	return 0;
}

void IosPrepatchSyscall54(void*BINAddr, u32 syscEntry)
{	ioshdr *IOSHead = (ioshdr*)BINAddr;
	Elf32_Ehdr *ELFHead = (Elf32_Ehdr*)(BINAddr + IOSHead->hdrsize + IOSHead->loadersize);
	Elf32_Phdr *progHeads = (Elf32_Phdr*)( ((void*)ELFHead) + ELFHead->e_phoff );
	void*flushMe = ((void*)ELFHead) + progHeads[ELFHead->e_phnum-2].p_offset + (0x54*4);
	*(u32*)(flushMe) = syscEntry;
	DCFlushRange(flushMe, 32);
	DEBUG("Syscall entry point reference patched at address 0x%08x", (u32)flushMe);
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
			*(u32*)0x81330120 = (X); \
			*(u32*)0x81330100 = 1; \
			DCFlushRange((void*)0x81330100, 64); \
			IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL ); \
			i = 0; \
			do{ i++; \
			DCInvalidateRange( (void*)0x81330100, 0x20 ); \
			}while(*(u32*)0x81330100 == 1 && i<300000);

int main(int argc, char **argv) {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	printf("\n\n\n");
	u32 i, reentry = (u32)stub_1800_1_512;
//	CheckArguments(argc, argv);
	
	if(AHBPROT_DISABLED){
	
		printf("Applying patches to IOS with AHBPROT\n");
		printf("IosPatch_RUNTIME(...) returned %i\n", IosPatch_RUNTIME(true, false, false, true));
		printf("ISFS_Initialize() returned %d\n", ISFS_Initialize());
		printf("loadBINfromNAND() returned %d .\n", loadBINfromNAND("/title/00000001/00000050/content/0000000d.app", 168512));
		//IosPrepatchSyscall54((void*)0x90100000, new_syscall_code);
			/** boot mini without BootMii IOS code by Crediar. **/
	
		DEBUG("** Running boot mini without BootMii IOS code by Crediar. **\n");

		unsigned char ES_ImportBoot2[16] =
		{
			0x68, 0x4B, 0x2B, 0x06, 0xD1, 0x0C, 0x68, 0x8B, 0x2B, 0x00, 0xD1, 0x09, 0x68, 0xC8, 0x68, 0x42
		};
		DEBUG("Searching for ES_ImportBoot2.\n");
		for( i = 0x939F0000; i < 0x939FE000; i+=4 )
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
				
				DCInvalidateRange( (void*)i, 0x20 );
				
				*(vu32*)(i+0x00)	= 0x48034904;	// LDR R0, 0x10, LDR R1, 0x14
				*(vu32*)(i+0x04)	= 0x477846C0;	// BX PC, NOP
				*(vu32*)(i+0x08)	= 0xE6000870;	// SYSCALL
				*(vu32*)(i+0x0C)	= 0xE12FFF1E;	// BLR
				*(vu32*)(i+0x10)	= 0x10100000;	// offset
				*(vu32*)(i+0x14)	= 0x00001C20;	// version

				DCFlushRange( (void*)i, 0x20 );
				break;
			}
		}
		__IOS_ShutdownSubsystems();
		s32 fd = IOS_Open( "/dev/es", 0 );
		
		u8 *buffer = (u8*)memalign( 32, 0x100 );
		memset( buffer, 0, 0x100 );
		
		//printf("Manually loading pre-loaded IOS80\n");
		//IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );
		tikview views[4] ATTRIBUTE_ALIGN(32);
		printf("Loading IOS 80 (%08x).\n", read32(0x80003140));
		__ES_Init();
		u32 numviews;
		ES_GetNumTicketViews(0x00000001000000FEULL, &numviews);
		ES_GetTicketViews(0x00000001000000FEULL, views, numviews);
		ES_LaunchTitleBackground(0x00000001000000FEULL, &views[0]);

		__ES_Reset();
		i=0;
		while ((read32(0x80003140) >> 16) == 0)
		{	printf("%08x %d\r", read32(0x80003140), i++);
			udelay(1000);
		}
		printf("\n");
		for (i = 0; !(read32(0x0d000004) & 2); i++) {
				udelay(1000);
				printf("%08x %d\r", read32(0x0d000004), i++);
				if (i >= MAX_IPC_RETRIES)
					break;
		}
		__IOS_InitializeSubsystems();
		printf("IOS80 manual reload complete. Testing by reloading back to 58\n");
		IOS_ReloadIOS(58);
		printf("All done. Exiting in 1 sec...\n");
		udelay(1000000);
	}else printf("No AHB Access (needs AHBPROT disabled) exiting.\n");
	return 0;
}
