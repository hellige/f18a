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

#ifndef opcodes_h
#define opcodes_h


#define FOR_EACH_OP(apply) \
  apply(OP_RET,   ";") \
  apply(OP_EXEC,  "ex") \
  apply(OP_JUMP,  "jump") \
  apply(OP_CALL,  "call") \
  apply(OP_UNXT,  "unext") \
  apply(OP_NEXT,  "next") \
  apply(OP_IF,    "if") \
  apply(OP_IFG,   "-if") \
  apply(OP_LVPI,  "@p") \
  apply(OP_LVAI,  "@+") \
  apply(OP_LVB,   "@b") \
  apply(OP_LVA,   "@") \
  apply(OP_SVPI,  "!p") \
  apply(OP_SVAI,  "!+") \
  apply(OP_SVB,   "!b") \
  apply(OP_SVA,   "!") \
  apply(OP_MULS,  "+*") \
  apply(OP_SHL,   "2*") \
  apply(OP_SHR,   "2/") \
  apply(OP_INV,   "-") \
  apply(OP_ADD,   "+") \
  apply(OP_AND,   "and") \
  apply(OP_OR,    "or") \
  apply(OP_DROP,  "drop") \
  apply(OP_DUP,   "dup") \
  apply(OP_POP,   "pop") \
  apply(OP_OVER,  "over") \
  apply(OP_A,     "a") \
  apply(OP_NOP,   ".") \
  apply(OP_PUSH,  "push") \
  apply(OP_SB,    "b!") \
  apply(OP_SA,    "a!")

#define ID(x, _) x,
enum opcode {
  FOR_EACH_OP(ID)
};
#undef ID

extern const char *opnames[];


#endif
