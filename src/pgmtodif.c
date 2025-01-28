#include <g2x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "imgdif.h"
#include "codex.h"

// variables g2x :
static G2Xpixmap *original_pixmap = NULL;
static G2Xpixmap *diff_pixmap = NULL;
static PGMImage *original_data = NULL;

static int8_t *diff_data = NULL;
static size_t compressed_size = 0;
static double compression_rate = 0.0;

// taille de la fenêtre
static const int WWIDTH = 800, WHEIGHT = 600;

// Coordonnées de fenêtre normalisées
static const double WXMIN = -10.0, WYMIN = -10.0, WXMAX = +10.0, WYMAX = +10.0;

// drapeaux de contrôle
static bool show_diff_image = false;
static bool show_histograms = false;

// callbacks g2x
static void initialize();
static void setup_controls();
static void handle_events();
static void render();
static void cleanup();

static void displayHistograms();
static void compress_to_diff();
static void toggle_histograms();

static void initialize() {
    g2x_SetWindowCoord(WXMIN, WYMIN, WXMAX, WYMAX);

    // allouer et initialiser le pixmap pour l'image originale (RGB)
    g2x_PixmapAlloc(&original_pixmap, original_data->width, original_data->height, 3, 255);
    for (size_t i = 0; i < original_data->width * original_data->height; i++) {
        original_pixmap->map[3 * i + 0] = original_data->map[i]; 
        original_pixmap->map[3 * i + 1] = original_data->map[i];
        original_pixmap->map[3 * i + 2] = original_data->map[i];
    }
    g2x_PixmapPreload(original_pixmap);

    // allouer et initialiser le pixmap pour l'image différentielle (RGB)
    g2x_PixmapAlloc(&diff_pixmap, original_data->width, original_data->height, 3, 255);
    /* Nous allons 'décaler' les différences signées pour les afficher en 0..255 
    */
    size_t N = (size_t)(original_data->width * original_data->height);
    for (size_t i = 0; i < N; i++) {
        int shifted = (int)diff_data[i] + 128; 
        if (shifted < 0)   shifted = 0;   // sauvegarde
        if (shifted > 255) shifted = 255; // clamp
        diff_pixmap->map[3*i + 0] = (uint8_t)shifted; 
        diff_pixmap->map[3*i + 1] = (uint8_t)shifted;
        diff_pixmap->map[3*i + 2] = (uint8_t)shifted;
    }
    g2x_PixmapPreload(diff_pixmap);
}

static void compress_to_diff() {
    // générer un nom de fichier unique
    char outputFilename[256];
    char generic_filename[64];
    time_t timer;
    srand((unsigned int) time(&timer));
    snprintf(generic_filename, sizeof(generic_filename), "encoded_%ld", random());
    snprintf(outputFilename, sizeof(outputFilename), "./DIFF/%s.dif", generic_filename);

    FILE *f = fopen(outputFilename, "wb");
    if (!f) {
        fprintf(stderr, "Erreur : Impossible de créer le fichier de sortie %s\n", outputFilename);
        return;
    }

    // Écrire l'en-tête
    uint8_t header[11];
    memset(header, 0, 11);
    header[0] = 0xD1;
    header[1] = 0xFF;

    unsigned short width = original_data->width;
    unsigned short height = original_data->height;
    header[2] = (width >> 8) & 0xFF;
    header[3] = (width) & 0xFF;
    header[4] = (height >> 8) & 0xFF;
    header[5] = (height) & 0xFF;

    header[6]  = 4; 
    header[7]  = 1; 
    header[8]  = 2; 
    header[9]  = 4;
    header[10] = 8; 

    if (fwrite(header, sizeof(uint8_t), 11, f) != 11) {
        fprintf(stderr, "Erreur lors de l'écriture de l'en-tête dans le fichier.\n");
        fclose(f);
        return;
    }

    // Écrire le premier pixel en brut
    uint8_t first_pixel = (uint8_t) original_data->map[0];
    if (fwrite(&first_pixel, 1, 1, f) != 1) {
        fprintf(stderr, "Erreur lors de l'écriture du premier pixel dans le fichier.\n");
        fclose(f);
        return;
    }

    // Encoder les données différentielles
    size_t N = (size_t)width * (size_t)height;
    uint8_t *coded_buf = (uint8_t *)malloc((N * 12 + 7) / 8); // 12 bits maintenant
    if (!coded_buf) {
        fprintf(stderr, "Erreur : Impossible d'allouer la mémoire pour coded_buf!\n");
        fclose(f);
        return;
    }
    memset(coded_buf,0,(N*12+7)/8);

    /* nous encodons diff_data+1 car la première différence est le pixel brut */
    if (!encode(coded_buf, diff_data + 1, (int)(N - 1), &compressed_size)) {
        fprintf(stderr, "Erreur : L'encodage a échoué!\n");
        free(coded_buf);
        fclose(f);
        return;
    }

    // Écrire les données compressées
    if (fwrite(coded_buf, 1, compressed_size, f) != compressed_size) {
        fprintf(stderr, "Erreur lors de l'écriture des données compressées dans le fichier.\n");
        free(coded_buf);
        fclose(f);
        return;
    }

    // calculer le taux de compression
    compression_rate = 100.0 * ((double)compressed_size / (double)N);
    fprintf(stderr, "Compressé en %lu octets : (%.3f%%)\n",
            (unsigned long)compressed_size, compression_rate);

    fclose(f);
    free(coded_buf);
    g2x_Refresh();
}

static void toggle_histograms() {
    show_histograms = !show_histograms;
    g2x_Refresh();
}

