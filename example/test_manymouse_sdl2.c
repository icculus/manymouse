/*
 * A test file for ManyMouse that visualizes input with the SDL2 library.
 *  Simple Directmedia Layer2 (SDL2) can be found at http://libsdl.org/
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon and Julian Winter.
 * 
 * Cross-compile for Windows on Linux with:
 * x86_64-w64-mingw32-gcc example/test_manymouse_sdl2.c manymouse.c windows_wminput.c x11_xinput2.c macosx_hidmanager.c macosx_hidutilities.c linux_evdev.c  -I./ -I"/usr/local/x86_64-w64-mingw32/include" -lSDL2main -lSDL2 -L/usr/local/x86_64-w64-mingw32/lib
 * 
 * Compile on linux with:
 * gcc example/test_manymouse_sdl2.c manymouse.c windows_wminput.c x11_xinput2.c macosx_hidmanager.c macosx_hidutilities.c linux_evdev.c  -I./ -lSDL2main -lSDL2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manymouse.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#define MAX_MICE 128
#define SCROLLWHEEL_DISPLAY_TICKS 100

const unsigned short int SCREEN_WIDTH = 600;
const unsigned short int SCREEN_HEIGHT = 480;

static int available_mice = 0;

typedef struct
{
    int connected;
    int x;
    int y;
    SDL_Color color;
    char name[64];
    Uint32 buttons;
    Uint32 scrolluptick;
    Uint32 scrolldowntick;
    Uint32 scrolllefttick;
    Uint32 scrollrighttick;
} Mouse;

static Mouse mice[MAX_MICE];

static void update_mice(int screen_w, int screen_h)
{
    ManyMouseEvent event;
    while (ManyMouse_PollEvent(&event))
    {
        Mouse *mouse;
        if (event.device >= (unsigned int)available_mice)
            continue;

        mouse = &mice[event.device];

        if (event.type == MANYMOUSE_EVENT_RELMOTION)
        {
            if (event.item == 0)
                mouse->x += event.value;
            else if (event.item == 1)
                mouse->y += event.value;
        }

        else if (event.type == MANYMOUSE_EVENT_ABSMOTION)
        {
            float val = (float)(event.value - event.minval);
            float maxval = (float)(event.maxval - event.minval);
            if (event.item == 0)
                mouse->x = (val / maxval) * screen_w;
            else if (event.item == 1)
                mouse->y = (val / maxval) * screen_h;
        }

        else if (event.type == MANYMOUSE_EVENT_BUTTON)
        {
            if (event.item < 32)
            {
                if (event.value)
                    mouse->buttons |= (1 << event.item);
                else
                    mouse->buttons &= ~(1 << event.item);
            }
        }

        else if (event.type == MANYMOUSE_EVENT_SCROLL)
        {
            if (event.item == 0)
            {
                if (event.value < 0)
                    mouse->scrolldowntick = SDL_GetTicks();
                else
                    mouse->scrolluptick = SDL_GetTicks();
            }
            else if (event.item == 1)
            {
                if (event.value < 0)
                    mouse->scrolllefttick = SDL_GetTicks();
                else
                    mouse->scrollrighttick = SDL_GetTicks();
            }
        }

        else if (event.type == MANYMOUSE_EVENT_DISCONNECT)
        {
            mice[event.device].connected = 0;
        }
    }
}

static void draw_mouse(SDL_Renderer *const renderer, int idx)
{
    Uint32 now = SDL_GetTicks();
    Mouse *mouse = &mice[idx];
    SDL_Rect r = {mouse->x, mouse->y, 10, 10};
    SDL_SetRenderDrawColor(renderer, mouse->color.r, mouse->color.g, mouse->color.b, 255);

    if (mouse->x < 0)
        mouse->x = 0;
    if (mouse->x >= SCREEN_WIDTH)
        mouse->x = SCREEN_WIDTH - 1;
    if (mouse->y < 0)
        mouse->y = 0;
    if (mouse->y >= SCREEN_HEIGHT)
        mouse->y = SCREEN_HEIGHT - 1;

    SDL_RenderFillRect(renderer, &r); /* draw a square for mouse position. */

    /* now draw some buttons... */
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < 32; i++)
    {
        if (mouse->buttons & (1 << i)) // pressed?
        {
            r.w = 20;
            r.x = i * r.w;
            r.h = 20;
            r.y = SCREEN_HEIGHT - ((idx + 1) * r.h);
            SDL_RenderFillRect(renderer, &r);
        }
    }

    /* draw scroll wheels... */
    #define DRAW_SCROLLWHEEL(var, item) \
        if (var > 0) \
        { \
            if ((now - var) > SCROLLWHEEL_DISPLAY_TICKS) \
                var = 0; \
            else \
            { \
                r.w = r.h = 20; \
                r.y = idx * r.h; \
                r.x = item * r.w; \
                SDL_RenderFillRect(renderer, &r); \
            } \
        }

    DRAW_SCROLLWHEEL(mouse->scrolluptick, 0);
    DRAW_SCROLLWHEEL(mouse->scrolldowntick, 1);
    DRAW_SCROLLWHEEL(mouse->scrolllefttick, 2);
    DRAW_SCROLLWHEEL(mouse->scrollrighttick, 3);

    #undef DRAW_SCROLLWHEEL    
}

