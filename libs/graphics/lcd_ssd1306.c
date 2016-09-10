/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Graphics Backend for drawing to simple black and white SPI displays
 * (PCD8544 - Nokia 5110 LCD)
 * FIXME: UNFINISHED
 * ----------------------------------------------------------------------------
 */

#include "platform_config.h"
#include "jsutils.h"
#include "lcd_ssd1306.h"
#include "jsspi.h"
#include "jsvar.h"
#include "jswrapper.h"

#define LCD_WIDTH 64
#define LCD_HEIGHT 32

const char *lcdInitData = {
  0xAe,
  0xD5,
  0x80,
  0xA8, 31,
  0xD3,0x0,
  0x40,
  0x8D, 0x14,
  0x20,0x0,
  0xA1,
  0xC8,
  0xDA, 0x12,
  0x81, 0xCF,
  0xD9, 0xF1,
  0xDb, 0x40,
  0xA4,
  0xA6,
  0xAf
};
const spi_sender_data lcdSPIInfo = {
    baudRate : 100000,
    baudRateSpec : SPIB_DEFAULT,
    pinSCK : LCD_SPI_SCK,
    pinMISO : PIN_UNDEFINED,
    pinMOSI : LCD_SPI_MOSI,
    spiMode : SPIF_SPI_MODE_0,
    spiMSB : true
};
#define LCD_SPI_SEND(X) jsspiFastSoftwareFunc(X, &lcdSPIInfo);
unsigned char lcdPixels[LCD_WIDTH*LCD_HEIGHT/8];

unsigned int lcdGetPixel_SSD1306(JsGraphics *gfx, short x, short y) {
  int yp = y>>3;
  int addr = x + (yp*gfx->data.width);
  return (lcdPixels[addr]>>(y&7)) & 1;
}


void lcdSetPixel_SSD1306(JsGraphics *gfx, short x, short y, unsigned int col) {
  int yp = y>>3;
  int addr = x + (yp*gfx->data.width);
  if (col) lcdPixels[addr] |= 1<<(y&7);
  else lcdPixels[addr] &= ~(1<<(y&7));
}

void lcdFlip_SSD1306(JsGraphics *gfx) {
  if (gfx->data.modMaxX>=gfx->data.modMinX && gfx->data.modMaxY>=gfx->data.modMinY) {
    // write...
    jshPinSetValue(LCD_SPI_CS, 0);
    jshPinSetValue(LCD_SPI_DC, 0);

    LCD_SPI_SEND(0x21);
    LCD_SPI_SEND(0);
    LCD_SPI_SEND(0x7F);
    LCD_SPI_SEND(0x22);
    LCD_SPI_SEND(0);
    LCD_SPI_SEND(7);
    int i,x;
    for (i=0;i<4;i++) {
      LCD_SPI_SEND(0xb0+i);
      LCD_SPI_SEND(0x00);
      LCD_SPI_SEND(0x12);
      jshPinSetValue(LCD_SPI_DC, 1);
      for (x=0;x<64;i++)
        LCD_SPI_SEND(lcdPixels[64*i + x]);
      jshPinSetValue(LCD_SPI_DC, 0);
    }
    jshPinSetValue(LCD_SPI_CS, 1);

    gfx->data.modMaxX = -32768;
    gfx->data.modMaxY = -32768;
    gfx->data.modMinX = 32767;
    gfx->data.modMinY = 32767;
  }
}

void SSD1306_flip_fn(JsVar *parent) {
  JsGraphics gfx;
  if (!graphicsGetFromVar(&gfx, parent)) return;
  lcdFlip_SSD1306(&gfx);
  graphicsSetVar(&gfx); // gfx data changed because modified area
}

void lcdInit_SSD1306(JsGraphics *gfx) {
  gfx->data.width = LCD_WIDTH;
  gfx->data.height = LCD_HEIGHT;
  gfx->data.bpp = 1;

  jshPinOutput(LCD_SPI_RST, 0); // actually reset
  jshPinOutput(LCD_SPI_CS, 1);
  jshPinOutput(LCD_SPI_DC, 1);
  jshPinOutput(LCD_SPI_MOSI, 0);
  jshPinOutput(LCD_SPI_SCK, 0);
  jshDelayMicroseconds(1000);
  jshPinSetValue(LCD_SPI_RST, 1); // un-reset
  jshDelayMicroseconds(5000);
  jshPinSetValue(LCD_SPI_CS, 0);
  jshPinSetValue(LCD_SPI_DC, 0);
  unsigned int i;
  for (i=0;i<sizeof(lcdInitData);i++)
    LCD_SPI_SEND(lcdInitData[i]);
  jshPinSetValue(LCD_SPI_DC, 1);
  jshPinSetValue(LCD_SPI_CS, 1);

  JsVar *flip = jsvNewNativeFunction((void (*)(void))SSD1306_flip_fn, JSWAT_THIS_ARG);
  jsvUnLock(jsvObjectSetChild(gfx->graphicsVar, "flip", flip));
}

void lcdSetCallbacks_SSD1306(JsGraphics *gfx) {
  gfx->data.width = LCD_WIDTH;
  gfx->data.height = LCD_HEIGHT;
  gfx->data.bpp = 1;
  gfx->setPixel = lcdSetPixel_SSD1306;
  gfx->getPixel = lcdGetPixel_SSD1306;
}

