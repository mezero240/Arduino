#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <TouchScreen.h>
#include <SPI.h>
#include <SD.h>

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define SD_CS 10

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, A4);

void setup() {
  Serial.begin(9600);
  tft.reset();
  delay(1000);
  uint16_t identifier = 0x9341;
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  tft.begin(identifier);
  if (!SD.begin(SD_CS)) {
    progmemPrintln(PSTR("SD initialization failed!"));
    return;
  }
  tft.setRotation(1);
}

void loop() {
  bmpDraw("1.bmp", 0, 0);
  delay(2000);
}

#define BUFFPIXEL 20

void bmpDraw(char *filename, int x, int y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset, rowSize;
  uint8_t sdbuffer[3 * BUFFPIXEL];
  uint16_t lcdbuffer[BUFFPIXEL];
  uint8_t buffidx = sizeof(sdbuffer);
  boolean goodBmp = false, flip = true;
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t lcdidx = 0;
  boolean first = true;

  if ((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  progmemPrint(PSTR("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }

  if (read16(bmpFile) == 0x4D42) {
    progmemPrint(PSTR("File size: "));
    Serial.println(read32(bmpFile));
    read32(bmpFile);
    bmpImageoffset = read32(bmpFile);
    progmemPrint(PSTR("Image Offset: "));
    Serial.println(bmpImageoffset, DEC);
    progmemPrint(PSTR("Header size: "));
    Serial.println(read32(bmpFile));
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);

    if (read16(bmpFile) == 1) {
      bmpDepth = read16(bmpFile);
      progmemPrint(PSTR("Bit Depth: "));
      Serial.println(bmpDepth);

      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) {
        goodBmp = true;
        progmemPrint(PSTR("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        rowSize = (bmpWidth * 3 + 3) & ~3;

        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width()) w = tft.width() - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        tft.setAddrWindow(0, 0, 319, 239);

        for (row = 0; row < h; row++) {
          pos = flip ? bmpImageoffset + (bmpHeight - 1 - row) * rowSize : bmpImageoffset + row * rowSize;
          if (bmpFile.position() != pos) {
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer);
          }

          for (col = 0; col < w; col++) {
            if (buffidx >= sizeof(sdbuffer)) {
              if (lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;
            }

            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r, g, b);
          }
        }

        if (lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        }

        progmemPrint(PSTR("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      }
    }
  }

  bmpFile.close();
  if (!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t*)&result)[0] = f.read();
  ((uint8_t*)&result)[1] = f.read();
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t*)&result)[0] = f.read();
  ((uint8_t*)&result)[1] = f.read();
  ((uint8_t*)&result)[2] = f.read();
  ((uint8_t*)&result)[3] = f.read();
  return result;
}

void progmemPrint(const char *str) {
  char c;
  while ((c = pgm_read_byte(str++))) Serial.print(c);
}

void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}
