/*
 * This is a more robust "pong" than mmpong.c ... it handles more than
 *  two people and puts paddles on all four sides of the screen, assuming
 *  you have that many mice plugged in.
 *
 * Player 1 is on the left, Player 2 on the right, 3 on the top, 4 on the
 *  bottom, and then 5 to n continue this pattern.
 *
 * This was written to show a robust use of the library with arbitrary
 *  numbers of players. mmpong.c is better if you want a small example
 *  to get you started on your own program.
 *
 * You need Simple Directmedia Layer (http://libsdl.org/) to use this code.
 *
 * Controls:
 *  Plus to add more balls to the game, minus to remove some. Escape to quit.
 *  Whatever mice you have will control the paddles.
 *
 * Written by Ryan C. Gordon (icculus@icculus.org).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"
#include "manymouse.h"

#ifdef _MSC_VER
#define inline __inline
#endif

/*
 * paddles and balls. Things. They're all basically rectangles that move.
 */
typedef struct
{
    Uint32 color;
    float x;  /* location is top left pixel. */
    float y;
    int w;  /* size is in pixels. */
    int h;
    float xvelocity;  /* velocity is in pixels per second. */
    float yvelocity;
    float delayUpdate;  /* seconds to delay before updating. */
    int exists;
    int blanked; /* already blanked this frame. */
} PongThing;

typedef struct
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
} RGB;

#define MAX_PADDLES 32
#define MAX_BALLS   32
#define MAX_UPDATERECTS 100

/* Paddle speed for keyboard input (in seconds to cross the screen). */
#define PADDLE_SPEED  1.0

/* default ball speeds (in seconds to cross the screen). */
#define BALL_VSPEED   5.0
#define BALL_HSPEED   1.7

/* time to wait between setting video mode and balls start moving. */
#define VIDEOINIT_DELAY 3.0f

typedef enum
{
    PADDLESIDE_LEFT=0,
    PADDLESIDE_RIGHT,
    PADDLESIDE_TOP,
    PADDLESIDE_BOTTOM,
    PADDLESIDE_MAX
} PaddleSide;

static PongThing paddles[MAX_PADDLES];
static PongThing balls[MAX_BALLS];
static int sideExists[PADDLESIDE_MAX];
static int sideScore[PADDLESIDE_MAX];
static SDL_Rect rects[MAX_UPDATERECTS];
static int rectCount = 0;
static SDL_Surface *screen = NULL;
static int available_mice = 0;
static Uint32 sdlvideoflags = SDL_FULLSCREEN;
static int sdlgrab = 1;
static int showfps = 0;
static int screenwidth = 640;
static int screenheight = 480;
static float screenWidthScale = 1.0f;
static float screenHeightScale = 1.0f;

static void renderOneThing(PongThing *thing, int blanking);

static int initVideo(void)
{
    int i;

    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        if (SDL_Init(SDL_INIT_VIDEO) == -1)
            return(0);
    } /* if */

    SDL_WM_GrabInput(sdlgrab ? SDL_GRAB_ON : SDL_GRAB_OFF);
    SDL_ShowCursor(0);
    SDL_WM_SetCaption("ManyMouse PONG!", "ManyMousePong");

    screen = SDL_SetVideoMode(screenwidth, screenheight, 0, sdlvideoflags);
    if (screen == NULL)
    {
        SDL_Quit();
        return(0);
    } /* if */

    /*
     * We consider 640x480 to be the "native" resolution of the game.
     *  Everything here scales to any arbitrary resolution, but relative
     *  mouse motion is presumably the same no matter what the resolution.
     *
     * Therefore, we modulate relative motion to make it match what it would
     *  we in 640x480...this way, mice don't move slower or faster at
     *  different resolutions...
     */

    screenWidthScale = ((float) screen->w) / 640.0f;
    screenHeightScale = ((float) screen->h) / 480.0f;

    SDL_FillRect(screen, NULL, 0);  /* blank surface */
    SDL_Flip(screen);  /* move it to the screen. */

    for (i = 0; i < MAX_PADDLES; i++)
        paddles[i].blanked = 1;

    for (i = 0; i < MAX_BALLS; i++)
        balls[i].blanked = 1;
    
    return(1);  /* we're good to go. */
} /* initVideo */


