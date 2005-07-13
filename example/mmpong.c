
/* MMPong - a Pong example using ManyMouse

Damon Ragno - with parts copied from the SDL manymouse example by Ryan Gordon

version 0.0.1

TODO: 
-clean up the code, so it actually looks like something that would be an example
-add increasing speed
-add an actual scoreboard and not stdout
	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "SDL.h"
#include "manymouse.h"
#define MAX_MICE 32
#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

int available_mice = 0;

typedef struct
{
    int connected;
    int x;
    int y;
    SDL_Color color;
    char name[64];
} Mouse;

typedef struct
{
	int x;
	int y;
	int speed;
	int dir_h;
	int dir_w;
} Ball;

static Mouse mice[MAX_MICE];
static Ball puck;
static int score1 = 0 , score2=0;

void score(int who)
{
    const char *fmt = "MMPong, an example of ManyMouse usage: Score: %d - %d";
    size_t len = sizeof (fmt) + 64;
    char *buf = (char *) alloca(len);
	Ball *ball = &puck;

	if(who == 1){
		score1++;
		printf("player 1 has %d goals\n", score1);
	} else{
		score2++;
		printf("player 2 has %d goals\n", score2);
	}

    snprintf(buf, len, fmt, score1, score2);
	SDL_WM_SetCaption(buf, "MMPong");
	ball->x = 320 ;
	ball->y = rand() % 480 ;
	ball->speed = 1;
	if(rand() % 2 == 0) ball->dir_h = UP; else ball->dir_h = DOWN;
	if(rand() % 2 == 0) ball->dir_w = LEFT; else ball->dir_w = RIGHT;
}

static void initial_setup(int screen_w, int screen_h)
{
    int i;
    memset(mice, '\0', sizeof (mice));
	Ball *ball = &puck;
	
    /* pick some random colors for each mouse. Sets position of both paddles */
    for (i = 0; i < MAX_MICE; i++)
    {
        Mouse *mouse = &mice[i];
        if((i % 2) == 0){
			mouse->x = 2;
			mouse->y = screen_h / 2;
		} else {
			mouse->x = screen_w -17 ;
			mouse->y = screen_h / 2;
		}
		
        mouse->color.r = (int) (255.0*rand()/(RAND_MAX+1.0));
        mouse->color.g = (int) (255.0*rand()/(RAND_MAX+1.0));
        mouse->color.b = (int) (255.0*rand()/(RAND_MAX+1.0));
    }
	ball->x = screen_w / 2;
	ball->y = rand() % screen_h;
	ball->speed = 1;
	ball->dir_h = UP;
	ball->dir_w = LEFT;
	
}


static void init_mice(void)
{
    available_mice = ManyMouse_Init();
    if (available_mice > MAX_MICE)
        available_mice = MAX_MICE;
	
    if (available_mice == 0)
        printf("No mice detected!\n");
    else
    {
        int i;
        for (i = 0; i < available_mice; i++)
        {
            const char *name = ManyMouse_DeviceName(i);
            strncpy(mice[i].name, name, sizeof (mice[i].name));
            mice[i].name[sizeof (mice[i].name) - 1] = '\0';
            mice[i].connected = 1;
            printf("#%d: %s\n", i, mice[i].name);
        }
    }
}


static void update_mice(int screen_w, int screen_h)
{
    ManyMouseEvent event;
    while (ManyMouse_PollEvent(&event))
    {
        Mouse *mouse;
        if (event.device >= (unsigned int) available_mice)
            continue;
		
        mouse = &mice[event.device];
		
        if (event.type == MANYMOUSE_EVENT_RELMOTION)
        {
			/* ignore side to side mouse movement */
		    if (event.item == 1) mouse->y += event.value;
			if (mouse->y < 0) mouse->y = 0;
			if (mouse->y > 410) mouse->y = 410;
		}
		
        else if (event.type == MANYMOUSE_EVENT_ABSMOTION)
            ; /* !!! FIXME: do something with this. */
		
        else if (event.type == MANYMOUSE_EVENT_BUTTON)
            ; /* !!! FIXME: do something with this. */
		
        else if (event.type == MANYMOUSE_EVENT_SCROLL)
            ; /* !!! FIXME: do something with this. */
		
        else if (event.type == MANYMOUSE_EVENT_DISCONNECT)
            mice[event.device].connected = 0;
    }
}

