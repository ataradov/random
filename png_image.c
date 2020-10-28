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
#include "png_image.h"

/*- Definitions -------------------------------------------------------------*/
#define PNG_HEADER_1   0x474e5089
#define PNG_HEADER_2   0x0a1a0a0d

#define PNG_IHDR       0x52444849
#define PNG_IDAT       0x54414449
#define PNG_IEND       0x444e4549

#define PNG_TYPE_RGB   2
#define PNG_TYPE_RGBA  6

#define FIXED_HLIT     288
#define FIXED_HDIST    32
#define MAX_LENGTH     15

/*- Types -------------------------------------------------------------------*/
typedef struct
{
  uint8_t  *data;
  int      size;
  bool     error;
} ByteStream;

typedef struct
{
  uint8_t  *data;
  int      size;
  int      bits;
  uint32_t word;
  bool     error;
} BitStream;

typedef struct
{
  uint8_t  *data;
  int      size;
  int      ptr;
} OutputBuffer;

/*- Constants ---------------------------------------------------------------*/
static const int length_index_map[19] =
{
  16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

static const int length_base[] =
{
  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
  35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

static const int length_extra_bits[] =
{
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
  3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

static const int dist_base[] =
{
  1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
  193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
  6145, 8193, 12289, 16385, 24577, 0, 0
};

static const int dist_extra_bits[] =
{
  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7,
  8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 0, 0
};

static const int fixed_lengths[FIXED_HLIT + FIXED_HDIST] =
{
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  8, 8, 8, 8, // 144
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 112
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, // 24
  8, 8, 8, 8, 8, 8, 8, 8, // 8
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 // 32
};

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
static void bit_stream_init(BitStream *stream, uint8_t *data, int size)
{
  stream->data  = data;
  stream->size  = size;
  stream->bits  = 0;
  stream->word  = 0;
  stream->error = false;
}

//-----------------------------------------------------------------------------
static uint32_t bit_stream_peek(BitStream *stream, int bits)
{
  uint32_t res = 0;

  while (bits > stream->bits)
  {
    if (stream->size > 0)
    {
      uint32_t byte = stream->data[0];

      stream->data++;
      stream->size--;

      stream->word |= (byte << stream->bits);
      stream->bits += 8;
    }
    else
    {
      stream->error = true;
      return 0;
    }
  }

  res = stream->word & ((1 << bits) - 1);

  return res;
}

//-----------------------------------------------------------------------------
static uint32_t bit_stream_bits(BitStream *stream, int bits)
{
  uint32_t res = bit_stream_peek(stream, bits);

  stream->word >>= bits;
  stream->bits -= bits;

  return res;
}

//-----------------------------------------------------------------------------
static inline bool bit_stream_empty(BitStream *stream)
{
  return (0 == stream->size && 0 == stream->bits);
}

//-----------------------------------------------------------------------------
static void bit_stream_buf(BitStream *stream, uint8_t *buf, int size)
{
  // We expect the stream to be aligned on a byte boundary
  // and have no data in the bit buffer
  if (stream->bits > 0 || stream->size < size)
  {
    stream->error = true;
  }
  else
  {
    memcpy(buf, stream->data, size);
    stream->data += size;
    stream->size -= size;
  }
}

//-----------------------------------------------------------------------------
static void byte_stream_init(ByteStream *stream, uint8_t *data, int size)
{
  stream->data  = data;
  stream->size  = size;
  stream->error = false;
}

//-----------------------------------------------------------------------------
static void byte_stream_buf(ByteStream *stream, uint8_t *buf, int size)
{
  if (stream->size < size)
  {
    stream->error = true;
  }
  else
  {
    if (buf)
      memcpy(buf, stream->data, size);

    stream->data += size;
    stream->size -= size;
  }
}

//-----------------------------------------------------------------------------
static uint8_t byte_stream_byte(ByteStream *stream)
{
  uint8_t res = 0;

  if (stream->size < (int)sizeof(uint8_t))
  {
    stream->error = true;
  }
  else
  {
    res = stream->data[0];
    stream->data += sizeof(uint8_t);
    stream->size -= sizeof(uint8_t);
  }

  return res;
}

//-----------------------------------------------------------------------------
static uint32_t byte_stream_word(ByteStream *stream)
{
  uint32_t res = 0;

  if (stream->size < (int)sizeof(uint32_t))
  {
    stream->error = true;
  }
  else
  {
    res = ((uint32_t)stream->data[3] << 24) | ((uint32_t)stream->data[2] << 16) |
          ((uint32_t)stream->data[1] << 8) | stream->data[0];
    stream->data += sizeof(uint32_t);
    stream->size -= sizeof(uint32_t);
  }

  return res;
}

//-----------------------------------------------------------------------------
static uint32_t byte_stream_word_be(ByteStream *stream)
{
  uint32_t res = 0;

  if (stream->size < (int)sizeof(uint32_t))
  {
    stream->error = true;
  }
  else
  {
    res = ((uint32_t)stream->data[0] << 24) | ((uint32_t)stream->data[1] << 16) |
          ((uint32_t)stream->data[2] << 8) | stream->data[3];
    stream->data += sizeof(uint32_t);
    stream->size -= sizeof(uint32_t);
  }

  return res;
}

//-----------------------------------------------------------------------------
static void build_huffman_table(uint16_t *table, const int bit_length[], int size)
{
  int bl_count[MAX_LENGTH+1];
  int next_code[MAX_LENGTH+1];
  int code = 0;

  for (int i = 0; i < MAX_LENGTH; i++)
    bl_count[i] = next_code[i] = 0;

  for (int i = 0; i < size; i++)
    bl_count[bit_length[i]]++;

  for (int i = 0; i < MAX_LENGTH; i++)
  {
    code = (code + bl_count[i]) << 1;
    next_code[i+1] = code;
  }

  for (int i = 0; i < size; i++)
  {
    int len = bit_length[i];
    int spare = 1 << (MAX_LENGTH - len);
    int value = (len << 12) | i;
    int rev = 0;

    if (len == 0)
      continue;

    code = next_code[len];
    next_code[len]++;

    for (int j = 0; j < len; j++, code >>= 1)
      rev = (rev << 1) | (code & 1);

    for (int j = 0; j < spare; j++)
      table[(j << len) | rev] = value;
  }
}

//-----------------------------------------------------------------------------
static int get_symbol(BitStream *stream, uint16_t *table)
{
  int index = bit_stream_peek(stream, 15);
  int len = table[index] >> 12;

  bit_stream_bits(stream, len);

  return table[index] & 0xfff;
}

//-----------------------------------------------------------------------------
static bool prepare_dynamic_tables(BitStream *stream, uint16_t *lit_table, uint16_t *dist_table)
{
  int alphabet[FIXED_HLIT + FIXED_HDIST];
  int hlit, hdist, hclen;
  int length[19];
  int index = 0;

  hlit  = bit_stream_bits(stream, 5) + 257;
  hdist = bit_stream_bits(stream, 5) + 1;
  hclen = bit_stream_bits(stream, 4) + 4;

  for (int i = 0; i < 19; i++)
    length[i] = 0;

  for (int i = 0; i < hclen; i++)
    length[length_index_map[i]] = bit_stream_bits(stream, 3);

  build_huffman_table(lit_table, length, 19);

  while (index < (hlit + hdist))
  {
    int sym = get_symbol(stream, lit_table);

    if (sym < 16)
    {
      alphabet[index++] = sym;
    }
    else
    {
      int repeat_count;
      int repeat_sym = 0;

      if (sym == 16)
      {
        if (0 == index)
          return false;

        repeat_count = bit_stream_bits(stream, 2) + 3;
        repeat_sym = alphabet[index-1];
      }
      else if (sym == 17)
      {
        repeat_count = bit_stream_bits(stream, 3) + 3;
      }
      else if (sym == 18)
      {
        repeat_count = bit_stream_bits(stream, 7) + 11;
      }
      else
      {
        // Should never happen due to the way Huffman table is built
        assert(false);
      }

      if ((index + repeat_count) > (hlit + hdist))
        return false;

      for (int i = 0; i < repeat_count; i++)
        alphabet[index++] = repeat_sym;
    }
  }

  if (index != (hlit + hdist))
    return false;

  if (alphabet[256] == 0)
    return false; // Must be at least one End-Of-Block symbol

  build_huffman_table(lit_table, alphabet, hlit);
  build_huffman_table(dist_table, &alphabet[hlit], hdist);

  return true;
}

//-----------------------------------------------------------------------------
static bool handle_decompressed_block(BitStream *stream, OutputBuffer *buf)
{
  int len, nlen;

  bit_stream_bits(stream, stream->bits % 8);

  len  = bit_stream_bits(stream, 16);
  nlen = bit_stream_bits(stream, 16);

  if ((len ^ nlen) != 0xffff)
    return false;

  if (len > stream->size || (buf->ptr + len) > buf->size)
    return false;

  bit_stream_buf(stream, &buf->data[buf->ptr], len);

  buf->ptr += len;

  return true;
}

//-----------------------------------------------------------------------------
static bool handle_compressed_block(BitStream *stream, uint16_t *lit_table,
    uint16_t *dist_table, OutputBuffer *buf)
{
  while (true)
  {
    int sym = get_symbol(stream, lit_table);

    if (stream->error)
      return false;

    if (sym < 256)
    {
      if (buf->ptr == buf->size)
        return false;

      buf->data[buf->ptr++] = sym;
    }
    else if (sym == 256)
    {
      return true;
    }
    else
    {
      int length_index = sym - 257;
      int duplicate_length = length_base[length_index] + bit_stream_bits(stream, length_extra_bits[length_index]);
      int dist_index = get_symbol(stream, dist_table);
      int distance = dist_base[dist_index] + bit_stream_bits(stream, dist_extra_bits[dist_index]);
      int back_ptr = buf->ptr - distance;

      if (back_ptr < 0 || (buf->ptr + duplicate_length) > buf->size)
        return false;

      while (duplicate_length)
      {
        buf->data[buf->ptr] = buf->data[back_ptr];
        buf->ptr++;
        back_ptr++;
        duplicate_length--;
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
static bool deflate_decompress(uint8_t *data, int size, uint8_t *dec_data, int *dec_size)
{
  BitStream stream;
  OutputBuffer buf;
  uint16_t *lit_table, *dist_table;
  int final, type, cmf, flg, chk;
  bool res = false;

  buf.data = dec_data;
  buf.size = *dec_size;
  buf.ptr  = 0;

  bit_stream_init(&stream, data, size);

  cmf = bit_stream_bits(&stream, 8);
  flg = bit_stream_bits(&stream, 8);
  chk = (cmf << 8) | flg;

  if ((cmf & 0x0f) != 0x08 || flg & (1 << 5) || (chk % 31) != 0)
    return false;

  lit_table  = (uint16_t *)malloc((1 << MAX_LENGTH) * sizeof(uint16_t));
  dist_table = (uint16_t *)malloc((1 << MAX_LENGTH) * sizeof(uint16_t));

  if (!lit_table || !dist_table)
  {
    free(lit_table);
    free(dist_table);
    return false;
  }

  do
  {
    if (stream.error)
      break;

    final = bit_stream_bits(&stream, 1);
    type  = bit_stream_bits(&stream, 2);
    res   = false;

    if (type == 0)
    {
      if (!handle_decompressed_block(&stream, &buf))
        break;
    }
    else if (type == 1)
    {
      build_huffman_table(lit_table, fixed_lengths, FIXED_HLIT);
      build_huffman_table(dist_table, &fixed_lengths[FIXED_HLIT], FIXED_HDIST);

      if (!handle_compressed_block(&stream, lit_table, dist_table, &buf))
        break;
    }
    else if (type == 2)
    {
      if (!prepare_dynamic_tables(&stream, lit_table, dist_table))
        break;

      if (!handle_compressed_block(&stream, lit_table, dist_table, &buf))
        break;
    }
    else
    {
      break;
    }

    res = true;
  } while (!final);

  *dec_size = buf.ptr;

  free(lit_table);
  free(dist_table);

  return res;
}

//-----------------------------------------------------------------------------
static bool png_image_defilter(uint8_t *data, int width, int height, int bpp)
{
  int line_size = width * bpp + 1;

  for (int i = 0; i < height; i++)
  {
    int offset = line_size * i;
    int filter = data[offset];
    uint8_t *line = &data[offset + 1];

    if (0 == filter)
    {
      continue;
    }
    else if (1 == filter)
    {
      for (int x = 0; x < width * bpp; x++)
        line[x] += (uint8_t)((x - bpp) < 0) ? 0 : line[x-bpp];
    }
    else if (2 == filter)
    {
      uint8_t *prior = line - line_size;

      for (int x = 0; x < width * bpp; x++)
        line[x] += (uint8_t)(i == 0) ? 0 : prior[x];
    }
    else if (3 == filter)
    {
      uint8_t *prior = line - line_size;

      for (int x = 0; x < width * bpp; x++)
      {
        int p = (uint8_t)(i == 0) ? 0 : prior[x];
        int m = (uint8_t)((x - bpp) < 0) ? 0 : line[x-bpp];
        line[x] += (m + p) / 2;
      }
    }
    else if (4 == filter)
    {
      uint8_t *prior = line - line_size;
      uint8_t paeth = 0;

      for (int x = 0; x < width * bpp; x++)
      {
        int a = ((x - bpp) < 0) ? 0 : line[x-bpp];
        int b = (i == 0) ? 0 : prior[x];
        int c = ((i == 0) || ((x - bpp) < 0)) ? 0 : prior[x-bpp];
        int p = a + b - c;
        int pa = abs(p - a);
        int pb = abs(p - b);
        int pc = abs(p - c);

        if (pa <= pb && pa <= pc)
          paeth = a;
        else if (pb <= pc)
          paeth = b;
        else
          paeth = c;

        line[x] += paeth;
      }
    }
    else
    {
      return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
static void png_image_convert(uint8_t *dst, uint8_t *src, int width, int height, int bpp)
{
  int src_line_size = width * bpp + 1;
  int dst_line_size = width * sizeof(uint32_t);

  if (bpp == 4)
  {
    for (int i = 0; i < height; i++)
      memcpy(dst + dst_line_size * i, src + src_line_size * i + 1, src_line_size - 1);
  }
  else
  {
    for (int i = 0; i < height; i++)
    {
      src++;

      for (int j = 0; j < width; j++)
      {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0xff;
        dst += 4;
        src += 3;
      }
    }
  }
}

//-----------------------------------------------------------------------------
int png_image_read(PNGImage *image, uint8_t *data, int size)
{
  int width = 0, height = 0, depth, type, comp, filter, interlace, bpp = 0;
  uint8_t *compressed_data = (uint8_t *)malloc(size);
  int compressed_size = 0;
  bool first_chunk = true;
  bool have_header = false;
  ByteStream stream;

  memset(image, 0, sizeof(PNGImage));

  if (!compressed_data)
    return PNG_IMAGE_MALLOC_ERROR;

  byte_stream_init(&stream, data, size);

  if (PNG_HEADER_1 != byte_stream_word(&stream))
    return PNG_IMAGE_HEADER_1_ERROR;

  if (PNG_HEADER_2 != byte_stream_word(&stream))
    return PNG_IMAGE_HEADER_2_ERROR;

  while (1)
  {
    int ch_len  = byte_stream_word_be(&stream);
    int ch_type = byte_stream_word(&stream);
    int letter  = ch_type & 0xff;
    bool mandatory = ('A' <= letter && letter <= 'Z');

    if (stream.error)
      return PNG_IMAGE_STREAM_ERROR;

    if (PNG_IHDR == ch_type)
    {
      if (!first_chunk || 13 != ch_len)
        return PNG_IMAGE_IHDR_HEADER_ERROR;

      width  = byte_stream_word_be(&stream);
      height = byte_stream_word_be(&stream);
      depth  = byte_stream_byte(&stream);
      type   = byte_stream_byte(&stream);
      comp   = byte_stream_byte(&stream);
      filter = byte_stream_byte(&stream);
      interlace = byte_stream_byte(&stream);

      if (depth != 8 || filter != 0 || interlace != 0 || comp != 0)
        return PNG_IMAGE_IHDR_OPTION_ERROR;

      if (PNG_TYPE_RGB != type && PNG_TYPE_RGBA != type)
        return PNG_IMAGE_IHDR_TYPE_ERROR;

      bpp = (PNG_TYPE_RGB == type) ? 3 : 4;
      have_header = true;
    }
    else if (PNG_IDAT == ch_type)
    {
      if ((compressed_size + ch_len) > size)
        return PNG_IMAGE_IDAT_SIZE_ERROR;

      byte_stream_buf(&stream, &compressed_data[compressed_size], ch_len);

      compressed_size += ch_len;
    }
    else if (PNG_IEND == ch_type)
    {
      int expected_size = width * height * bpp + height;
      int decompressed_size = expected_size;
      uint8_t *decompressed_data;
      bool res;

      if (ch_len > 0)
        return PNG_IMAGE_SIZE_ERROR;

      byte_stream_word(&stream); // Skip CRC32

      if (stream.size || stream.error || !have_header)
        return PNG_IMAGE_STREAM_ERROR;

      decompressed_data = (uint8_t *)malloc(decompressed_size);

      if (!decompressed_data)
      {
        free(compressed_data);
        return PNG_IMAGE_MALLOC_ERROR;
      }

      res = deflate_decompress(compressed_data, compressed_size, decompressed_data, &decompressed_size);
      free(compressed_data);

      if (!res || expected_size != decompressed_size)
      {
        free(decompressed_data);
        return PNG_IMAGE_DECOMPRESS_ERROR;
      }

      if (!png_image_defilter(decompressed_data, width, height, bpp))
      {
        free(decompressed_data);
        return PNG_IMAGE_DEFILTER_ERROR;
      }

      image->width  = width;
      image->height = height;
      image->data = (uint8_t *)malloc(width * height * sizeof(uint32_t));

      if (!image->data)
      {
        free(decompressed_data);
        return PNG_IMAGE_MALLOC_ERROR;
      }

      png_image_convert(image->data, decompressed_data, width, height, bpp);

      free(decompressed_data);

      return PNG_IMAGE_SUCCESS;
    }
    else if (mandatory)
    {
      return PNG_IMAGE_UNKNOWN_CHUNK_ERROR;
    }
    else
    {
      byte_stream_buf(&stream, NULL, ch_len);
    }

    byte_stream_word(&stream); // Skip CRC32

    first_chunk = false;
  }

  return PNG_IMAGE_ERROR;
}

//-----------------------------------------------------------------------------
void png_image_free(PNGImage *image)
{
  assert(image);
  free(image->data);
}

