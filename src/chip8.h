// THIS FILE CONTAINS THE DECLARATION OF THE FUNCTIONS USED

/*
 * We need to ensure that this file is not gonna include in other files
 * To avoid this we'll put an inclusion gard, which ensures that this file is inserted only once
 * If a symbol hasn't been defined, it will be defined, and included all the code up to "endif"
 * otherwise it jump directly till "endif"
 */

#ifndef CHIP8_H_ 
#define CHIP8_H_

typedef struct Chip8 {
    unsigned char running;
} Chip8;


// Allocate the memory and return emulator' struct
Chip8* chip8Init(void);

// Deallocate the memory
void chip8Destroy(Chip8*);

// Load the program
void chip8LoadProgram(Chip8*, char*);

// Execute the opcode
void chip8ExecuteOpcode(Chip8*, short);


#endif // CHIP8_H_
