#ifndef _IMGDIF_H_
#define _IMGDIF_H_

#include <g2x_types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned char *map;
} PGMImage;

void diffImage(uint8_t* dest, uint8_t* src, int width, int height);
int reconstructImage(uint8_t* dest, int width, int height);
PGMImage* loadPGM(const char *filename);
void freePGM(PGMImage *img);

#endif

