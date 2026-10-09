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

    // This flag tell to us if chip8 is running
    unsigned char running;

    // Opcode that is running at that moment (CHIP8 has 35 opcode, eachone two bytes long)
    unsigned short opcode;

    /*
     * Memory is a sequence of bytes (standard chip8 supports 4KB)
     * 0x000-0x1FF: Reserved for the CHIP-8 interpreter (includes the emulator's font set)
     * 0x050-0x0A0: Stores the built-in 4×5 pixel font set (0–F)
     * 0x200-0xFFF: Program ROM and working RAM
     */
    unsigned char memory[4096];

    /*
     * A register is a memory container
     * The chip8 has: 15 general porpuse registers (eachone long 8bit) named V0 through VE
     */
    unsigned char V[16];

    /*
     * Chip8 has two registers of 16-bit (therefore their type is short):
     *      Index (I)
     *      program counter (pc): indicates the memory address of the next instruction to perform
     * Both can range between 0x000 and 0xFFF
     */
    unsigned short I;
    unsigned short pc;

    /*
     * Frame buffer containing pixel data to drow on the schreen (contains totally 2048px)
     * The graphics is black and white
     */
    unsigned char gfx[64*32];
    
    // binary flag to draw or not, this to know if the schreen shoul be updated
    unsigned char draw;

    /*
     * The chip8 has two timer registers (they cont 60Hz)
     * These timers count down to 0
     * The system buzzer sounds whenever the sound timer reaches 0
     */
    unsigned char delay_timer;
    unsigned char sound_timer;

    /*
     * The stack is implemented as an array with 16 stack levels
     * It serves to preserve the current execution address prior to a jump operation
     * Whenever a jump or subroutine call is executed, the program counter must be pushed onto the stack before continuing
     */
    unsigned char stack[16];

    // Stack pointer that always points to the current top level of the stack
    unsigned short sp;

    // The CHIP-8 includes a 16-key hex keypad (0x0–0xF). An array stores each key's current state.
    unsigned char key[16];


} Chip8;

/*
 * This enum maps descriptive labels to a sequence of numbers.
 * This way, instead of remembering the numeric value of a pressed key, we can simply refer to its descriptive label.
 * It is only necessary to initialize the first value, as the subsequent fields are automatically incremented by 1 (CHIP8_KEY_2 will be 1, etc)
 */
typedef enum {
    CHIP8_KEY_1 = 0,
    CHIP8_KEY_2,
    CHIP8_KEY_3,
    CHIP8_KEY_4,
    CHIP8_KEY_Q,
    CHIP8_KEY_W,
    CHIP8_KEY_E,
    CHIP8_KEY_R,
    CHIP8_KEY_A,
    CHIP8_KEY_S,
    CHIP8_KEY_D,
    CHIP8_KEY_F,
    CHIP8_KEY_Z,
    CHIP8_KEY_X,
    CHIP8_KEY_C,
    CHIP8_KEY_V 
} CHIP8_KEY;


// Allocate the memory and return emulator' struct
Chip8* chip8Init(void);

// Deallocate the memory
void chip8Destroy(Chip8*);

// Load the program
void chip8LoadProgram(Chip8*, char*);

// Execute the opcode
void chip8ExecuteOpcode(Chip8*, short);


#endif // CHIP8_H_
