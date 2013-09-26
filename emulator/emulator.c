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

#include <arpa/inet.h>
#include <assert.h>
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

  int img_size = fread(f18a->ram, 4, RAM_WORDS, img);
  if (ferror(img)) {
    f18a_exitmsg("error reading image '%s': %s\n", image, strerror(errno));
    return false;
  }
  img_size += fread(f18a->rom, 4, ROM_WORDS, img);
  if (ferror(img)) {
    f18a_exitmsg("error reading image '%s': %s\n", image, strerror(errno));
    return false;
  }

  for (int i = 0; i < RAM_WORDS; i++) {
    f18a->ram[i] = ntohl(f18a->ram[i]);
    if (f18a->ram[i] & ~MAX_VAL) {
      f18a_msg(
          "ram word at 0x%02x (0x%08x) has high bits set! clipping to range!\n",
          i, f18a->ram[i]);
      f18a->ram[i] &= MAX_VAL;
    }
  }
  for (int i = 0; i < ROM_WORDS; i++) {
    f18a->rom[i] = ntohl(f18a->rom[i]);
    if (f18a->rom[i] & ~MAX_VAL) {
      f18a_msg(
          "rom word at 0x%02x (0x%08x) has high bits set! clipping to range!\n",
          i, f18a->rom[i]);
      f18a->rom[i] &= MAX_VAL;
    }
  }

  f18a_msg("loaded image from %s: 0x%05x words\n", image, img_size);
  fclose(img);
  return true;
}


bool f18a_present(u32 addr) {
  addr &= ADDR_MASK;
  if (addr < 0x100) return true;

  if (addr == IO_ADDR) return true;

  // TODO other io addresses...
  return false;
}


u32 f18a_load(f18a *f18a, u32 addr) {
  addr &= ADDR_MASK;
  if (addr < 0x080)
    return f18a->ram[addr & 0x3f];
  if (addr < 0x100)
    return f18a->rom[addr & 0x3f];

  if (addr == IO_ADDR) return f18a->io;

  // TODO other io addresses...
  return 0;
}


static void store(f18a *f18a, u32 addr, u32 val) {
  addr &= ADDR_MASK;
  if (addr < 0x080)
    f18a->ram[addr & 0x3f] = val;
  if (addr < 0x100) {
    f18a_msg("attempt to write 0x%05x to rom address 0x%02x!\n", val, addr);
    return;
  }

  // TODO is this right?
  if (addr == IO_ADDR) f18a->io = val;

  // TODO other io addresses...
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
  u32 result = f18a_load(f18a, *addr);
  inc(addr);
  return result;
}


static void skip(f18a *f) {
  f->slot = 4;
}


static const u8 rshifts[] = {13, 8, 3, 0};
static const u8 masks[] = {31, 31, 31, 7};
static const u8 lshifts[] = {0, 0, 0, 2};

u8 f18a_decode_op(f18a *f18a) {
  u32 word = f18a->i ^ OP_XOR_MASK;
  return ((word >> rshifts[f18a->slot]) & masks[f18a->slot])
    << lshifts[f18a->slot];
}


static const u32 dmasks[] = {0x3ff, 0xff, 0x7};

static void jump(f18a *f) {
  // slot has already been incremented... correct it.
  u8 slot = f->slot - 1;

  // jumps aren't even decodable from slot 3...
  assert(slot < 3);

  // we can always safely force p8 to 0. either it's a slot 1/2 jump,
  // in which case it should be forced, or it's a slot 0 jump, in which
  // case it'll be overwritten anyway.
  f->p &= ~0x100;
  u32 dest = f->i & dmasks[slot];
  f->p = (f->p | ~dmasks[slot]) & dest;

  // and we're done with this instruction word...
  skip(f);
}


static void next(f18a *f18a) {
  if (f18a->slot > 3) {
    // fetch next instruction word
    f18a->i = loadinc(f18a, &f18a->p);
    f18a->slot = 0;
  }
}


