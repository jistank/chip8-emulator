#include <stdio.h>
#include <stdlib.h>
#include "chip8.h" // The double quotes mean that this file is taken from the current directory
#include <SDL2/SDL.h>

/*
 *  main: will take arguments from the command line
 *  argv: is an array of strings (char sequence), it points to the argument passed
 *           The first argument is the program's name
 *           The second argument is gonna be the ROM
 */

#define MODIFIER 15
#define SCREEN_WIDTH (MODIFIER * (CHIP8_WIDTH))
#define SCREEN_HEIGHT (MODIFIER * (CHIP8_HEIGHT))

/*
 * INITIALIZATION FUNCTION
 * 
 * Initializes the SDL2 video subsystem and creates the main application 
 * window and a hardware-accelerated renderer for drawing graphics.
 * 
 * The window and renderer parameters are double pointers 
 * because we need to modify the original pointer variables declared in main
 */
void initSDL(SDL_Window **window, SDL_Renderer **renderer){

    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        LOG_ERROR_ARGS("SDL Could not initialize: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Create the main window and assign it to the pointer in main
    *window = SDL_CreateWindow("CHIP8",
                                SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED,
                                SCREEN_WIDTH,
                                SCREEN_HEIGHT,
                                SDL_WINDOW_SHOWN);

    if (!(*window)){
        LOG_ERROR_ARGS("SDL window could not be created: %s", SDL_GetError());
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

    // Create a hardware-accelerated renderer for the window
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if(!(*renderer)) {
         LOG_ERROR_ARGS("SDL renderer could not be created: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(EXIT_FAILURE);
       
    }
}

/*
 * DRAW SCREEN FUNCTION
 * 
 * Reads the CHIP-8 graphics buffer (chip8->gfx) and renders it to the 
 * SDL window. 
 */
void drawScreen(Chip8 *chip8, SDL_Renderer *renderer){
    // Assume chip8->draw == 1, as this is checked already in the emulation loop

    // Clear the entire screen
    SDL_RenderClear(renderer);

    // Iterate through the CHIP-8 graphics buffer (64x32)
    for (int y = 0; y < CHIP8_HEIGHT; y++){
        for (int x = 0; x < CHIP8_WIDTH; x++){

            if (chip8->gfx[y * CHIP8_WIDTH + x] == 1){
                // setting the color: r, g, b, alpha
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            }

            SDL_Rect rect = {
                .x = x * MODIFIER,
                .y = y * MODIFIER,
                .w = MODIFIER,
                .h = MODIFIER
            };

            SDL_RenderFillRect(renderer, &rect);

         }
    }

    // Update the screen with the newly drawn frame
    SDL_RenderPresent(renderer);
}

/*
 * INPUT HANDLING FUNCTION
 * 
 * Polls the SDL event queue to process operating system events, 
 * such as the user closing the window or pressing keyboard keys.
 */
void handleInput(Chip8 *chip8){
    SDL_Event event;

    /*
     * As long as there are pending events in the SDL queue,
     * extract the event data and populate the event struct
     */
    while (SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT: {
                // User clicked the window's close button
                chip8->running = 0;
            } break;
        }
    }
}


// ======= MAIN =======
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

    SDL_Window *window;
    SDL_Renderer *renderer;
    initSDL(&window, &renderer);

    Chip8 *chip8 = chip8Init();
    LOG_INFO_ARGS("Allocated CHIP-8 at 0x%p", (void*)chip8);

    chip8LoadProgram(chip8, rom);
    LOG_INFO_ARGS("\033[0;32mLoading ROM\033[0m <%s>", rom);

    // --- Emulation Loop
    while (chip8->running){
        handleInput(chip8);
        chip8Step(chip8);
        if (chip8->draw == 1){
            drawScreen(chip8, renderer);
            chip8->draw = 0;
        }
    }

    // --- Cleanup
    chip8Destroy(chip8);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
