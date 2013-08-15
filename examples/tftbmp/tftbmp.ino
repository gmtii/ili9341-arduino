#include <SD.h>
#include <SPI.h>
#include "TFTv2.h"
File bmpFile;

unsigned char saved_spimode;
int bmpWidth, bmpHeight;
uint8_t bmpDepth, bmpImageoffset;

#define chipSelect 4

//set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

void setup()
{
  pinMode(11,INPUT);
  pinMode(12,INPUT);
  pinMode(13,INPUT);
  TFT_CS_HIGH;
  pinMode(chipSelect,OUTPUT);
  digitalWrite(chipSelect,HIGH);

  Serial.begin(38400);
  SPI.begin(); 
  Tft.TFTinit();
  //SPI.setClockDivider(SPI_CLOCK_DIV4);
  //SDcard_info();
  /**/
  DDRB |= 0x04;
  card.init(SPI_FULL_SPEED,chipSelect);//SPI_QUARTER_SPEED   SPI_HALF_SPEED, SPI_FULL_SPEED,
  if(!SD.begin(chipSelect))//SPI_QUARTER_SPEED,
  { //53 is used as chip select pin
    Serial.println("failed!");
    while(1);
  }
  Serial.println("SD OK!");

  Tft.setCol(0,239);
  Tft.setPage(0,319);
  Tft.sendCMD(0x2c);//start to write to display ram
  TFT_BL_ON;
}

void loop()
{
  /*
   */
  char bmpfiles[][18]=
  {
    "123.bmp","hibiscus.bmp","test.bmp"
 };
  for(unsigned char i=0;i<3;i++)
  {
    //TFT_BL_OFF;
    bmpFile = SD.open(bmpfiles[i]);
    if (! bmpFile) 
    {
      Serial.println("didnt find image");
      while (1);
    }

    if (! bmpReadHeader(bmpFile)) { 
      Serial.println("bad bmp");
      return;
    }

    Serial.print("image size "); 
    Serial.print(bmpWidth, DEC);
    Serial.print(", ");
    Serial.println(bmpHeight, DEC);
    bmpdraw(bmpFile, 0, 0);
    bmpFile.close();
    //TFT_BL_ON;
    delay(1000);
  }
}

/*********************************************/
// This procedure reads a bitmap and draws it to the screen
// its sped up by reading many pixels worth of data at a time
// instead of just one pixel at a time. increading the buffer takes
// more RAM but makes the drawing a little faster. 20 pixels' worth
// is probably a good place


#define BUFFPIXEL 20

void bmpdraw(File f, int x, int y) 
{
  bmpFile.seek(bmpImageoffset);

  uint32_t time = millis();
  uint16_t p;
  uint8_t g, b;
  int i, j;

  uint8_t sdbuffer[3 * BUFFPIXEL];  // 3 * pixels to buffer
  uint8_t buffidx = 3*BUFFPIXEL;


  for (i=0; i< bmpHeight; i++) 
  {

    for (j=bmpWidth; j>0; j--) 
    {
      // read more pixels
      if (buffidx >= 3*BUFFPIXEL) 
      {
        bmpFile.read(sdbuffer, 3*BUFFPIXEL);
        buffidx = 0;
      }

      // convert pixel from 888 to 565
      b = sdbuffer[buffidx++];     // blue
      g = sdbuffer[buffidx++];     // green
      p = sdbuffer[buffidx++];     // red

      p >>= 3;
      p <<= 6;

      g >>= 2;
      p |= g;
      p <<= 5;

      b >>= 3;
      p |= b;

      // write out the 16 bits of color
      Tft.setPixel(j,i,p);
    }
  }
  Serial.print(millis() - time, DEC);
  Serial.println(" ms");
}

boolean bmpReadHeader(File f) {
  // read header
  uint32_t tmp;

  if (read16(f) != 0x4D42) {
    // magic bytes missing
    return false;
  }

  // read file size
  tmp = read32(f);  
  Serial.print("size 0x"); 
  Serial.println(tmp, HEX);

  // read and ignore creator bytes
  read32(f);

  bmpImageoffset = read32(f);  
  Serial.print("offset "); 
  Serial.println(bmpImageoffset, DEC);

  // read DIB header
  tmp = read32(f);
  Serial.print("header size "); 
  Serial.println(tmp, DEC);
  bmpWidth = read32(f);
  bmpHeight = read32(f);


  if (read16(f) != 1)
    return false;

  bmpDepth = read16(f);
  Serial.print("bitdepth "); 
  Serial.println(bmpDepth, DEC);

  if (read32(f) != 0) {
    // compression not supported!
    return false;
  }

  Serial.print("compression "); 
  Serial.println(tmp, DEC);

  return true;
}

/*********************************************/
// These read data from the SD card file and convert them to big endian 
// (the data is stored in little endian format!)

// LITTLE ENDIAN!
uint16_t read16(File f) 
{
  uint16_t d;
  uint8_t b;
  b = f.read();
  d = f.read();
  d <<= 8;
  d |= b;
  return d;
}

// LITTLE ENDIAN!
uint32_t read32(File f) 
{
  uint32_t d;
  uint16_t b;

  b = read16(f);
  d = read16(f);
  d <<= 16;
  d |= b;
  return d;
}
