/* 
 *  checksums.h
 *
 *  Voxi function to calculate checksums. Only CRC32 currently
 *  implemented.
 *
 *  CRC32 code is modified from http://paul.rutgers.edu/~rhoads/Code/crc-32b.c
 */
#ifndef VOXI_CHECKSUMS_H
#define VOXI_CHECKSUMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <voxi/util/libcCompat.h>

EXTERN_UTIL unsigned long crc32_memory( const unsigned char *memory, size_t byteCount );

#ifdef __cplusplus
}
#endif

#endif