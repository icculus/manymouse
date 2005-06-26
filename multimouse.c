/* Please see LICENSE in the root of this source tree. */

#include "multimouse.h"


extern const MultiMouseDriver MultiMouseDriver_windows;
extern const MultiMouseDriver MultiMouseDriver_evdev;
extern const MultiMouseDriver MultiMouseDriver_mousedev;
extern const MultiMouseDriver MultiMouseDriver_macosx;
extern const MultiMouseDriver MultiMouseDriver_xinput;

static const MultiMouseDriver *mice_drivers[] =
{
    #if SUPPORT_XINPUT
    &MultiMouseDriver_xinput,
    #endif
    #ifdef WINDOWS
    &MultiMouseDriver_windows,
    #endif
    #ifdef __LINUX__
    &MultiMouseDriver_evdev,
    /*&MultiMouseDriver_mousedev,*/
    #endif
    #if ( (defined(__MACH__)) && (defined(__APPLE__)) )
    &MultiMouseDriver_macosx,
    #endif
    NULL
};


static MultiMouseDriver *driver = NULL;

int MultiMouse_Init(void)
{
    int i;

    if (driver != NULL)
        return(-1);

    for (i = 0; mice_drivers[i]; i++)
    {
        int mice = mice_drivers[i]->init();
        if (mice >= 0)
        {
            driver = mice_drivers[i];
            return(mice);
        } /* if */
    } /* for */

    return(-1);
} /* MultiMouse_Init */


void MultiMouse_Quit(void)
{
    if (driver != NULL)
        driver->quit();
    driver = NULL;
} /* MultiMouse_Quit */


int MultiMouse_PollEvent(MultiMouseEvent *event)
{
    if (driver != NULL)
        return(driver->poll(event));
    return(0);
} /* MultiMouse_PollEvent */

/* end of multimouse.c ... */

