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

#ifndef f18a_h
#define f18a_h

#include <stdint.h>
#include <stdbool.h>


typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t tstamp_t;

#define F18A_VERSION  "1.0-mh"

#define RAM_WORDS 64
#define ROM_WORDS 64
#define STACK_WORDS 8
#define RSTACK_WORDS 8
#define IO_ADDR 0x15d
#define BOOT_ADDR 0x0aa
#define OP_XOR_MASK 0x15555
#define ADDR_MASK 0x1ff
#define MAX_VAL 0x3ffff
#define MAX_P 0x3ff
#define MAX_B 0x1ff

#define SCR_HEIGHT 1

typedef struct f18a_t {
  u32 p; // 10 bits
  u32 io;
  u32 r;
  u32 t;
  u32 s;
  u32 i;
  u32 a;
  u32 b; // 9 bits
  u8 sp;
  u8 rsp;
  u8 slot;
  u32 stack[STACK_WORDS];
  u32 rstack[RSTACK_WORDS];
  u32 ram[RAM_WORDS];
  u32 rom[ROM_WORDS];
} f18a;

typedef enum {
  A_CONTINUE,
  A_BREAK,
  A_EXIT
} action_t;


// disassembler.c
extern u16 *f18a_disassemble(u16 *pc, char *out);

// emulator.c
extern void f18a_init(f18a *f18a);
extern bool f18a_loadcore(f18a *f18a, const char *image);
extern bool f18a_present(u32 addr);
extern u32 f18a_load(f18a *f18a, u32 addr);
extern u8 f18a_decode_op(f18a *f18a);
extern void f18a_run(f18a *f18a, bool debugboot);
extern action_t f18a_step(f18a *f18a);

// debugger.c
extern bool f18a_debug(f18a *f18a);

// terminal.c
extern void f18a_initterm(void);
extern void f18a_msg(char *fmt, ...)
  __attribute__ ((format (printf, 1, 2)));
extern int f18a_getch(void);
extern int f18a_getstr(char *buf, int n);
extern void f18a_runterm(void);
extern void f18a_dbgterm(void);
extern void f18a_killterm(void);
extern void f18a_exitmsg(char *fmt, ...);
extern volatile bool f18a_break;
extern volatile bool f18a_die;


#endif
