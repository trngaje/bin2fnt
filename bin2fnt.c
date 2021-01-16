/* 이 코드는 rgui용 font를 .fnt 로 변환하는 코드입니다.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "options.h"
#include "fnt.h"

#define WIDTH 10
#define HEIGHT  10	
#define BUFFERSIZE ((WIDTH * HEIGHT + 7) / 8)

int main(int argc, char **argv) 
{
	int c;
    int option_index = 0;

	while ((c = getopt_long(argc, argv, "c:u:l:s:e:vh", longopts, &option_index)) != -1)
	{
		switch (c)
		{
			case 'c': 
						if ((optarg[0] & 0xf0) == 0xe0)
						{
							if ((optarg[1] & 0xc0) == 0x80)
								if ((optarg[2] & 0xc0) == 0x80)
								{
									g_opt.usExtractUnicode = ((optarg[0] & 0xf) << 12) | ((optarg[1] & 0x3f) << 6) | (optarg[2] & 0x3f);
								}
						}
						else
						{
							g_opt.usExtractUnicode = (unsigned short) optarg[0];	
						}
				break;

			case 'u': g_opt.usExtractUnicode = (unsigned short)strtol(optarg, NULL, 0);
				break;

			case 'l' : g_opt.strExtractListPath = optarg;
				break;

			case 's' : g_opt.iUnicode_start = (int)strtol(optarg, NULL, 0); // hex 값
				break;
			case 'e' : g_opt.iUnicode_end = (int)strtol(optarg, NULL, 0); // hex 값
				break;
				
			case 'v' : g_opt.opt_verbose = 1;
				break;
			case 'h' : 
				break;
	
			default:
				printf("Unknown option. '%s'\n", longopts[option_index].name);
                exit(EXIT_FAILURE);
		}
	}
	
	
	printf("[trngaje] usExtractUnicode=0x%x\n", g_opt.usExtractUnicode);
	printf("[trngaje] strExtractListPath=%s\n", g_opt.strExtractListPath);
	printf("[trngaje] iUnicode_start=0x%x\n", g_opt.iUnicode_start);
	printf("[trngaje] iUnicode_end=0x%x\n", g_opt.iUnicode_end);
	printf("[trngaje] opt_verbose=%d\n", g_opt.opt_verbose);
	//exit(1);
	
	

	FILE *pFile_Output = NULL;
	FILE *pFile_Input = NULL;
	fnt_df_headers fnth;
	fnt_df_char_table fntctable;
	unsigned char *fnt_image = NULL;
	unsigned char *bin_image = NULL;
	
	unsigned long size;
	char strInputName[256];
	char strOutputName[256];
	
	// 한글 : 빠, 별, 열, 일, 얼, 열, 앤
	//char font[][4] = {"빠", "별", "열", "일", "얼", "열", "앤"};
	char font[][4] = {"할", "변", "실", "기", "알"};
	
	int i, fntcnt;
	unsigned short usUnicode=0;
	unsigned short usUnicodeBase=0;
	
	if (g_opt.usExtractUnicode != 0)
		fntcnt = 1;
	else if (g_opt.iUnicode_start != 0 || g_opt.iUnicode_end != 0)
		fntcnt = g_opt.iUnicode_end - g_opt.iUnicode_start + 1;
	else 
		fntcnt = sizeof(font) / 4;
	
	for (i=0; i<fntcnt; i++)
	{
		if (g_opt.usExtractUnicode != 0)
		{
			usUnicode = g_opt.usExtractUnicode;
			if (g_opt.usExtractUnicode >= 0x80)
			{
				// unicode to utf8
				font[0][2] = (usUnicode & 0x3f) | 0x80;
				font[0][1] = ((usUnicode >> 6) & 0x3f) | 0x80;
				font[0][0] = ((usUnicode >> 12) & 0xf) | 0xe0;
				font[0][3] = 0;
			}
			else{
				// english, ascii code range
				font[0][0] = (unsigned char)g_opt.usExtractUnicode;
				font[0][1] = 0;			
			}
		}
		else if (g_opt.iUnicode_start != 0 || g_opt.iUnicode_end != 0)
		{
			usUnicode = g_opt.iUnicode_start + i;
			if (usUnicode >= 0x80)
			{
				// unicode to utf8
				font[0][2] = (usUnicode & 0x3f) | 0x80;
				font[0][1] = ((usUnicode >> 6) & 0x3f) | 0x80;
				font[0][0] = ((usUnicode >> 12) & 0xf) | 0xe0;
				font[0][3] = 0;
			}
			else{
				// english, ascii code range
				font[0][0] = (unsigned char)(g_opt.iUnicode_start + i);
				font[0][1] = 0;			
			}			
		}
		else
		{
			if ((font[i][0] & 0xf0) == 0xe0)
			{
				if ((font[i][1] & 0xc0) == 0x80)
					if ((font[i][2] & 0xc0) == 0x80)
					{
						usUnicode = ((font[i][0] & 0xf) << 12) | ((font[i][1] & 0x3f) << 6) | (font[i][2] & 0x3f);
					}
			}
			else{
				usUnicode = (unsigned short) font[i][0];
			}
		}
		
		if (g_opt.iUnicode_start != 0 || g_opt.iUnicode_end != 0)
			printf("%s, code=0x%x\n", font[0], usUnicode);
		else
			printf("%s, code=0x%x\n", font[i], usUnicode);
		
		if (usUnicode >= 0xac00 && usUnicode <= 0xd7a3)
		{
			usUnicodeBase = 0xac00;
			strcpy(strInputName, "bitmap10x10_kor.bin");
			if (g_opt.iUnicode_start != 0 && g_opt.iUnicode_end != 0)
				sprintf(strOutputName, "%s.fnt", font[0]);
			else
				sprintf(strOutputName, "%s.fnt", font[i]);
			
			if (fnt_image == NULL)
				fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (0xd7a3 - 0xac00 + 1));
			
			
			pFile_Input = fopen(strInputName, "rb");
			if (pFile_Input != NULL)
			{
				fread(bin_image, 1, BUFFERSIZE * (0xd7a3 - 0xac00 + 1), pFile_Input);
				fclose(pFile_Input);
			}			
		}
		else if (usUnicode >= 0x400 && usUnicode <= 0x45f) // russian
		{
			usUnicodeBase = 0x400;
			strcpy(strInputName, "bitmap10x10_rus.bin");
			if (g_opt.iUnicode_start != 0 && g_opt.iUnicode_end != 0)
				sprintf(strOutputName, "0x%02x.fnt", usUnicode);
/*			
				sprintf(strOutputName, "%s.fnt", font[0]);
			else
				sprintf(strOutputName, "%s.fnt", font[i]);
*/			
			if (fnt_image == NULL)
				fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (0x45f - 0x400 + 1));
			
			
			pFile_Input = fopen(strInputName, "rb");
			if (pFile_Input != NULL)
			{
				fread(bin_image, 1, BUFFERSIZE * (0x45f - 0x400 + 1), pFile_Input);
				fclose(pFile_Input);
			}			
		}		
		else if (usUnicode >= 0x00 && usUnicode <= 0xff)
		{
			usUnicodeBase = 0x0;
			strcpy(strInputName, "bitmap10x10_eng.bin.org");
			if (g_opt.iUnicode_start != 0 || g_opt.iUnicode_end != 0)
			{

				sprintf(strOutputName, "0x%02x.fnt", usUnicode);
				//	sprintf(strOutputName, "%s.fnt", font[0]);
			}
			
			if (fnt_image == NULL)
				fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (256));
			
			
			pFile_Input = fopen(strInputName, "rb");
			if (pFile_Input != NULL)
			{
				fread(bin_image, 1, BUFFERSIZE * 256, pFile_Input);
				fclose(pFile_Input);
			}			
		}			
		
		int ii, empty;
		empty = BUFFERSIZE;
		for (ii=0; ii<BUFFERSIZE; ii++)
			if (bin_image[(usUnicode - usUnicodeBase)*BUFFERSIZE + ii] == 0)
				empty--;
		if (empty == 0) 
		{
			printf("[trngaje] 0x%x, %s is empty\n", usUnicode, strOutputName);
			continue; // skip empty characters
		}
			
		pFile_Output = fopen(strOutputName, "wb");
		
		if (pFile_Output != NULL)
		{
			memset(&fnth, 0, sizeof(fnth));
			fnth.Version = 0x200;
			strcpy(fnth.Copyright, "trngaje font");
			fnth.Type = 0;
			fnth.PixWidth = 10;
			fnth.PixHeight = 10;
			fnth.FirstChar = 0;
			fnth.LastChar = 0;
			
			size = sizeof(fnth) + 4 * 1 + 20 * 1;
			printf("[trngaje] size of header = 0x%x, filesize = 0x%x\n", sizeof(fnth), size);
			fnth.Size[0] = size & 0xff;
			size = size >> 8;
			fnth.Size[1] = size & 0xff;
			size = size >> 8;
			fnth.Size[2] = size & 0xff;
			size = size >> 8;
			fnth.Size[3] = size & 0xff;
			printf("[trngaje] fnth.Size = 0x%x, 0x%x, 0x%x, 0x%x\n", fnth.Size[0], fnth.Size[1], fnth.Size[2], fnth.Size[3]);
			
			fnth.WidthBytes[0] = 0x2;
			fnth.AvgWidth[0] = 0xa;
			fnth.MaxWidth[0] = 0xa;

			fntctable.width = 10;
			fntctable.offset = sizeof(fnth) + 4 * 1;
			
			

			if (fnt_image != NULL)
			{

				
				// transfer bin to fnt
				int x,y;
				// for fnt
				int xbit;
				int xoffset;
				// for bin
				unsigned char rem;
				unsigned char offset;
				
				memset(fnt_image, 0, 20);		
				
				for (y=0; y<10; y++) // 폰트 크기로 값 변경 필요
				{
					for (x=0; x<10; x++) // 폰트 크기로 값 변경 필요
					{
						// for .fnt
						xbit = 1 << (7-(x%8));
						xoffset = x / 8;
						// for .bin
						rem = 1 << ((x + y * 10) & 7);	// 폰트 크기로 값 변경 필요
						offset  = (x + y * 10) >> 3;	// 폰트 크기로 값 변경 필요				
						
						//if (font->bitmaps[pos + xoffset*10 + y] & xbit) // 폰트 크기로 값 변경 필요
						if (bin_image[(usUnicode - usUnicodeBase)*BUFFERSIZE + offset] & rem)
						{
							fnt_image[xoffset*10 + y] |= xbit;	
						}
					}
				}
			
			}
			


			fwrite(&fnth, 1, sizeof(fnth), pFile_Output);
			fwrite(&fntctable, 1, sizeof(fntctable), pFile_Output);
			fwrite(fnt_image, 1, 20, pFile_Output);
		}
		
		
		if (pFile_Output != NULL)
			fclose(pFile_Output);		

		
	}
	

