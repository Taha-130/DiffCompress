#ifndef _CODEX_H_
#define _CODEX_H_

#include <g2x_types.h>
#include <stddef.h>
#include <stdint.h> 

typedef struct {
    uint8_t* ptr;   	
    size_t capa;  		// Capacité totale du tampon
    size_t bitpos; 		// Position actuelle du bit dans le tampon
} BitStream;

BitStream  g2x_CreateBitStream(size_t capa);
void       g2x_FreeBitStream(BitStream* bs);
size_t     pushbits(BitStream *curr, uint8_t src, size_t nbit);
size_t     pullbits(BitStream *curr, uint8_t *dest, size_t nbit);
int        encode(uint8_t* dest, uint8_t* src, int N, size_t *compSize);
int        decode(uint8_t* dest, uint8_t* src, size_t P, int N);

/* quantificateurs dynamiques */
void setQuantifierParams(const size_t *new_prefix_lens,
                         const int    *new_value_lens,
                         const int    *new_value_offsets,
                         int nLevels);

#endif

