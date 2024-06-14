#include "chip8.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint8_t font[] = {
    /* 0 */ 0xF0, 0x90, 0x90, 0x90, 0xF0,
    /* 1 */ 0x20, 0x60, 0x20, 0x20, 0x70,
    /* 2 */ 0xF0, 0x10, 0xF0, 0x80, 0xF0,
    /* 3 */ 0xF0, 0x10, 0xF0, 0x10, 0xF0,
    /* 4 */ 0x90, 0x90, 0xF0, 0x10, 0x10,
    /* 5 */ 0xF0, 0x80, 0xF0, 0x10, 0xF0,
    /* 6 */ 0xF0, 0x80, 0xF0, 0x90, 0xF0,
    /* 7 */ 0xF0, 0x10, 0x20, 0x40, 0x40,
    /* 8 */ 0xF0, 0x90, 0xF0, 0x90, 0xF0,
    /* 9 */ 0xF0, 0x90, 0xF0, 0x10, 0xF0,
    /* A */ 0xF0, 0x90, 0xF0, 0x90, 0x90,
    /* B */ 0xE0, 0x90, 0xE0, 0x90, 0xE0,
    /* C */ 0xF0, 0x80, 0x80, 0x80, 0xF0,
    /* D */ 0xE0, 0x90, 0x90, 0x90, 0xE0,
    /* E */ 0xF0, 0x80, 0xF0, 0x80, 0xF0,
    /* F */ 0xF0, 0x80, 0xF0, 0x80, 0x80,
};

Chip8* cpu_init()
{
    Chip8* cpu;

    cpu = calloc(1, sizeof(Chip8));
    cpu->screen = calloc(SCREEN_WIDTH * SCREEN_HEIGHT, sizeof(uint8_t));
    cpu->rom = calloc(ROM_SIZE, sizeof(uint8_t));
    cpu->ram = calloc(RAM_SIZE, sizeof(uint8_t));
    cpu->stack = calloc(STACK_SIZE, sizeof(uint16_t));

    return cpu;
}

