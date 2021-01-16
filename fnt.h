#ifndef __FNT_H__
#define __FNT_H__

typedef struct fnt_header
{
unsigned short Version; // offset 0
unsigned char Size[4]; // offset 2
char Copyright[60];		// offset 6
unsigned short Type;	// offset 66
unsigned short Points;	// offset 68
unsigned short VertRes;	// offset 70
unsigned short HorizRes; // offset 72
unsigned short Ascent;	// offset 74
unsigned short InternalLeading;  // offset 76
unsigned short ExternalLeading;
unsigned char Italic; // offset 80
unsigned char Underline; // offset 81
unsigned char StrikeOut; // offset 82
unsigned char Weight[2];	// offset 83
unsigned char CharSet; 	// offset 85
unsigned short PixWidth; // offset 86 (0x56)
unsigned short PixHeight; // offset 88 (0x58)
unsigned char PitchAndFamily; // offset 90
unsigned char AvgWidth[2]; // offset 91
unsigned char MaxWidth[2]; // offset 93
unsigned char FirstChar; // offset 95 (0x5f)
unsigned char LastChar;	// offset 96
unsigned char DefaultChar; // offset 97
unsigned char BreakChar; // offset 98
unsigned char WidthBytes[2]; // offset 99
char Device[4]; 				// offset 101
char Face[4]; 					// offset 105
unsigned char BitsPointer[4];	// offset 109
unsigned char Bitsoffset[4];	// offset 113
unsigned char Reserved;			// offset 117
/*unsigned char Flags[4];			// offset 118 (0x76) , v2
unsigned short Aspace;			// offset 122
unsigned short Bspace;			// offset 124
unsigned short Cspace;			// offset 126
unsigned long ColorPointer;		// offset 128
unsigned char Reserved1[16];	// offset 132
*/
} fnt_df_headers;
// offset 148 (0x94)

typedef struct fnt_char_table
{
unsigned short width;
unsigned short offset;
} fnt_df_char_table;

#endif
