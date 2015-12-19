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
#include "jsvar.h"
#include "jsvarcache.h"
#include "jslex.h"
#include "jsparse.h"
#include "jswrap_json.h"
#include "jsinteractive.h"

/** Basically, JsVars are stored in one big array, so save the need for
 * lots of memory allocation. On Linux, the arrays are in blocks, so that
 * more blocks can be allocated. We can't use realloc on one big block as
 * this may change the address of vars that are already locked!
 *
 */

#ifdef RESIZABLE_JSVARS
JsVar **jsVarBlocks = 0;
unsigned int jsVarsSize = 0;
#define JSVAR_BLOCK_SIZE 4096
#define JSVAR_BLOCK_SHIFT 12
#else
JsVar jsVars[JSVAR_CACHE_SIZE];
unsigned int jsVarsSize = JSVAR_CACHE_SIZE;
#endif

JsVarRef jsVarFirstEmpty; ///< reference of first unused variable (variables are in a linked list)

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


/** Return a pointer - UNSAFE for null refs.
 * This is effectively a Lock without locking! */
static ALWAYS_INLINE JsVar *jsvGetAddressOf(JsVarRef ref) {
  assert(ref);
#ifdef RESIZABLE_JSVARS
  JsVarRef t = ref-1;
  return &jsVarBlocks[t>>JSVAR_BLOCK_SHIFT][t&(JSVAR_BLOCK_SIZE-1)];
#else
  return &jsVars[ref-1];
#endif
}

JsVar *_jsvGetAddressOf(JsVarRef ref) {
  return jsvGetAddressOf(ref);
}

// For debugging/testing ONLY - maximum # of vars we are allowed to use
void jsvSetMaxVarsUsed(unsigned int size) {
#ifdef RESIZABLE_JSVARS
  assert(size < JSVAR_BLOCK_SIZE); // remember - this is only for DEBUGGING - as such it doesn't use multiple blocks
#else
  assert(size < JSVAR_CACHE_SIZE);
#endif
  jsVarsSize = size;
}

// Create a linked list of all empty variables, in order
void jsvCreateEmptyVarList() {
  jsVarFirstEmpty = 0;
  JsVar *lastEmpty = 0;
  JsVarRef i;
  for (i=1;i<=jsVarsSize;i++) {
    JsVar *var = jsvGetAddressOf(i);
    if ((var->flags&JSV_VARTYPEMASK) == JSV_UNUSED) {
      jsvSetNextSibling(var, 0);
      if (lastEmpty)
        jsvSetNextSibling(lastEmpty, i);
      else
        jsVarFirstEmpty = i;
      lastEmpty = var;
    } else if (jsvIsFlatString(var)) {
      // skip over used blocks for flat strings
      i = (JsVarRef)(i+jsvGetFlatStringBlocks(var));
    }
  }
}

/** Removes the empty variable counter, cleaving clear runs of 0s
 where no data resides. This helps if compressing the variables
 for storage. */
void jsvClearEmptyVarList() {
  jsVarFirstEmpty = 0;
  JsVarRef i;
  for (i=1;i<=jsVarsSize;i++) {
    JsVar *var = jsvGetAddressOf(i);
    if ((var->flags&JSV_VARTYPEMASK) == JSV_UNUSED) {
      // completely zero it (JSV_UNUSED==0, so it still stays the same)
      memset((void*)var,0,sizeof(JsVar));
    } else if (jsvIsFlatString(var)) {
      // skip over used blocks for flat strings
      i = (JsVarRef)(i+jsvGetFlatStringBlocks(var));
    }
  }
}

void jsvSoftInit() {
  jsvCreateEmptyVarList();
}

void jsvSoftKill() {
  jsvClearEmptyVarList();
}

/** This links all JsVars together, so we can have our nice
 * linked list of free JsVars. It returns the ref of the first
 * item - that we should set jsVarFirstEmpty to (if it is 0) */
static JsVarRef jsvInitJsVars(JsVarRef start, unsigned int count) {
  JsVarRef i;
  for (i=start;i<start+count;i++) {
    JsVar *v = jsvGetAddressOf(i);
    v->flags = JSV_UNUSED;
    // v->locks = 0; // locks is 0 anyway because it is stored in flags
    jsvSetNextSibling(v, (JsVarRef)(i+1)); // link to next
  }
  jsvSetNextSibling(jsvGetAddressOf((JsVarRef)(start+count-1)), (JsVarRef)0); // set the final one to 0
  return start;
}

void jsvInit() {
#ifdef RESIZABLE_JSVARS
  jsVarsSize = JSVAR_BLOCK_SIZE;
  jsVarBlocks = malloc(sizeof(JsVar*)); // just 1
  jsVarBlocks[0] = malloc(sizeof(JsVar) * JSVAR_BLOCK_SIZE);
#endif

  jsVarFirstEmpty = jsvInitJsVars(1/*first*/, jsVarsSize);
  jsvSoftInit();
}

