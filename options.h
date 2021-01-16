#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <getopt.h>
#include <stddef.h>

struct opt
{
	char *strInputName;
	char *strOutputName;

	int iOutputFontType; // 0:bin, 1:c header
	int iInputFontWidth;
	int iInputFontHeight;
	int iUnicode_start;
	int iUnicode_end;
	
	char *strExtractListPath;	
	unsigned short usExtractUnicode;
	
	int opt_no_align;
	int opt_verbose;
};

extern struct opt g_opt;
extern struct option longopts[];

#endif