static void initial_setup(int screen_w, int screen_h)
{
    int i;
    memset(mice, '\0', sizeof(mice));

    /* pick some random colors for each mouse. */
    for (i = 0; i < MAX_MICE; i++)
    {
        Mouse *mouse = &mice[i];
        mouse->x = screen_w / 2;
        mouse->y = screen_h / 2;
        mouse->color.r = (int)(255.0 * rand() / (RAND_MAX + 1.0));
        mouse->color.g = (int)(255.0 * rand() / (RAND_MAX + 1.0));
        mouse->color.b = (int)(255.0 * rand() / (RAND_MAX + 1.0));
    }
}

static void init_mice(void)
{
    int i;

    available_mice = ManyMouse_Init();

    if (available_mice < 0)
    {
        printf("Error initializing ManyMouse!\n");
        return;
    }

    printf("ManyMouse driver: %s\n", ManyMouse_DriverName());

    if (available_mice == 0)
    {
        printf("No mice detected!\n");
        return;
    }

    for (i = 0; i < available_mice; i++)
    {
        const char *name = ManyMouse_DeviceName(i);
        printf("#%d: %s\n", i, name);
    }

    if (available_mice > MAX_MICE)
    {
        printf("Clamping to first %d mice.\n", available_mice);
        available_mice = MAX_MICE;
    }

    for (i = 0; i < available_mice; i++)
    {
        const char *name = ManyMouse_DeviceName(i);
        strncpy(mice[i].name, name, sizeof(mice[i].name));
        mice[i].name[sizeof(mice[i].name) - 1] = '\0';
        mice[i].connected = 1;
    }
}

int main(int argc, char **argv)
{
    int must_quit = 0;
    int cursor = 0;

    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        fprintf(stderr, "SDL_Init() failed! %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "ManyMouse SDL2 test",
        100, 100,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        SDL_Quit();
        SDL_Log("App-error: Could not open the window! :D");
        return 1;
    }

    SDL_Renderer *Renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_PRESENTVSYNC |
            SDL_RENDERER_TARGETTEXTURE |
            SDL_RENDERER_ACCELERATED);

    if (!Renderer)
    {
        SDL_DestroyWindow(window);
        SDL_Quit();
        SDL_Log("App-error: Could not create a rendering-context!");
        return 1;
    }

    SDL_ShowCursor(cursor);

    initial_setup(SCREEN_WIDTH, SCREEN_HEIGHT);
    init_mice();

    while (!must_quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                must_quit = 1;
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                    must_quit = 1;
                else if (e.key.keysym.sym == SDLK_s)
                {
                    cursor = (cursor) ? 0 : 1;
                    SDL_ShowCursor(cursor);
                }
                else if (e.key.keysym.sym == SDLK_r)
                {
                    printf("\n\nRESCAN!\n\n");
                    ManyMouse_Quit();
                    init_mice();
                }
            }
        }
        update_mice(SCREEN_WIDTH, SCREEN_HEIGHT);

        // render mice
        SDL_SetRenderDrawColor(Renderer, 0, 0, 24, 255);
        SDL_RenderClear(Renderer);
        for (int i = 0; i < available_mice; i++)
        {
            if (mice[i].connected)
                draw_mouse(Renderer, i);
        }
        SDL_RenderPresent(Renderer);
    }
    ManyMouse_Quit();
    SDL_DestroyRenderer(Renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
