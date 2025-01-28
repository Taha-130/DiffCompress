#include <g2x.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "imgdif.h"
#include "codex.h"

static G2Xpixmap *diff_image = NULL;
static G2Xpixmap *reconstructed_image = NULL;
static uint8_t *reconstructed_data = NULL;
static uint8_t *coded_data = NULL;
static size_t data_size = 0;
static uint8_t *diff_data = NULL;

static int N;

// Taille de la fenêtre
static int WWIDTH = 800, WHEIGHT = 600;
static unsigned short img_width, img_height;
static double wxmin = -10., wymin = -10., wxmax = +10., wymax = +10.; // Coordonnées normalisées
static bool show_reconstructed_image = false;
static bool show_histograms = false;

// Garder une trace des coordonnées initiales de la fenêtre
static double init_wxmin, init_wymin, init_wxmax, init_wymax;

// fonctions callback g2x
static void init();
static void ctrl();
static void evts();
static void draw();
static void quit();

// afficher les histogrammes
static void displayHistograms();

// sauvegarder l'image reconstruite
static void save_reconstructed();

// basculer les histogrammes
static void toggle_histograms();

static void init() {
    // Stocker les coordonnées initiales de la fenêtre
    init_wxmin = wxmin;
    init_wymin = wymin;
    init_wxmax = wxmax;
    init_wymax = wymax;

    // Définir les coordonnées de la fenêtre aux valeurs normalisées
    g2x_SetWindowCoord(wxmin, wymin, wxmax, wymax);

    // Précharger les pixmaps
    if (diff_image != NULL)
        g2x_PixmapPreload(diff_image);
    if (reconstructed_image != NULL)
        g2x_PixmapPreload(reconstructed_image);
}

static void save_reconstructed() {
    if (reconstructed_data == NULL) {
        fprintf(stderr, "Aucune donnée reconstruite à sauvegarder!\n");
        return;
    }

    char outputFilename[256];
    char generic_filename[64];
    time_t timer;
    srand((unsigned int) time(&timer));
    snprintf(generic_filename, sizeof(generic_filename), "decoded_%ld", random());
    snprintf(outputFilename, sizeof(outputFilename), "./PGM/%s.dif.pgm", generic_filename);

    // Créer un en-tête d'image PGM
    FILE *fp = fopen(outputFilename, "wb");
    if (!fp) {
        perror("Erreur lors de l'ouverture du fichier de sortie");
        return;
    }

    fprintf(fp, "P5\n%d %d\n255\n", img_width, img_height);

    // Écrire les données des pixels
    if (N > 0) {
        fwrite(reconstructed_data, sizeof(uint8_t), N, fp);
    }

    fclose(fp);

    fprintf(stderr, "Image reconstruite sauvegardée dans %s\n", outputFilename);

    g2x_Quit();
}

static void toggle_histograms() {
    show_histograms = !show_histograms;
    if (show_histograms) {
        // ajuster le zoom pour les histogrammes lors de l'activation
        int hist_height = img_height / 4;
        g2x_SetWindowCoord(-20, -hist_height, 256 + 20, hist_height * 2);
    } else {
        // réinitialiser aux coordonnées initiales de la fenêtre lors de la désactivation
        g2x_SetWindowCoord(init_wxmin, init_wymin, init_wxmax, init_wymax);
    }
    g2x_Refresh();
}

static void ctrl() {
    // créer un interrupteur pour voir l'image reconstruite
    g2x_CreateSwitch("Voir l'image reconstruite", &show_reconstructed_image, "Basculer entre l'image différentielle et l'image reconstruite");

    // créer un bouton pour sauvegarder en PGM
    g2x_CreatePopUp("Sauvegarder en PGM", save_reconstructed, "Sauvegarder l'image reconstruite au format PGM");

    // créer un bouton pour voir les histogrammes
    g2x_CreatePopUp("Basculer les histogrammes", toggle_histograms, "Afficher les histogrammes des images");
}

static void evts(void) { }

static void draw() {
    if (show_histograms) {
        displayHistograms();
    } else if (show_reconstructed_image) {
        if (reconstructed_image) {
            g2x_PixmapRecall(reconstructed_image, false);
            g2x_StaticPrint(img_width / 2, 20, G2Xr, "IMAGE RECONSTRUITE");
        } else {
            g2x_StaticPrint(img_width / 2, img_height / 2, G2Xr, "AUCUNE IMAGE RECONSTRUITE");
        }
    } else {
        if (diff_image) {
            g2x_PixmapRecall(diff_image, false);
            g2x_StaticPrint(img_width / 2, 20, G2Xr, "IMAGE DIFFÉRENTIELLE");
        } else {
            g2x_StaticPrint(img_width / 2, img_height / 2, G2Xr, "AUCUNE IMAGE DIFFÉRENTIELLE");
        }
    }
}

