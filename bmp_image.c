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

/*- Includes ----------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include "file_io.h"
#include "bmp_image.h"

/*- Definitions -------------------------------------------------------------*/

/*- Types -------------------------------------------------------------------*/
typedef struct __attribute__((packed))
{
  uint16_t     bfType;
  uint32_t     bfSize;
  uint16_t     bfReserved1;
  uint16_t     bfReserved2;
  uint32_t     bfOffBits;
} BMPFileHeader;

typedef struct __attribute__((packed))
{
  uint32_t     biSize;
  uint32_t     biWidth;
  uint32_t     biHeight;
  uint16_t     biPlanes;
  uint16_t     biBitCount;
  uint32_t     biCompression;
  uint32_t     biSizeImage;
  uint32_t     biXPelsPerMeter;
  uint32_t     biYPelsPerMeter;
  uint32_t     biClrUsed;
  uint32_t     biClrImportant;
} BMPRGBImageHeader;

typedef struct __attribute__((packed))
{
  uint32_t     x;
  uint32_t     y;
  uint32_t     z;
} BMPCIEXYZ;

typedef struct __attribute__((packed))
{
  uint32_t     biSize;
  uint32_t     biWidth;
  uint32_t     biHeight;
  uint16_t     biPlanes;
  uint16_t     biBitCount;
  uint32_t     biCompression;
  uint32_t     biSizeImage;
  uint32_t     biXPelsPerMeter;
  uint32_t     biYPelsPerMeter;
  uint32_t     biClrUsed;
  uint32_t     biClrImportant;
  uint32_t     biRedMask;
  uint32_t     biGreenMask;
  uint32_t     biBlueMask;
  uint32_t     biAlphaMask;
  uint32_t     biCSType;
  BMPCIEXYZ    ciexyzRed;
  BMPCIEXYZ    ciexyzGreen;
  BMPCIEXYZ    ciexyzBlue;
  uint32_t     biGammaRed;
  uint32_t     biGammaGreen;
  uint32_t     biGammaBlue;
  uint32_t     bi5Intent;
  uint32_t     bi5ProfileData;
  uint32_t     bi5ProfileSize;
  uint32_t     bi5Reserved;
} BMPRGBAImageHeader;

typedef struct __attribute__((packed))
{
  BMPFileHeader     file;
  BMPRGBImageHeader image;
} BMPRGBFileHeader;

typedef struct __attribute__((packed))
{
  BMPFileHeader      file;
  BMPRGBAImageHeader image;
} BMPRGBAFileHeader;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
void bmp_image_write_rgb(const char *name, int width, int height, uint8_t *data)
{
  BMPRGBFileHeader header;
  int size = height * width;
  uint8_t *abgr;
  int fd;

  memset(&header, 0, sizeof(header));

  header.file.bfType           = 0x4d42; // BM
  header.file.bfSize           = sizeof(BMPRGBFileHeader) + size * 3;
  header.file.bfOffBits        = sizeof(BMPRGBFileHeader);
  header.image.biSize          = sizeof(BMPRGBImageHeader);
  header.image.biWidth         = width;
  header.image.biHeight        = -height;
  header.image.biPlanes        = 1;
  header.image.biBitCount      = 24;
  header.image.biCompression   = 0;
  header.image.biSizeImage     = 0;
  header.image.biXPelsPerMeter = 2835; // 72 DPI
  header.image.biYPelsPerMeter = 2835; // 72 DPI
  header.image.biClrUsed       = 0;
  header.image.biClrImportant  = 0;

  abgr = malloc(size * sizeof(uint32_t));

  for (int i = 0; i < size; i++)
  {
    int dst_offset = i * 3;
    int src_offset = i * sizeof(uint32_t);

    abgr[dst_offset + 0] = data[src_offset + 3];
    abgr[dst_offset + 1] = data[src_offset + 2];
    abgr[dst_offset + 2] = data[src_offset + 1];
  }

  fd = file_io_open_for_write(name);
  file_io_write(fd, (uint8_t *)&header, sizeof(header));
  file_io_write(fd, abgr, width * height * 3);
  file_io_close(fd);

  free(abgr);
}

//-----------------------------------------------------------------------------
void bmp_image_write_rgba(const char *name, int width, int height, uint8_t *data)
{
  BMPRGBAFileHeader header;
  int size = height * width;
  int fd;

  memset(&header, 0, sizeof(header));

  header.file.bfType           = 0x4d42; // BM
  header.file.bfSize           = sizeof(BMPRGBAFileHeader) + size * sizeof(uint32_t);
  header.file.bfOffBits        = sizeof(BMPRGBAFileHeader);
  header.image.biSize          = sizeof(BMPRGBAImageHeader);
  header.image.biWidth         = width;
  header.image.biHeight        = -height;
  header.image.biPlanes        = 1;
  header.image.biBitCount      = 32;
  header.image.biCompression   = 3; // BI_BITFIELDS
  header.image.biSizeImage     = 0;
  header.image.biXPelsPerMeter = 2835; // 72 DPI
  header.image.biYPelsPerMeter = 2835; // 72 DPI
  header.image.biClrUsed       = 0;
  header.image.biClrImportant  = 0;
  header.image.biRedMask       = 0x000000ff;
  header.image.biGreenMask     = 0x0000ff00;
  header.image.biBlueMask      = 0x00ff0000;
  header.image.biAlphaMask     = 0xff000000;
  header.image.biCSType        = 0x73524742; // sRGB

  fd = file_io_open_for_write(name);
  file_io_write(fd, (uint8_t *)&header, sizeof(header));
  file_io_write(fd, data, size * sizeof(uint32_t));
  file_io_close(fd);
}