void jsvKill() {
#ifdef RESIZABLE_JSVARS
  unsigned int i;
  for (i=0;i<jsVarsSize>>JSVAR_BLOCK_SHIFT;i++)
    free(jsVarBlocks[i]);
  free(jsVarBlocks);
  jsVarBlocks = 0;
  jsVarsSize = 0;
#endif
}

/// Get number of memory records (JsVars) used
unsigned int jsvGetMemoryUsage() {
  unsigned int usage = 0;
  unsigned int i;
  for (i=1;i<=jsVarsSize;i++) {
    JsVar *v = jsvGetAddressOf((JsVarRef)i);
    if ((v->flags&JSV_VARTYPEMASK) != JSV_UNUSED) {
      usage++;
      if (jsvIsFlatString(v)) {
        unsigned int b = (unsigned int)jsvGetFlatStringBlocks(v);
        i+=b;
        usage+=b;
      }
    }
  }
  return usage;
}

/// Get total amount of memory records
unsigned int jsvGetMemoryTotal() {
  return jsVarsSize;
}

/// Try and allocate more memory - only works if RESIZABLE_JSVARS is defined
void jsvSetMemoryTotal(unsigned int jsNewVarCount) {
#ifdef RESIZABLE_JSVARS
  if (jsNewVarCount <= jsVarsSize) return; // never allow us to have less!
  // When resizing, we just allocate a bunch more
  unsigned int oldSize = jsVarsSize;
  unsigned int oldBlockCount = jsVarsSize >> JSVAR_BLOCK_SHIFT;
  unsigned int newBlockCount = (jsNewVarCount+JSVAR_BLOCK_SIZE-1) >> JSVAR_BLOCK_SHIFT;
  jsVarsSize = newBlockCount << JSVAR_BLOCK_SHIFT;
  // resize block table
  jsVarBlocks = realloc(jsVarBlocks, sizeof(JsVar*)*newBlockCount);
  // allocate more blocks
  unsigned int i;
  for (i=oldBlockCount;i<newBlockCount;i++)
    jsVarBlocks[i] = malloc(sizeof(JsVar) * JSVAR_BLOCK_SIZE);
  /** and now reset all the newly allocated vars. We know jsVarFirstEmpty
   * is 0 (because jsiFreeMoreMemory returned 0) so we can just assign it.  */
  assert(!jsVarFirstEmpty);
  jsVarFirstEmpty = jsvInitJsVars(oldSize+1, jsVarsSize-oldSize);
  // jsiConsolePrintf("Resized memory from %d blocks to %d\n", oldBlockCount, newBlockCount);
#else
  NOT_USED(jsNewVarCount);
  assert(0);
#endif
}

/// Get whether memory is full or not
bool jsvIsMemoryFull() {
  return !jsVarFirstEmpty;
}

// Show what is still allocated, for debugging memory problems
void jsvShowAllocated() {
  JsVarRef i;
  for (i=1;i<=jsVarsSize;i++) {
    if ((jsvGetAddressOf(i)->flags&JSV_VARTYPEMASK) != JSV_UNUSED) {
      jsiConsolePrintf("USED VAR #%d:",i);
      jsvTrace(jsvGetAddressOf(i), 2);
    }
  }
}

/// Get a reference from a var - SAFE for null vars
ALWAYS_INLINE JsVarRef jsvGetRef(JsVar *var) {
  if (!var) return 0;
#ifdef RESIZABLE_JSVARS
  unsigned int i, c = jsVarsSize>>JSVAR_BLOCK_SHIFT;
  for (i=0;i<c;i++) {
    if (var>=jsVarBlocks[i] && var<&jsVarBlocks[i][JSVAR_BLOCK_SIZE]) {
      JsVarRef r = (JsVarRef)(1 + (i<<JSVAR_BLOCK_SHIFT) + (var - jsVarBlocks[i]));
      return r;
    }
  }
  return 0;
#else
  return (JsVarRef)(1 + (var - jsVars));
#endif
}



/** Recursively mark the variable */
static void jsvGarbageCollectMarkUsed(JsVar *var) {
  var->flags &= (JsVarFlags)~JSV_GARBAGE_COLLECT;

  if (jsvHasCharacterData(var)) {
    // non-recursively scan strings
    JsVarRef child = jsvGetLastChild(var);
    while (child) {
      JsVar *childVar;
      childVar = jsvGetAddressOf(child);
      childVar->flags &= (JsVarFlags)~JSV_GARBAGE_COLLECT;
      child = jsvGetLastChild(childVar);
    }
  }
  // intentionally no else
  if (jsvHasSingleChild(var)) {
    if (jsvGetFirstChild(var)) {
      JsVar *childVar = jsvGetAddressOf(jsvGetFirstChild(var));
      if (childVar->flags & JSV_GARBAGE_COLLECT)
        jsvGarbageCollectMarkUsed(childVar);
    }
  } else if (jsvHasChildren(var)) {
    JsVarRef child = jsvGetFirstChild(var);
    while (child) {
      JsVar *childVar;
      childVar = jsvGetAddressOf(child);
      if (childVar->flags & JSV_GARBAGE_COLLECT)
        jsvGarbageCollectMarkUsed(childVar);
      child = jsvGetNextSibling(childVar);
    }
  }
}

