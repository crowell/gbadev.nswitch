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

#include "runtimeiospatch.h"
#include "armboot.h"

// Uncomment to enable certain printf functions
//#DEFINE DEBUG_OUTPUT

#define le32(i) (((((u32) i) & 0xFF) << 24) | ((((u32) i) & 0xFF00) << 8) | \
				((((u32) i) & 0xFF0000) >> 8) | ((((u32) i) & 0xFF000000) >> 24))
// Enable interrupt flag.
#define MSR_EE 0x00008000
 
// Disable interrupts.
#define _CPU_ISR_Disable( _isr_cookie ) \
  { register u32 _disable_mask = MSR_EE; \
    _isr_cookie = 0; \
    asm volatile ( \
	"mfmsr %0; andc %1,%0,%1; mtmsr %1" : \
	"=&r" ((_isr_cookie)), "=&r" ((_disable_mask)) : \
	"0" ((_isr_cookie)), "1" ((_disable_mask)) \
	); \
  }
 
// Alignment required for USB structures (I don't know if this is 32 or less).
#define USB_ALIGN __attribute__ ((aligned(32)))
 
char bluetoothResetData1[] USB_ALIGN = {0x20}; // bmRequestType
char bluetoothResetData2[] USB_ALIGN = {0x00}; // bmRequest
char bluetoothResetData3[] USB_ALIGN = {0x00, 0x00}; // wValue
char bluetoothResetData4[] USB_ALIGN = {0x00, 0x00}; // wIndex
char bluetoothResetData5[] USB_ALIGN = {0x03, 0x00}; // wLength
char bluetoothResetData6[] USB_ALIGN = {0x00}; // unknown; set to zero.
char bluetoothResetData7[] USB_ALIGN = {0x03, 0x0c, 0x00}; // Mesage payload.
 
/** Vectors of data transfered. */
ioctlv bluetoothReset[] USB_ALIGN = {
	{	bluetoothResetData1,
		sizeof(bluetoothResetData1)
	},{	bluetoothResetData2,
		sizeof(bluetoothResetData2)
	},{	bluetoothResetData3,
		sizeof(bluetoothResetData3)
	},{	bluetoothResetData4,
		sizeof(bluetoothResetData4)
	},{	bluetoothResetData5,
		sizeof(bluetoothResetData5)
	},{	bluetoothResetData6,
		sizeof(bluetoothResetData6)
	},{	bluetoothResetData7,
		sizeof(bluetoothResetData7)
	}
};
 
void BTShutdown()
{	s32 fd;
	int rv;
	uint32_t level;
 
	printf("Open Bluetooth Dongle\n");
	fd = IOS_Open("/dev/usb/oh1/57e/305", 2 /* 2 = write, 1 = read */);
	printf("fd = %d\n", fd);
 
	printf("Closing connection to blutooth\n");
	rv = IOS_Ioctlv(fd, 0, 6, 1, bluetoothReset);
	printf("ret = %d\n", rv);
 
	IOS_Close(fd);
}

typedef struct dol_t dol_t;
struct dol_t
{
	u32 offsetText[7];
	u32 offsetData[11];
	u32 addressText[7];
	u32 addressData[11];
	u32 sizeText[7];
	u32 sizeData[11];
	u32 addressBSS;
	u32 sizeBSS;
	u32 entrypt;
	u8 pad[0x1C];
};

