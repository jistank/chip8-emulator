// THIS FILE CONTAINS THE IMPLEMENTATION OF CHIP-8

#include "chip8.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * CHIP-8 has 16 built-in hexadecimal font sprites (0-F), that represent characters from 0 through F
 * A sprite is a 2D image that is placed onto a larger scene
 * Each sprite is 4x5 pixels, represented by 5 bytes (1 byte per row, representing 4 pixels)
 * Total size: 16 sprites * 5 bytes = 80 bytes
 */
unsigned char CHIP8_FONTSET[80] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Allocate the memory and return emulator' struct
Chip8* chip8Init(void){
    /*
     * The chip8 memory is initialized only one time, it will be released once the execution is over
     * During initialization the font is loaded in the first 80 bytes.
     */
    Chip8* chip8 = malloc(sizeof(Chip8));
    if (chip8 == NULL){
        fprintf(stderr, "[ERROR]: Memory can't be allocated\n");
        exit(EXIT_FAILURE);
    }

    // clean up memory returned by malloc initializing all those byte to 0
    memset(chip8, 0, sizeof(Chip8));

    // load fontset into the emulator's memory
    memcpy(chip8-> memory, CHIP8_FONTSET, sizeof(CHIP8_FONTSET));
}

// Deallocate the memory
void chip8Destroy(Chip8* chip8){
    return;
}

// Load the program
void chip8LoadProgram(Chip8* chip8, char* rom){
    return;
}

// Execute the opcode
void chip8ExecuteOpcode(Chip8* chip8, short opcode){
    return;
}
