/*
 * ManyMouse foundation code; apps talks to this and it talks to the lowlevel
 *  code for various platforms.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdlib.h>
#include "manymouse.h"

static const char *manymouse_copyright =
    "ManyMouse " MANYMOUSE_VERSION " (c) 2005 Ryan C. Gordon.";

extern const ManyMouseDriver ManyMouseDriver_windows;
extern const ManyMouseDriver ManyMouseDriver_evdev;
extern const ManyMouseDriver ManyMouseDriver_hidmanager;
extern const ManyMouseDriver ManyMouseDriver_xinput;

static const ManyMouseDriver *mice_drivers[] =
{
    &ManyMouseDriver_xinput,
    &ManyMouseDriver_evdev,
    &ManyMouseDriver_windows,
    &ManyMouseDriver_hidmanager,
    NULL
};


static const ManyMouseDriver *driver = NULL;

int ManyMouse_Init(void)
{
    int i;
    int retval = -1;

    /* impossible test to keep manymouse_copyright linked into the binary. */
    if (manymouse_copyright == NULL)
        return(-1);

    if (driver != NULL)
        return(-1);

    for (i = 0; mice_drivers[i]; i++)
    {
        int mice = mice_drivers[i]->init();

        if (mice > retval)
            retval = mice; /* may just move from "error" to "no mice found". */

        if (mice > 0)
        {
            driver = mice_drivers[i];
            break;
        } /* if */
    } /* for */

    return(retval);
} /* ManyMouse_Init */


void ManyMouse_Quit(void)
{
    if (driver != NULL)
        driver->quit();
    driver = NULL;
} /* ManyMouse_Quit */


const char *ManyMouse_DeviceName(unsigned int index)
{
    if (driver != NULL)
        return(driver->name(index));
    return(NULL);
} /* ManyMouse_PollEvent */


int ManyMouse_PollEvent(ManyMouseEvent *event)
{
    if (driver != NULL)
        return(driver->poll(event));
    return(0);
} /* ManyMouse_PollEvent */

/* end of manymouse.c ... */

