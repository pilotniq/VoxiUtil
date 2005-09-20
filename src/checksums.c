/*
 * Voxi Checksums functions
 *
 * CRC32 code modified from http://paul.rutgers.edu/~rhoads/Code/crc-32b.c
 *
 */

#include <assert.h>
#include <voxi/util/checksums.h>

/*
 *  static function prototypes
 */

/* generate the crc table. Must be called before calculating the crc value */
static void gen_table(void);

/*
 *  static data
 */
static unsigned long crc_table[256] = { 1 };

/*
 * Start of code
 */
static void gen_table(void)                /* build the crc table */
{
  unsigned long crc, poly;
  int i, j;

  poly = 0xEDB88320L;
  for (i = 0; i < 256; i++)
  {
    crc = i;
    for (j = 8; j > 0; j--)
    {
      if (crc & 1)
        crc = (crc >> 1) ^ poly;
      else
        crc >>= 1;
    }
    crc_table[i] = crc;
  }
}

/* calculate the crc value */
unsigned long crc32_memory( const unsigned char *memory, size_t byteCount )
{
  register unsigned long crc;
  unsigned int i;

  if( crc_table[0] == 1 )
  {
    gen_table();
    assert( crc_table[0] != 1 );
  }

  crc = 0xFFFFFFFF;
  for( i = 0; i < byteCount; i++, memory++ )
    crc = (crc>>8) ^ crc_table[ (crc^(*memory)) & 0xFF ];

  return( crc^0xFFFFFFFF );
}
