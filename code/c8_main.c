#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "SDL3/SDL.h"

/*
    
    TODO(werkor): 
      -find out correct size for pixel width and height in frect 
      -find out how why rendering is not changing correctly
        - I think it has to do with either the code that determines when to switch rows
          or the emulation function or if the pixels are not aligned with the screen coordinates
            - I think it has to do with either the code that determines when to switch rows
              or the emulation function
            - perhaps make a way to render each pixel indiviualy
            
      -figured out the display out of range stuff now more pixels are being rendered 
       have to look into emulate_instructions or update_screen again
       -getting like horitzontal lines on one half and a black half on the right
      
 */

// value typedefs----------------------------------
//
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t   uint8;
typedef int16_t   int64;
typedef int32_t   int32;
typedef int16_t   int16;
typedef int8_t     int8;
typedef uint32   bool32;
typedef uint8     bool8;
typedef double  float64;
typedef float   float32;



#define SDL_WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED

// sdl_t struct-------------------------------------
//
typedef struct{
    SDL_Window     *window;
    SDL_Renderer *renderer;
    SDL_Event       event;
}sdl_t;

// config_t struct------------------------------------
//
typedef struct{
    uint32 window_width;
    uint32 window_height;
    uint32 bg_color; // foreground color RGBA8888
    uint32 fg_color; // background color RGBA8888
    uint32 scale_factor; // amount to scale screen by (ex: 20 means scale screen by 20 times)
}config_t;

// emulator states--------------------------------------
//
typedef enum{
    QUIT =    0,
    RUNNING = 1,
    PAUSED =  2,
}emulator_state_t;

// chip8 instruction format-------------------------------------
//
typedef struct{
   uint16 opcode;
   uint16    NNN;  // 12 bit address/constant
   uint8      NN;  // 8 bit constant
   uint8       N;  // 4 bit constant
   uint8       X;  // 4 bit register identifier
   uint8       Y;  // 4 bit register identifier
}instruction_t;

// Chip8_t-----------------------------------------------
//
typedef struct{
   instruction_t instruction;
   bool           keypad[16];  // Hexadecimal keypad 0x0-0xF
   bool     display[64 * 32];  // Original chip8 resolution pixels
   uint16          stack[12];  // Subroutine stack
   uint16                  I;  // Index register
   uint16                 pc;  // Program counter
   uint8           ram[4096];
   uint8               v[16];  // Data registers V0-VF
   uint8         delay_timer;  // Decrements at 60hz when > 0
   uint16*         stack_ptr;
   uint8         sound_timer;  // Decrements at 60hz when > 0
   emulator_state_t    state;
   char*            rom_name;  // Currently running rom
}chip8_t;

// init_sdl----------------------------------------
//
bool8 init_sdl(sdl_t *sdl, config_t *config){
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)){
        fprintf(stderr,"Failed to init SDL: %s\n",SDL_GetError());
        return 0;
    }

    // NOTE(werkor): find out how to make it so i can resize the window to be start floating and be able tiled again (SDL_WINDOW_RESIZABLE)
   sdl->window = SDL_CreateWindow("Chip8", config->window_width * config->scale_factor, config->window_height * config->scale_factor, SDL_WINDOW_FLAGS);
   sdl->renderer = SDL_CreateRenderer(sdl->window, NULL);

   // SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, 255); // BLACK
   // SDL_RenderClear(sdl->renderer);
   // SDL_RenderPresent(sdl->renderer);

   return 1;
}

// set_config_from_args---------------------------------------
//
bool8 set_config_from_args(config_t *config, const int argc, char *argv){

    // defaults
    // config->window_width = 64;
    // config->window_height = 32;
    *config = (config_t){
        .window_width  = 64, // original chip8 width
        .window_height = 32, // original chip8 height
        .fg_color      = 0x00000000, // on color (WHITE)
        .bg_color      = 0xFFFFFFFF, // off color (BLACK)
        .scale_factor  = 20,         // ammount to scale screen by
    };

    for(uint8 i = 0; i > argc; ++i){
        (void)argv[i]; // so compilet does throw errors
    }

    // TODO(werkor): add code to override defaults

    return 1;
}

