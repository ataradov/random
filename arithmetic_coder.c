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

/*- Includes ----------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdbool.h>
#include "arithmetic_coder.h"

/*- Definitions -------------------------------------------------------------*/
#define TOP_VALUE      0xffff
#define MAX_SCALE      0x3fff
#define FIRST_QTR      (TOP_VALUE / 4 + 1)
#define HALF	       (2 * FIRST_QTR)
#define THIRD_QTR      (3 * FIRST_QTR)

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void output_bit(acoder_t *coder, int value)
{
  coder->byte |= (value << coder->bit);

  if (8 == ++coder->bit)
  {
    coder->callback(coder->byte);
    coder->byte = 0;
    coder->bit = 0;
  }
}

//-----------------------------------------------------------------------------
static void output_flush(acoder_t *coder)
{
  coder->callback(coder->byte);
}

//-----------------------------------------------------------------------------
static int input_bit(acoder_t *coder)
{
  int res;

  if (8 == ++coder->bit)
  {
    coder->byte = coder->callback(0);
    coder->bit = 0;
  }

  res = coder->byte & 1;
  coder->byte >>= 1;

  return res;
}

//-----------------------------------------------------------------------------
static void model_init(acoder_t *coder)
{
  for (int i = 0; i < ACODER_N + 1; i++)
    coder->cdf[i] = i;
}

//-----------------------------------------------------------------------------
static void model_update(acoder_t *coder, int byte)
{
  int last = 0;
  int value;

  if (coder->cdf[ACODER_N] == MAX_SCALE)
  {
    for (int i = 0; i < ACODER_N; i++)
    {
      value = coder->cdf[i+1] - last;
      last = coder->cdf[i+1];
      coder->cdf[i+1] = coder->cdf[i] + (value + 1) / 2;
    }
  }

  for (int i = byte; i < ACODER_N; i++)
    coder->cdf[i+1]++;
}

//-----------------------------------------------------------------------------
static void output_bit_and_pending(acoder_t *coder, int bit)
{
  output_bit(coder, bit);

  for (; coder->pending; coder->pending--)
    output_bit(coder, !bit);
}

//-----------------------------------------------------------------------------
void acoder_init(acoder_t *coder, acoder_mode_t mode, int (*callback)(int))
{
  coder->mode = mode;
  coder->callback = callback;

  if (ACODER_ENCODE == coder->mode)
  {
    coder->low     = 0;
    coder->high    = TOP_VALUE;
    coder->pending = 0;
    coder->byte    = 0;
    coder->bit     = 0;
  }
  else
  {
    coder->low   = 0;
    coder->high  = TOP_VALUE;
    coder->value = 0;
    coder->bit   = 7;

    for (int i = 0; i < 16; i++)
      coder->value = (coder->value << 1) | input_bit(coder);
  }

  model_init(coder);
}

//-----------------------------------------------------------------------------
void acoder_encode(acoder_t *coder, int byte)
{
  uint32_t range = coder->high - coder->low + 1;

  coder->high = coder->low + (range * coder->cdf[byte+1]) / coder->cdf[ACODER_N] - 1;
  coder->low  = coder->low + (range * coder->cdf[byte]) / coder->cdf[ACODER_N];

  while (1)
  {
    if (coder->high < HALF)
    {
      output_bit_and_pending(coder, 0);
    } 
    else if (coder->low >= HALF)
    {
      output_bit_and_pending(coder, 1);
      coder->low  -= HALF;
      coder->high -= HALF;
    }
    else if (coder->low >= FIRST_QTR && coder->high < THIRD_QTR)
    {
      coder->pending++;
      coder->low  -= FIRST_QTR;
      coder->high -= FIRST_QTR;
    }
    else
      break;

    coder->low  = coder->low * 2;
    coder->high = coder->high * 2 + 1;
  }

  model_update(coder, byte);
}

//-----------------------------------------------------------------------------
int acoder_decode(acoder_t *coder)
{
  uint32_t range = coder->high - coder->low + 1;
  uint32_t val = ((coder->value - coder->low + 1) * coder->cdf[ACODER_N] - 1) / range;
  int byte;

  for (byte = ACODER_N; val < coder->cdf[byte]; byte--);

  coder->high = coder->low + (range * coder->cdf[byte+1]) / coder->cdf[ACODER_N] - 1;
  coder->low  = coder->low + (range * coder->cdf[byte]) / coder->cdf[ACODER_N];

  while (1)
  {
    if (coder->high < HALF)
    {
      // Do nothing
    } 
    else if (coder->low >= HALF)
    {
      coder->value -= HALF;
      coder->low   -= HALF;
      coder->high  -= HALF;
    }
    else if (coder->low >= FIRST_QTR && coder->high < THIRD_QTR)
    {
      coder->value -= FIRST_QTR;
      coder->low   -= FIRST_QTR;
      coder->high  -= FIRST_QTR;
    }
    else
      break;

    coder->low   = coder->low * 2;
    coder->high  = coder->high * 2 + 1;
    coder->value = (coder->value << 1) | input_bit(coder);
  }

  model_update(coder, byte);

  return byte;
}

//-----------------------------------------------------------------------------
void acoder_finish(acoder_t *coder)
{
  if (ACODER_ENCODE == coder->mode)
  {
    coder->pending++;

    if (coder->low < FIRST_QTR)
      output_bit_and_pending(coder, 0);
    else
      output_bit_and_pending(coder, 1);

    output_flush(coder);
  }
}


