#include <stdio.h>
#include <SDL.h>

#include "chip8.h"
#include "tinyfiledialogs.h"

#define WINDOW_TITLE            "Chip-8"
#define WINDOW_WIDTH            SCREEN_WIDTH  * 20
#define WINDOW_HEIGHT           SCREEN_HEIGHT * 20
#define FOREGROUND_COLOR        0xEABE6C
#define PAUSED_FOREGROUND_COLOR 0xABABAB
#define BACKGROUND_COLOR        0x240A34
#define PAUSED_BACKGROUND_COLOR 0x1F1F1F

int main(int argc, char* argv[])
{
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Event event;

    Chip8* cpu;

    uint8_t running;
    uint8_t paused;
    uint8_t fps;
    uint32_t timerTicksA;
    uint32_t timerTicksB;
    uint32_t timerDelta;

    uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

    /* Init SDL */
    if (SDL_Init( SDL_INIT_VIDEO ) < 0)
    {
        tinyfd_messageBox("SDL_Init", SDL_GetError(), "ok", "error", 1);
        exit(1);
    }

    window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        tinyfd_messageBox("SDL_CreateWindow", SDL_GetError(), "ok", "error", 1);
        exit(1);
    }

    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        tinyfd_messageBox("SDL_CreateRenderer", SDL_GetError(), "ok", "error", 1);
        exit(1);
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (texture == NULL)
    {
        tinyfd_messageBox("SDL_CreateTexture", SDL_GetError(), "ok", "error", 1);
        exit(1);
    }

    /* Init CPU */
    cpu = cpu_init();

    /* Enter main loop */
    tinyfd_messageBox("Information", "Welcome!\n\nThe CPU is currently paused.\nFPS: Unlimited\n\nControl:\nU - Toggle FPS (120/Unlimited)\nI - Reset CPU\nO - Open ROM\nP - Toggle pause", "ok", "info", 1);

    running = 1;
    paused = 1;
    fps = 0;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type) 
            {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    uint8_t state = event.key.type == SDL_KEYDOWN;
                    switch (event.key.keysym.scancode)
                    {
                        case SDL_SCANCODE_U:
                            if (state)
                            {
                                switch (fps)
                                {
                                    case 0:  fps = 120; break;
                                    default: fps = 0; break;
                                }
                            }
                            break;
                        case SDL_SCANCODE_I:
                            if (state)
                            {
                                cpu_reset(cpu);
                            }
                            break;
                        case SDL_SCANCODE_P:
                            if (state)
                            {
                                paused = !paused;
                            }
                            break;
                        case SDL_SCANCODE_O:
                            if (state)
                            {
                                const char* lFilterPatterns[3] = {"*.bin", "*.ch8", "*.c8"};
                                char* file_name = tinyfd_openFileDialog("Open ROM", "./", 3, lFilterPatterns, NULL, 0);
                                if (file_name)
                                {
                                    switch (cpu_load_rom(cpu, file_name))
                                    {
                                        case 1:
                                            tinyfd_messageBox("cpu_load_rom", "File not found", "ok", "error", 1);
                                            break;
                                        case 2:
                                            tinyfd_messageBox("cpu_load_rom", "File is larger than 3584 bytes.", "ok", "error", 1);
                                            break;
                                        default:
                                            cpu_reset(cpu);
                                            paused = 0;
                                            break;
                                    }
                                }
                            }
                            break;
                        case SDL_SCANCODE_1: cpu->keyboard[0x1] = state; break;
                        case SDL_SCANCODE_2: cpu->keyboard[0x2] = state; break;
                        case SDL_SCANCODE_3: cpu->keyboard[0x3] = state; break;
                        case SDL_SCANCODE_4: cpu->keyboard[0xC] = state; break;
                        case SDL_SCANCODE_Q: cpu->keyboard[0x4] = state; break;
                        case SDL_SCANCODE_W: cpu->keyboard[0x5] = state; break;
                        case SDL_SCANCODE_E: cpu->keyboard[0x6] = state; break;
                        case SDL_SCANCODE_R: cpu->keyboard[0xD] = state; break;
                        case SDL_SCANCODE_A: cpu->keyboard[0x7] = state; break;
                        case SDL_SCANCODE_S: cpu->keyboard[0x8] = state; break;
                        case SDL_SCANCODE_D: cpu->keyboard[0x9] = state; break;
                        case SDL_SCANCODE_F: cpu->keyboard[0xE] = state; break;
                        case SDL_SCANCODE_Z: cpu->keyboard[0xA] = state; break;
                        case SDL_SCANCODE_X: cpu->keyboard[0x0] = state; break;
                        case SDL_SCANCODE_C: cpu->keyboard[0xB] = state; break;
                        case SDL_SCANCODE_V: cpu->keyboard[0xF] = state; break;
                        default: break;
                    }
                    break;

                case SDL_QUIT:
                    running = 0;
                    break;
            }
        }

        if (!paused)
        {
            timerTicksA = SDL_GetTicks();
            timerDelta = timerTicksA - timerTicksB;

            if (timerDelta > 1000 / 60)
            {
                timerTicksB = timerTicksA;
                if (cpu->delay) cpu->delay--;
                if (cpu->sound) cpu->sound--;
            }

            if (fps == 0 || timerDelta > 1000 / fps)
            {
                cpu_tick(cpu);
            }
        }

        for (int y = 0; y < SCREEN_HEIGHT; y++)
        for (int x = 0; x < SCREEN_WIDTH; x++)
        {
            int pos = y * SCREEN_WIDTH + x;
            pixels[pos] = cpu->screen[pos] ? (paused ? PAUSED_FOREGROUND_COLOR : FOREGROUND_COLOR) :
                                             (paused ? PAUSED_BACKGROUND_COLOR : BACKGROUND_COLOR);
        }

        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * 4);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    /* Destroy everything */
    cpu_destroy(cpu);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