int loadDOLfromNAND(const char *path, bool debug)
{
	int fd ATTRIBUTE_ALIGN(32);
	s32 fres;
	//fstats *status ATTRIBUTE_ALIGN(32);
	dol_t dol_hdr ATTRIBUTE_ALIGN(32);
	
	if(debug) printf("Loading DOL file: %s .\n", path);
	fd = ISFS_Open(path, ISFS_OPEN_READ);
	if (fd < 0)
		return fd;
	//printf("ISFS_GetFileStats() returned %d .\n", ISFS_GetFileStats(fd, status));
	if(debug) printf("Reading header.\n");
	fres = ISFS_Read(fd, &dol_hdr, sizeof(dol_t));
	if (fres < 0)
		return fres;
	if(debug)printf("Loading sections.\n");
	int ii;

	/* TEXT SECTIONS */
	for (ii = 0; ii < 7; ii++)
	{
		if (!dol_hdr.sizeText[ii])
			continue;
		fres = ISFS_Seek(fd, dol_hdr.offsetText[ii], 0);
		if (fres < 0)
			return fres;
		fres = ISFS_Read(fd, (void*)dol_hdr.addressText[ii], dol_hdr.sizeText[ii]);
		if (fres < 0)
			return fres;
		if(debug)
		{	printf("Text section of size %08x loaded from offset %08x to memory %08x.\n", dol_hdr.sizeText[ii], dol_hdr.offsetText[ii], dol_hdr.addressText[ii]);
			printf("Memory area starts with %08x and ends with %08x (at address %08x)\n", *(u32*)(dol_hdr.addressData[ii]), *(u32*)((dol_hdr.addressText[ii]+(dol_hdr.sizeText[ii] - 1)) & ~3),(dol_hdr.addressText[ii]+(dol_hdr.sizeText[ii] - 1)) & ~3);
		}
	}

	/* DATA SECTIONS */
	for (ii = 0; ii < 11; ii++)
	{
		if (!dol_hdr.sizeData[ii])
			continue;
		fres = ISFS_Seek(fd, dol_hdr.offsetData[ii], 0);
		if (fres < 0)
			return fres;
		fres = ISFS_Read(fd, (void*)dol_hdr.addressData[ii], dol_hdr.sizeData[ii]);
		if (fres < 0)
			return fres;
		if(debug)
		{	printf("Data section of size %08x loaded from offset %08x to memory %08x.\n", dol_hdr.sizeData[ii], dol_hdr.offsetData[ii], dol_hdr.addressData[ii]);
			printf("Memory area starts with %08x and ends with %08x (at address %08x)\n", *(u32*)(dol_hdr.addressData[ii]), *(u32*)((dol_hdr.addressData[ii]+(dol_hdr.sizeData[ii] - 1)) & ~3),(dol_hdr.addressData[ii]+(dol_hdr.sizeData[ii] - 1)) & ~3);
		}
	}
	
	ISFS_Close(fd);
	return 0;
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

int main() {
	GXRModeObj *rmode;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	initialize(rmode);
	
	#ifdef DEBUG_OUTPUT
	printf("Applying patches to IOS with AHBPROT\n");
	printf("IosPatch_RUNTIME(...) returned %i\n", IosPatch_RUNTIME(true, false, false, true));
	printf("ISFS_Initialize() returned %d\n", ISFS_Initialize());
	printf("loadDOLfromNAND() returned %d .\n", loadDOLfromNAND("/title/00000001/00000200/content/00000003.app", true);//);
	printf("Setting magic word.\n");
	char*redirectedGecko = (char*)0x81200000;
	*redirectedGecko = (char)(0);
	*(redirectedGecko+1) = (char)(0);
	*((u16*)(redirectedGecko+2)) = 0xDEB6;
	DCFlushRange(redirectedGecko, 32);
	#else
	IosPatch_RUNTIME(true, false, false, false);
	ISFS_Initialize();
	if(loadDOLfromNAND("/title/00000001/00000200/content/00000003.app", false))
		printf("Load 1-512 from NAND failed.\n");
	else printf("1-512 loaded from NAND.\n");
	#endif
	 
	// ** Boot mini from mem code by giantpune ** //
/*	void *mini = memalign(32, armboot_size);  
	if(!mini) 
		  return 0;    

	memcpy(mini, armboot, armboot_size);  
	DCFlushRange(mini, armboot_size);               

	*(u32*)0xc150f000 = 0x424d454d;  
	asm volatile("eieio");  

	*(u32*)0xc150f004 = MEM_VIRTUAL_TO_PHYSICAL(mini);  
	asm volatile("eieio");

	tikview views[4] ATTRIBUTE_ALIGN(32);
	//printf("Shutting down IOS subsystems.\n");
	__IOS_ShutdownSubsystems();
	printf("Loading IOS 254.\n");
	__ES_Init();
	u32 numviews;
	ES_GetNumTicketViews(0x00000001000000FEULL, &numviews);
	ES_GetTicketViews(0x00000001000000FEULL, views, numviews);
	ES_LaunchTitleBackground(0x00000001000000FEULL, &views[0]);

  free(mini);
*/
	// ** boot mini without BootMii IOS code by Crediar ** //

	unsigned char ES_ImportBoot2[16] =
	{
		0x68, 0x4B, 0x2B, 0x06, 0xD1, 0x0C, 0x68, 0x8B, 0x2B, 0x00, 0xD1, 0x09, 0x68, 0xC8, 0x68, 0x42
	};

	u32 i;
	for( i = 0x939F0000; i < 0x939FE000; i+=2 )
	{
		if( memcmp( (void*)(i), ES_ImportBoot2, sizeof(ES_ImportBoot2) ) == 0 )
		{
			DCInvalidateRange( (void*)i, 0x20 );
			
			*(vu32*)(i+0x00)	= 0x48034904;	// LDR R0, 0x10, LDR R1, 0x14
			*(vu32*)(i+0x04)	= 0x477846C0;	// BX PC, NOP
			*(vu32*)(i+0x08)	= 0xE6000870;	// SYSCALL
			*(vu32*)(i+0x0C)	= 0xE12FFF1E;	// BLR
			*(vu32*)(i+0x10)	= 0x10100000;	// offset
			*(vu32*)(i+0x14)	= 0x0000FF01;	// version

			DCFlushRange( (void*)i, 0x20 );

			void *mini = (void*)0x90100000;
			memcpy(mini, armboot, armboot_size);
			DCFlushRange( mini, armboot_size );
			
			s32 fd = IOS_Open( "/dev/es", 0 );
			
			u8 *buffer = (u8*)memalign( 32, 0x100 );
			memset( buffer, 0, 0x100 );
			
			#ifdef DEBUG_OUTPUT
			printf("ES_ImportBoot():%d\n", IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL ) );
			#else
			IOS_IoctlvAsync( fd, 0x1F, 0, 0, (ioctlv*)buffer, NULL, NULL );
			#endif
		}
	}
	#ifdef DEBUG_OUTPUT
	printf("Waiting for mini gecko output.\n");
	while(true)
	{ do DCInvalidateRange(redirectedGecko, 32);
	  while(!*redirectedGecko);
	  printf(redirectedGecko);
	  *redirectedGecko = (char)(0);
	  DCFlushRange(redirectedGecko, 32);
	}
	#else
	printf("Waiting for ARM to reset PPC.");
	#endif
	return 0;
}
