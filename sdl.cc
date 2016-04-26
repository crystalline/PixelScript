
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <math.h>

int screenW, screenH;
uint8_t pause = 0;
uint8_t quit = 0;
float fps = 50.0;

int posX = 0;
int posY = 0;

void clearScreen(uint32_t* pixels) {
    int i;
    int N = screenW*screenH;
    for (i=0; i<N; i++) {
        pixels[i] = 0x00000000;
    }
}

void updateScreen(uint32_t* pixels, float t) {
    
}

void handleMouseDown(SDL_MouseButtonEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleMouseUp(SDL_MouseButtonEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleMouseMove(SDL_MouseMotionEvent* event) {
    int x, y;    
    x = event->x;
    y = event->y;
}

void handleKeyDown(SDL_KeyboardEvent* event) {
    SDL_Keysym key = event->keysym;
    int Scode = key.scancode;
}

void handleKeyUp(SDL_KeyboardEvent* event) {
    SDL_Keysym key = event->keysym;
    int Scode = key.scancode;
}

void handleQuit() {
    quit = 1;
}

int main(int argc, char *argv[]) {

    screenW = 800;
    screenH = 600;

    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *screen = NULL;

    SDL_Init(SDL_INIT_VIDEO);

    win = SDL_CreateWindow("SDL V8", posX, posY, screenW, screenH, 0);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    screen = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                screenW, screenH);

    uint32_t* pixels = (uint32_t*) malloc(sizeof(uint32_t)*screenW*screenH);
    float t = 0;
    uint32_t beforeT, afterT;

    while (!quit) {

        beforeT = SDL_GetTicks();
        
        t++;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                handleQuit();
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                handleMouseDown(&e.button);
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                handleMouseUp(&e.button);
            } else  if (e.type == SDL_MOUSEMOTION) {
                handleMouseMove(&e.motion);
            } else if (e.type == SDL_KEYDOWN) {
                handleKeyDown(&e.key);
            } else if (e.type == SDL_KEYUP) {
                handleKeyUp(&e.key);
            }
        }
        
        if (!pause) {

            clearScreen(pixels);

            updateScreen(pixels, t);

            SDL_UpdateTexture(screen, NULL, pixels, screenW * sizeof (uint32_t));

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, screen, NULL, NULL);
            SDL_RenderPresent(renderer);

            afterT = SDL_GetTicks();

            if (afterT - beforeT < 1000.0/fps) {
                SDL_Delay(1000.0/fps - (afterT - beforeT));
                //printf("delay: %f\n", 1000.0/fps - (afterT - beforeT));
            }
        } else {
            SDL_Delay(20);
        }
    }

    SDL_DestroyTexture(screen);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();

    return 0;
}