static void quit() {
    if (diff_image != NULL)
        g2x_PixmapFree(&diff_image);
    if (reconstructed_image != NULL)
        g2x_PixmapFree(&reconstructed_image);
    if (reconstructed_data != NULL)
        free(reconstructed_data);
    if (coded_data != NULL)
        free(coded_data);
}

static void displayHistograms() {
    // calculer les histogrammes pour les images différentielles et reconstruites
    int hist_diff[256] = {0};
    int hist_recon[256] = {0};
    int max_hist_diff = 0;
    int max_hist_recon = 0;

    if (diff_image) {
        for (int i = 0; i < N; i++) {
            uint8_t pixel = diff_image->map[3*i + 0];
            hist_diff[pixel]++;
        }
    }

    if (reconstructed_data) {
        for (int i = 0; i < N; i++) {
            hist_recon[reconstructed_data[i]]++;
        }
    }

    // trouver les valeurs max des histogrammes, en excluant une petite marge sur les extrêmes
    for (int i = 10; i < 246; i++) {
        if (hist_diff[i] > max_hist_diff)
            max_hist_diff = hist_diff[i];
        if (hist_recon[i] > max_hist_recon)
            max_hist_recon = hist_recon[i];
    }

    // définir la zone d'affichage de l'histogramme avec une hauteur ajustée et un zoom
    int hist_height = img_height / 4;
    int start_y_diff = -hist_height / 2;
    int start_y_recon = hist_height / 2 + 20;

    // réduire les points extrêmes
    for (int i = 0; i < 256; i++) {
        if (hist_diff[i] > max_hist_diff * 0.8)
            hist_diff[i] = (int)(max_hist_diff * 0.8);
        if (hist_recon[i] > max_hist_recon * 0.8)
            hist_recon[i] = (int)(max_hist_recon * 0.8);
    }

    // centrer les histogrammes
    double center_x = 256 / 2.0;

    // dessiner les histogrammes
    for (int i = 0; i < 256; i++) {
        // Histogramme de l'image différentielle (Vert)
        if (max_hist_diff > 0 && diff_image) {
            double bar_height_diff = ((double)hist_diff[i] / max_hist_diff) * hist_height;
            g2x_Line(i, start_y_diff, i, start_y_diff + bar_height_diff * 0.8, G2Xg, 1);
        }

        // Histogramme de l'image reconstruite (Bleu)
        if (max_hist_recon > 0 && reconstructed_data) {
            double bar_height_recon = ((double)hist_recon[i] / max_hist_recon) * hist_height;
            g2x_Line(i, start_y_recon, i, start_y_recon + bar_height_recon * 0.8, G2Xb, 1);
        }
    }

    // étiquettes
    g2x_StaticPrint(center_x - 80, start_y_recon + hist_height + 40, G2Xg, "Histogramme de l'image différentielle");
    g2x_StaticPrint(center_x - 80, start_y_recon + hist_height + 80, G2Xb, "Histogramme de l'image reconstruite");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage : %s <chemin_vers_dif>\n", argv[0]);
        return 1;
    }

    // initialiser la fenêtre libg2x avec un titre descriptif et des coordonnées normalisées
    g2x_InitWindow("Décodeur DIFF en PGM - Mode RGB", WWIDTH, WHEIGHT);

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "<diftopgm>: impossible d'ouvrir %s\n", argv[1]);
        return 1;
    }

    // Lire l'en-tête
    uint8_t header[11];
    if (fread(header, sizeof(uint8_t), 11, f) != 11) {
        fprintf(stderr, "impossible de lire l'en-tête\n");
        fclose(f);
        return 1;
    }

    if (header[0] != 0xD1 || header[1] != 0xFF) {
        fprintf(stderr, "mauvais numéro magique %x %x\n", header[0], header[1]);
        fclose(f);
        return 1;
    }

    img_width = (unsigned short)((header[2] << 8) | header[3]);
    img_height = (unsigned short)((header[4] << 8) | header[5]);
    N = img_width * img_height;
    printf("Informations de l'en-tête : Largeur=%d, Hauteur=%d, N=%d\n", img_width, img_height, N);

    // Récupérer combien de niveaux de quantification sont dans le fichier 
    uint8_t nLevels = header[6];

    size_t  new_prefix_lens[4];
    int     new_value_lens[4];
    int     new_value_offsets[4];

    new_value_offsets[0] = 0;
    new_value_offsets[1] = 2;
    new_value_offsets[2] = 6;
    new_value_offsets[3] = 22;

    new_value_lens[0] = header[7];
    new_value_lens[1] = header[8];
    new_value_lens[2] = header[9];
    new_value_lens[3] = header[10];

    new_prefix_lens[0] = 1;
    new_prefix_lens[1] = 2;
    new_prefix_lens[2] = 3;
    new_prefix_lens[3] = 3;

    // Informer codex de ces nouveaux paramètres
    setQuantifierParams(new_prefix_lens, new_value_lens, new_value_offsets, nLevels);

    // allouer le Pixmap pour l'image différentielle avec 3 canaux de couleur (RGB)
    g2x_PixmapAlloc(&diff_image, img_width, img_height, 3, 255);
    diff_data = (uint8_t *)calloc(N, sizeof(uint8_t)); // Allouer pour les données différentielles
    if (diff_data == NULL) {
        fprintf(stderr, "Impossible d'allouer diff_data!\n");
        fclose(f);
        return 1;
    }
    printf("Alloué %lu octets pour diff_data\n", (unsigned long)N * sizeof(uint8_t)); // Impression de débogage

    // allouer reconstructed_data
    reconstructed_data = (uint8_t *)calloc(N, sizeof(uint8_t));
    if (reconstructed_data == NULL) {
        fprintf(stderr, "Impossible d'allouer reconstructed_data!\n");
        fclose(f);
        return 1;
    }
    printf("Alloué %lu octets pour reconstructed_data\n", (unsigned long)N * sizeof(uint8_t)); // Impression de débogage

    // Lire le premier pixel
    uint8_t firstpixel;
    if (fread(&firstpixel, 1, 1, f) != 1) {
        fprintf(stderr, "Erreur lors de la lecture du premier pixel\n");
        fclose(f);
        return 1;
    }
    printf("Premier pixel : %u\n", firstpixel);
    reconstructed_data[0] = firstpixel; // Définir le premier pixel
    diff_data[0] = 0; // Aucune différence pour le premier pixel

    // Lire les données compressées
    fseek(f, 0, SEEK_END);
    data_size = (size_t)(ftell(f) - 11 - 1);
    printf("Taille des données à lire : %lu octets\n", (unsigned long)data_size);
    fseek(f, 11 + 1, SEEK_SET); // revenir aux premiers octets des données codées

    coded_data = (uint8_t*)calloc(data_size, sizeof(uint8_t));
    if (coded_data == NULL)  {
        fprintf(stderr, "Impossible d'allouer coded_data!\n");
        fclose(f);
        return 1;
    }
    if (fread(coded_data, sizeof(uint8_t), data_size, f) != data_size) {
        fprintf(stderr, "Erreur lors de la lecture des données compressées\n");
        free(coded_data);
        fclose(f);
        return 1;
    }
    printf("Lu %lu octets de données compressées\n", (unsigned long)data_size);

    // Décode les données compressées (les différences commencent à l'index=1, car le premier pixel est stocké séparément)
    int decoded_pixels = decode(reconstructed_data + 1, coded_data, data_size, N - 1);
    printf("Pixels décodés : %d\n", decoded_pixels);

    fclose(f);

    // Reconstruire l'image
    if (reconstructImage(reconstructed_data, img_width, img_height) == 0) {
        fprintf(stderr, "Impossible de reconstruire l'image, abandon\n");
        return 1;
    }

    // Allouer le Pixmap pour l'image reconstruite avec 3 canaux de couleur (RGB)
    g2x_PixmapAlloc(&reconstructed_image, img_width, img_height, 3, 255);
    printf("Alloué pixmap pour l'image reconstruite : %d x %d\n", img_width, img_height);
    
    // Mapper les données reconstruites au pixmap RGB
    for (size_t i = 0; i < (size_t)N; i++) {
        reconstructed_image->map[3*i + 0] = reconstructed_data[i]; // Rouge
        reconstructed_image->map[3*i + 1] = reconstructed_data[i]; // Vert
        reconstructed_image->map[3*i + 2] = reconstructed_data[i]; // Bleu
    }

    // Extraire les données différentielles en calculant la différence
    for (size_t i = 1; i < (size_t)N; i++) {
        /* stocker la différence en complément à 2 pour l'affichage */
        int diff_val = (int)reconstructed_data[i] - (int)reconstructed_data[i - 1];
        diff_data[i] = (uint8_t)((int8_t)diff_val);
    }

    // Mapper les données différentielles au pixmap RGB
    for (size_t i = 0; i < (size_t)N; i++) {
        diff_image->map[3*i + 0] = diff_data[i]; // Rouge
        diff_image->map[3*i + 1] = diff_data[i]; // Vert
        diff_image->map[3*i + 2] = diff_data[i]; // Bleu
    }
    
    g2x_PixmapPreload(diff_image);
    g2x_PixmapPreload(reconstructed_image);

    // définir les fonctions Callback
    g2x_SetInitFunction(init);
    g2x_SetCtrlFunction(ctrl);
    g2x_SetEvtsFunction(evts);
    g2x_SetDrawFunction(draw);
    g2x_SetExitFunction(quit);

    // démarrer la boucle principale
    return g2x_MainStart();
}

