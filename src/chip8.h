#ifndef _chip8_h
#define _chip8_h

#include <stdint.h>

#define ROM_SIZE         3584
#define RAM_SIZE         4096
#define STACK_SIZE       16
#define SCREEN_WIDTH     64
#define SCREEN_HEIGHT    32
#define PROGRAM_LOCATION 0x200
#define FONT_LOCATION    0x000

typedef struct Chip8
{
    /* CPU registers */
    uint8_t V[16];
    uint16_t I : 12;
    uint8_t delay;
    uint8_t sound;

    /* VM registers */
    uint16_t PC : 12;
    uint8_t SP;
    uint8_t HALT;

    /* Data */
    uint8_t keyboard[16];
    uint8_t* screen;
    uint8_t* rom;
    uint8_t* ram;
    uint16_t* stack;
} Chip8;

Chip8*  cpu_init();
uint8_t cpu_load_rom(Chip8* cpu, const char* file_name);
void    cpu_reset(Chip8* cpu);
void    cpu_tick(Chip8* cpu);
void    cpu_destroy(Chip8* cpu);

#endif
