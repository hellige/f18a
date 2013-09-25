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
#include <ncurses.h>
#include <stdarg.h>
#include <string.h>

#include "f18a.h"

struct term_t {
  WINDOW *border;
  WINDOW *vidwin;
  WINDOW *dbgwin;
};

static struct term_t term;

static inline u16 color(int fg, int bg) {
  return COLORS > 8
    ? fg * 16 + bg + 1
    : (fg % 8) * 8 + (bg % 8) + 1;
}

void f18a_exitmsg(char *fmt, ...) {
  f18a_killterm();
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

int f18a_getstr(char *buf, int n) {
  return wgetnstr(term.dbgwin, buf, n) == OK;
}

void f18a_msg(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vwprintw(term.dbgwin, fmt, args);
  wrefresh(term.dbgwin);
  va_end(args);
}

void f18a_runterm(void) {
  curs_set(0);
  timeout(0);
  noecho();
}

void f18a_dbgterm(void) {
  curs_set(1);
  timeout(-1);
  echo();
}

void f18a_initterm(void) {
  // set up curses...
  initscr();
  start_color();
  cbreak();
  keypad(stdscr, true);
  term.border = subwin(stdscr, 14, 36, 0, 0);
  term.vidwin = subwin(stdscr, 12, 32, 1, 2);
  term.dbgwin = subwin(stdscr, LINES - (SCR_HEIGHT+3), COLS, SCR_HEIGHT+2, 0);
  keypad(term.vidwin, true);
  keypad(term.border, true);
  scrollok(term.dbgwin, true);
  keypad(term.dbgwin, true);

  // set up colors...
  if (COLORS > 8) {
    // nice terminals...
    int colors[] = {0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15};
    for (int i = 0; i < 16; i++)
      for (int j = 0; j < 16; j++)
        init_pair(color(i, j), colors[i], colors[j]);
  } else {
    // crappy terminals at least get something...
    int colors[] = {0, 4, 2, 6, 1, 5, 3, 7};
    for (int i = 0; i < 8; i++)
      for (int j = 0; j < 8; j++)
        init_pair(color(i, j), colors[i], colors[j]);
  }
  f18a_msg("terminal colors: %d, pairs %d, %s change colors: \n", COLORS,
      COLOR_PAIRS, can_change_color() ? "*can*" : "*cannot*");
}

void f18a_killterm(void) {
  endwin();
}
