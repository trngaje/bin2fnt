/*
 * This program converts Windows .fnt fonts to C sources.
 * Copyright (c) 2001 Alexander Babichev <asso@3dstation.ru>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "options.h"
#include "font.h"
#include "fnt.h" // added by trngaje


#define VERSION "1,0"



static FILE* inFile;
static FILE* outFile;
static int genStatic = 0;
static int genConst = 0;
static char* inputFileName = NULL;
static char* outputFileName = NULL;
static char* fontName = NULL;
static int genInclude = 0;
static int useGlobalInclude = 0;
static char* includePath = "";

fnt_df_headers *g_fnt;

static void* smalloc(size_t len)
{
    void *d = malloc(len);
    if (! d)
    {
        fprintf(stderr, "Error allocating %i bytes\n", len);
        exit(1);
    }
    return d;
}



/* print short help and terminate program */
void printHelp()
{
    printf("USAGE: bin2c [options]\n");
    printf("Options:\n");
    printf("  -i filename   input file name, standardt input by default\n");
    printf("  -o filename   output file name, standardt output by default\n");
    printf("  -n name       generated variable name\n");
    printf("  -s            decalre variables as static\n");
    printf("  -c            decalre variables as const\n");
    printf("  -l            generate include \"font.h\" statenment\n");
    printf("  -g            use '<' instead od '\"' in -l option\n");
    printf("  -p            optional path to font.h for -l option\n");
    printf("  --help        this message\n");
    printf("  --version     version number\n");
    exit(0);
}


/* print version number */
void printVersion()
{
    printf(".fnt to .c converter\nversion %s\n", VERSION);
    printf("Copyright 2001 Alexander Babichev <asso@3dstation.ru>\n");
    exit(0);
}


/* parse command line arguments */
static void parseParams(int argc, char* argv[])
{
    int i = 1;
    
    while (i < argc)
    {
        if (! strcmp(argv[i], "--help"))
            printHelp();
        else if (! strcmp(argv[i], "--version"))
            printVersion();
        else if (! strcmp(argv[i], "-s"))
            genStatic = 1;
        else if (! strcmp(argv[i], "-c"))
            genConst = 1;
        else if (! strcmp(argv[i], "-l"))
            genInclude = 1;
        else if (! strcmp(argv[i], "-g"))
        {
            useGlobalInclude = 1;
            genInclude = 1;
        }
        else if ((! strcmp(argv[i], "-i")) && (i < argc))
            inputFileName = argv[++i];
        else if ((! strcmp(argv[i], "-o")) && (i < argc))
            outputFileName = argv[++i];
        else if ((! strcmp(argv[i], "-n")) && (i < argc))
        {
            fontName = strdup(argv[++i]);
            if (! fontName)
            {
                fprintf(stderr, "Error allocating memory\n");
                exit(1);
            }
        }
        else if ((! strcmp(argv[i], "-p")) && (i < argc))
        {
            includePath = argv[++i];
            genInclude = 1;
        }
        else
        {
            fprintf(stderr, "Invalid option '%s'\n", argv[i]);
            printHelp();
        }
        i++;
    }
}


/* return allocated file name without extension */
static char* extractFileName(char *fileName)
{
    char *dp, *s, *p;
    int len;

    if (! fileName) return NULL;

    dp = strchr(fileName, '.');
    if ((! dp) || (dp == fileName))
        return NULL;
    
    p = dp;
    while ((p != fileName) && (*p != '\\') && (*p != '/'))
        p--;
    
    len = dp - p;
    s = (char*)smalloc(len + 1);
    memcpy(s, p, len);
    s[len] = 0;

    return s;
}