static void setup_controls() {
    g2x_CreateSwitch("Voir l'image différentielle", &show_diff_image, "Basculer entre l'image originale et l'image différentielle");
    g2x_CreatePopUp("Compresser en DIFF", compress_to_diff, "Compresser l'image au format DIFF");
    g2x_CreatePopUp("Basculer les histogrammes", toggle_histograms, "Afficher les histogrammes des images");
}

static void handle_events() { }

static void render() {
    if (show_histograms) {
        displayHistograms();
    }
    else if (show_diff_image) {
        if (diff_pixmap) {
            g2x_PixmapRecall(diff_pixmap, false);
            g2x_StaticPrint(WXMIN + 0.0, WYMAX - 1.0, G2Xr, "IMAGE DIFFÉRENTIELLE");
        }
        else {
            g2x_StaticPrint(0.0, 0.0, G2Xr, "AUCUNE IMAGE DIFFÉRENTIELLE");
        }

        if (compression_rate > 0.0) {
            char rate_str[50];
            snprintf(rate_str, sizeof(rate_str), "Taux de compression : %.2f%%", compression_rate);
            g2x_StaticPrint(WXMIN + 1.0, WYMAX - 2.0, G2Xg, rate_str);
        }
    }
    else {
        if (original_pixmap) {
            g2x_PixmapRecall(original_pixmap, false);
            g2x_StaticPrint(WXMIN + 0.0, WYMAX - 1.0, G2Xr, "IMAGE ORIGINALE");
        }
        else {
            g2x_StaticPrint(0.0, 0.0, G2Xr, "AUCUNE IMAGE ORIGINALE");
        }
    }
}

static void cleanup() {
    if (original_pixmap != NULL)
        g2x_PixmapFree(&original_pixmap);
    if (diff_pixmap != NULL)
        g2x_PixmapFree(&diff_pixmap);
    if (original_data != NULL)
        freePGM(original_data);
    if (diff_data != NULL)
        free(diff_data);
}

static void displayHistograms() {
    int hist_original[256] = {0};
    int hist_diff[256]     = {0};
    int max_hist_original  = 0;
    int max_hist_diff      = 0;

    size_t N = (size_t)original_data->width * original_data->height;
    for (size_t i = 0; i < N; i++) {
        uint8_t px_orig = original_pixmap->map[3*i + 0]; 
        hist_original[px_orig]++;
        
        /* le pixmap de diff est affiché comme décalé */
        uint8_t px_diff = diff_pixmap->map[3*i + 0]; 
        hist_diff[px_diff]++;
    }

    for (int i = 10; i < 246; i++) {
        if (hist_original[i] > max_hist_original)
            max_hist_original = hist_original[i];
        if (hist_diff[i] > max_hist_diff)
            max_hist_diff = hist_diff[i];
    }

    double hist_width_norm  = (WXMAX - WXMIN) / 255.0;
    double hist_height_norm = (WYMAX - WYMIN) / 6.0;

    double start_y_orig = WYMIN + (WYMAX - WYMIN) / 2.0 + 1.0;
    double start_y_diff = start_y_orig + hist_height_norm + 1.0;

    for (int i = 0; i < 256; i++) {
        double x_norm = WXMIN + hist_width_norm * i;
        if (i >= 10 && i < 246 && max_hist_original > 0) {
            double bh = ((double)hist_original[i] / max_hist_original) * hist_height_norm;
            g2x_Line(x_norm, start_y_orig, x_norm, start_y_orig + bh, G2Xg, 1);
        }
        if (i >= 10 && i < 246 && max_hist_diff > 0) {
            double bh = ((double)hist_diff[i] / max_hist_diff) * hist_height_norm;
            g2x_Line(x_norm, start_y_diff, x_norm, start_y_diff + bh, G2Xb, 1);
        }
    }

    g2x_StaticPrint(WXMIN + 30.0, start_y_orig + hist_height_norm + 430, G2Xg, "Histogramme de l'image originale");
    g2x_StaticPrint(WXMIN + 30.0, start_y_diff + hist_height_norm + 570, G2Xb, "Histogramme de l'image différentielle");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage : %s <chemin_vers_pgm>\n", argv[0]);
        return EXIT_FAILURE;
    }

    original_data = loadPGM(argv[1]);
    if (!original_data) {
        fprintf(stderr, "Échec du chargement de l'image PGM originale.\n");
        return EXIT_FAILURE;
    }

    size_t N = original_data->width * original_data->height;
    /* Allouer un tableau SIGNÉ pour les différences */
    diff_data = (int8_t *)calloc(N, sizeof(int8_t));
    if (!diff_data) {
        fprintf(stderr, "Erreur : Impossible d'allouer la mémoire pour diff_data!\n");
        freePGM(original_data);
        return EXIT_FAILURE;
    }

    uint8_t *temp_diffs = (uint8_t*)calloc(N,1);
    diffImage(temp_diffs, original_data->map, original_data->width, original_data->height);

    /* Maintenant copier dans le tableau */
    for (size_t i=0; i<N; i++) {
        /* Si les différences peuvent dépasser 128, gérer le wrap ou clamp si nécessaire */
        int d = (int) temp_diffs[i];
        if (d>127) d -= 256;
        diff_data[i] = (int8_t)d;
    }
    free(temp_diffs);

    // initialiser libg2x
    g2x_InitWindow("Encodeur PGM en DIFF - Mode RGB", WWIDTH, WHEIGHT);

    // initialiser les pixmaps
    initialize();
    g2x_SetInitFunction(initialize);
    g2x_SetCtrlFunction(setup_controls);
    g2x_SetEvtsFunction(handle_events);
    g2x_SetDrawFunction(render);
    g2x_SetExitFunction(cleanup);

    return g2x_MainStart();
}

