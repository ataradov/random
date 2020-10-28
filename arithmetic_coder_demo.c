#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "arithmetic_coder.h"

/*- Definitions -------------------------------------------------------------*/
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*- Variables ---------------------------------------------------------------*/
static uint8_t encoded_data[8*1024*1024];
static int encoded_size = 0;

static uint8_t decoded_data[8*1024*1024];
static int decoded_size = 0;
static int decoded_ptr = 0;

/*- Implementations ---------------------------------------------------------*/

//-----------------------------------------------------------------------------
bool load_file(char *name, uint8_t **data, int *size)
{
  struct stat stat;
  int fd, rsize;

  fd = open(name, O_RDONLY | O_BINARY);

  if (fd < 0)
    return false;

  fstat(fd, &stat);

  *data = malloc(stat.st_size);
  *size = stat.st_size;

  if (NULL == *data)
    return false;

  rsize = read(fd, *data, *size);

  close(fd);

  return (rsize == *size);
}

//-----------------------------------------------------------------------------
static int encoder_callback(int value)
{
  encoded_data[encoded_size++] = value;
  return 0;
}

//-----------------------------------------------------------------------------
static int decoder_callback(int value)
{
  (void)value;
  return encoded_data[decoded_ptr++];
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  acoder_t coder;
  uint8_t *data;
  int size;

  if (argc != 2)
  {
    printf("File name required\n");
    return 0;
  }

  printf("Encoding %s\n", argv[1]);

  if (!load_file(argv[1], &data, &size))
  {
    printf("Error: can't open the file\n");
    return 0;
  }

  printf("Original size: %d\n", size);

  //------------------
  printf("Encoding\n");

  encoded_size = 0;

  acoder_init(&coder, ACODER_ENCODE, encoder_callback);

  for (int i = 0; i < size; i++)
    acoder_encode(&coder, data[i]);

  acoder_finish(&coder);

  float ratio = (1.0 - (float)encoded_size/size) * 100.0;

  printf("Encoded size: %d (ratio = %.3f %%)\n", encoded_size, ratio);

  //------------------
  printf("Decoding\n");

  decoded_ptr = 0;

  acoder_init(&coder, ACODER_DECODE, decoder_callback);

  while (decoded_size < size)
    decoded_data[decoded_size++] = acoder_decode(&coder);

  acoder_finish(&coder);

  //------------------
  printf("Comparing\n");

  for (int i = 0; i < size; i++)
  {
    if (data[i] != decoded_data[i])
    {
      printf("Incorrect data at %d: exp = 0x%02x, got = 0x%02x\n", i, data[i], decoded_data[i]);
      exit(1);
    }
  }

  printf("SUCCESS\n");

  //------------------
  printf("Saving the result\n");

  int fd, r;
  fd = open("z_out.bin", O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0644);
  r = write(fd, encoded_data, encoded_size);
  close(fd);

  (void)r;

  return 0;
}

