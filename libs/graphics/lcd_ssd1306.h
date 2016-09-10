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
 * ----------------------------------------------------------------------------
 */
#include "graphics.h"

void lcdInit_SSD1306(JsGraphics *gfx);
void lcdIdle_SSD1306();
void lcdSetCallbacks_SSD1306(JsGraphics *gfx);
