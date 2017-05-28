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
 * Graphics Backend for 16 bit parallel LCDs (ILI9325 and similar)
 * ----------------------------------------------------------------------------
 */
#include "graphics.h"

void lcdInit_FSMC(JsGraphics *gfx);
void lcdSetCallbacks_FSMC(JsGraphics *gfx);