uint8_t cpu_load_rom(Chip8* cpu, const char* file_name)
{
    FILE* file;
    uint8_t* buffer;
    uint16_t file_size;

    file = fopen(file_name, "rb");
    if (file == NULL)
    {
        return 1;
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    buffer = malloc(file_size);
    fread(buffer, sizeof(uint8_t), file_size, file);
    fclose(file);

    if (file_size > ROM_SIZE)
    {
        free(buffer);
        return 2;
    }

    memcpy(cpu->rom, buffer, file_size);
    free(buffer);

    return 0;
}

void cpu_reset(Chip8* cpu)
{
    memset(cpu->V, 0, sizeof(cpu->V));
    cpu->I = 0;
    cpu->delay = 0;
    cpu->sound = 0;

    cpu->PC = PROGRAM_LOCATION;
    cpu->SP = 0;
    cpu->HALT = 0;

    memset(cpu->screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    memset(cpu->ram, 0, RAM_SIZE);
    memset(cpu->stack, 0, sizeof(uint16_t) * STACK_SIZE);

    memcpy(cpu->ram + FONT_LOCATION, font, sizeof(font));
    memcpy(cpu->ram + PROGRAM_LOCATION, cpu->rom, ROM_SIZE);
}

void cpu_tick(Chip8* cpu)
{
    uint16_t code = (cpu->ram[cpu->PC] << 8) + cpu->ram[cpu->PC + 1];
    uint8_t prefix = code >> 12;
    uint8_t x = (code >> 8) & 0xF;
    uint8_t y = (code >> 4) & 0xF;
    uint8_t n = code & 0xF;
    uint8_t kk = code & 0xFF;
    uint16_t nnn = code & 0xFFF;
    cpu->PC += 2;

    switch (code)
    {
        case 0x00E0: memset(cpu->screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT); break;
        case 0x00EE: cpu->SP--; cpu->PC = cpu->stack[cpu->SP]; break;
        default:
            uint8_t flag;
            switch (prefix)
            {
                case 0x1: cpu->PC = nnn; break;
                case 0x2: cpu->stack[cpu->SP] = cpu->PC; cpu->SP++; cpu->PC = nnn; break;
                case 0x3: if (cpu->V[x] == kk) cpu->PC += 2; break;
                case 0x4: if (cpu->V[x] != kk) cpu->PC += 2; break;
                case 0x5: if (cpu->V[x] == cpu->V[y]) cpu->PC += 2; break;
                case 0x6: cpu->V[x] = kk; break;
                case 0x7: cpu->V[x] += kk; break;
                case 0x8:
                    switch (n)
                    {
                        case 0x0: cpu->V[x] = cpu->V[y]; break;
                        case 0x1: cpu->V[x] |= cpu->V[y]; cpu->V[0xF] = 0; break;
                        case 0x2: cpu->V[x] &= cpu->V[y]; cpu->V[0xF] = 0; break;
                        case 0x3: cpu->V[x] ^= cpu->V[y]; cpu->V[0xF] = 0; break;
                        case 0x4: flag = cpu->V[x] + cpu->V[y] > 0xFF; cpu->V[x] += cpu->V[y]; cpu->V[0xF] = flag; break;
                        case 0x5: flag = cpu->V[x] >= cpu->V[y]; cpu->V[x] -= cpu->V[y]; cpu->V[0xF] = flag; break;
                        case 0x6: flag = cpu->V[x] & 1; cpu->V[x] = cpu->V[y] >> 1; cpu->V[0xF] = flag; break;
                        case 0x7: flag = cpu->V[y] >= cpu->V[x]; cpu->V[x] = cpu->V[y] - cpu->V[x]; cpu->V[0xF] = flag; break;
                        case 0xE: flag = cpu->V[x] >> 7; cpu->V[x] = cpu->V[y] << 1; cpu->V[0xF] = flag; break;
                        default: goto error;
                    }
                    break;
                case 0x9: if (cpu->V[x] != cpu->V[y]) cpu->PC += 2; break;
                case 0xA: cpu->I = nnn; break;
                case 0xB: cpu->PC = nnn + cpu->V[0x0]; break;
                case 0xC: cpu->V[x] = (rand() & 0xFF) & kk; break;
                case 0xD:
                    flag = 0;

                    uint8_t X = cpu->V[x] % SCREEN_WIDTH;
                    uint8_t Y = cpu->V[y] % SCREEN_HEIGHT;

                    for (uint8_t row = 0; row < n; ++row)
                    {
                        uint8_t byte = cpu->ram[cpu->I + row];
                        if ((Y + row) >= SCREEN_HEIGHT) break;
                        for (uint8_t column = 0; column < 8; ++column)
                        {
                            uint8_t bit = (byte & (0x80 >> column)) >> (7 - column);
                            uint16_t position = (Y + row) * SCREEN_WIDTH + (X + column);

                            if ((X + column) >= SCREEN_WIDTH) break;
                            if (bit && cpu->screen[position]) flag = 1;
                            cpu->screen[position] ^= bit;
                        }
                    }

                    cpu->V[0xF] = flag;
                    break;
                case 0xE:
                    switch (kk)
                    {
                        case 0x9E: if ( cpu->keyboard[cpu->V[x]]) { cpu->PC += 2; } break;
                        case 0xA1: if (!cpu->keyboard[cpu->V[x]]) { cpu->PC += 2; } break;
                        default: goto error;
                    }
                    break;
                case 0xF:
                    switch (kk)
                    {
                        case 0x0A:
                            flag = 0;

                            for (uint8_t key = 0; key < 0x10; key++)
                                if (cpu->keyboard[key])
                                    flag = key;

                            if (flag) cpu->V[x] = flag;
                            else      cpu->PC -= 2;

                            break;
                        case 0x07: cpu->V[x] = cpu->delay; break;
                        case 0x15: cpu->delay = cpu->V[x]; break;
                        case 0x18: cpu->sound = cpu->V[x]; break;
                        case 0x1E: cpu->I += cpu->V[x]; break;
                        case 0x29: cpu->I = FONT_LOCATION + cpu->V[x] * 5; break;
                        case 0x33:
                            uint8_t value = cpu->V[x];
                            cpu->ram[cpu->I + 2] = value % 10; value /= 10;
                            cpu->ram[cpu->I + 1] = value % 10; value /= 10;
                            cpu->ram[cpu->I] =     value % 10;
                            break;
                        case 0x55: for (int i = 0; i <= x; i++) { cpu->ram[cpu->I + i] = cpu->V[i]; } cpu->I += x + 1; break;
                        case 0x65: for (int i = 0; i <= x; i++) { cpu->V[i] = cpu->ram[cpu->I + i]; } cpu->I += x + 1; break;
                        default: goto error;
                    }
                    break;
                default:
error:              break;
            }
            break;
    }
}

void cpu_destroy(Chip8* cpu)
{
    FILE* file;
    file = fopen("dump.bin", "wb");
    fwrite(cpu->ram, 1, RAM_SIZE, file);
    fclose(file);

    free(cpu->stack);
    free(cpu->ram);
    free(cpu->rom);
    free(cpu->screen);
    free(cpu);
}
