#include <stdlib.h>
#include <stdio.h>

#include <SDL.h>

static SDL_DisplayMode modes[5];

static int mode_count = 0, current_mode = 0;

void print_info(SDL_Window *window, SDL_Renderer *renderer)
{
    int w, h;
    SDL_DisplayMode mode;

    SDL_GetWindowSize(window, &w, &h);
    SDL_Log("window size: %i x %i\n", w, h);
    SDL_GetRendererOutputSize(renderer, &w, &h);
    SDL_Log("renderer size: %i x %i\n", w, h);

    SDL_GetCurrentDisplayMode(0, &mode);
    SDL_Log("display mode: %i x %i @ %i bpp (%s)",
            mode.w, mode.h,
            SDL_BITSPERPIXEL(mode.format),
            SDL_GetPixelFormatName(mode.format));
}

void change_mode(SDL_Window *window)
{
    current_mode++;
    if (current_mode == mode_count) {
        current_mode = 0;
    }

    SDL_SetWindowDisplayMode(window, &modes[current_mode]);
}

void draw_rects(SDL_Renderer *renderer, int x, int y)
{
    // R
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect r = {x, y, 64, 64};
    SDL_RenderFillRect(renderer, &r);

    // G
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect g = {x + 64, y, 64, 64};
    SDL_RenderFillRect(renderer, &g);

    // B
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    SDL_Rect b = {x + 128, y, 64, 64};
    SDL_RenderFillRect(renderer, &b);
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    SDL_Window *window;
    SDL_Renderer *renderer;
    int done = 0, x = 0, w, h;

    // mandatory at least on switch, else gfx is not properly closed
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        SDL_Log("SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    // create a window (OpenGL always enabled)
    // available switch SDL2 video modes :
    // 1920 x 1080 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888) (docked only)
    // 1280 x 720 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    // 960 x 540 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    // 800 x 600 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    // 640 x 480 @ 32 bpp (SDL_PIXELFORMAT_RGBA8888)
    window = SDL_CreateWindow("sdl2_gles2", 0, 0, 640, 480, SDL_WINDOW_FULLSCREEN);
    if (!window) {
        SDL_Log("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // create a renderer (OpenGL ES2)
    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // print some info about display/window/renderer, for "educational purpose"
    print_info(window, renderer);

    // list available display modes, for "educational purpose"
    mode_count = SDL_GetNumDisplayModes(0);
    for (int i = 0; i < mode_count; i++) {
        SDL_GetDisplayMode(0, i, &modes[i]);
    }

    // open CONTROLLER_PLAYER_1 and CONTROLLER_PLAYER_2
    // when railed, both joycons are mapped to joystick #0,
    // else joycons are individually mapped to joystick #0, joystick #1, ...
    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L45
    for (int i = 0; i < 2; i++) {
        if (SDL_JoystickOpen(i) == NULL) {
            SDL_Log("SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }
    }

    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_JOYAXISMOTION:
                    SDL_Log("Joystick %d axis %d value: %d\n",
                            event.jaxis.which,
                            event.jaxis.axis, event.jaxis.value);
                    break;

                case SDL_JOYBUTTONDOWN:
                    SDL_Log("Joystick %d button %d down\n",
                            event.jbutton.which, event.jbutton.button);
                    // seek for joystick #0 down (A)
                    // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L52
                    if (event.jbutton.which == 0 && event.jbutton.button == 0) {
                        change_mode(window);
                        print_info(window, renderer);
                    }
                    // seek for joystick #0 down (B)
                    if (event.jbutton.which == 0 && event.jbutton.button == 1) {
                        done = 1;
                    }
                    break;

                default:
                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Fill renderer bounds
        SDL_SetRenderDrawColor(renderer, 111, 111, 111, 255);
        SDL_GetRendererOutputSize(renderer, &w, &h);
        SDL_Rect f = {0, 0, w, h};
        SDL_RenderFillRect(renderer, &f);

        draw_rects(renderer, x, 0);
        draw_rects(renderer, x, h - 64);

        SDL_RenderPresent(renderer);

        x++;
        if (x > w - 192) {
            x = 0;
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
