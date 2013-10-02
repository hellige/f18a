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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "f18a.h"
#include "opcodes.h"

static bool prefix(char *pre, char *full) {
  return !strncasecmp(pre, full, strlen(pre));
}

static bool matches(char *tok, char *min, char *full) {
  return prefix(min, tok) && prefix(tok, full);
}

static void dumpram(f18a *f18a, u32 addr, int len) {
  while (len > 0 && addr <= ADDR_MASK) {
    u32 base = addr & ~7;
    f18a_msg("\n%02x:", base);
    int pad = addr % 8;
    f18a_msg("%*s", 5 * pad, "");
    do {
      if (f18a_present(addr))
        f18a_msg(" %05x", f18a_load(f18a, addr));
      else
        f18a_msg("      ");
    } while (--len && ++addr % 8);
  }
  f18a_msg("\n");
}

static void dumpheader(void) {
  f18a_msg(
      "p   r     t     s     a     b   io    i     @ opcode\n"
      "--- ----- ----- ----- ----- --- ----- ----- - --------\n");
}

static void dumpstate(f18a *f) {
  u8 op = f18a_decode_op(f);
  f18a_msg(
      "%03x %05x %05x %05x %05x %03x %05x %05x %d %03x %s\n",
      f->p, f->r, f->t, f->s, f->a, f->b, f->io, f->i, f->slot, op,
      opnames[op]);
  f18a_msg("   stack: [%d]", f->sp);
  for (int i = 0; i < STACK_WORDS; i++)
    f18a_msg(" %05x", f->stack[(f->sp + STACK_WORDS - i) % STACK_WORDS]);
  f18a_msg("\n");
  f18a_msg("  rstack: [%d]", f->rsp);
  for (int i = 0; i < RSTACK_WORDS; i++)
    f18a_msg(" %05x", f->rstack[(f->rsp + RSTACK_WORDS - i) % RSTACK_WORDS]);
  f18a_msg("\n");
}

bool f18a_debug(f18a *f18a) {
  static char buf[BUFSIZ];
  f18a_msg("entering emulator debugger: enter 'h' for help.\n");
  dumpheader();
  dumpstate(f18a);
  for (;;) {
    f18a_msg(" * ");
    if (!f18a_getstr(buf, BUFSIZ)) return false;

    char *delim = " \t\n";
    char *tok = strtok(buf, delim);
    if (!tok) continue;
    if (matches(tok, "h", "help")
        || matches(tok, "?", "?")) {
      f18a_msg(
          "  help, ?: show this message\n"
          "  continue: resume running\n"
          "  step [n]: execute a single instruction (or n instructions)\n"
          "  dump: display the state of the cpu\n"
          "  print addr [len]: display memory contents in hex\n"
          "      (addr is hex, len decimal)\n"
          "  exit, quit: exit emulator\n"
          "unambiguous abbreviations are recognized "
            "(e.g., s for step or con for continue).\n"
          );
    } else if (matches(tok, "con", "continue")) {
      return true;
    } else if (matches(tok, "s", "step")) {
      uint32_t steps = 1;
      tok = strtok(NULL, delim);
      if (tok) {
        char *endptr;
        steps = strtoul(tok, &endptr, 10);
        if (*endptr) {
          f18a_msg("argument to 'step' must be a decimal number\n");
          continue;
        }
      }
      for (uint32_t i = 0; i < steps; i++) {
        f18a_runterm();
        f18a_step(f18a);
        f18a_dbgterm();
        dumpstate(f18a);
      }
    } else if (matches(tok, "d", "dump")) {
      dumpheader();
      dumpstate(f18a);
    } else if (matches(tok, "p", "print")) {
      tok = strtok(NULL, delim);
      if (!tok) {
        f18a_msg("print requires an argument\n");
        continue;
      }
      char *endptr;
      u32 addr = strtoul(tok, &endptr, 16);
      if (*endptr) {
        f18a_msg("addr argument to 'print' must be a hex number: %s\n", endptr);
        continue;
      }
      u16 length = 1;
      tok = strtok(NULL, delim);
      if (tok) {
        length = strtoul(tok, &endptr, 10);
        if (*endptr) {
          f18a_msg("len argument to 'print' must be a decimal number\n");
          continue;
        }
      }
      dumpram(f18a, addr, length);
    } else if (matches(tok, "e", "exit")
        || matches(tok, "q", "quit")) {
      return false;
    } else {
      f18a_msg("unrecognized or ambiguous command: %s\n", tok);
    }
  }
}

