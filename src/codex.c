#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <g2x_types.h>
#include "codex.h"

// Codes Huffman et longueurs pour les préfixes
static uint8_t prefix_codes[4]  = {0b0, 0b10, 0b110, 0b111};
static size_t  prefix_lens[4]   = {1,  2,   3,    3};

// Longueurs des valeurs et décalages pour chaque préfixe
static int     value_lens[4]    = {1,  2,   4,    8};
static int     value_offsets[4] = {0,  2,   6,   22};

/* 
 * Fonction d'assistance pour mettre à jour dynamiquement les paramètres des préfixes/valeurs
 */
void setQuantifierParams(const size_t *new_prefix_lens,
                         const int    *new_value_lens,
                         const int    *new_value_offsets,
                         int nLevels)
{
    /* nLevels <= 4 */
    for (int i = 0; i < nLevels && i < 4; i++)
    {
        prefix_lens[i]   = new_prefix_lens[i];
        value_lens[i]    = new_value_lens[i];
        value_offsets[i] = new_value_offsets[i];
    }
}

/* Crée un flux de bits avec la capacité donnée */
BitStream g2x_CreateBitStream(size_t capa)
{
    BitStream  bs;
    bs.ptr = (uint8_t*)calloc(capa, sizeof(uint8_t));
    if (!bs.ptr)
    { 
        fprintf(stderr,"g2x_CreateBitStream : impossible d'allouer un tampon de %lu octets\n",(unsigned long)capa); 
        bs.capa=0; 
        return bs;
    }
    bs.capa   = capa;
    bs.bitpos = 0;
    return bs;
}

/* Libère un flux de bits */
void g2x_FreeBitStream(BitStream* bs)
{
    if(bs->ptr) free(bs->ptr);
    bs->ptr = NULL;
    bs->capa = 0;
    bs->bitpos = 0;
}

/* Écrit nbit bits depuis src dans le flux de bits actuel */
size_t pushbits(BitStream* curr, uint8_t src, size_t nbit)
{
    size_t k;
    for (k = 0; k < nbit; ++k)
    {
        if (curr->bitpos >= curr->capa * 8) {
            fprintf(stderr,"pushbits(): dépassement du tampon %lu/%lu\n",
                    (unsigned long)curr->bitpos,(unsigned long)curr->capa*8);
            return curr->bitpos;
        }
        /* décaler de MSB vers LSB pour nbit bits */
        if ((src >> (nbit - k - 1)) & 0x1)
            curr->ptr[(curr->bitpos / 8)] |= (0x1 << (7 - (curr->bitpos % 8)));
        curr->bitpos++;
    }
    return curr->bitpos;
}

/* Extrait nbit bits de la position actuelle dans le flux de bits dans *dest */
size_t pullbits(BitStream* curr, uint8_t* dest, size_t nbit)
{
    size_t k;
    *dest = 0;
    for (k = 0; k < nbit; ++k)
    {
        if(curr->bitpos >= curr->capa*8)
        {
            fprintf(stderr,"pullbits(): sous-débordement du tampon %lu/%lu\n",
                    (unsigned long)curr->bitpos,(unsigned long)curr->capa*8);
            return curr->bitpos;
        }
        /* lire le prochain bit du flux */
        if ((curr->ptr[(curr->bitpos / 8)] >> (7 - (curr->bitpos % 8))) & 0x1)
            *dest |= (1<<(nbit - k - 1));
        curr->bitpos++;
    }
    return curr->bitpos;
}

/* Trouve quelle catégorie de préfixe convient à la magnitude donnée */
static int getPrefix(uint16_t abs_diff, 
                     uint8_t *prefix, 
                     size_t  *prefix_len, 
                     int     *offset, 
                     size_t  *value_len)
{
    int i;
    for (i = 0; i < 4; i++) 
    {
        /* si abs_diff est dans [value_offsets[i], value_offsets[i]+(1<<value_lens[i])) */
        if ((abs_diff >= (uint16_t)value_offsets[i]) &&
            (abs_diff <  (uint16_t)(value_offsets[i] + (1 << value_lens[i]))))
        {
            *prefix      = prefix_codes[i];
            *prefix_len  = prefix_lens[i];
            *offset      = value_offsets[i];
            *value_len   = value_lens[i];
            return 1;
        }
    }
    return 0;
}

