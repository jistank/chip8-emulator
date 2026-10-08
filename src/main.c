#include <stdio.h>
#include <stdlib.h>
#include "chip8.h" // The double quotes mean that this file is taken from the current directory

/*
 *  main: will take arguments from the command line
 *  argv: is an array of strings (char sequence), it points to the argument passed
 *           The first argument is the program's name
 *           The second argument is gonna be the ROM
 */

int main(int argc, char **argv){

    // We need at least two command-line arguments: program name and ROM
    if (argc < 2) {
        printf("[ERROR] Missing arguments. Try: %s <CHIP-8 ROM>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Take the ROM we've decided to load and print it
    char *rom = argv[1];
    printf("Loading ROM: '%s'\n", rom);
    
    // --- Initialization

    // --- Emulation Loop

    // --- Cleanup

    return 0;
}
