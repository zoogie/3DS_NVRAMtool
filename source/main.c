#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include "checksum.h"

void *buffer;
int size = 1024 * 128;
int cursor = 0;

int dumpNVRAMbin()
{
	Result res = 0;

	FILE *out = fopen("/nvram.bin", "wb");
	if (out == NULL)
	{
		printf("Error opening nvram.bin\n");
		return 1;
	}

	res = fwrite(buffer, 1, size, out);
	fclose(out);
	if (res != size)
	{
		printf("Something went wrong writing data, aborting...\n");
		return 3;
	}

	return 0;
}
int loadNVRAMbin()
{
	Result res = 0;

	FILE *in = fopen("/nvram.bin", "rb");
	if (in == NULL)
	{
		printf("Error opening nvram.bin\n");
		return 1;
	}

	res = fread(buffer, 1, size, in);
	fclose(in);
	if (res != size)
	{
		printf("Something went wrong loading data, aborting...\n");
		return 3;
	}

	return 0;
}

u32 checkCRC16s(u8 *buff, bool fix)
{
	u32 result = 0;
	u16 messageCRC = 0;
	u16 existingCRC = 0;
	u32 index = 0;

	for (int i = 0x1f400; i < 0x1f400+0x600; i+=0x200)
	{
		//wifi settings (slot 4,5,6  - 2 crc16s each)
		if (*(u8*)(buff + i + 0xe7) == 0xff)  //check for unused slot
		{ 
			index+=2;
			continue;
		}

		messageCRC = crc_16(buff + i, 0xFE);	//every crc16 is spaced 0x100 apart
		existingCRC = *(u16*)(buff + i + 0xFE);
		if (fix)
		{
			if (messageCRC != existingCRC) *(u16*)(buff + i + 0xFE) = messageCRC;
			existingCRC = *(u16*)(buff + i + 0xFE);
		}
		if (messageCRC != existingCRC) result |= 1 << index;
		index++;
		
		messageCRC = crc_16(buff + i + 0x100, 0xFE);	//every crc16 is spaced 0x100 apart
		existingCRC = *(u16*)(buff + i + 0x100 + 0xFE);
		if (fix)
		{
			if (messageCRC != existingCRC) *(u16*)(buff + i + 0x100 + 0xFE) = messageCRC;
			existingCRC = *(u16*)(buff + i + 0x100 + 0xFE);
		}
		if (messageCRC != existingCRC) result |= 1 << index;
		index++;
	}

	for (int i = 0x1fa00; i < 0x1fa00+0x300; i+=0x100)
	{
		// slot 1,2,3 - 1 crc16 each)
		if (*(u8*)(buff + i + 0xe7) == 0xff)  //check for unused slot
		{ 
			index++;
			continue;
		}

		messageCRC = crc_16(buff + i, 0xFE);	//every crc16 is spaced 0x100 apart
		existingCRC = *(u16*)(buff + i + 0xFE);
		if (fix)
		{
			if (messageCRC != existingCRC) *(u16*)(buff + i + 0xFE) = messageCRC;
			existingCRC = *(u16*)(buff + i + 0xFE);
		}
		if (messageCRC != existingCRC) result |= 1 << index;
		index++;
	}

	for (int i = 0x1fe00; i < 0x1fe00+0x200; i+=0x100)
	{
		messageCRC = crc_modbus(buff + i, 0x70);
		existingCRC = *(u16*)(buff + i + 0x72);
		if (fix)
		{
			if (messageCRC != existingCRC) *(u16*)(buff + i + 0x72) = messageCRC;
			existingCRC = *(u16*)(buff + i + 0x72);
		}
		if (messageCRC != existingCRC) result |= 1 << index;
		//printf("%04X %04X\n",messageCRC, existingCRC)
		index++;

		messageCRC = crc_modbus(buff + i + 0x74, 0x8a);
		existingCRC = *(u16*)(buff + i + 0xfe);
		if (fix)
		{
			if (messageCRC != existingCRC) *(u16*)(buff + i + 0xfe) = messageCRC;
			existingCRC = *(u16*)(buff + i + 0xfe);
		}
		if (messageCRC != existingCRC) result |= 1 << index;
		//printf("%04X %04X\n",messageCRC, existingCRC);
		index++;
	}

	return result;
}

