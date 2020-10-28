/*
 * Copyright (c) 2019, Alex Taradov <alex@taradov.com>
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

#ifndef _PNG_IMAGE_H_
#define _PNG_IMAGE_H_

/*- Definitions -------------------------------------------------------------*/
enum
{
  PNG_IMAGE_SUCCESS             = 0,
  PNG_IMAGE_ERROR               = -1,
  PNG_IMAGE_MALLOC_ERROR        = -2,
  PNG_IMAGE_HEADER_1_ERROR      = -3,
  PNG_IMAGE_HEADER_2_ERROR      = -4,
  PNG_IMAGE_STREAM_ERROR        = -5,
  PNG_IMAGE_IHDR_HEADER_ERROR   = -6,
  PNG_IMAGE_IHDR_OPTION_ERROR   = -7,
  PNG_IMAGE_IHDR_TYPE_ERROR     = -8,
  PNG_IMAGE_IDAT_SIZE_ERROR     = -9,
  PNG_IMAGE_SIZE_ERROR          = -10,
  PNG_IMAGE_DECOMPRESS_ERROR    = -11,
  PNG_IMAGE_DEFILTER_ERROR      = -12,
  PNG_IMAGE_UNKNOWN_CHUNK_ERROR = -13,
};

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  int      width;
  int      height;
  uint8_t  *data;
} PNGImage;

/*- Prototypes --------------------------------------------------------------*/
int png_image_read(PNGImage *image, uint8_t *data, int size);
void png_image_free(PNGImage *image);

#endif // _PNG_IMAGE_H_

