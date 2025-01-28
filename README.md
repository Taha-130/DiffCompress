# Projet PGM vers DIFF CoDec

Ce projet implémente un encodeur et un décodeur d'images en niveaux de gris à l'aide du codage différentiel et de la compression avec Code à Longueur Variable (CLV). L'encodeur (`pgmtodif`) transforme les fichiers `.pgm` en un format personnalisé `.dif`, tandis que le décodeur (`diftopgm`) inverse le processus pour reconstruire l'image originale. Le projet respecte les spécifications fournies dans le tutoriel sur le codage CLV et utilise le format DIFF avec une structure de fichier stricte et une logique de quantification.

Le projet est composé des fichiers source suivants : `src/pgmtodif.c` (application de l'encodeur), `src/diftopgm.c` (application du décodeur), `src/imgdif.c` (calcul de l'image différentielle et logique de reconstruction), et `src/codex.c` (fonctions de codage et décodage CLV). Les fichiers d'en-tête associés sont `include/imgdif.h` (pour le traitement différentiel des images) et `include/codex.h` (pour les fonctions CLV). Le fichier Makefile contient les règles de compilation du projet.

Pour compiler, assurez-vous que les dépendances nécessaires sont installées, y compris la bibliothèque graphique `g2x`.

## Nom des particpants

- SEFOUDINE Taha Thierry

---

## Structure des fichiers

### Fichiers sources :

- `src/pgmtodif.c` : Application de l'encodeur.
- `src/diftopgm.c` : Application du décodeur.
- `src/imgdif.c` : Logique de calcul de l'image différentielle et de reconstruction.
- `src/codex.c` : Fonctions de codage et décodage Code à Longueur Variable (CLV).

### Fichiers d'en-tête :

- `include/imgdif.h` : En-tête pour les fonctions de traitement des images différentielles.
- `include/codex.h` : En-tête pour les fonctions liées au CLV.

### Fichiers de support :

- `Makefile` : Règles de compilation du projet.

---

## Compilation Instructions

1. Assurez-vous que les dépendances nécessaires, y compris la bibliothèque graphique `g2x`, sont installées. Si vous compilez sur votre propre machine en tant qu'administrateur, utilisez les commandes suivantes pour installer ces dépendances :
   ```bash
   $ sudo apt install freeglut3 freeglut3-dev
   $ sudo apt install libglu1-mesa-dev mesa-common-dev
   $ sudo apt install libgl1-mesa-dev
   ```
   - Assurez-vous également d'installer la bibliothèque libg2x globalement sur votre système avant de poursuivre.
2. Exécutez la commande suivante dans le répertoire racine du projet pour compiler les fichiers :

   ```
   $ make
   ```

   Cela générera deux exécutables :

   - `pgmtodif` (encodeur)
   - `diftopgm` (décodeur)

3. Pour nettoyer les fichiers générés pendant la compilation, exécutez :
   ```
   $ make clean
   ```

---

## Exemples d'exécution

### Encodage :

1. Placez le fichier PGM d'entrée dans le répertoire `PGM/`.
2. Exécutez l'encodeur :
   ```bash
   ./pgmtodif ./PGM/image.pgm
   ```
3. Interagissez avec l'interface graphique pour :
   - Visualiser les images originales et différentielles.
   - Afficher les taux de compression.
   - Enregistrer le fichier compressé sous `./DIFF/image.dif`.

### Décodage:

1. Placez le fichier `.dif` dans le répertoire `DIFF/`.
2. Exécutez le décodeur :
   ```
   ./diftopgm ./DIFF/image.dif
   ```
3. Interagissez avec l'interface graphique pour :
   - Visualiser les images différentielles et reconstruites.
   - Enregistrer l'image reconstruite sous `./PGM/image.dif.pgm`.

---

## Formats de Fichiers

### Format DIFF

1. **En-tête** (11 octets) :

- Numéro magique : `0xD1FF` (2 octets).
- Largeur et hauteur : Entier non signé (2 octets chacun).
- Informations sur le quantificateur :
  - Nombre de niveaux (1 octet).
  - Bits par niveau (4 octets).

2. **Premier Pixel** (1 octet) : Valeur brute du premier pixel.
3. **Données Compressées** : Valeurs différentielles encodées.

### Format PGM

- Format : P5 (niveaux de gris binaires).
- L'en-tête inclut la largeur, la hauteur et la valeur maximale de gris.
- Les données des pixels sont stockées sous forme de valeurs non signées de 8 bits.

---

## Fonctionnalités

### Encodeur (`pgmtodif`) :

- Lit les images PGM en niveaux de gris.
- Affiche les images originales et différentielles.
- Calcule l'image différentielle.
- Compresse les données différentielles en utilisant VLC.
- Affiche les statistiques de compression.
- Sauvegarde la sortie compressée au format `.dif`.
- Fournit des histogrammes des distributions de pixels.

### Décodeur (`diftopgm`) :

- Lit les fichiers `.dif`.
- Affiche les images différentielles et reconstruites.
- Décode et reconstruit l'image originale.
- Sauvegarde la sortie reconstruite au format PGM.
- Fournit des histogrammes des distributions de pixels.

---

## Défis et Améliorations

### Défis :

- Mise en œuvre d'opérations précises au niveau des bits pour l'encodage/décodage VLC.
- Gestion des débordements de tampon et des bits de remplissage pendant l'encodage/décodage.
- Assurer la réactivité de l'interface graphique et l'échelle précise des histogrammes.

### Améliorations :

- Gestion améliorée des erreurs pour les formats de fichiers invalides et les entrées inattendues.
- Allocation de mémoire optimisée pour les grandes images.
- Logique de quantification améliorée pour de meilleures performances de compression.

---

## Problèmes Connus

- Les taux de compression peuvent se dégrader pour les images avec des distributions de pixels non standard.
- Les histogrammes excluent les valeurs extrêmes pour une meilleure lisibilité.

---

## References

- Tutoriel binaire sur l'encodage VLC.
- Documentation sur le format d'image PGM.
- Documentation de la bibliothèque graphique `g2x`.
