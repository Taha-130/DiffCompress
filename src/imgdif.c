#include "imgdif.h"
#include <stdio.h>

/*
 * calc differential : 
 *   d(i,j) = I(i,j) - I(i,j-1)
 * Stocker en complément à 2 dans un uint8_t donc les différences négatives
 * deviennent 128..255. 
 */
void diffImage(uint8_t* dest, uint8_t* src, int width, int height)
{
    for (int l = 0; l < height; l++)
    {
        for (int c = 0; c < width; c++)
        {
            if (c == 0) 
            {
                /* le premier pixel de chaque ligne est la valeur originale */
                dest[l*width + c] = src[l*width + c];
            }
            else 
            {
                /* différence en int, puis cast en int8_t => stocker en complément à 2 */
                int diff = (int)src[l*width + c] - (int)src[l*width + c - 1];
                dest[l*width + c] = (uint8_t)((int8_t)diff);
            }
        }
    }
}

/*
 * Reconstruire l'image à partir des données différentielles :
 *   I(i,j) = I(i,j-1) + d(i,j)
 * où d(i,j) est une différence signée en complément à 2
 */
int reconstructImage(uint8_t* dest, int width, int height)
{
    printf("Début de la reconstruction de l'image...\n");
    for (int l = 0; l < height; l++)
    {
        for (int c = 0; c < width; c++)
        {
            if (c == 0)
            {
                /* le premier pixel de chaque ligne est déjà la valeur originale */
                continue;
            }
            /* caster la différence en (int8_t) pour que le négatif soustraie correctement */
            int8_t d = (int8_t)dest[l * width + c];
            dest[l * width + c] = (uint8_t)((int)dest[l * width + c - 1] + d);
        }
    }
    printf("Reconstruction de l'image terminée.\n");
    return 1;
}

PGMImage* loadPGM(const char *filename) {
    FILE *fp;
    char buffer[1024];
    PGMImage *img = NULL;
    int width, height, maxval;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Erreur : Impossible d'ouvrir le fichier %s\n", filename);
        return NULL;
    }

    /* lire l'en-tête PGM */
    if (fgets(buffer, sizeof(buffer), fp) == NULL || strncmp(buffer, "P5", 2) != 0) {
        fprintf(stderr, "Erreur : Format PGM invalide\n");
        fclose(fp);
        return NULL;
    }

    /* sauter les commentaires */
    while (fgets(buffer, sizeof(buffer), fp) != NULL && buffer[0] == '#');

    if (sscanf(buffer, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "Erreur : Dimensions de l'image invalides\n");
        fclose(fp);
        return NULL;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL && buffer[0] == '#');

    if (sscanf(buffer, "%d", &maxval) != 1 || maxval > 255) {
        fprintf(stderr, "Erreur : Valeur maximale de gris invalide\n");
        fclose(fp);
        return NULL;
    }

    /* allouer la mémoire pour les données de l'image */
    img = (PGMImage *)malloc(sizeof(PGMImage));
    if (!img) {
        fprintf(stderr, "Erreur : Échec de l'allocation mémoire\n");
        fclose(fp);
        return NULL;
    }
    img->width  = (unsigned short)width;
    img->height = (unsigned short)height;
    img->map = (unsigned char *)malloc(width * height * sizeof(unsigned char));
    if (!img->map) {
        fprintf(stderr, "Erreur : Échec de l'allocation mémoire\n");
        fclose(fp);
        free(img);
        return NULL;
    }

    /* lire les données des pixels */
    if (fread(img->map, sizeof(unsigned char), width * height, fp) != (size_t)(width * height)) {
        fprintf(stderr, "Erreur : Impossible de lire les données de l'image\n");
        fclose(fp);
        free(img->map);
        free(img);
        return NULL;
    }

    fclose(fp);
    return img;
}

void freePGM(PGMImage *img) {
    if (img) {
        if (img->map) free(img->map);
        free(img);
    }
}

