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
 * Variables
 * ----------------------------------------------------------------------------
 */
#ifndef JSVARCACHE_H_
#define JSVARCACHE_H_

#include "jsutils.h"
#include "jsvar.h"

/// reference of first unused variable (variables are in a linked list)s
extern volatile JsVarRef jsVarFirstEmpty;

/// Are we doing garbage collection or similar, so can't access memory?
extern volatile bool isMemoryBusy;

// For debugging/testing ONLY - maximum # of vars we are allowed to use
void jsvSetMaxVarsUsed(unsigned int size);

// Init/kill vars as a whole
void jsvInit();
void jsvKill();
void jsvSoftInit(); ///< called when loading from flash
void jsvSoftKill(); ///< called when saving to flash
unsigned int jsvGetMemoryUsage(); ///< Get number of memory records (JsVars) used
unsigned int jsvGetMemoryTotal(); ///< Get total amount of memory records
bool jsvIsMemoryFull(); ///< Get whether memory is full or not
void jsvShowAllocated(); ///< Show what is still allocated, for debugging memory problems
/// Try and allocate more memory - only works if RESIZABLE_JSVARS is defined
void jsvSetMemoryTotal(unsigned int jsNewVarCount);

/// Get a reference from a var - SAFE for null vars
ALWAYS_INLINE JsVarRef jsvGetRef(JsVar *var);

/// SCARY - only to be used for vital stuff like load/save
#ifdef VAR_CACHE
JsVar *_jsvGetAddressOf(JsVarRef ref);
#else
ALWAYS_INLINE JsVar *_jsvGetAddressOf(JsVarRef ref);
#endif

/** Run a garbage collection sweep - return true if things have been freed */
bool jsvGarbageCollect();

// Create a linked list of all empty variables, in order
void jsvCreateEmptyVarList();

#endif /* JSVARCACHE_H_ */
