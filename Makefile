# Compilateur et Options
CC = gcc
CFLAGS = -O2

# Chemins d'inclusion
INC = -I./include -I/usr/include/g2x

# Options de bibliothèque
LIB_DIR = -L/usr/lib/g2x
LIBS = -lg2x  # Lier contre la bibliothèque g2x

# Options de Compilation et d'édition de liens
PFLAGS = $(INC)
CFLAGS += $(PFLAGS)
LFLAGS = $(LIB_DIR) $(LIBS)

# Fichiers source partagés
COMMON_SRC = src/imgdif.c src/codex.c
COMMON_OBJ = $(COMMON_SRC:.c=.o)

# Fichiers source spécifiques à l'exécutable
PGM_SRC = src/pgmtodif.c
PGM_OBJ = $(PGM_SRC:.c=.o)
PGM_EXE = pgmtodif

DIF_SRC = src/diftopgm.c
DIF_OBJ = $(DIF_SRC:.c=.o)
DIF_EXE = diftopgm

# Répertoires à créer
DIRS = DIFF PGM

# Tous les Cibles
all: dirs $(PGM_EXE) $(DIF_EXE)

# Créer les répertoires nécessaires silencieusement
dirs:
	@mkdir -p $(DIRS)/

# Construire l'exécutable pgmtodif
$(PGM_EXE): $(PGM_OBJ) $(COMMON_OBJ)
	@echo "Édition de liens [$^] -> $@"
	@$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@
	@echo "------------------------"

# Construire l'exécutable diftopgm
$(DIF_EXE): $(DIF_OBJ) $(COMMON_OBJ)
	@echo "Édition de liens [$^] -> $@"
	@$(CC) $(CFLAGS) $^ $(LFLAGS) -o $@
	@echo "------------------------"

# Règle de modèle pour compiler les fichiers .c en fichiers .o
%.o: %.c
	@echo "Compilation $< -> $@"
	@$(CC) $(CFLAGS) -c $< -o $@

# Nettoyer les artefacts de construction
clean:
	@echo "Nettoyage en cours..."
	@rm -f $(PGM_EXE) $(DIF_EXE) $(COMMON_OBJ) $(PGM_OBJ) $(DIF_OBJ)
	@echo "Nettoyé."

.PHONY: all clean dirs

