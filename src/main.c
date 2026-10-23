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
        LOG_ERROR_ARGS("Missing arguments. Try: %s <CHIP-8 ROM>", argv[0]);
        return EXIT_FAILURE;
    }

    // Take the ROM we've decided to load and print it
    char *rom = argv[1];
    printf("ROM selected: '%s'\n", rom);
    
    // --- Initialization
    Chip8 *chip8 = chip8Init();
    LOG_INFO_ARGS("Allocated CHIP-8 at 0x%p", (void*)chip8);

    chip8LoadProgram(chip8, rom);
    LOG_INFO_ARGS("\033[0;32mLoading ROM\033[0m <%s>", rom);

    // --- Emulation Loop
    while (chip8->running){
        chip8Step(chip8);
    }

    // --- Cleanup
    chip8Destroy(chip8);    

    return 0;
}