static inline PaddleSide getPaddleSide(int paddleidx)
{
    return((PaddleSide) (paddleidx % 4));
} /* getPaddleSide */


static void setSidesExists(void)
{
    int i;

    sideExists[PADDLESIDE_LEFT] = 0;
    sideExists[PADDLESIDE_RIGHT] = 0;
    sideExists[PADDLESIDE_TOP] = 0;
    sideExists[PADDLESIDE_BOTTOM] = 0;

    for (i = 0; i < available_mice; i++)
    {
        PaddleSide side = getPaddleSide(i);
        if (paddles[i].exists)
            sideExists[side] = 1;
    } /* for */
} /* setSidesExists */


static int initMice(void)
{
    available_mice = ManyMouse_Init();
    if (available_mice > MAX_PADDLES)
        available_mice = MAX_PADDLES;

    if (available_mice == 0)
        printf("No mice detected!\n");
    else
    {
        int i;
        for (i = 0; i < available_mice; i++)
        {
            const char *name = ManyMouse_DeviceName(i);
            paddles[i].exists = 1;
            printf("#%d: %s\n", i, name);
        } /* for */
    } /* else */

    setSidesExists();

    return(1);
} /* initMice */


static Uint32 standardizedColor(void)
{
    /*
     * How I got these numbers:
     *   These color choices are ripped directly from QuickBasic's
     *   COLOR command in text mode. So, (1) is dark blue, (2) is
     *   dark green, etc...Then I used the GIMP color tool
     *   (http://www.gimp.org/), to find the equivalents in RGB format.
     */
    static RGB colorArray[] =
    {
        #if 0  /* these don't stand out well, just use the bright ones. */
        { 0, 0, 0 },       /* black */
        { 0, 26, 196 },    /* dark blue */
        { 0, 153, 45 },    /* dark green */
        { 0, 144, 138 },   /* dark cyan */
        { 160, 0, 0 },     /* dark red */
        { 197, 0, 202 },   /* dark magenta */
        { 129, 90, 16 },   /* brown */
        { 200, 200, 200 }, /* gray */
        { 90, 90, 90 },    /* dark gray */
        #endif
        { 0, 200, 255 },   /* bright blue */
        { 0, 90, 38 },     /* bright green */
        { 91, 110, 255 },  /* bright cyan */
        { 255, 0, 0 },     /* bright red */
        { 255, 0, 255 },   /* bright pink */
        { 255, 255, 0 },   /* bright yellow */
        { 255, 255, 255 }, /* bright white */
    };

    static int nextcolor = 0;
    const RGB *color = &colorArray[nextcolor++];
    nextcolor %= (sizeof (colorArray) / sizeof (colorArray[0]));
    return(SDL_MapRGB(screen->format, color->r, color->g, color->b));
} /* standardizedColor */


#if 0
static inline Uint32 randomizedColor(void)
{
    Uint8 r = (Uint8) (255.0f*rand()/(RAND_MAX+1.0f));
    Uint8 g = (Uint8) (255.0f*rand()/(RAND_MAX+1.0f));
    Uint8 b = (Uint8) (255.0f*rand()/(RAND_MAX+1.0f));
    return(SDL_MapRGB(screen->format, r, g, b));
} /* randomizedColor */
#endif


static inline void resetBallVelocity(PongThing *ball)
{
    /* divided by 3.0 would == 3 seconds from one side of screen to other. */
    float origx = ball->xvelocity < 0.0f ? -1.0f : 1.0f;
    float origy = ball->yvelocity < 0.0f ? -1.0f : 1.0f;
    ball->xvelocity = (((float) screen->w) / BALL_HSPEED) * origx;
    ball->yvelocity = (((float) screen->h) / BALL_VSPEED) * origy;
} /* resetBallVelocity */


