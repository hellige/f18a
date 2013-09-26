/*
 * Copyright (c) 2013, Matt Hellige
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "f18a.h"
#include "opcodes.h"


void f18a_init(f18a *f18a) {
  f18a->p = BOOT_ADDR; // or multiport execute, depending on node config
  f18a->slot = 4; // force instruction fetch on boot
  f18a->io = 0x15555;
  f18a->b = IO_ADDR;
  f18a->sp = f18a->rsp = 0;

  // everything else is "not directly affected by reset," but we might as well
  // initialize it to something sensible.
  f18a->r = f18a->t = f18a->s = f18a->i = f18a->a = 0;
  for (int i = 0; i < STACK_WORDS; i++) f18a->stack[i] = 0;
  for (int i = 0; i < RSTACK_WORDS; i++) f18a->rstack[i] = 0;
  for (int i = 0; i < RAM_WORDS; i++) f18a->ram[i] = 0;
  for (int i = 0; i < ROM_WORDS; i++) f18a->rom[i] = 0;
}


bool f18a_loadcore(f18a *f18a, const char *image) {
  FILE *img = fopen(image, "r");
  if (!img) {
    f18a_exitmsg("error reading image '%s': %s\n", image, strerror(errno));
    return false;
  }

  // TODO load/verify rom...
  int img_size = fread(f18a->ram, 4, RAM_WORDS, img);
  if (ferror(img)) {
    f18a_exitmsg("error reading image '%s': %s\n", image, strerror(errno));
    return false;
  }

  for (int i = 0; i < RAM_WORDS; i++) {
    if (f18a->ram[i] & ~MAX_VAL) {
      f18a_msg(
          "word at 0x%x (0x%x) has high bits set! clipping to range!\n",
          i, f18a->ram[i]);
      f18a->ram[i] &= MAX_VAL;
    }
  }

  f18a_msg("loaded image from %s: 0x%05x words\n", image, img_size);
  fclose(img);
  return true;
}


static u32 load(f18a *f18a, u32 addr) {
  addr &= ADDR_MASK;
  if (addr < 0x080)
    return f18a->ram[addr & 0x3f];
  if (addr < 0x100)
    return f18a->rom[addr & 0x3f];

  f18a_msg("io addr access from %x! returning 0...\n", addr);
  return 0;
}


static void inc(u32 *addr) {
  // in the case of p, wrapping behavior is well specified. in the case of a,
  // however, it's not clear what we should do when bits higher than 10 are
  // set. so what i do is rather arbitrary, but i think it's quite plausible.
  
  u32 a = *addr;

  // do nothing at all for io range...
  if (a & 0x100) return;

  // else increment bottom 7 bits without carry...
  u8 l7 = (a+1) & 0x7f;
  *addr = (a & ~0x7f) | l7;
}


static u32 loadinc(f18a *f18a, u32 *addr) {
  u32 result = load(f18a, *addr);
  inc(addr);
  return result;
}


static const u8 rshifts[] = {13, 8, 3, 0};
static const u8 masks[] = {31, 31, 31, 7};
static const u8 lshifts[] = {0, 0, 0, 2};

u8 f18a_decode_op(f18a *f18a) {
  u32 word = f18a->i ^ OP_XOR_MASK;
  return ((word >> rshifts[f18a->slot]) & masks[f18a->slot])
    << lshifts[f18a->slot];
}


static u8 next(f18a *f18a) {
  if (f18a->slot > 3) {
    // fetch next instruction word
    f18a->i = loadinc(f18a, &f18a->p);
    f18a->slot = 0;
  }

  u8 op = f18a_decode_op(f18a);
  f18a->slot++;
  return op;
}


static void popr(f18a *f) {
  f->r = f->rstack[f->rsp];
  f->rsp = (f->rsp + RSTACK_WORDS - 1) % RSTACK_WORDS;
}


static void skip(f18a *f) {
  f->slot = 4;
}


static action_t execute(f18a *f, u8 op) {
  switch (op) {
    case OP_RET:
      f->p = f->r & MAX_P;
      popr(f);
      skip(f);
      break;

    case OP_EXEC:
      {
        u32 tmp = f->r;
        f->r = f->p;
        f->p = tmp & MAX_P;
      }
      skip(f);
      break;

    // TODO...
  }
  return A_CONTINUE;
}


action_t f18a_step(f18a *f18a) {
  return execute(f18a, next(f18a));
}


void f18a_run(f18a *f18a, bool debugboot) {
  bool running = true;
  if (debugboot) running = f18a_debug(f18a);
  f18a_msg("running...\n");
  f18a_runterm();
  while (running && !f18a_die) {
    action_t action = f18a_step(f18a);
    if (action == A_EXIT) running = false;
    if (action == A_BREAK || f18a_break) {
      f18a_break = false;
      f18a_dbgterm();
      running = f18a_debug(f18a);
      if (running) f18a_msg("running...\n");
      f18a_runterm();
    }
  }
  f18a_dbgterm();
}
