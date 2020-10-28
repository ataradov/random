/*
 * Copyright (c) 2018, Alex Taradov <alex@taradov.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARITHMETIC_CODER_H_
#define _ARITHMETIC_CODER_H_

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>

/*- Definitions -------------------------------------------------------------*/
#define ACODER_N 257 // 256 + 1 for End-Of-Stream marker

/*- Types -------------------------------------------------------------------*/
typedef enum
{
  ACODER_ENCODE,
  ACODER_DECODE,
} acoder_mode_t;

typedef struct
{
  acoder_mode_t mode;
  uint32_t      value;
  uint32_t      low;
  uint32_t      high;
  int           pending;
  int           bit;
  int           byte;
  uint16_t      cdf[ACODER_N + 1];
  int           (*callback)(int);
} acoder_t;

/*- Prototypes --------------------------------------------------------------*/
void acoder_init(acoder_t *coder, acoder_mode_t mode, int (*callback)(int));
void acoder_encode(acoder_t *coder, int byte);
int acoder_decode(acoder_t *coder);
void acoder_finish(acoder_t *coder);

#endif // _ARITHMETIC_CODER_H_