static void initThings(void)
{
    int i = 0;
    int ball_width = (int) (screen->w * 0.017);
    int ball_height = ball_width; /* eh. */
    int vert_paddle_width = (int) (screen->w * 0.02);
    int vert_paddle_height = (int) (screen->h * 0.10);
    int centerx = ((screen->w - ball_width) / 2);
    int centery = ((screen->h - ball_height) / 2);

    srand(time(NULL));

    memset(sideExists, '\0', sizeof (sideExists));
    memset(sideScore, '\0', sizeof (sideScore));
    memset(paddles, '\0', sizeof (paddles));
    memset(balls, '\0', sizeof (balls));

    for (i = 0; i < MAX_PADDLES; i++)
    {
        PongThing *thing = &paddles[i];
        PaddleSide side = getPaddleSide(i);
        thing->color = standardizedColor();
        thing->blanked = 1;

        if ((side == PADDLESIDE_LEFT) || (side == PADDLESIDE_RIGHT))
        {
            thing->w = vert_paddle_width;
            thing->h = vert_paddle_height;
            thing->x = ((i % 2) ? screen->w - vert_paddle_width : 0);
            thing->y = (screen->h - vert_paddle_height) / 2;
        } /* if */

        else  /* top or bottom */
        {
            thing->h = vert_paddle_width;
            thing->w = vert_paddle_height;
            thing->x = (screen->w - vert_paddle_height) / 2;
            thing->y = ((i % 2) ? screen->h - vert_paddle_width : 0);
        } /* else */
    } /* for */

    for (i = 0; i < MAX_BALLS; i++)
    {
        PongThing *thing = &balls[i];
        thing->color = standardizedColor();
        thing->w = ball_width;
        thing->h = ball_height;
        thing->x = centerx;
        thing->y = centery;
        thing->blanked = 1;
        thing->xvelocity = 1.0f;
        thing->yvelocity = 1.0f;
        resetBallVelocity(thing);
    } /* for */

    /*
     * Just put one ball into play to start, and don't kick it off
     *  for three seconds, so going to fullscreen mode doesn't
     *  cause a score before the monitor catches up.
     */
    balls[0].exists = 1;
    balls[0].delayUpdate = VIDEOINIT_DELAY;
} /* initThings */



static void updateMice(int screen_w, int screen_h)
{
    ManyMouseEvent event;
    while (ManyMouse_PollEvent(&event))
    {
        PongThing *paddle = NULL;
        PaddleSide side = 0;

        if (event.device >= (unsigned int) available_mice)
            continue;

        paddle = &paddles[event.device];
        side = getPaddleSide(event.device);

        if (event.type == MANYMOUSE_EVENT_RELMOTION)
        {
            if (event.item == 0)
            {
                if ((side == PADDLESIDE_TOP) || (side == PADDLESIDE_BOTTOM))
                {
                    renderOneThing(paddle, 1); /* blank in framebuffer. */
                    paddle->x += ((float) event.value) * screenWidthScale;
                } /* if */
            } /* if */

            else if (event.item == 1)
            {
                if ((side == PADDLESIDE_LEFT) || (side == PADDLESIDE_RIGHT))
                {
                    renderOneThing(paddle, 1); /* blank in framebuffer. */
                    paddle->y += ((float) event.value) * screenHeightScale;
                } /* if */
            } /* else if */
        } /* if */

        else if (event.type == MANYMOUSE_EVENT_ABSMOTION)
        {
            if (event.item == 0)
            {
                if ((side == PADDLESIDE_TOP) || (side == PADDLESIDE_BOTTOM))
                {
                    float val = (float) (event.value - event.minval);
                    float maxval = (float) (event.maxval - event.minval);
                    renderOneThing(paddle, 1); /* blank in framebuffer. */
                    paddle->x = (float) ((val / maxval) * screen_w);
                } /* if */
            } /* if */

            else if (event.item == 1)
            {
                if ((side == PADDLESIDE_LEFT) || (side == PADDLESIDE_RIGHT))
                {
                    float val = (float) (event.value - event.minval);
                    float maxval = (float) (event.maxval - event.minval);
                    renderOneThing(paddle, 1); /* blank in framebuffer. */
                    paddle->y = (float) ((val / maxval) * screen_h);
                } /* if */
            } /* else if */
        } /* else if */

        else if (event.type == MANYMOUSE_EVENT_BUTTON)
        {
            if (event.value == 1)  /* pressed */
                paddle->color = standardizedColor();
        } /* else if */

        else if (event.type == MANYMOUSE_EVENT_DISCONNECT)
        {
            renderOneThing(paddle, 1); /* blank in framebuffer. */
            paddle->exists = 0;
            setSidesExists();
        } /* else if */
    } /* while */
} /* updateMice */


