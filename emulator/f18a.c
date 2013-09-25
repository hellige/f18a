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

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

/* SDL on Mac OS wants to replace main, and must use funky macros
   to do it. we don't need any SDL stuff here, but we do need to include
   SDL.h prior to the definition of main(). ugly. */
#ifdef USE_SDL
#include <SDL.h>
#endif

#include "f18a.h"
#include "opcodes.h"

volatile bool f18a_break = false;
volatile bool f18a_die = false;
static struct termios old_termios;

static void usage(char **argv) {
  fprintf(stderr, "usage: %s [options] <image>\n", argv[0]);
  fprintf(stderr, "   -h, --help           display this message\n");
  fprintf(stderr, "   -v, --version        display the version and exit\n");
  fprintf(stderr, "   -g, --graphics       enable graphical display window\n");
  fprintf(stderr, "   -d, --debug-boot     enter debugger on boot\n");
} 

static void int_handler(int signum) {
  (void)signum;
  f18a_break = true;
}

static void quit_handler(int signum) {
  (void)signum;
  f18a_die = true;
}

static void block_signals() {
  struct sigaction sa;
  sa.sa_handler = int_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL)) {
    fprintf(stderr, "error setting signal handler: %s\n", strerror(errno));
    fprintf(stderr, "continuing without signal support...");
  }

  sa.sa_handler = quit_handler;
  if (sigaction(SIGQUIT, &sa, NULL)) {
    fprintf(stderr, "error setting signal handler: %s\n", strerror(errno));
    fprintf(stderr, "continuing without signal support...");
  }

  struct termios new_termios;
  tcgetattr(0, &old_termios);
  new_termios = old_termios;
  new_termios.c_cc[VQUIT] = 0x04; // ctrl-d
  tcsetattr(0, TCSANOW, &new_termios);
}

int main(int argc, char **argv) {
  bool debug = false;
  f18a f18a;

  for (;;) {
    int c;

    static struct option long_options[] = {
      {"help", 0, 0, 'h'},
      {"version", 0, 0, 'v'},
      {"graphics", 0, 0, 'g'},
      {"debug-boot", 0, 0, 'd'},
      {0, 0, 0, 0},
    };

    c = getopt_long(argc, argv, "hvgd", long_options, NULL);

    if (c == -1) break;

    switch (c) {
      case 'h':
        usage(argv);
        return 0;
      case 'v':
        puts("f18a" F18A_VERSION);
        return 0;
      case 'g':
#ifdef USE_SDL
        // TODO graphics = true;
        break;
#else
        fprintf(stderr, "graphics not supported in this build!\n");
        fprintf(stderr, "  (perhaps try installing SDL and rebuilding?)\n");
        return 1;
#endif
      case 'd':
        debug = true;
        break;
      default:
        usage(argv);
        return 1;
    }
  }

  if (argc - optind != 1) {
    usage(argv);
    return 1;
  }
  
  const char *image = argv[optind];

  // init term first so that image load status is visible...
  block_signals();
  f18a_init(&f18a);
  f18a_initterm();
  // TODO if (graphics) dcpu_initlem(&dcpu);
  if (!f18a_loadcore(&f18a, image)) {
    tcsetattr(0, TCSANOW, &old_termios);
    return -1;
  }

  f18a_msg("welcome to f18a, version " F18A_VERSION "\n");
  f18a_msg("press ctrl-c or send SIGINT for debugger, ctrl-d to exit.\n");
  f18a_run(&f18a, debug);

  f18a_killterm();
  // TODO if (graphics) vram = f18a_killlem();
  puts(" * f18a halted.");

  tcsetattr(0, TCSANOW, &old_termios);
  return 0;
}