/** Run a garbage collection sweep - return true if things have been freed */
bool jsvGarbageCollect() {
  JsVarRef i;
  // clear garbage collect flags
  for (i=1;i<=jsVarsSize;i++)  {
    JsVar *var = jsvGetAddressOf(i);
    if ((var->flags&JSV_VARTYPEMASK) != JSV_UNUSED) { // if it is not unused
      var->flags |= (JsVarFlags)JSV_GARBAGE_COLLECT;
      // if we have a flat string, skip that many blocks
      if (jsvIsFlatString(var))
        i = (JsVarRef)(i+jsvGetFlatStringBlocks(var));
    }
  }
  // recursively add 'native' vars
  for (i=1;i<=jsVarsSize;i++)  {
    JsVar *var = jsvGetAddressOf(i);
    if ((var->flags & JSV_GARBAGE_COLLECT) && // not already GC'd
        jsvGetLocks(var)>0) // or it is locked
      jsvGarbageCollectMarkUsed(var);
    // if we have a flat string, skip that many blocks
    if (jsvIsFlatString(var))
      i = (JsVarRef)(i+jsvGetFlatStringBlocks(var));
  }
  // now sweep for things that we can GC!
  bool freedSomething = false;
  for (i=1;i<=jsVarsSize;i++)  {
    JsVar *var = jsvGetAddressOf(i);
    if (var->flags & JSV_GARBAGE_COLLECT) {
      freedSomething = true;
      if (jsvIsFlatString(var)) {
        // if we're a flat string, there are more blocks to free
        // work backwards, so our free list is in the right order
        unsigned int count = 1 + (unsigned int)jsvGetFlatStringBlocks(var);
        while (count-- > 0) {
          var = jsvGetAddressOf((JsVarRef)(i+count));
          var->flags = JSV_UNUSED;
          // add this to our free list
          jsvSetNextSibling(var, jsVarFirstEmpty);
          jsVarFirstEmpty = jsvGetRef(var);
        }
      } else {
        // otherwise just free 1 block
        if (jsvHasSingleChild(var)) {
          /* If this had a child that wasn't listed for GC then we need to
           * unref it. Everything else is fine because it'll disappear anyway.
           * We don't have to check if we should free this other variable
           * here because we know the GC picked up it was referenced from
           * somewhere else. */
          JsVarRef ch = jsvGetFirstChild(var);
          if (ch) {
            JsVar *child = jsvGetAddressOf(ch); // not locked
            if (child->flags!=JSV_UNUSED && // not already GC'd!
                !(child->flags&JSV_GARBAGE_COLLECT)) // not marked for GC
              jsvUnRef(child);
          }
        }
        /* Sanity checks here. We're making sure that any variables that are
         * linked from this one have either already been garbage collected or
         * are marked for GC */
        assert(!jsvHasChildren(var) || !jsvGetFirstChild(var) ||
            jsvGetAddressOf(jsvGetFirstChild(var))->flags==JSV_UNUSED ||
            (jsvGetAddressOf(jsvGetFirstChild(var))->flags&JSV_GARBAGE_COLLECT));
        assert(!jsvHasChildren(var) || !jsvGetLastChild(var) ||
            jsvGetAddressOf(jsvGetLastChild(var))->flags==JSV_UNUSED ||
            (jsvGetAddressOf(jsvGetLastChild(var))->flags&JSV_GARBAGE_COLLECT));
        assert(!jsvIsName(var) || !jsvGetPrevSibling(var) ||
            jsvGetAddressOf(jsvGetPrevSibling(var))->flags==JSV_UNUSED ||
            (jsvGetAddressOf(jsvGetPrevSibling(var))->flags&JSV_GARBAGE_COLLECT));
        assert(!jsvIsName(var) || !jsvGetNextSibling(var) ||
            jsvGetAddressOf(jsvGetNextSibling(var))->flags==JSV_UNUSED ||
            (jsvGetAddressOf(jsvGetNextSibling(var))->flags&JSV_GARBAGE_COLLECT));
        // free!
        var->flags = JSV_UNUSED;
        // add this to our free list
        jsvSetNextSibling(var, jsVarFirstEmpty);
        jsVarFirstEmpty = jsvGetRef(var);
      }
    } else if (jsvIsFlatString(var)) {
      // if we have a flat string, skip that many blocks
      i = (JsVarRef)(i+jsvGetFlatStringBlocks(var));
    }
  }
  return freedSomething;
}
