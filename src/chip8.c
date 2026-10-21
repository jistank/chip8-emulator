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
     *            We read into memory after the byte 0x200 (the 512th byte)
     *            because the first 512 bytes are occupied,
     *            so the pointer offset is the memory start + 0x200
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
    
    // Cast void to disable warning by compiler,
    // otherwise it complains about unused variables
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
            // 00E0, clear the screen
            // if the last digit is 0 we execute this instruction
            if ((opcode & 0x000F) == 0x0000){
                 // Clear the graphics buffer by setting all bytes to 0 (2048 bytes)
                memset(chip8->gfx, 0, CHIP8_WIDTH*CHIP8_HIGHT);
            }

            // 00EE, Return from a subroutine
            else if ((opcode & 0x00FF) == 0x00EE){
                // Decrement SP before reading the stack, to not read the wrong address.
                chip8->sp--;
                // Restore the program counter to the return address previously 
                // saved at the top of the stack by the 2NNN instruction.
                chip8->pc = chip8->stack[chip8->sp];

            /*
             * 0NNN: Call machine code routine at address NNN.
             * This is rarely used in modern CHIP-8 ROMs. We log it for debugging 
             * purposes but do not execute actual machine code in this emulator.
             */
            } else {
                LOG_INFO("Executing 0NNN");
            }
        } break;

        // 1NNN, Jump to address NNN
        case 0x1: {
            chip8->pc = NNN;
        } break;

        // 2NNN, Execute subroutine starting at address NNN
        case 0x2: {

            /*
             * Save the current PC (return address) to the stack before jumping
             * First, ensure there is enough space in the stack (max 16 nested calls)
             */
            if (chip8->sp == 16){
                LOG_ERROR("Call stack overflow");
                exit(EXIT_FAILURE);
            }

            // Save the return address and increment the stack pointer.
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;

           /*
            * Jump to the new subroutine address.
            * changing the program counter to NNN address
            */
            chip8->pc = NNN;
        } break;

        // 3XNN, skip the following instruction if the value of register VX equals NN
        case 0x3: {

            // Ensure X is a valid register index (0-15) before accessing the array.
            if (X >= 16){
                LOG_ERROR_ARGS("3XNN, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }

            // If the register value equals NN, skip the next 2-byte instruction.
            // (because every instruction occupies 2 byte)
            if (chip8->V[X] == NN){
                chip8->pc += 2;
            }
        } break;

        // 4XNN, skip the following instruction if the value of register VX is not equal to NN
        case 0x4: {
            if (X >= 16){
                LOG_ERROR_ARGS("4XNN, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }

            if (chip8->V[X] != NN){
                chip8->pc += 2;
            }
        } break;

        // 5XY0, skip the following instruction if the value of register VX is equal to the value of register VY
        case 0x5: {
            if (X >= 16){
                LOG_ERROR_ARGS("5XY0, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }

            if (Y >= 16){
                LOG_ERROR_ARGS("5XY0, invalid value for Y: %d > 15", Y);
                exit(EXIT_FAILURE);
            }

            if (chip8->V[X] == chip8->V[Y]){
                chip8->pc += 2;
            }
        } break;

        // 6XNN, store number NN in register VX
        case 0x6: {
            if (X >= 16){
                LOG_ERROR_ARGS("6XNN, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }
            chip8->V[X] = NN;
        } break;

        // 7XNN, Add the value NN to register VX
        case 0x7: {
             if (X >= 16){
                LOG_ERROR_ARGS("7XNN, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }
            chip8->V[X] += NN;
        } break;

        case 0x8: {
             if (X >= 16){
                // *: represents all instructions with the 0x8 prefix
                LOG_ERROR_ARGS("0x8*, invalid value for X: %d > 15", X);
                exit(EXIT_FAILURE);
            }
                
             if (Y >= 16){
                LOG_ERROR_ARGS("0x8*, invalid value for Y: %d > 15", Y);
                exit(EXIT_FAILURE);
            }

            // 8XY0, Store the value of register VY in register VX
            if ((opcode & 0x000F) == 0x0000){
                chip8->V[X] = chip8->V[Y];
            }

            // 8XY1, Set VX to VX OR VY
            else if ((opcode & 0x000F) == 0x0001) {
                chip8->V[X] |= chip8->V[Y];      
            }

            // 8XY2, Set VX to VX AND VY
            else if ((opcode & 0x000F) == 0x0002) {
                chip8->V[X] &= chip8->V[Y];            
            }

            // 8XY3, Set VX to VX XOR VY
            else if ((opcode & 0x000F) == 0x0003) {
                chip8->V[X] ^= chip8->V[Y];        
            }

            /* 8XY4
             *  1. Add the value of register VY to register VX
             *  2. Set VF to 01 if a carry occurs
             *  3. Set VF to 00 if a carry does not occur
             */
            else if ((opcode & 0x000F) == 0x0004) {
                /*
                 * Since a register holds an 8-bit value (maximum 255),
                 * if the sum of V[X] and V[Y] > 255, we have a carry
                 */
                unsigned char carry = (chip8->V[X] + chip8->V[Y] > 255) ? 1 : 0;
                chip8->V[X] += chip8->V[Y];
                // VF is the flag register: set it to 1 if there was a carry, else 0
                chip8->V[15] = carry;
            }

            /* 8XY5
             * 1. Subtract the value of register VY from register VX
             * 2. Set VF to 00 if a borrow occurs
             * 3. Set VF to 01 if a borrow does not occur
             */
            else if ((opcode & 0x000F) == 0x0005) {

              // A borrow occurs only if VX < VY.
              // In all other cases, no borrow occurs, and we can set VF to 1.
              unsigned char notBorrow = chip8->V[X] >= chip8->V[Y] ? 1 : 0;

                /*
                 * Be careful with the order of operations here.
                 * First, we compute the flag, then we update VX,
                 * and we finally assign the flag.
                 * This correctly handles the case where X = 15
                 */ 
              chip8->V[X] = chip8->V[X] - chip8->V[Y];
              chip8->V[15] = notBorrow;
            }

            /* 8XY6
             * 1. Store the value of register VY shifted right one bit in
             * register VX
             * 2. Set register VF to the least significant bit prior to the
             * shift. VY is unchanged
             */
            else if ((opcode & 0x000F) == 0x0006) {
              // TODO: add quirk support
              
              // lsb stands for: least significant bit
              unsigned char lsb = chip8->V[Y] & 0x1;
              chip8->V[X] = chip8->V[Y] >> 1;
              chip8->V[15] = lsb;
            } 

            /* 8XY7
             * 1. Set register VX to the value of VY minus VX
             * 2. Set VF to 00 if a borrow occurs
             * 3. Set VF to 01 if a borrow does not occur
            */
            else if ((opcode & 0x000F) == 0x0007) {
              unsigned char notBorrow = chip8->V[Y] >= chip8->V[X] ? 1 : 0;
              chip8->V[X] = chip8->V[Y] - chip8->V[X];
              chip8->V[15] = notBorrow;
            }

            /* 8XYE
             * 1. Store the value of register VY shifted left one bit in register VX
             * 2. Set register VF to the most significant bit prior to the shift.
             *    VY is unchanged
            */
            else if ((opcode & 0x000F) == 0x000E) {
              // TODO: add quirk support
              
              unsigned char msb = (chip8->V[Y] & 0x80) >> 7;
              chip8->V[X] = chip8->V[Y] << 1;
              chip8->V[15] = msb;
            }        

            else {
              LOG_ERROR_ARGS("Unknown opcode '0x%x'", opcode);
              exit(EXIT_FAILURE);
            }
        } break;

        /*
         * Skips the next instruction if VX does not equal VY.
         * This is achieved by increasing the PC by 2.
         */
        case 0x9: {
            if (chip8->V[X] != chip8->V[Y])
                chip8->pc += 2;
        } break;
                  
        // Sets I to the address NNN
        case 0xA: {
            chip8->I = NNN;
        } break;

        // Jumps to the address NNN plus V0
        case 0xB: {
            chip8->pc = NNN + chip8->V[0];
        } break;

        /* 
         * Sets VX to the result of a bitwise and operation on a random number 
         * (Typically: 0 to 255) and NN.
         */
        case 0xC: {
            /*
             * rand(): generate random number (of 32-bit)
             * (rand() & 0xFF): with this bitmask extracts the 8 least significant bits
             * all other bits are set to 0, which is equivalent to generating 8 random bits.
             * But we want to mask this random value with NN (so we do another AND bit to bit)
             */
            chip8->V[X] = (unsigned char)((rand() & 0xFF) & NN);
        } break;

        /*
         * Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
         * Each row of 8 pixels is read as bit-coded starting from memory location I;
         * The "I" value does not change after the execution of this instruction.
         * As described above, VF is set to 1 if any screen pixels are flipped from set
         * to unset when the sprite is drawn, and to 0 if that does not happen
         */ 
        case 0xD: {
        } break;

        case 0xE: {
            /*
             * Skips the next instruction if the key stored in VX (only consider the lowest nibble)
             * is pressed (usually the next instruction is a jump to skip a code block).
             */
                if ((opcode & 0x00FF) == 0x009E){
                    // taking the last 4-bits of the register V[X]
                    unsigned char key_index = chip8->V[X] & 0x0F;
                    if (chip8->key[key_index] == 1){
                        chip8->pc += 2;
                    }
                }

                /*
                 * Skips the next instruction if the key stored in VX (only consider the lowest nibble)
                 * is not pressed (usually the next instruction is a jump to skip a code block).
                 */
                else if ((opcode & 0x00FF) == 0x00A1){
                    unsigned char key_index = chip8->V[X] & 0x0F;
                    if (chip8->key[key_index] == 0){
                        chip8->pc += 2;
                    }
                }

                else {
                    LOG_ERROR_ARGS("Unknown opcode: '0x%x'\n", opcode);
                    exit(EXIT_FAILURE);
                }

        } break;

        case 0xF: {

            // FX07, Sets VX to the value of the delay timer
            if ((opcode & 0x00FF) == 0x0007){
                chip8->V[X] = chip8->delay_timer;
            }

            /*
             * FX0A, A key press is awaited, and then stored in VX
             * (blocking operation, all instruction halted until next key event,
             *  delay and sound timers should continue processing)
             */
            else if ((opcode & 0x00FF) == 0x000A){
                unsigned char key_press = 0;
                // cycle through the keyboard
                for (int i = 0; i <= 15; i++){
                    // if key[i] is true (!= 0)
                    if (chip8->key[i] != 0){
                        // store the index of that key in V[X]
                        chip8->V[X] = i;
                        key_press = 1;
                        break;
                    }
                }
                // continue with the next instruction

                /* 
                 * If any key hasn't been pressed, decrement the program counter by 2
                 * (This means we re-execute the current instruction, that is that one
                 *  we're implementing in this moment)
                 *  This because we incremented it in chip8Step() before execute this
                 *  funcion. To execute the same opcode we have to decrement it.
                 */
                if (!key_press){
                    chip8->pc -= 2;
                    return;
                }
            }

            // FX15, Sets the delay timer to VX
            else if ((opcode & 0x00FF) == 0x0015){
                chip8->delay_timer = chip8->V[X];
            }

            // FX18, Sets the sound timer to VX
            else if ((opcode & 0x00FF) == 0x0018){
                chip8->sound_timer = chip8->V[X];
            }

            // FX1E, Adds VX to I. VF is not affected
            else if ((opcode & 0x00FF) == 0x001E){
                chip8->I += chip8->V[X];
            }

            /*
             * FX29, Sets I to the location of the sprite for the character in VX
             * (only consider the lowest nibble).
             * Characters 0-F (in hexadecimal) are represented by a 4x5 font.
             */
            else if ((opcode & 0x00FF) == 0x0029){

                /*
                 * Sprite data is represented by a row of CHIP8_FONTSET
                 * The sprite data used to render that digit occupies 5 bytes
                 *
                 * 0: we load the fontset starting by the address 0
                 * (chip8->V[X]): the index of the row 
                 * (chip8->V[X] & 0x0F): extracts the last 4 bits (from 0 to 15)
                 * *0x5: once the row is found we multiply by 5
                 *       (because each row is 5bytes long)
                 */
                chip8->I = 0 + (chip8->V[X] & 0x0F) * 0x5;
            }
            
            /*
             * FX33, Stores the binary-coded decimal representation of VX,
             * with the hundreds digit in memory at location in I,
             * the tens digit at location I+1, and the ones digit at location I+2
             */ 
            else if ((opcode & 0x00FF) == 0x0033){
                unsigned char value = chip8->V[X];

                /*
                 * Convert the value in decimal:
                 * ex: we have 143
                 *     143 / 100 = 1
                 *     143 % 100 = 43, 43 / 10 = 4
                 *     143 % 100 = 43 % 10 = 3
                 */
                chip8->memory[chip8->I + 0] = value / 100;
                chip8->memory[chip8->I + 1] = (value % 100) / 10;
                chip8->memory[chip8->I + 2] = (value % 100) % 10;
            }

            /*
             * FX55, Stores from V0 to VX (including VX) in memory, starting at address I.
             * The offset from I is increased by 1 for each value written,
             * but I itself is left unmodified
             */
            else if ((opcode & 0x00FF) == 0x0055){

                // Ensure memory access is within bounds
                if (chip8->I + X >= 4096){
                    LOG_ERROR("Memory access out of bounds in FX55\n");
                    exit(EXIT_FAILURE);
                }

                for (unsigned char i = 0; i <= X; i++){
                    // Copy the value from the register to the memory
                    chip8->memory[chip8->I + i] = chip8->V[i];
                }

                // quirk
                chip8->I = chip8->I + X + 1;
            }

            /*
             * FX65, Fills from V0 to VX (including VX) with values from memory, starting at address I.
             * The offset from I is increased by 1 for each value read, but I itself is left unmodified
             */
            else if ((opcode & 0x00FF) == 0x0065){

                // Ensure memory access is within bounds
                if (chip8->I + X >= 4096){
                    LOG_ERROR("Memory access out of bounds in FX65\n");
                    exit(EXIT_FAILURE);
                }

                for (unsigned char i = 0; i <= X; i++){
                    // Read from the memory and writing on the register
                    chip8->V[i] = chip8->memory[chip8->I + i];
                }

                // quirk
                chip8->I = chip8->I + X + 1;
            }

            else {
                LOG_ERROR_ARGS("Unknown opcode: '0x%x'\n", opcode);
                exit(EXIT_FAILURE);
            }

        } break;

        default:{
            LOG_ERROR("Unreachable default case in chip8ExecuteOpcode");
            exit(EXIT_FAILURE);
        } break;
    }
    return;

}