// init_chip8--------------------------------------------
//
bool8 init_chip8(chip8_t *chip8, char* rom_name){
    const uint32 entry_point = 0x200; // chip8 rom entry point

    const uint8 font[80] ={
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
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };

    // Load font
    memcpy(&chip8->ram, font, sizeof(font));

    // Open rom
    FILE *rom = fopen(rom_name, "rb");
    if(!rom){
        fprintf(stderr, "Rom file %s is invalid or does not exist!\n", rom_name);
        return -1;
    }

    // get/check rom size
    fseek(rom , 0, SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t max_size = sizeof(chip8->ram) - entry_point;
    rewind(rom);

    if(rom_size > max_size){
        fprintf(stderr, "Rom %s is too large! Rom size: %llu Max size allowed: %llu\n", rom_name, (unsigned long long)rom_size, (unsigned long long)max_size);
        return -1;
    }

    if(fread(&chip8->ram[entry_point], rom_size, 1, rom) != 1){
        fprintf(stderr, "Could not read rom file %s into chip8 memory\n", rom_name);
        return -1;
    }

    fclose(rom);

    // Set chip8 defaults
    chip8->state = RUNNING; // default state on startup
    chip8->pc = entry_point; // default state on startup
    chip8->rom_name = rom_name;
    chip8->stack_ptr = &chip8->stack[0];

   return 1;
}

#ifdef DEBUG
// print_debug_info-----------------------------------------
//
void print_debug_info(chip8_t *chip8){
    printf("Address: 0x%04X Opcode: 0x%04X Desc: ", chip8->pc-2, chip8->instruction.opcode);
    switch((chip8->instruction.opcode >> 12) & 0x0F){
        case 0x0:
          if(chip8->instruction.NN == 0xE0){
              // 0x00E0: Clear the screen
              printf("Clear screen\n");

          } else if(chip8->instruction.NN == 0xEE){
              // 0x00EE: Return from subroutine
              // Set program counter to last address on subroutine stack ("pop" it off the stack)
              // so that next opcode will be gotten from that address
              printf("Return from subroutine to address: 0x%04X\n", *(chip8->stack_ptr - 1));
          } else{
              printf("Unimplemented opcode\n");
          }
          break;
        case 0x01:
          // 0x1NNN: jump to NNN
          printf("Jump to address NNN (0x%04X)\n", chip8->instruction.NNN);
        break;

        case 0x02:
          // 0x2NNN: Call subroutine at NNN
          printf("Call subroutine at NNN (0x%04X)\n", chip8->instruction.NNN);
          // *chip8->stack_ptr++ = chip8->pc;       // store current address to return to on subroutine stack ("push" it on the stack)
          // chip8->pc = chip8->instruction.NNN;  // set program counter to subroutine address
          break;

          case 0x06:
            // 0x06NN: Sets VX to NN
            printf("Set register V%X to NN (0x%02X)\n", chip8->instruction.X, chip8->instruction.NN);
            // chip8->v[chip8->instruction.X] = chip8->instruction.NN;
          break;

          case 0x07:
            // 0x07NN: Sets VX += NN
            // chip8->v[chip8->instruction.X] += chip8->instruction.NN;
            printf("Set V%X (0x%02X) += NN (0x%02X). Result: 0x%02X\n", chip8->instruction.X, chip8->v[chip8->instruction.X], chip8->instruction.NN, chip8->v[chip8->instruction.X] + chip8->instruction.NN);
          break;

         case 0x0A:
           // 0xANNN: set I to NNN
           printf("Set I to NNN(0x%04X)\n", chip8->instruction.NNN);
           // chip8->I = chip8->instruction.NNN;
         break;

         case 0x0D:
           // 0xDXYN: Draw N-sprite at coords X,Y; Read at memory location I;
           printf("Draw N (%u) height sprite V%X (0x%02X) and V%X (0x%02X) from memory location I (0x%04X), Set VF = 1 if any pixels are turned off.\n", chip8->instruction.N, chip8->instruction.X, chip8->v[chip8->instruction.X], chip8->instruction.Y, chip8->v[chip8->instruction.Y], chip8->I);
         break;

        default:
          printf("Unimplemented opcode\n");
          break;
    }
}
#endif

// emulate_instruction---------------------------------------
//
void emulate_instruction(chip8_t *chip8, config_t config){
    // Get next opcode from ram
   chip8->instruction.opcode = (chip8->ram[chip8->pc] << 8) | chip8->ram[chip8->pc + 1];
   chip8->pc += 2;

   // Fill out instruction format
   chip8->instruction.NNN = chip8->instruction.opcode & 0x0FFF;
   chip8->instruction.NN = chip8->instruction.opcode & 0x0FF;
   chip8->instruction.N = chip8->instruction.opcode & 0x0F;
   chip8->instruction.X = (chip8->instruction.opcode >> 8) & 0x0F;
   chip8->instruction.Y = (chip8->instruction.opcode >> 4) & 0x0F;

   #ifdef DEBUG
   print_debug_info(chip8);
   #endif
   // emualte opcode
   switch((chip8->instruction.opcode >> 12) & 0x0F){
       case 0x0:
         if(chip8->instruction.NN == 0xE0){
             // 0x00E0: Clear the screen
             memset(&chip8->display[0], 0, sizeof(chip8->display));

         } else if(chip8->instruction.NN == 0xEE){
             // 0x00EE: Return from subroutine
             // Set program counter to last address on subroutine stack ("pop" it off the stack)
             // so that next opcode will be gotten from that address
             chip8->pc = *--chip8->stack_ptr;
         }
       break;

       case 0x01:
         // 0x1NNN: jump
         chip8->pc = chip8->instruction.NNN;
       break;

       case 0x02:
         // 0x2NNN: Call subroutine at NNN
         *chip8->stack_ptr++ = chip8->pc;       // store current address to return to on subroutine stack ("push" it on the stack)
         chip8->pc = chip8->instruction.NNN;  // set program counter to subroutine address
       break;
         
       case 0x03:
       break;

       case 0x06:
         // 0x06NN: Sets VX to NN
         chip8->v[chip8->instruction.X] = chip8->instruction.NN;
       break;

       case 0x07:
         // 0x07NN: Sets VX += NN
         chip8->v[chip8->instruction.X] += chip8->instruction.NN;
       break;

       case 0x0A:
         // 0xANNN: set I to NNN
         chip8->I = chip8->instruction.NNN;
       break;

       case 0x0D:{
         // 0xDXYN: Draw N-height sprite at coords X,Y: Read from memory location I;
         // screen pixles are XOR'd with spirte bits;
         // VF (carry flag) is set if any screen pixels are set off;
         // This is usefull for collision detection or other reasons
         uint8 X_cord = chip8->v[chip8->instruction.X] % config.window_width;
         uint8 Y_cord = chip8->v[chip8->instruction.Y] % config.window_height;
         const uint8 original_X = X_cord;
         
         chip8->v[0xF] = 0; // init carry flag to 0

         // loop over all rows of the N sprite
         for(uint8 i = 0; i < chip8->instruction.N; i++){
             // get next byte / row of data
             const uint8 sprite_data = chip8->ram[chip8->I + i];
             X_cord = original_X; // Reset X for next row to draw

             for(int8 j = 7; j >= 0; --j){
                 // if sprite/pixel bit is on and display pixel is on set carry flag
                 bool *pixel = &chip8->display[Y_cord * config.window_width + X_cord];
                 const bool sprite_bit = (sprite_data & (1 << j));
                 
                 if(sprite_bit && *pixel){
                     chip8->v[0xF] = 1; // set carry flag
                 }

                 //XOR display pixel with sprite pixel/bit to set it on/off
                 *pixel ^= sprite_bit;
                 

                 // Stop drawing this row if right edge of screen
                 if(++X_cord >= config.window_width){
                     break;
                 }
              }
            // Stop drawing entire sprite if the the bottom of screen
            if(++Y_cord >= config.window_height){
                break;
            }
         }
         break;
       }
       default:
         break;
   }

}

// final_cleanup---------------------------------------
//
void final_cleanup(sdl_t *sdl, config_t *config, chip8_t *chip8){
   SDL_DestroyRenderer(sdl->renderer);
   SDL_DestroyWindow(sdl->window);
   SDL_Quit(); // shutdown SDL subsystems
   free(sdl);
   free(config);
   free(chip8);
}

// clear_screen-------------------------------------------
//
void clear_screen(sdl_t *sdl,config_t *config){
  const uint8 r = (config->fg_color >> 24) & 0xFF;
  const uint8 g = (config->fg_color >> 16) & 0xFF;
  const uint8 b = (config->fg_color >>  8) & 0xFF;
  const uint8 a = (config->fg_color >>  0) & 0xFF;

  SDL_SetRenderDrawColor(sdl->renderer, r, g, b, a);
  SDL_RenderClear(sdl->renderer);
}

// update_screen------------------------------------
void update_screen(sdl_t *sdl, config_t *config, chip8_t *chip8){
  // SDL_Rect rect = {.x = 0,.y = 0, .w = config->scale_factor, .h = config->scale_factor};
  SDL_FRect frect = {.x = 0.0f, .y = 0.0f, .w = config->scale_factor, .h = config->scale_factor};

  // Grab color values to draw
 const uint8 fg_r = (config->fg_color >> 24) & 0xFF;
 const uint8 fg_g = (config->fg_color >> 16) & 0xFF;
 const uint8 fg_b = (config->fg_color >> 8) & 0xFF;
 const uint8 fg_a = (config->fg_color >> 0) & 0xFF;

 const uint8 bg_r = (config->bg_color >> 24) & 0xFF;
 const uint8 bg_g = (config->bg_color >> 16) & 0xFF;
 const uint8 bg_b = (config->bg_color >> 8) & 0xFF;
 const uint8 bg_a = (config->bg_color >> 0) & 0xFF;

  // Loop through display pixels, draw a rect per pixel to the sdl window
  for(uint32 i = 0; i < sizeof(chip8->display); ++i){
      // Translate 1d index i value to 2d x/y coordinates
      // rect.x = (i % config->window_width) * config->scale_factor;
      // rect.y = (i / config->window_width) * config->scale_factor;
      // frect.x = (float32)(i % config->window_height) * (float32)config->scale_factor;
      // frect.y = (float32)(i / config->window_height) * (float32)config->scale_factor;

      frect.x = (i % config->window_height) * config->scale_factor;
      frect.y = (i / config->window_height) * config->scale_factor;
      
      if(chip8->display[i]){
          // if pixel is on draw fg color (white)
          // SDL_RectToFRect(&rect,&frect);
          SDL_SetRenderDrawColor(sdl->renderer, fg_r, fg_g, fg_b, fg_a);
          SDL_RenderFillRect(sdl->renderer, &frect);
          // SDL_RenderRect(sdl->renderer, &frect);
      } else {
          // else draw bg color (black)
          // SDL_RectToFRect(&rect,&frect);
          SDL_SetRenderDrawColor(sdl->renderer, bg_r, bg_g, bg_b, bg_a);
          SDL_RenderFillRect(sdl->renderer, &frect);
          // SDL_RenderRect(sdl->renderer, &frect);
      }
  }
  SDL_RenderPresent(sdl->renderer);
}

// handle_input---------------------------------------
void handle_input(sdl_t *sdl, chip8_t *chip8){

    while(SDL_PollEvent(&sdl->event)){
        switch(sdl->event.type){
            case SDL_EVENT_QUIT:
            // exit window
            chip8->state = QUIT; // will exit main emulator loop
            break; // find a way to tell loop to quit

            case SDL_EVENT_KEY_DOWN:
              switch(sdl->event.key.scancode){
                  case SDL_SCANCODE_ESCAPE:
                    // escape key
                    chip8->state = QUIT;
                    return;
                  case SDL_SCANCODE_SPACE:
                    // space key
                    if(chip8->state == RUNNING){
                    chip8->state = PAUSED;
                    puts("===PAUSED===");
                    } else{
                        chip8->state = RUNNING;
                    };
                    return;

                default:
                break;
              }
              break;

            case SDL_EVENT_KEY_UP:
              break;

            default:
              break;
        }
    }
}

// main---------------------------------------
//
int main(int argc, char** argv){

  if(argc < 2){
      fprintf(stderr, "Usage: <command> <rom_name>\n");
      return -1;
  }

  // init emulation configuration
  config_t *config = malloc(sizeof(config_t));
  // config_t *config = {0};
  if(!set_config_from_args(config, argc, *argv)){
      fprintf(stderr, "could not set config from arguements\n");
      return -1;
  }

  // init SDL
  sdl_t *sdl = malloc(sizeof(sdl_t));
  // sdl_t *sdl = {0};
  if(!init_sdl(sdl, config)){
    fprintf(stderr, "failed to init sdl %s\n", SDL_GetError());
    return -1;
  }

  // init chip8
  chip8_t *chip8 = malloc(sizeof(chip8_t));
  // chip8_t *chip8 = {0};
  if(!init_chip8(chip8, argv[1])){
      fprintf(stderr, "failed to init chip8\n");
      return -1;
  }

  // test screen
  clear_screen(sdl, config);

  // main loop
  //
  while(chip8->state != QUIT){

      // psuedo code on running instructions in 16 ms
      //
      // check if PAUSED
      // if(chip8.state == PAUSED) continue;
      //
      // get_time()
      //
      // emulate instruction
      emulate_instruction(chip8, *config);
      // handle user input
      handle_input(sdl, chip8);
      //
      // get_time(16 - elapsed since last get time)

      // delay for aprox. 16 ms (60hz /fps)
      // SDL_Delay(16 - actual_time_elapsed);
      SDL_Delay(16);

      update_screen(sdl, config, chip8);
  }


  // then cleanup
  final_cleanup(sdl, config, chip8);

  printf("passed cleanup function\n");


    return 0;
}