static void push(f18a *f, u32 val) {
  f->sp = (f->sp + 1) % STACK_WORDS;
  f->stack[f->sp] = f->s;
  f->s = f->t;
  f->t = val;
}


static u32 pop(f18a *f) {
  u32 t = f->t;
  f->t = f->s;
  f->s = f->stack[f->sp];
  f->sp = (f->sp + STACK_WORDS - 1) % STACK_WORDS;
  return t;
}


static u32 pops(f18a *f) {
  u32 s = f->s;
  f->s = f->stack[f->sp];
  f->sp = (f->sp + STACK_WORDS - 1) % STACK_WORDS;
  return s;
}


static void pushr(f18a *f, u32 val) {
  f->rsp = (f->rsp + 1) % RSTACK_WORDS;
  f->rstack[f->rsp] = f->r;
  f->r = val;
}


static u32 popr(f18a *f) {
  u32 r = f->r;
  f->r = f->rstack[f->rsp];
  f->rsp = (f->rsp + RSTACK_WORDS - 1) % RSTACK_WORDS;
  return r;
}


static action_t execute(f18a *f, u8 op) {
  switch (op) {
    case OP_RET: f->p = f->r & MAX_P; popr(f); skip(f); break;
    case OP_EXEC: { u32 tmp = f->r; f->r = f->p; f->p = tmp & MAX_P; }
                  skip(f); break;
    case OP_JUMP: jump(f); break;
    case OP_CALL: pushr(f, f->p); jump(f); break;
    case OP_UNXT: if (f->r) { f->r--; f->slot = 0; } else popr(f); break;
    case OP_NEXT: if (f->r) { f->r--; jump(f); }
                    else { popr(f); skip(f); } break;
    case OP_IF: if (f->t) skip(f); else jump(f); break;
    case OP_IFG: if (f->t & 0x20000) skip(f); else jump(f); break;
    case OP_LVPI: push(f, loadinc(f, &f->p)); break;
    case OP_LVAI: push(f, loadinc(f, &f->a)); break;
    case OP_LVB: push(f, f18a_load(f, f->b)); break;
    case OP_LVA: push(f, f18a_load(f, f->a)); break;
    case OP_SVPI: store(f, f->p, pop(f)); inc(&f->p); break;
    case OP_SVAI: store(f, f->a, pop(f)); inc(&f->a); break;
    case OP_SVB: store(f, f->b, pop(f)); break;
    case OP_SVA: store(f, f->a, pop(f)); break;
    case OP_MULS: /* TODO */ break;
    case OP_SHL: f->t <<= 1; break;
    // implementation-defined, correct on gcc/x86
    case OP_SHR: f->t = ((int32_t)f->t) >> 1; break;
    case OP_INV: f->t = ~f->t; break;
    // TODO add with carry in case of p9
    case OP_ADD: f->t += pops(f); break;
    // spec says "boolean" but surely means "bitwise"
    case OP_AND: f->t = f->t & pops(f); break;
    case OP_OR: f->t = f->t ^ pops(f); break;
    case OP_DROP: pop(f); break;
    case OP_DUP: push(f, f->t); break;
    case OP_POP: push(f, popr(f)); break;
    case OP_OVER: push(f, f->s); break;
    case OP_A: push(f, f->a); break;
    case OP_NOP: break;
    case OP_PUSH: pushr(f, pop(f)); break;
    case OP_SB: f->b = pop(f) & MAX_B; break;
    case OP_SA: f->a = pop(f); break;
  }

  return A_CONTINUE; // TODO make some use of this or refactor it all away...
}


action_t f18a_step(f18a *f18a) {
  u8 op = f18a_decode_op(f18a);
  // increment must occur prior to execute, so ops can reset slot as needed
  f18a->slot++;
  action_t result = execute(f18a, op);
  next(f18a);
  return result;
}


void f18a_run(f18a *f18a, bool debugboot) {
  bool running = true;
  next(f18a);
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
