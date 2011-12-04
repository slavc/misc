/* Displays a nice plasma effect. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <err.h>

#include <SDL/SDL.h>

#define NELEMS(a) (sizeof(a)/sizeof((a)[0]))

#define SCREEN_W 640
#define SCREEN_H 480
#define SCREEN_B 8
#define SCREEN_F SDL_SWSURFACE|SDL_HWPALETTE

void plasma(SDL_Surface *screen);
void gen_cos_tab(int nelems, int *tab);

int
main(int argc, char **argv) {
    SDL_Surface *screen;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
        errx(1, "SDL_Init: %s", SDL_GetError());
    atexit(SDL_Quit);

    if ((screen = SDL_SetVideoMode(SCREEN_W, SCREEN_H, SCREEN_B, SCREEN_F)) == NULL)
        errx(1, "SDL_SetVideoMode: %s", SDL_GetError());

    plasma(screen);

    exit(0);
}

void
plasma(SDL_Surface *screen) {
    int i, j;
    SDL_Color palette[256];
    SDL_Event event;
    Uint32 last_ticks, ticks;
    int costab[256];

    bzero(palette, sizeof palette);
    for (i = 0; i < NELEMS(palette); ++i) {
        //palette[i].g = 255 - i;
        palette[i].b = i;
    }

    gen_cos_tab(NELEMS(costab), costab);

    SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, palette, 0, NELEMS(palette));

    last_ticks = SDL_GetTicks();
    for ( ;; ) {
        ticks = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    exit(0);
                    break;
                }
            }
        }

        if (!SDL_MUSTLOCK(screen) || SDL_LockSurface(screen) == 0) {
            for (i = 0; i < screen->h; ++i) {
                for (j = 0; j < screen->w; ++j) {
                    /*
                    ((unsigned char *) screen->pixels)[i * screen->w + j] =
                        costab[i & 255] +
                        costab[(i * 3) & 255] +
                        costab[(j) & 255] +
                        costab[(j * 13) & 255];
                    */
                    //((unsigned char *) screen->pixels)[i * screen->w + j] = (int) (i * (256.0 / SCREEN_H) * j * (256.0 / SCREEN_W));
                    /*
                    ((unsigned char *) screen->pixels)[i * screen->w + j] =
                       costab[(int) (3 * j * (SCREEN_W / sizeof costab) + 7 * i * (SCREEN_H / sizeof costab)) & 255] +
                       costab[(int) (4 * j * (SCREEN_W / sizeof costab) + 9 * i * (SCREEN_H / sizeof costab)) & 255] +
                       costab[(int) (5 * i * (SCREEN_W / sizeof costab) + 0.09 * ticks * (SCREEN_H / sizeof costab)) & 255] +
                       costab[(int) (8 * j * (SCREEN_W / sizeof costab) + 0.9 * ticks * (SCREEN_H / sizeof costab)) & 255];
                    */
                    //((unsigned char *) screen->pixels)[i * screen->w + j] = 128 * (cos((M_PI*2/SCREEN_H) * i + (0.0003 * ticks)) * cos((M_PI*2/SCREEN_W) * j + (0.0001 * ticks))) + 128;
                    //((unsigned char *) screen->pixels)[i * screen->w + j] = 255 * sin((M_PI/SCREEN_H) * i + (0.0003 * ticks)) * sin((M_PI/SCREEN_W) * j + (0.0001 * ticks));
                        //((unsigned char *) screen->pixels)[i * screen->w + j] = 256 * sin((M_PI_2/screen->w) * (i + ticks));
                    /*
                    if (i * j & 1)
                        ((unsigned char *) screen->pixels)[i * screen->w + j] = 128 * (cos((M_PI*2/SCREEN_H) * i + (0.0003 * ticks)) * cos((M_PI*2/SCREEN_W) * j + (0.0001 * ticks))) + 128;
                    else
                        ((unsigned char *) screen->pixels)[i * screen->w + j] = 128 * (cos((M_PI/SCREEN_H) * i + (0.0007 * ticks)) * cos((M_PI*2/SCREEN_W) * j + (0.0031 * ticks))) + 128;
                    */
                }
            }
            SDL_UnlockSurface(screen);
        }

        SDL_Delay(1);

        SDL_Flip(screen);
    }
}

void
gen_cos_tab(int nelems, int *tab) {
    int i;
    double t, step;

    t = 0.0;
    step = (M_PI) / (double) nelems;

    for (i = 0; i < nelems; ++i) {
        tab[i] = (nelems / (4 * 2)) * cos(t) + (nelems / (4 * 2));
        t += step;
    }

    printf("costab:\n");
    for (i = 0; i < nelems; ++i) {
        if (i != 0 && i % 4 == 0)
            printf("\n");
        printf("%i ", tab[i]);
    }
    printf("\n");
}