static int updateInput(void)
{
    SDL_Event event;

    updateMice(screen->w, screen->h);

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:  /* window was closed, etc. */
                return(0);

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    return(0);  /* quit. */


                else if (event.key.keysym.sym == SDLK_KP_PLUS)
                {
                    /* put a new ball into play */
                    int i;
                    for (i = 0; i < MAX_BALLS; i++)
                    {
                        PongThing *ball = &balls[i];
                        if (ball->exists)
                            continue;

                        /* !!! FIXME: subroutine this. */
                        ball->x = ((screen->w - ball->w) / 2);
                        ball->y = ((screen->h - ball->h) / 2);
                        ball->color = standardizedColor();
                        ball->delayUpdate = 0.25f;
                        ball->exists = 1;
                        ball->blanked = 1;
                        ball->xvelocity = ball->yvelocity = 1.0f;
                        resetBallVelocity(ball);
                        break;
                    } /* for */
                } /* else if */

                else if (event.key.keysym.sym == SDLK_KP_MINUS)
                {
                    /* remove a ball from play */
                    int i;
                    for (i = MAX_BALLS - 1; i >= 0; i--)
                    {
                        PongThing *ball = &balls[i];
                        if (!ball->exists)
                            continue;

                        renderOneThing(ball, 1);
                        ball->exists = 0;
                        break;
                    } /* for */
                } /* else if */

                else if (event.key.keysym.sym == SDLK_g)
                {
                    if (event.key.keysym.mod & KMOD_CTRL)
                    {
                        sdlgrab = sdlgrab ? 0 : 1;
                        SDL_WM_GrabInput(sdlgrab ? SDL_GRAB_ON : SDL_GRAB_OFF);
                    } /* if */
                } /* else if */

                else if (event.key.keysym.sym == SDLK_RETURN)
                {
                    if (event.key.keysym.mod & KMOD_ALT)
                    {
                        int i;

                        if (sdlvideoflags & SDL_FULLSCREEN)
                            sdlvideoflags &= ~SDL_FULLSCREEN;
                        else
                            sdlvideoflags |= SDL_FULLSCREEN;

                        if (!initVideo())
                        {
                            printf("window/fullscreen toggle failed!\n");
                            return(0);  /* quit */
                        } /* if */

                        /*
                         * Render all balls then freeze them so the monitor
                         *  has time to catch up and players can get oriented.
                         */
                        for (i = 0; i < MAX_BALLS; i++)
                        {
                            PongThing *ball = &balls[i];
                            if (ball->exists)
                            {
                                resetBallVelocity(ball);
                                ball->delayUpdate = VIDEOINIT_DELAY;
                            } /* if */
                        } /* for */
                    } /* if */
                } /* else if */
                break;
        } /* switch */
    } /* while */

    return(1);  /* no quit events this time. */
} /* updateInput */


static void renderOneThing(PongThing *thing, int blanking)
{
    SDL_Rect *rect;

    if ((!thing->exists) || (blanking && thing->blanked))
        return;

    if (rectCount < MAX_UPDATERECTS)
        rect = &rects[rectCount++];
    else
        rect = &rects[0];  /* oh well. */

    /* !!! FIXME: need clipping. */

    rect->x = thing->x;
    rect->y = thing->y;
    rect->w = thing->w;
    rect->h = thing->h;

    SDL_FillRect(screen, rect, blanking ? 0x00000000 : thing->color);
    thing->blanked = blanking;

    /* note that this isn't on the screen until we call SDL_UpdateRects(). */

} /* renderOneThing */


static void renderThings(void)
{
    int i;

    for (i = 0; i < available_mice; i++)
        renderOneThing(&paddles[i], 0);

    for (i = 0; i < MAX_BALLS; i++)
        renderOneThing(&balls[i], 0);

    if (rectCount < MAX_UPDATERECTS)
        SDL_UpdateRects(screen, rectCount, rects);
    else
    {
        printf("Developer warning! overflowed updaterects!\n");
        SDL_Flip(screen);  /* just do the whole screen. */
    } /* else */

    rectCount = 0;
} /* renderThings */