Result render()
{
	consoleClear();
	int wait=12;
	printf("3DS NVRAM tool v2.0 - zoogie\n\n\n");

	char *choices[] = { 
		"Dump nvram.bin",
		"Restore nvram.bin",
		"Fix nvram.bin crc16s",
		"Exit" };

	int MAX_MENU = sizeof(choices) / 4;   //not portable to the 3ds64XXL

	if (cursor >= MAX_MENU) cursor = 0;
	else if (cursor < 0) cursor = MAX_MENU - 1;

	for (int i = 0; i < MAX_MENU; i++)
	{
		printf("%s %s\n", cursor == i ? "->" : "  ", choices[i]);
	}
	
	printf("\n-------------------------------------------------\n\n");
	//svcSleepThread(150 * 1000 *1000);
	while(wait--) gspWaitForVBlank();  //better than sleepthread methinks
	
	return 0;
}

int main(int argc, char **argv)
{
	u32 res = 0;
	u32 end = 0;
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	res = cfgnorInit(1);
	if (res) printf("cfgnor failed!\n");
	res = CFGNOR_Initialize(1);
	if (res) printf("cfgnor initialize failed!\n");
	buffer = malloc(size);
	if (!buffer) printf("Error allocating buffer\n");
	if(res || !buffer) while(1) gspWaitForVBlank();   //lol
	
	char *slotnames[]={
		"DSi slot1 - 1st half",
		"DSi slot1 - 2nd half",
		"DSi slot2 - 1st half",
		"DSi slot2 - 2nd half",
		"DSi slot3 - 1st half",
		"DSi slot3 - 2nd half",
		"DS  slot1",
		"DS  slot2",
		"DS  slot3",
		"DS profile1 - 1st half",
		"DS profile1 - 2nd half",
		"DS profile2 - 1st half",
		"DS profile2 - 2nd half"
	};

	render();

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();
		u32 kDown = hidKeysDown();

		if (kDown & KEY_UP) cursor--;
		else if (kDown & KEY_DOWN) cursor++;

		if (kDown)
		{
			render();
			if (kDown & KEY_A)
			{
				switch (cursor)
				{
					case 0:
						res = cfgnorDumpFlash(buffer, size);
						if(!res) res = dumpNVRAMbin();
						printf("dump %08lX %s\n", res, res==0 ? "success":"fail");
						break;

					case 1:
						res = loadNVRAMbin();
						if(!res) res = cfgnorWriteFlash(buffer, size);
						printf("write %08lX %s\n", res, res==0 ? "success":"fail");
						break;

					case 2:
						res = loadNVRAMbin();
						printf("load %08lX %s\n", res, res==0 ? "success":"fail");
						if(res) break;
						else res = checkCRC16s(buffer, false);
						if (res)
						{
							printf("nvram.bin has bad crc16(s) %08lX:\n", res);
							for(int i=0; i<sizeof(slotnames)/4; i++){
								if(1<<i & res) printf("%s\n", slotnames[i]);
							}				
							
							printf("\nfixing... ");
							res = checkCRC16s(buffer, true);
							
							if (res)
							{
								printf("failed!\n");
							}
							else
							{
								printf("success!\n");
								dumpNVRAMbin();
							}
						}
						else printf("nvram.bin is fine! %08lX\n", res);
						break;

					case 3:
						end = 1;
						break;

					default:
						;
				}
				//if(!end) svcSleepThread(2000 * 1000 * 1000);
			}

			if (end) break;

			// Flush and swap framebuffers
			gfxFlushBuffers();
			gfxSwapBuffers();

			//Wait for VBlank
			gspWaitForVBlank();
		}
	}

	free(buffer);
	CFGNOR_Shutdown();
	cfgnorExit();

	gfxExit();
	return 0;
}