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

#define FPS 60
#define MILLISECS_PER_FRAME (1000/FPS) // number of millisecond per frame
#define INSTRUCTION_PER_FRAME 10

#define MODIFIER 15
#define SCREEN_WIDTH (MODIFIER * (CHIP8_WIDTH))
#define SCREEN_HEIGHT (MODIFIER * (CHIP8_HEIGHT))

#define AMPLITUDE 1500
#define FREQUENCY 440

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
   * This function is called by the SDL audio system on a separate
   * thread that will communicate with the audio hardware device.
   * 
   * Depending on the value of the CHIP-8 sound timer we will either
   * emit a sound or not. The sound is emitted by sampling from a
   * classical square wave that is generated on the fly.    
  */
void audio_callback(void* userdata, Uint8* stream, int len) {

  Chip8 *chip8 = (Chip8*)userdata;
  
  // The sample is 16-bits
  Sint16 *buffer = (Sint16*)stream;
  int length = len / 2;

  /*
   * Useful to remember where we are in the infinite SQUARE WAVE
   * This resets back to 0 when we the sound timer is no longer active.
   */
  static uint32_t sample_index = 0;
  
  for (int i = 0; i < length; i++) {
    if (chip8->sound_timer == 0) {
      buffer[i] = 0;
      sample_index = 0;
    }

    else {

      /* 
       * PLAY SOUND
       *
       * Here we consider the total samples in one cycle and we divide
       * it by two, since in a square wave is "high" for half the time
       * and "low" for half the time.
       * (sample frequency / pitch frequency) * (1/2) = samples per cycle / (frequency * 2)
       */
      int half_period = 44100 / (FREQUENCY * 2);

      /*
       * Depending on the position of the index we compute the
       * appropriate value. If we are in the "high" half_period we
       * maximize the value, and if we are in the "low"
       * half_period we minimize the value.
       */
      buffer[i] = ((sample_index++ / half_period) % 2) ? AMPLITUDE : -AMPLITUDE;     
    }
  }
}

CHIP8_KEY translateKey(SDL_Keycode sym) {
    CHIP8_KEY key_index;
    switch (sym) {
        case SDLK_1: key_index = CHIP8_KEY_1; break;
        case SDLK_2: key_index = CHIP8_KEY_2; break;
        case SDLK_3: key_index = CHIP8_KEY_3; break;
        case SDLK_4: key_index = CHIP8_KEY_4; break;
        case SDLK_q: key_index = CHIP8_KEY_Q; break;
        case SDLK_w: key_index = CHIP8_KEY_W; break;
        case SDLK_e: key_index = CHIP8_KEY_E; break;
        case SDLK_r: key_index = CHIP8_KEY_R; break;
        case SDLK_a: key_index = CHIP8_KEY_A; break;
        case SDLK_s: key_index = CHIP8_KEY_S; break;
        case SDLK_d: key_index = CHIP8_KEY_D; break;
        case SDLK_f: key_index = CHIP8_KEY_F; break;
        case SDLK_z: key_index = CHIP8_KEY_Z; break;
        case SDLK_x: key_index = CHIP8_KEY_X; break;
        case SDLK_c: key_index = CHIP8_KEY_C; break;
        case SDLK_v: key_index = CHIP8_KEY_V; break;
        default:     key_index = CHIP8_KEY_UNDEFINED; break;
    }
    return key_index;
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
                chip8->running = 0;
            } break;

            case SDL_KEYDOWN: {
                CHIP8_KEY key_index;
                key_index = translateKey(event.key.keysym.sym);

                if (key_index != CHIP8_KEY_UNDEFINED)
                    chip8->key[key_index] = CHIP8_KEY_DOWN;
            } break;

            case SDL_KEYUP: {
                CHIP8_KEY key_index;

                key_index = translateKey(event.key.keysym.sym);

                if (key_index != CHIP8_KEY_UNDEFINED)
                    chip8->key[key_index] = CHIP8_KEY_UP;
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

    // setting the audio
    SDL_AudioSpec want, have;
    SDL_zero(want);

    want.freq = 44100;              // (sempling frequency) standard CD quality frequency
    want.format = AUDIO_S16SYS;     // 16-bit signed samples
    want.channels = 1;              // Mono
    want.samples = 4096;            // Buffer data
    want.userdata = chip8;          // Pass chip8 so the callback can check sound_timer
    want.callback = audio_callback; // Function that generate the sound wave

    if (SDL_OpenAudio(&want, &have) < 0){
        LOG_ERROR_ARGS("Failed to open audio: %s", SDL_GetError());
    }

    // The audio is setted as paused by default
    SDL_PauseAudio(0);





    /*
     * EMULATION lOOP
     * Emulation Loop Timing:
     * - Target: 60 FPS (~16.6ms per frame).
     * - Timers & Display: Updated once per frame at 60Hz (graphics only if draw == 1).
     * - CPU Execution: Executes exactly 10 opcodes per frame, resulting in a 
     *   CPU speed of 600 Hz (10 ops × 60 frames), matching classic CHIP-8 behavior.
     * - Frame Pacing: A delay is applied at the end of each iteration to maintain 
     *   the 16.6ms target and prevent excessive CPU usage.
     */
    while (chip8->running){

        uint32_t startFrameTime = SDL_GetTicks();

        handleInput(chip8);

        for (int i = 0; i < INSTRUCTION_PER_FRAME; i++){
            chip8Step(chip8);
        }

        chip8UpdateTimers(chip8);


        // sound

        if (chip8->draw == 1){
            drawScreen(chip8, renderer);
            chip8->draw = 0;
        }

        uint32_t frameDuration = SDL_GetTicks() - startFrameTime;
        if (frameDuration < MILLISECS_PER_FRAME){
            SDL_Delay(MILLISECS_PER_FRAME - frameDuration);
        }
    }

    // --- Cleanup
    chip8Destroy(chip8);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