#if 0	
	pFile_Output = fopen("trngaje.fnt", "wb");
	
	if (pFile_Output != NULL)
	{
		memset(&fnth, 0, sizeof(fnth));
		fnth.Version = 0x200;
		strcpy(fnth.Copyright, "trngaje font");
		fnth.Type = 0;
		fnth.PixWidth = 10;
		fnth.PixHeight = 10;
		fnth.FirstChar = 0;
		fnth.LastChar = 0xff;
		
		size = sizeof(fnth) + 4 * 256 + 20 * 256;
		printf("[trngaje] size of header = 0x%x, filesize = 0x%x\n", sizeof(fnth), size);
		fnth.Size[0] = size & 0xff;
		size = size >> 8;
		fnth.Size[1] = size & 0xff;
		size = size >> 8;
		fnth.Size[2] = size & 0xff;
		size = size >> 8;
		fnth.Size[3] = size & 0xff;
		printf("[trngaje] fnth.Size = 0x%x, 0x%x, 0x%x, 0x%x\n", fnth.Size[0], fnth.Size[1], fnth.Size[2], fnth.Size[3]);
		
		fnth.WidthBytes[0] = 0x2;
		fnth.AvgWidth[0] = 0xa;
		fnth.MaxWidth[0] = 0xa;
		int i;
		for(i=0; i<256; i++)
		{
			fntctable[i].width = 10;
			fntctable[i].offset = sizeof(fnth) + 4 * 256 + 20 * i;
		}
		
		fnt_image = (unsigned char *)malloc(20 * 256);
		bin_image = (unsigned char *)malloc(BUFFERSIZE * 256);
		if (fnt_image != NULL)
		{
			pFile_Input = fopen("bitmap10x10_eng.bin", "rb");
			if (pFile_Input != NULL)
			{
				fread(bin_image, 1, BUFFERSIZE * 256, pFile_Input);
				fclose(pFile_Input);
			}
			
			// transfer bin to fnt
			int x,y;
			// for fnt
			int xbit;
			int xoffset;
			// for bin
			unsigned char rem;
			unsigned char offset;
			memset(fnt_image, sizeof(fnt_image), 0);								

			for (i=0; i<256; i++)
			{
				for (y=0; y<10; y++) // 폰트 크기로 값 변경 필요
				{
					for (x=0; x<10; x++) // 폰트 크기로 값 변경 필요
					{
						// for .fnt
						xbit = 1 << (7-(x%8));
						xoffset = x / 8;
						// for .bin
						rem = 1 << ((x + y * 10) & 7);	// 폰트 크기로 값 변경 필요
						offset  = (x + y * 10) >> 3;	// 폰트 크기로 값 변경 필요				
						
						//if (font->bitmaps[pos + xoffset*10 + y] & xbit) // 폰트 크기로 값 변경 필요
						if (bin_image[i*BUFFERSIZE + offset] & rem)
						{
							fnt_image[i*20+ xoffset*10 + y] |= xbit;	
							//printf("#");
						}
						else
						{
							//printf(" ");
						}			
					}
			
					//printf("\n");
				}
			}			
		}
		

		fwrite(&fnth, 1, sizeof(fnth), pFile_Output);
		fwrite(fntctable, 1, sizeof(fntctable), pFile_Output);
		fwrite(fnt_image, 1, 20*256, pFile_Output);
	}
	
	
	if (pFile_Output != NULL)
		fclose(pFile_Output);	
#endif	
    return 0;

}