/* Encode les données sources (qui sont des différences signées en complément à 2) */
int encode(uint8_t* dest, uint8_t* src, int N, size_t *compSize)
{
    printf("Encodage en cours...\n");
    /*
     * Bits max = préfixe (3) + valeur (8) + signe (1) = 12 bits par différence
     */
    size_t max_comp_size = ((size_t)N * 12 + 7) / 8; 
    BitStream bs = g2x_CreateBitStream(max_comp_size);
    if (bs.capa == 0) return 0;

    size_t k;
    for (k = 0; k < (size_t)N; k++)
    {
        /* interpréter src[k] comme une différence signée (int8_t) */
        int8_t d = (int8_t)src[k]; 
        uint8_t sign = (d < 0) ? 1 : 0;
        uint16_t absd = (sign ? -d : d);

        uint8_t prefix;
        size_t prefix_len;
        int offset;
        size_t value_len;

        if (!getPrefix(absd, &prefix, &prefix_len, &offset, &value_len))
        {
            fprintf(stderr, "encode(): aucun préfixe valide trouvé pour %u\n", (unsigned)absd);
            g2x_FreeBitStream(&bs);
            return 0;
        }

        /* pousser le préfixe */
        if (pushbits(&bs, prefix, prefix_len) == 0)
        {
            g2x_FreeBitStream(&bs);
            return 0;
        }
        /* pousser (|d| - offset) en value_len bits */
        uint16_t coded_val = (uint16_t)(absd - offset);
        if (pushbits(&bs, (uint8_t)coded_val, value_len) == 0)
        {
            g2x_FreeBitStream(&bs);
            return 0;
        }
        /* pousser le bit de signe (1 bit) */
        if (pushbits(&bs, sign, 1) == 0)
        {
            g2x_FreeBitStream(&bs);
            return 0;
        }
    }

    /* copier les données compressées */
    memcpy(dest, bs.ptr, (bs.bitpos + 7) / 8);
    *compSize = (bs.bitpos + 7) / 8;
    g2x_FreeBitStream(&bs);

    printf("Encodage terminé!!\n");
    return 1;
}

/* Décode les données sources en différences signées */
int decode(uint8_t* dest, uint8_t* src, size_t P, int N)
{
    BitStream bs;
    bs.ptr    = src;
    bs.capa   = P;
    bs.bitpos = 0;

    int index = 0;

    while (bs.bitpos < bs.capa * 8 && index < N)
    {
        uint8_t prefix = 0;
        size_t  prefix_len = 0;
        uint8_t bit;

        for (int i = 0; i < 3; i++)
        {
            if (pullbits(&bs, &bit, 1) == 0) 
                break; /* sous-débordement ou problème */
            prefix = (uint8_t)((prefix << 1) | bit);
            prefix_len++;

            for (int j = 0; j < 4; j++) 
            {
                /* correspond exactement aux tableaux de codes préfixes et de longueurs */
                if ((prefix == prefix_codes[j]) && (prefix_len == prefix_lens[j]))
                {
                    size_t value_len = (size_t)value_lens[j];
                    int    offset    = value_offsets[j];

                    /* lire les bits de magnitude */
                    uint8_t buffer;
                    if (pullbits(&bs, &buffer, value_len) == 0)
                        goto enddec; /* erreur */
                    uint16_t absd = (uint16_t)(buffer + offset);

                    /* lire le bit de signe */
                    uint8_t sign;
                    if (pullbits(&bs, &sign, 1) == 0)
                        goto enddec; /* erreur */

                    int16_t d = (sign ? -(int16_t)absd : (int16_t)absd);
                    /* stocker de nouveau en complément à 2 (uint8_t) */
                    dest[index++] = (uint8_t)((int8_t)d);

                    goto prefix_done;
                }
            }
        }

        fprintf(stderr, "Avertissement : Préfixe non trouvé.\n");
        break;
    prefix_done:;
    }

enddec:
    return index; /* nombre de différences effectivement décodées */
}