static void triggerScore(PongThing *ball, PaddleSide scoreAgainst)
{
    #define INCR_SCORE(side) if (scoreAgainst != side) { sideScore[side]++; }
    INCR_SCORE(PADDLESIDE_LEFT);
    INCR_SCORE(PADDLESIDE_RIGHT);
    INCR_SCORE(PADDLESIDE_TOP);
    INCR_SCORE(PADDLESIDE_BOTTOM);
    #undef INCR_SCORE

    #define PRINT_SCORE(side) { \
        if (sideExists[PADDLESIDE_##side]) \
            printf("  %s side: %d.\n", #side, sideScore[PADDLESIDE_##side]); \
    }
    printf("SCORE!\n");
    PRINT_SCORE(LEFT);
    PRINT_SCORE(RIGHT);
    PRINT_SCORE(TOP);
    PRINT_SCORE(BOTTOM);
    printf("\n");
    #undef PRINT_SCORE

    /* center it again. */
    ball->x = ((screen->w - ball->w) / 2);
    ball->y = ((screen->h - ball->h) / 2);

    /* pick a new color for the next ball. */
    ball->color = standardizedColor();

    /* wait two seconds before ball is back in play. */
    ball->delayUpdate = 2.0f;

    /* point next ball at opposing side. */
    if ((scoreAgainst==PADDLESIDE_LEFT) || (scoreAgainst==PADDLESIDE_RIGHT))
        ball->xvelocity *= -1.0f;
    else
        ball->yvelocity *= -1.0f;

    resetBallVelocity(ball);
} /* triggerScore */


static void updatePaddle(PongThing *thing, float fractionalTime)
{
    if (!thing->exists)
        return;

    renderOneThing(thing, 1); /* blank existing thing in framebuffer. */

    /* this is for keyboard input, mostly...mice don't move here... */
    thing->x += thing->xvelocity * fractionalTime;
    thing->y += thing->yvelocity * fractionalTime;

    /* collided with left wall. */
    if (thing->x < 0.0f)
    {
        thing->x = 0.0f;
    } /* if */

    /* collided with right wall. */
    else if (thing->x > ((float) screen->w) - ((float) thing->w))
    {
        thing->x = ((float) screen->w) - ((float) thing->w);
    } /* if */

    /* collided with top wall. */
    if (thing->y < 0.0f)
    {
        thing->y = 0.0f;
    } /* if */

    /* collided with bottom wall. */
    else if (thing->y > ((float) screen->h) - ((float) thing->h))
    {
        thing->y = ((float) screen->h) - ((float) thing->h);
    } /* if */
} /* updatePaddle */


static inline int inRect(int x, int y, int bx1, int by1, int bx2, int by2)
{
    /* bounding box must be normalized. */

    if ((x < bx1) || (x > bx2))
        return(0);

    if ((y < by1) || (y > by2))
        return(0);

    return(1);
} /* inRect */


static inline int intersection(int px1, int py1, int px2, int py2,
                               int bx1, int by1, int bx2, int by2)
{
    /* if any corner of a rect is in the other, it's a collision. */
    return((inRect(px1, py1, bx1, by1, bx2, by2)) ||
           (inRect(px2, py1, bx1, by1, bx2, by2)) ||
           (inRect(px1, py2, bx1, by1, bx2, by2)) ||
           (inRect(px2, py2, bx1, by1, bx2, by2)) ||
           (inRect(bx1, by1, px1, py1, px2, py2)) ||
           (inRect(bx2, by1, px1, py1, px2, py2)) ||
           (inRect(bx1, by2, px1, py1, px2, py2)) ||
           (inRect(bx2, by2, px1, py1, px2, py2)));
} /* intersection */


/* returns non-zero if bounced, zero if score against existing team. */
static inline int scoreOrBounce(PongThing *ball, PaddleSide side)
{
    if (sideExists[side])
    {
        triggerScore(ball, side);
        return(0);
    } /* if */

    return(1);
} /* scoreOrBounce */


static void updateBall(PongThing *ball, float fractionalTime)
{
    /* !!! FIXME: be nice if bounces randomized direction */
    /* !!! FIXME: be nice if bounces slightly accelerate ball. */

    int i;
    float xorig, yorig;
    int bx1, bx2, by1, by2;
    int bounced = 0;

    if (!ball->exists)
        return;

    ball->delayUpdate -= fractionalTime;
    if (ball->delayUpdate > 0.0f)
        return;

    renderOneThing(ball, 1); /* blank existing ball in framebuffer. */

    xorig = ball->x;
    yorig = ball->y;

    ball->x += ball->xvelocity * fractionalTime;
    ball->y += ball->yvelocity * fractionalTime;

    if (xorig < ball->x)
    {
        bx1 = xorig;
        bx2 = ball->x + ball->w;
    } /* if */
    else
    {
        bx1 = ball->x;
        bx2 = xorig + ball->w;
    } /* else */

    if (yorig < ball->y)
    {
        by1 = yorig;
        by2 = ball->y + ball->h;
    } /* if */
    else
    {
        by1 = ball->y;
        by2 = yorig + ball->h;
    } /* else */

    /* check paddle collisions. */
    for (i = 0; i < available_mice; i++)
    {
        int px1, px2, py1, py2;
        PongThing *paddle = &paddles[i];
        if (!paddle->exists)
            continue;

        /* !!! FIXME: should round to nearest, not truncate! */
        /* cast to int only once. */
        px1 = (int) paddle->x;
        px2 = px1 + paddle->w;
        py1 = (int) paddle->y;
        py2 = py1 + paddle->h;

        /*
         * The trick here is to not care where the ball is currently, but
         *  rather whether it would overlap or pass through a given paddle
         *  on this frame.
         *
         * We go for cheap, not accurate, erring on the side of the player
         *  (the bigger the lag spike, the more likely they are to trigger
         *  a collision, even when there shouldn't be one, but in Pong,
         *  there'd have to be a BIG lag spike to screw this up).
         *
         * We construct a bounding rectangle of the entire surface area
         *  the ball could have touched when going from point A to point B
         *  on this frame, and then see if the paddle's rectangle touches
         *  it. If so, it's a collision, even though the paddle might have
         *  scraped a corner of the bounding box where the ball never
         *  actually touched. When you're getting hundreds of frames per
         *  second however, the likelihood of this ever failing, let alone
         *  failing noticably, is basically null.
         *
         * My devbox currently renders this game at over 15,000 fps.
         */
        if (intersection(px1, py1, px2, py2, bx1, by1, bx2, by2))
        {
            /* It's a hit, bounce the ball. */
            /* Push it to the edge of the paddle and reverse velocity. */
            PaddleSide side = getPaddleSide(i);
            if (side == PADDLESIDE_LEFT)
            {
                ball->x = (px2 + ball->w) + 1;
                ball->xvelocity *= -1.0f;
            } /* else if */
            else if (side == PADDLESIDE_RIGHT)
            {
                ball->x = (px1 - ball->w) - 1;
                ball->xvelocity *= -1.0f;
            } /* if */
            else if (side == PADDLESIDE_TOP)
            {
                ball->y = (py2 + ball->h) + 1;
                ball->yvelocity *= -1.0f;
            } /* else if */
            else if (side == PADDLESIDE_BOTTOM)
            {
                ball->y = (py1 - ball->h) - 1;
                ball->yvelocity *= -1.0f;
            } /* else if */

            bounced = 1;
            break; /* detected a collision; stop checking paddles. */
        } /* if */
    } /* for */

    /* no collisions with paddles this frame? See about wall collisions... */
    /* Multiply velocity by -1.1 so it reverses and gets faster. */
    if (i == available_mice)
    {
        /*
         * If we hit a wall, we need to see if there's at least one paddle
         *  guarding that side. If so, it's a score. If not, just bounce off
         *  the wall.
         */

        /* collided with left wall. */
        if (ball->x < 0.0f)
        {
            if ((bounced = scoreOrBounce(ball, PADDLESIDE_LEFT)) != 0)
            {
                ball->x = 0.0f;
                ball->xvelocity *= -1.0f;
            } /* else */
        } /* if */

        /* collided with right wall. */
        else if (ball->x > ((float) screen->w) - ((float) ball->w))
        {
            if ((bounced = scoreOrBounce(ball, PADDLESIDE_RIGHT)) != 0)
            {
                ball->x = ((float) screen->w) - ((float) ball->w);
                ball->xvelocity *= -1.0f;
            } /* else */
        } /* if */

        /* collided with top wall. */
        if (ball->y < 0.0f)
        {
            if ((bounced = scoreOrBounce(ball, PADDLESIDE_TOP)) != 0)
            {
                ball->y = 0.0f;
                ball->yvelocity *= -1.0f;
            } /* else */
        } /* if */

        /* collided with bottom wall. */
        else if (ball->y > ((float) screen->h) - ((float) ball->h))
        {
            if ((bounced = scoreOrBounce(ball, PADDLESIDE_BOTTOM)) != 0)
            {
                ball->y = ((float) screen->h) - ((float) ball->h);
                ball->yvelocity *= -1.0f;
            } /* else */
        } /* if */
    } /* if */

    if (bounced)
    {
        /* increase velocities from 0% to 5% */
        ball->xvelocity *= 1.0f + ((10.0f*rand()/(RAND_MAX+1.0f)) / 200.0f);
        ball->yvelocity *= 1.0f + ((10.0f*rand()/(RAND_MAX+1.0f)) / 200.0f);
    } /* if */
} /* updateBall */


static void updateThings(void)
{
    int i;
    static int firstRun = 1;
    static Uint32 lastTicks = 0;
    Uint32 ticks = SDL_GetTicks();
    float fractionalTime;

    if (firstRun)  /* since setting fullscreen can take several seconds... */
    {
        lastTicks = ticks;
        firstRun = 0;
    } /* if */

    fractionalTime = ((ticks - lastTicks) / 1000.0f);

    for (i = 0; i < available_mice; i++)
        updatePaddle(&paddles[i], fractionalTime);

    for (i = 0; i < MAX_BALLS; i++)
        updateBall(&balls[i], fractionalTime);

    lastTicks = ticks;
} /* updateThings */


static int processCmdLines(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        if (strcmp(arg, "--windowed") == 0)
            sdlvideoflags &= ~SDL_FULLSCREEN;
        else if (strcmp(arg, "--fullscreen") == 0)
            sdlvideoflags |= SDL_FULLSCREEN;
        else if (strcmp(arg, "--grabinput") == 0)
            sdlgrab = 1;
        else if (strcmp(arg, "--nograbinput") == 0)
            sdlgrab = 0;
        else if (strcmp(arg, "--showfps") == 0)
            showfps = 1;
        else if (strcmp(arg, "--noshowfps") == 0)
            showfps = 0;

        #define SCREENRESOPT(w,h) \
        else if (strcmp(arg, "--" #w "x" #h) == 0) { \
            screenwidth = w; \
            screenheight = h; \
        }
        SCREENRESOPT(320, 240)
        SCREENRESOPT(512, 384)
        SCREENRESOPT(640, 480)
        SCREENRESOPT(800, 500)
        SCREENRESOPT(800, 600)
        SCREENRESOPT(1024, 640)
        SCREENRESOPT(1024, 768)
        SCREENRESOPT(1152, 768)
        SCREENRESOPT(1152, 864)
        SCREENRESOPT(1280, 800)
        SCREENRESOPT(1280, 854)
        SCREENRESOPT(1280, 960)
        SCREENRESOPT(1280, 1024)
        SCREENRESOPT(1600, 1024)
        SCREENRESOPT(1600, 1200)
        SCREENRESOPT(1680, 1050)
        SCREENRESOPT(1920, 1200)
        #undef SCREENRESOPT

        else
        {
            printf("Unknown command line '%s'\n", arg);
            return(0);
        } /* else */
    } /* for */

    return(1);
} /* processCmdLines */


static void updateFPS(int reinit)
{
    /* quick and dirty frame counter. */
    static Uint32 fpsticks = 0;
    static Uint32 frames = 0;

    if (reinit)
    {
        frames = 0;
        fpsticks = SDL_GetTicks() + 5000;
    } /* if */
    else
    {
        frames++;
        if (SDL_GetTicks() >= fpsticks)
        {
            if (showfps)
                printf("fps == %d\n", (int) (frames / 5));
            fpsticks += 5000;
            frames = 0;
        } /* if */
    } /* else */
} /* updateFPS */


int main(int argc, char **argv)
{
    if (!processCmdLines(argc, argv))
        return(42);

    if (!initVideo())
        return(42);

    initThings();

    if (!initMice())
    {
        SDL_Quit();
        return(42);
    } /* if */

    updateFPS(1);
    while (updateInput())  /* go until quit event of some sort. */
    {
        updateThings();  /* move things to new locations. */
        renderThings();  /* draw moved things to screen. */
        updateFPS(0);
    } /* while */

    ManyMouse_Quit();
    SDL_Quit();  /* clean up video mode, etc. */
    return(0);
} /* main */

/* end of manymousepong.c ... */

