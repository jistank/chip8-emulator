// THIS FILE CONTAINS THE IMPLEMENTATION OF CHIP-8

#include "chip8.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

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

// Allocate the memory and return the emulator struct
Chip8* chip8Init(void){
    /*
     * The chip8 memory is initialized only one time, it will be released once the execution is over
     * During initialization the font is loaded in the first 80 bytes.
     */
    Chip8* chip8 = malloc(sizeof(Chip8));
    if (chip8 == NULL){
        LOG_ERROR("Memory can't be allocated");
        exit(EXIT_FAILURE);
    }

    /*
     * Clean up memory returned by malloc initializing all those bytes to 0, so:
     *    opcode = 0
     *    sp (stack pointer) = 0
     *    I (index register) = 0
     *    draw (binary flag to update the screen) = 0
     */
    memset(chip8, 0, sizeof(Chip8));

    // Load fontset into the emulator's memory
    memcpy(chip8->memory, CHIP8_FONTSET, sizeof(CHIP8_FONTSET));

    chip8->running = 1; // at the beginning the chip8 is running
    chip8->pc = 0x200;  // (program counter) the address where we start reading our program is 0x200

    /*
     * Initialize the PRNG (Pseudo-Random Number Generator) with the current time
     * This is required for the CHIP-8 CXNN instruction, which generates a random number
     *      C: (12 in decimal) used by the CHIP-8 instruction set to identify the random
     *          number generation family
     *      X: (indicates which memory register receives the number)
     *      NN: (Hex value used as a mask) The emulator generates a random 8-bit number (0-255)
     *          and applies a bitwise AND operation with NN
     * Using time(NULL) as the seed ensures a different sequence of random numbers 
     * on every program run, preventing predictable gameplay
     * The formula is: V[X] = (random_number) & NN
     */
    srand(time(NULL));

    return chip8;
}

// Deallocate the memory
void chip8Destroy(Chip8 *chip8){
    if (chip8 != NULL)
        free(chip8);
}

// Load the program
// @param program The path to the ROM file
void chip8LoadProgram(Chip8 *chip8, char* program){
    /*
     * The ROM is loaded into memory starting at address 0x200 (after the first 512 bytes)
     * The first 512 bytes (from 0x000 to 0x1FF) are reserved for the CHIP-8 interpreter or system area
     */
    FILE *f = fopen(program, "rb"); // read binary, we are reading every single byte
    if (f == NULL){
        LOG_ERROR("Failed to open the ROM");
        exit(EXIT_FAILURE);
    }

    /*
     * Now we need to compute the program's length
     * To do that, we go to the end of the file using fseek function
     * which moves the cursor to a specific position (at the end of the file in this case)
     */
    fseek(f, 0, SEEK_END);

    /*
     * ftell() returns the current cursor position in the file (which is at the end)
     * The number returned is a long, not an unsigned long
     * In this way we know the file size
     */
    long fSize = ftell(f);
    
    /*
     * It is necessary to check if the file size respects the memory availability
     * Memory available is: 4096 - 512 = 3584 bytes
     * <= is used because ftell() could return -1 on error
     */
    if ((fSize <= 0) || (fSize > (0x1000 - 0x200))){ 
        LOG_ERROR_ARGS("Operation Failed: ROM size too large: %ld bytes, the file must be less than or equal to 3584 bytes", fSize);
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // Move the file cursor to the beginning of the file using rewind()
    rewind(f);

    /*
     * 1st param: Pointer to the memory block where data is read.
     *      We read into memory after the byte 0x200 (the 512th byte)
     *      because the first 512 bytes are occupied, so the pointer offset is the memory start + 0x200
     * 2nd param: Size of each element to read (1 byte at a time)
     * 3rd param: Number of elements to read (file size)
     * 4th param: Pointer to the FILE stream (the file itself)
     */
    size_t result = fread(chip8->memory + 0x200, 1, fSize, f);

    /*
     * Check if the number of bytes returned by fread() is different from the file's size
     * In other words, we're checking whether all bytes have been successfully read
     */
    if (result != (size_t)fSize){
        LOG_ERROR("Operation Failed: can't read program bytes");
        fclose(f);
        exit(EXIT_FAILURE);
     }

    fclose(f);
}

/* 
 * This function handles the "Fetch" and "PC advancement" phases of the 
 * Fetch-Decode-Execute cycle.
 */
void chip8Step(Chip8 *chip8){

    // Ensure the Program Counter (PC) is within the valid 4KB memory bounds.
    if (chip8->pc >= 4095){

        /*
         * 0x%03X is a format specifier: prints the number in uppercase (X) hexadecimal format
         * 03: pad with zeros on the left until a length of at least 3 characters is reached
         *
         * ex: if the number (in decimal) is: 10, in Hex is: A. With %03X becomes: 00A
         */
        LOG_ERROR_ARGS("Impossible increment PC: 0x%03X", chip8->pc);
        exit(EXIT_FAILURE);
    }

    /*
     * FETCH the 16-bit (2-byte) opcode from memory at the current program counter address.
     * The opcode contains both:
     *   - The instruction itself (ex: ADD, JP, DRAW)
     *   - The operands (ex: register indices, immediate values, or memory addresses)
     */
    unsigned short opcode = FETCH_OPCODE(chip8);
    chip8->opcode = opcode;

    /*
     * Increment the program counter by 2 bytes to point to the next instruction.
     * Note: FETCH_OPCODE already used the previous PC value to get the current opcode.
     */
    chip8->pc += 2;
    
    // DECODE & EXECUTE: Pass the fetched opcode to the execution dispatcher
    chip8ExecuteOpcode(chip8, opcode);
}

// Decodes and executes a given 16-bit CHIP-8 opcode.
void chip8ExecuteOpcode(Chip8 *chip8, short opcode){

    /*
     * DECODE: Extract the components of the 16-bit opcode using bitwise AND
     * and right shift operations.
     */
    unsigned char X   = (opcode & 0x0F00) >> 8;
    unsigned char Y   = (opcode & 0x00F0) >> 4;
    unsigned char N   = opcode & 0x000F;
    unsigned char NN  = opcode & 0x00FF;
    unsigned char NNN = opcode & 0x0FFF;
    
    // Cast void to disable warning by compiler, otherwise it complains about unused variables
    (void)X;
    (void)Y;
    (void)N;
    (void)NN;
    (void)NNN;

    // compile the program with DEBUG=1 (ex: make DEBUG=1) to see the debug log
    #ifdef DEBUG
        LOG_INFO_ARGS("[0x%03X] Executing 0x%04X", chip8->pc -2, opcode);
    #endif

    /*
     * EXECUTE: Dispatch to the correct instruction handler based on the most 
     * significant 4 bits (the first hexadecimal digit) of the opcode.
     */
    switch ((opcode & 0x0F00) >> 12){
        case 0x0: {
        } break;
        case 0x1: {
        } break;
        case 0x2: {
        } break;
        case 0x3: {
        } break;
        case 0x4: {
        } break;
        case 0x5: {
        } break;
        case 0x6: {
        } break;
        case 0x7: {
        } break;
        case 0x8: {
        } break;
        case 0x9: {
        } break;
        case 0xA: {
        } break;
        case 0xB: {
        } break;
        case 0xC: {
        } break;
        case 0xD: {
        } break;
        case 0xE: {
        } break;
        case 0xF: {
        } break;
        default:{
            LOG_ERROR("Unreachable default case in chip8ExecuteOpcode");
            exit(EXIT_FAILURE);
        } break;
    }
    return;

}



