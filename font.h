#ifndef __FONT_H__
#define __FONT_H__


typedef struct
{
    unsigned short int width;         /* character width in pixels */
    unsigned short int byteWidth;     /* glyph bitmap width in bytes */
    unsigned int offset;              /* bitmap offset to bitmap in bytes */
} SGlyphInfo;


typedef struct
{
    unsigned char firstChar;       /* first character in font */
    unsigned char lastChar;        /* last character in font */
    unsigned char defaultChar;     /* draw this character if requested symbvol not in range */
    unsigned short int height;     /* characters height */
    unsigned short int fixedWidth; /* width of characters in pixels if fixed width font */
    SGlyphInfo *chars;             /* glyphs info array */
    unsigned char *bitmaps;        /* characters bitmaps */
} SFont;


#endif