/* calculates name of font variable if not specified */
static void setFontName()
{
    if (fontName) return;

    if (outputFileName)
    {
        fontName = extractFileName(outputFileName);
        if (fontName) return;
    }

    if (inputFileName)
    {
        fontName = extractFileName(inputFileName);
        if (fontName) return;
    }

    fontName = strdup("font");
    if (! fontName)
    {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
}




/* open input and output files */
static int openFiles()
{
    if (! inputFileName)
        inFile = stdin;
    else
    {
        inFile = fopen(inputFileName, "rb");
        if (! inFile)
        {
            fprintf(stderr, "Error opening input file\n");
            return -1;
        }
    }

    if (! outputFileName)
        outFile = stdout;
    else
    {
        outFile = fopen(outputFileName, "wb");
        if (! outFile)
        {
            fprintf(stderr, "Error opening output file\n");
            if (inputFileName)
                fclose(inFile);
            return -1;
        }
    }

    return 0;
}



/* close file handles */
static void closeFiles()
{
    if (inputFileName)
        fclose(inFile);
    if (outputFileName)
        fclose(outFile);
}



/* read input stream to memory */
unsigned char* readFile()
{
    unsigned char b[6], *buf;
    unsigned long int size;

    if (fread(b, 1, 6, inFile) != 6) 
        return NULL;

    if (! ((b[0] == 0) && ((b[1] == 0x02) || (b[1] == 0x03))))
    {
        fprintf(stderr, "Invalid version of font file.\n");
        return NULL;
    }
    
    size = b[2] | (b[3] << 8) | (b[4] << 16) | (b[5] << 24);
    if ((size < 0x86) || (size > 1024000000))
    {
        fprintf(stderr, "Invalid size of font file.\n");
        return NULL;
    }

    buf = (unsigned char*)smalloc(size);
    if (fread(buf + 6, 1, size - 6, inFile) != size - 6) return NULL;
    memcpy(buf, b, 6);

    return buf;
}



/* parse font */
SFont* parseFont(unsigned char *buf)
{
    SFont font, *f;
    int type, i, charCnt, pos, gsize, version, size, glyphsSize;
    unsigned long int offset, gpos;

    memset(&font, 0, sizeof(SFont));
    type = buf[0x43];
    if ((type & 0x05) != 0x00)
    {
        fprintf(stderr, "Invalid font type.  Only raster fonts supported.\n");
        return NULL;
    }

    version = buf[1];
    size = buf[2] | (buf[3] << 8) | (buf[4] << 16) | (buf[5] << 24);

    font.firstChar = buf[0x5F];
    font.lastChar = buf[0x60];
    font.defaultChar = buf[0x61] /*+ font.firstChar*/;
    font.height = buf[0x58] | (buf[0x59] << 8);
    font.fixedWidth = buf[0x56] | (buf[0x57] << 8);
    
    charCnt = font.lastChar - font.firstChar + 1;
    glyphsSize = 0;
    font.chars = (SGlyphInfo*)smalloc(charCnt * sizeof(SGlyphInfo));
    if (font.fixedWidth)
    {
        int bytesWidth = font.fixedWidth / 8;
        if (bytesWidth * 8 < font.fixedWidth)
            bytesWidth++;
        for (i = 0; i < charCnt; i++)
        {
            font.chars[i].width = font.fixedWidth;
            font.chars[i].byteWidth = bytesWidth;
        }
        glyphsSize = bytesWidth * charCnt * font.height;
    }
    else
    {
        int charWidth;
        if (version == 3)
            pos = 0x94;
        else
            pos = 0x76;
        for (i = 0; i < charCnt; i++)
        {
            charWidth = buf[pos] | (buf[pos + 1] << 8);
            font.chars[i].width = charWidth;
            font.chars[i].byteWidth = charWidth / 8;
            if (font.chars[i].byteWidth * 8 < charWidth)
                font.chars[i].byteWidth++;
            glyphsSize += font.chars[i].byteWidth;
            if (version == 3)
                pos += 6;
            else
                pos += 4;
        }
        glyphsSize = glyphsSize * charCnt * font.height;
    }

    font.bitmaps = (unsigned char*)smalloc(glyphsSize);
    offset = 0;
    if (version == 3)
        pos = 0x94;
    else
        pos = 0x76;
    for (i = 0; i < charCnt; i++)
    {
        gsize = font.chars[i].byteWidth * font.height;
        if (version == 3)
            gpos = buf[pos + 2] | (buf[pos + 3] << 8) | (buf[pos + 4] << 16) | (buf[pos + 5] << 24);
        else
            gpos = buf[pos + 2] | (buf[pos + 3] << 8);
		
		printf("[trngaje] pos=0x%x, [0x%02x, 0x%02x, 0x%02x, 0x%02x]\n", pos, buf[pos], buf[pos+1], buf[pos+2], buf[pos+3]);
		printf("[trngaje] i=%d:gpos=0x%x\n", i, gpos);
		
        memcpy(font.bitmaps + offset, buf + gpos, gsize);
        font.chars[i].offset = offset;
        offset += gsize;
        if (version == 3)
            pos += 6;
        else
            pos += 4;
    }

    f = (SFont*)smalloc(sizeof(SFont));
    memcpy(f, &font, sizeof(SFont));

    return f;
}



/* free font */
static void freeFont(SFont *font)
{
    if (! font) return;

    if (font->chars)
        free(font->chars);
    if (font->bitmaps)
        free(font->bitmaps);
    free(font);
}


/* print prefix of variable declaration */
static void printVarPrefix()
{
    if (genStatic)
        fprintf(outFile, "static ");
    if (genConst)
        fprintf(outFile, "const ");
}


#define WIDTH 10
#define HEIGHT  10	
#define BUFFERSIZE ((WIDTH * HEIGHT + 7) / 8)

static unsigned char image[BUFFERSIZE];


/* write font to C file */
static void dumpFont(SFont *font)
{
    int charCnt, i, pos, j, k;
	
    
	
    if (! font) return;
    
    fprintf(outFile, "/* This is generated file.  Do not edit! */\n\n");
    if (genInclude)
    {
        if (useGlobalInclude)
            fprintf(outFile, "#include <%sfont.h>\n\n", includePath);
        else
            fprintf(outFile, "#include \"%sfont.h\"\n\n", includePath);
    }
    
    charCnt = font->lastChar - font->firstChar + 1;
    
    printVarPrefix();
    fprintf(outFile, "SGlyphInfo %s_glyphs_info[] = {\n", fontName);
    for (i = 0; i < charCnt; i++)
    {
        fprintf(outFile, "    /* 0x%.2X */   { %i, %i, %i }", i + font->firstChar, 
                font->chars[i].width, font->chars[i].byteWidth, font->chars[i].offset);
        if (i != charCnt - 1)
            fprintf(outFile, ", ");
        fprintf(outFile, "\n");
    }
    fprintf(outFile, "};\n\n");
        
    printVarPrefix();
    fprintf(outFile, "unsigned char %s_bitmaps[] = {\n", fontName);
    for (i = 0; i < charCnt; i++)
    {
        fprintf(outFile, "    /* 0x%.2X */\n    ", i + font->firstChar);
        pos = font->chars[i].offset;
        for (j = 0; j < font->chars[i].byteWidth; j++)
        {
            for (k = 0; k < font->height; k++)
            {
                fprintf(outFile, "0x%.2X", font->bitmaps[pos]);
                if (! ((i == charCnt - 1) && (j == font->chars[i].byteWidth - 1) && 
                            (k == font->height - 1)))
                    fprintf(outFile, ", ");
                pos++;
            }
            fprintf(outFile, "\n");
            if (j != font->chars[i].byteWidth - 1) 
                fprintf(outFile, "    ");
        }
		
		// display font
		//font->bitmaps[pos]
		printf("\n");
		int x,y;
		// for fnt
		int xbit;
		int xoffset;
		// for bin
		unsigned char rem;
		unsigned char offset;
		memset (image, 0, BUFFERSIZE);
		
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
				if (font->bitmaps[pos + xoffset*10 + y] & xbit) // 폰트 크기로 값 변경 필요
				{
					image[offset] |= rem;	
					printf("#");
				}
				else
				{
					printf(" ");
				}			
			}
	
			printf("\n");
		}
		
		for(int ii=0; ii<BUFFERSIZE; ii++)
		{
			printf("0x%02x,", image[ii]);
		}
		printf("//0x%x \n", i);
		
    }
    fprintf(outFile, "};\n\n");
    
	
	
    printVarPrefix();
    fprintf(outFile, "SFont %s = {\n", fontName);
    fprintf(outFile, "    0x%.2X, 0x%.2X, 0x%.2X,\n", font->firstChar, font->lastChar, font->defaultChar);
    fprintf(outFile, "    %i, %i,\n", font->height, font->fixedWidth);
    if (genConst)
    {
        fprintf(outFile, "    (SGlyphInfo*)%s_glyphs_info,\n", fontName);
        fprintf(outFile, "    (unsigned char*)%s_bitmaps\n};\n\n", fontName);
    }
    else
    {
        fprintf(outFile, "    %s_glyphs_info,\n", fontName);
        fprintf(outFile, "    %s_bitmaps\n};\n\n", fontName);
    }
}