static void update_ball(int screen_w, int screen_h)
{
    static Uint32 lastupdate = 0;
    Uint32 now = SDL_GetTicks();
	Ball *ball = &puck;
	int i = 0;

    if (now - lastupdate < 10)
        return;

    lastupdate += 10;

	if(ball->y < 0) ball->dir_h = DOWN;
	else if(ball->y > screen_h - 10) ball->dir_h = UP;

	if(ball->x < 15){
		int scored = 1;
		for (i = 0; i < available_mice; i += 2) {
			if((ball->y >= mice[i].y) && (ball->y < mice[i].y + 70)) {
				scored = 0;
				break;
			}
		}
		if (scored)
			score(2);
		else
			ball->dir_w = RIGHT;
	}
	else if(ball->x > (screen_w - 25)) {
		int scored = 1;
		for (i = 1; i < available_mice; i += 2) {
			if((ball->y >= mice[i].y) && (ball->y < mice[i].y + 70)) {
				scored = 0;
				break;
			}
		}
		if (scored)
			score(1);
		else
			ball->dir_w = LEFT;
	}

	if(ball->dir_w == LEFT && ball->dir_h == UP){
		ball->x -= 3 ;
		ball->y -= 3 ;
	}
	if(ball->dir_w == RIGHT && ball->dir_h == UP){
		ball->x += 3;
		ball->y -= 3 ;
	}
	if(ball->dir_w == LEFT && ball->dir_h == DOWN){
		ball->x -= 3;
		ball->y += 3;
	}
	if(ball->dir_w == RIGHT && ball->dir_h == DOWN){
		ball->x +=3;
		ball->y += 3;
	}
}


static void draw_mouse(SDL_Surface *screen, int idx)
{
	SDL_Rect r = { mice[idx].x, mice[idx].y, 15, 70 };
	Uint32 color;
	
	if (!mice[idx].connected)
		return;
	
	if (mice[idx].x < 0) mice[idx].x = 0;
	if (mice[idx].x >= screen->w) mice[idx].x = screen->w-1;
	if (mice[idx].y < 0) mice[idx].y = 0;
	if (mice[idx].y >= screen->h) mice[idx].y = screen->h-1;
	
	color = SDL_MapRGB(screen->format, mice[idx].color.r,
					   mice[idx].color.g, mice[idx].color.b);
	SDL_FillRect(screen, &r, color);
}


static void draw_screen(SDL_Surface *screen)
{
	int i;
	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 50, 0, 0));
	for (i = 0; i < available_mice; i++)
		draw_mouse(screen, i);
	Ball *ball = &puck;
	SDL_Rect b = {ball->x, ball->y, 10, 10};
	Uint32 color = SDL_MapRGB(screen->format, 255, 255, 43);
	SDL_FillRect(screen, &b, color);
	SDL_Flip(screen);
}

int main(int argc, char *argv[])
{
	Uint32 initflags = SDL_INIT_VIDEO;  /* See documentation for details */
	SDL_Surface *screen;
	Uint8  video_bpp = 0;
	Uint32 videoflags = (SDL_HWSURFACE || SDL_DOUBLEBUF);
	int    done;
	int cursor = 0;
	SDL_Event event;
	/* Initialize the SDL library */
	if ( SDL_Init(initflags) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
				SDL_GetError());
		exit(1);
	}
	
	/* Set 640x480 video mode */
	screen=SDL_SetVideoMode(640,480, video_bpp, videoflags);
	if (screen == NULL) {
		fprintf(stderr, "Couldn't set 640x480x%d video mode: %s\n",
				video_bpp, SDL_GetError());
		SDL_Quit();
		exit(2);
	}
	SDL_WM_SetCaption("MMPong, an example of ManyMouse usage", "MMPong");
	SDL_ShowCursor(cursor);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	initial_setup(screen->w, screen->h);
	init_mice();
	
	
	done = 0;
	while ( !done ) {
		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					/* Any keypress quits the app... */
				case SDL_QUIT:
					done = 1;
					break;
				default:
					break;
			}
		}
		update_mice(screen->w, screen->h);
		update_ball(screen->w, screen->h);
		draw_screen(screen);
		
	}
	
	/* Clean up the SDL library */
	ManyMouse_Quit();
	SDL_Quit();
	return(0);
}
