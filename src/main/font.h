#ifndef FONT_H
#define FONT_H


typedef struct{
  int16_t   fontSize;
  uint8_t   bitField;   //1 bits  2 bit 0: smooth; bit 1: unicode; bit 2: italic; bit 3: bold; bit 4: fixedHeigth; bits 5-7: reserved
  uint8_t   charSet;
  uint16_t  stretchH;
  uint8_t   aa;
  uint8_t   paddingUp;
  uint8_t   paddingRight;
  uint8_t   paddingDown;
  uint8_t   paddingLeft;
  uint8_t   spacingHoriz;
  uint8_t   spacingVert;
  uint8_t   outline;    //added with version 2
  char      fontName[]; //n+1 string  14  null terminated string with length n
} __attribute__ ((__packed__)) fontInfo_t;

typedef struct{
  uint16_t lineHeight;
  uint16_t base;
  uint16_t scaleW;
  uint16_t scaleH;
  uint16_t pages;
  uint8_t bitField;     //bits 0-6: reserved; bit 7: packed
  uint8_t alphaChnl;
  uint8_t redChnl;
  uint8_t greenChnl;
  uint8_t blueChnl;
} __attribute__ ((__packed__)) fontCommon_t;

typedef struct{
  uint32_t id;          //The character id.
  uint16_t x;           //The left position of the character image in the texture.
  uint16_t y;           //The top position of the character image in the texture.
  uint16_t width;       //The width of the character image in the texture.
  uint16_t height;      //The height of the character image in the texture.
  int16_t  xoffset;     //How much the current position should be offset when copying the image from the texture to the screen.
  int16_t  yoffset;     //How much the current position should be offset when copying the image from the texture to the screen.
  int16_t  xadvance;    //How much the current position should be advanced after drawing the character.
  uint8_t  page;        //The texture page where the character image is found.
  uint8_t  chnl;        //The texture channel where the character image is found (1 = blue; 2 = green; 4 = red; 8 = alpha; 15 = all channels).
} __attribute__ ((__packed__)) fontChar_t;

typedef struct{
  uint32_t first;       //These fields are repeated until all kerning pairs have been described
  uint32_t second;
  int16_t  amount;
} __attribute__ ((__packed__)) fontKern_t;

typedef struct{
  fontInfo_t *info;
  fontCommon_t *common;
  char *pageNames;      // Zero terminated strings
  int pageNamesLen;     // [bytes]
  fontChar_t *chars;
  int charsLen;         // [bytes]
  fontKern_t *kerns;
  int kernsLen;         // [bytes]
} font_t;

font_t *loadFntFile( char *fileName );
void printFntFile( font_t *fDat );
fontChar_t *getCharInfo( font_t *fDat, char c );
void freeFntFile( font_t *fDat );
void initFont( const char *filePrefix );
void drawChar( char c, uint8_t layer, uint32_t color, uint8_t chOffset );
void setCur( int x, int y );

// draws a zero terminated string into `layer` at x,y with colors cOutline and cFill
void drawStr( const char *str, int x, int y, uint8_t layer, uint32_t cOutline, uint32_t cFill );

// returns expected width of the string rectangle
int getStrWidth( const char *str );

// draws a zero terminated string into `layer` centered on the screen with colors cOutline and cFill
void drawStrCentered( const char *str, uint8_t layer, uint32_t cOutline, uint32_t cFill );

// Returns the number of consecutive `path/0.fnt` files
int cntFntFiles( const char* path );

#endif
