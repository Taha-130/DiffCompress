# Projet PGM vers DIFF CoDec

This project implements an image encoder and decoder for grayscale images using differential coding and compression with Variable Length Coding (VLC). The encoder (pgmtodif) converts .pgm files into a custom .dif format, while the decoder (diftopgm) reverses the process to reconstruct the original image. The project follows the specifications provided in the VLC coding tutorial and uses the DIFF format with a strict file structure and quantization logic.

The project consists of the following source files: src/pgmtodif.c (encoder application), src/diftopgm.c (decoder application), src/imgdif.c (differential image calculation and reconstruction logic), and src/codex.c (VLC encoding and decoding functions). The associated header files are include/imgdif.h (for differential image processing) and include/codex.h (for VLC functions). The Makefile contains the project's compilation rules.

To compile, ensure that the necessary dependencies are installed, including the g2x graphics library. 

## Name of participants

- SEFOUDINE Taha Thierry

---

## File structure

### Source files :

- `src/pgmtodif.c` : Encoder application.
- `src/diftopgm.c` : Decoder application.
- `src/imgdif.c` : Logic for differential image computation and reconstruction.
- `src/codex.c` : Functions for Variable-Length Coding (VLC) encoding and decoding.

### Header Files :

- `include/imgdif.h` : Header file for differential image processing functions.
- `include/codex.h` : Header file for VLC-related functions.

### Support Files:

- `Makefile` : Compilation rules for the project.
---

## Compilation Instructions

1. Ensure that the necessary dependencies, including the `g2x` graphics library, are installed. If you are compiling on your own machine as an administrator, use the following commands to install these dependencies:
   ```bash
   $ sudo apt install freeglut3 freeglut3-dev
   $ sudo apt install libglu1-mesa-dev mesa-common-dev
   $ sudo apt install libgl1-mesa-dev
   ```
   
   -Also make sure to install the libg2x library globally on your system before proceeding.
2. Run the following command in the project root directory to compile the files:
   ```
   $ make
   ```

   This will generate two executables:

   - `pgmtodif` (encodeur)
   - `diftopgm` (décodeur)

3. To clean the files generated during compilation, run:
   ```
   $ make clean
   ```

---

## Execution examples

### Encoding :

1. Place the input PGM file in the `PGM/` directory.
2. Run the encoder:
   ```bash
   ./pgmtodif ./PGM/image.pgm
   ```
3. Interact with the GUI to:
   - View original and differential images.
   - Show compression ratios.
   - Save the compressed file as `./DIFF/image.dif`.

### Decoding:

1. Place the `.dif` file in the `DIFF/` directory.
2. Run the decoder:
   ```
   ./diftopgm ./DIFF/image.dif
   ```
3. Interact with the GUI to:
   - View differential and reconstructed images.
   - Save the reconstructed image as `./PGM/image.dif.pgm`.
     
---

## File Formats

### DIFF format

1. **Header** (11 octets) :
- Magic number: `0xD1FF` (2 bytes).
- Width and height: Unsigned integer (2 bytes each).
- Quantifier information:
- Number of levels (1 byte).
- Bits per level (4 bytes).

2. **First Pixel** (1 byte): Raw value of the first pixel.
3. **Compressed Data**: Encoded differential values.

### PGM Format

- Format: P5 (binary grayscale).
- Header includes width, height, and maximum gray value.
- Pixel data is stored as 8-bit unsigned values.

---

## Features

### Encoder (`pgmtodif`):

- Reads grayscale PGM images.
- Displays original and differential images.
- Calculates the differential image.
- Compresses the differential data using VLC.
- Displays compression statistics.
- Saves the compressed output in `.dif` format.
- Provides histograms of pixel distributions.

### Decoder (`diftopgm`):

- Reads `.dif` files.
- Displays differential and reconstructed images.
- Decodes and reconstructs the original image.
- Saves the reconstructed output in PGM format.
- Provides histograms of pixel distributions.

--

## Challenges and Improvements

### Challenges:

- Implement bit-precise operations for VLC encoding/decoding.
- Handle buffer overflows and padding bits during encoding/decoding.
- Ensure GUI responsiveness and accurate histogram scaling.

### Improvements:

- Improved error handling for invalid file formats and unexpected inputs.
- Optimized memory allocation for large images.
- Improved quantization logic for better compression performance.

---

## Known Issues

- Compression ratios may degrade for images with non-standard pixel distributions.
- Histograms exclude outliers for better readability.

---

## References

- Binary tutorial on VLC encoding.
- Documentation on the PGM image format.
- Documentation of the `g2x` graphics library.