/* main function, if you didn't notice */
int main(int argc, char *argv[])
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
	
    SFont *f;
    
    //parseParams(argc, argv);
    //setFontName();


	FILE *pFile_Output = NULL;
	FILE *pFile_Input = NULL;
	fnt_df_headers fnth;
	fnt_df_char_table fntctable;
	unsigned char *fnt_image=NULL;
	unsigned char *bin_image=NULL;
	
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
			strcpy(strOutputName, "bitmap10x10_kor.bin");
			
			if (g_opt.iUnicode_start != 0 && g_opt.iUnicode_end != 0)
				sprintf(strInputName, "%s.fnt", font[0]);
			else			
			    sprintf(strInputName, "%s.fnt", font[i]);
			
			fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
			{
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (0xd7a3 - 0xac00 + 1));
				
				pFile_Output = fopen(strOutputName, "rb");
				if (pFile_Output != NULL)
				{
					fread(bin_image, 1, BUFFERSIZE * (0xd7a3 - 0xac00 + 1), pFile_Output);
					fclose(pFile_Output);
				}
			}			
		}	
		else if (usUnicode >= 0x400 && usUnicode <= 0x45f)
		{
			usUnicodeBase = 0x400;
			strcpy(strOutputName, "bitmap10x10_rus.bin");
			
			if (g_opt.iUnicode_start != 0 && g_opt.iUnicode_end != 0)
			{
				sprintf(strInputName, "0x%02x.fnt", usUnicode);
			}
			
			fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
			{
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (0x45f - 0x400 + 1));
				
				pFile_Output = fopen(strOutputName, "rb");
				if (pFile_Output != NULL)
				{
					fread(bin_image, 1, BUFFERSIZE * (0x45f - 0x400 + 1), pFile_Output);
					fclose(pFile_Output);
				}
			}			
		}	
		else if (usUnicode >= 0x00 && usUnicode <= 0xff)
		{
			usUnicodeBase = 0x0;
			strcpy(strOutputName, "bitmap10x10_eng.bin");
			if (g_opt.iUnicode_start != 0 || g_opt.iUnicode_end != 0)
			{
				sprintf(strInputName, "0x%02x.fnt", usUnicode);
			}
			
			if (fnt_image == NULL)
				fnt_image = (unsigned char *)malloc(20);
			if (bin_image == NULL)
			{
				bin_image = (unsigned char *)malloc(BUFFERSIZE * (256));
			
				pFile_Output = fopen(strOutputName, "rb");
				if (pFile_Output != NULL)
				{
					fread(bin_image, 1, BUFFERSIZE * 256, pFile_Output);
					fclose(pFile_Output);
				}	
			}			
		}
				
		pFile_Input = fopen(strInputName, "rb");
		if (pFile_Input != NULL)
		{
			unsigned char *buf;
			//buf = readFile();
			unsigned char b[6];
			unsigned long int size;

			if (fread(b, 1, 6, pFile_Input) != 6) 
				return NULL;

			if (! ((b[0] == 0) && ((b[1] == 0x02) || (b[1] == 0x03))))
			{
				fprintf(stderr, "Invalid version of font file.\n");
				return NULL;
			}
			
			size = b[2] | (b[3] << 8) | (b[4] << 16) | (b[5] << 24);
			if ((size < 0x86) || (size > 1024000000))
			{
				fprintf(stderr, "Invalid size of font file.\n");
				return NULL;
			}

			buf = (unsigned char*)smalloc(size);
			if (fread(buf + 6, 1, size - 6, pFile_Input) != size - 6) return NULL;
			memcpy(buf, b, 6);
	
			if (buf)
			{
				g_fnt = (fnt_df_headers *)buf;
				printf("[trngaje] Version:0x%x\n", g_fnt->Version);
				printf("[trngaje] Size:0x%x\n", g_fnt->Size[0] | g_fnt->Size[1] << 8 | g_fnt->Size[2] << 16 | g_fnt->Size[3] << 24);
				printf("[trngaje] Copyright:%s\n", g_fnt->Copyright);
				printf("[trngaje] Type:0x%x\n", g_fnt->Type);
				printf("[trngaje] PixWidth:0x%x\n", g_fnt->PixWidth);
				printf("[trngaje] PixHeight:0x%x\n", g_fnt->PixHeight);
				printf("[trngaje] FirstChar:0x%x\n", g_fnt->FirstChar);
				printf("[trngaje] LastChar:0x%x\n", g_fnt->LastChar);
				
				//f = parseFont(buf);

				int type, i, charCnt, pos, gsize, version, size, glyphsSize;
				unsigned long int gpos;

				type = g_fnt->Type;
				if ((type & 0x05) != 0x00)
				{
					fprintf(stderr, "Invalid font type.  Only raster fonts supported.\n");
					return NULL;
				}

				version = g_fnt->Version;
				size = g_fnt->Size[0] | g_fnt->Size[1] << 8 | g_fnt->Size[2] << 16 | g_fnt->Size[3] << 24;
				charCnt = g_fnt->LastChar - g_fnt->FirstChar + 1;

				//offset = 0;
				if (version == 3)
					pos = 0x94;
				else
					pos = 0x76;

				if (version == 3)
					gpos = buf[pos + 2] | (buf[pos + 3] << 8) | (buf[pos + 4] << 16) | (buf[pos + 5] << 24);
				else
					gpos = buf[pos + 2] | (buf[pos + 3] << 8);
				
				printf("[trngaje] gpos=0x%x\n", gpos);
				
				printf("\n");
				int x,y;
				// for fnt
				int xbit;
				int xoffset;
				// for bin
				unsigned char rem;
				unsigned char offset;
				memset (image, 0, BUFFERSIZE);
				
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
						if (buf[gpos + xoffset*10 + y] & xbit) // 폰트 크기로 값 변경 필요
						{
							image[offset] |= rem;	
							printf("#");
						}
						else
						{
							printf(" ");
						}			
					}
			
					printf("\n");
				}
				
				for(int ii=0; ii<BUFFERSIZE; ii++)
				{
					printf("0x%02x,", image[ii]);
				}
				
				// 원래 폰트에 변경된 .fnt 로 overwritng 시킨다.
				memcpy(bin_image + (usUnicode - usUnicodeBase) * BUFFERSIZE, image, BUFFERSIZE);
						
			
	
			
				printf("//0x%x \n", i);
			}

			if (buf != NULL)
			{
				free(buf);
				buf = NULL;
			}
			
			fclose(pFile_Input);
		}
		
	}
	
	if (usUnicode >= 0xac00 && usUnicode <= 0xd7a3)
	{
		pFile_Output = fopen("bitmap10x10_kor.bin.new", "wb");
		if (pFile_Output != NULL)
		{
			fwrite(bin_image, 1, BUFFERSIZE * (0xd7a3 - 0xac00 + 1), pFile_Output);
			fclose(pFile_Output);
		}
	}
	else if (usUnicode >= 0x400 && usUnicode <= 0x45f)
	{
		pFile_Output = fopen("bitmap10x10_rus.bin.new", "wb");
		if (pFile_Output != NULL)
		{
			fwrite(bin_image, 1, BUFFERSIZE * (0x45f - 0x400 + 1), pFile_Output);
			fclose(pFile_Output);
		}
	}
	else if (usUnicode >= 0x0 && usUnicode <= 0xff)
	{
		pFile_Output = fopen("bitmap10x10_eng.bin.new", "wb");
		if (pFile_Output != NULL)
		{
			fwrite(bin_image, 1, BUFFERSIZE * 256, pFile_Output);
			fclose(pFile_Output);
		}
	}	
    return 0;
}

