/*
 * Support for the X11 XInput extension.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if SUPPORT_XINPUT

#error this code is incomplete. Do not use unless you are fixing it.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>
#include <X11/Xutil.h>

#include "manymouse.h"

/* 32 is good enough for now. */
#define MAX_MICE 32
typedef struct
{
    XDevice *device;
    int min_x;
    int min_y;
    int max_x;
    int max_y;
    char name[64];
} MouseStruct;

static MouseStruct mice[MAX_MICE];
static unsigned int available_mice = 0;

static Display *display = NULL;
static XExtensionVersion *extver = NULL;
static XDeviceInfo *device_list = NULL;
static int device_count = 0;

static void xinput_cleanup(void)
{
    int i;

    if (display != NULL)
    {
        for (i = 0; i < available_mice; i++)
        {
            if (mice[i].device)
                XCloseDevice(display, mice[i].device);
        } /* for */
    } /* if */

    if (extver != NULL)
    {
        XFree(extver);
        extver = NULL;
    } /* if */

    if (device_list != NULL)
    {
        XFreeDeviceList(device_list);
        device_list = NULL;
    } /* if */

    if (display != NULL)
    {
        XCloseDisplay(display);
        display = NULL;
    } /* if */

    memset(mice, '\0', sizeof (mice));
    available_mice = 0;
} /* xinput_cleanup */


/* Just in case this is compiled as a C++ module... */
static XID get_x11_any_class(const XAnyClassPtr anyclass)
{
#if defined(__cplusplus) || defined(c_plusplus)
    return anyclass->c_class;
#else
    return anyclass->class;
#endif
} /* get_x11_any_class */


static int init_mouse(MouseStruct *mouse, const XDeviceInfo *devinfo)
{
    int i;
    int has_axes = 0;
    int has_buttons = 0;
    XAnyClassPtr any = devinfo->inputclassinfo;

    for (i = 0; i < devinfo->num_classes; i++)
    {
        XID cls = get_x11_any_class(any);

        if (cls == KeyClass)
            return(0);  /* a keyboard? */

        else if (cls == ButtonClass)
        {
            const XButtonInfo *info = (const XButtonInfo *) any;
            if (info->num_buttons > 0)
                has_buttons = 1;
        } /* else if */

        else if (cls == ValuatorClass)
        {
            const XValuatorInfo *info = (const XValuatorInfo *) any;
            if (info->num_axes != 2)  /* joystick? */
                return 0;

            has_axes = 1;
            mouse->min_x = info->axes[0].min_value;
            mouse->max_x = info->axes[0].max_value;
            mouse->min_y = info->axes[1].min_value;
            mouse->max_y = info->axes[1].max_value;
        } /* else if */

        any = (XAnyClassPtr) ((char *) any + any->length);
    } /* for */

    if ((!has_axes) || (!has_buttons))
        return(0);  /* probably not a mouse. */

    mouse->device = XOpenDevice(display, devinfo->id);
    if (mouse->device == NULL)
        return(0);

    strncpy(mouse->name, devinfo->name, sizeof (mouse->name));
    mouse->name[sizeof (mouse->name) - 1] = '\0';
    return(1);
} /* init_mouse */


static int x11_xinput_init(void)
{
    int i;

    xinput_cleanup();  /* just in case... */

    display = XOpenDisplay(NULL);
    if (display == NULL)
        goto x11_xinput_init_failed;  /* no X server at all */

    extver = XGetExtensionVersion(display, INAME);
    if ((extver == NULL) || (extver == (XExtensionVersion *) NoSuchExtension))
        goto x11_xinput_init_failed;  /* no such extension */

    if (extver->present == XI_Absent)
        goto x11_xinput_init_failed;

    device_list = XListInputDevices(display, &device_count);
    if (device_list == NULL)
        goto x11_xinput_init_failed;

    for (i = 0; i < device_count; i++)
    {
        MouseStruct *mouse = &mice[available_mice];
        if (init_mouse(mouse, &device_list[i]))
            available_mice++;
    } /* for */

    /* just one? Maybe misconfigured X server where evdev could do better... */
/*
    if (available_mice <= 1)
        goto x11_xinput_init_failed;
*/

    return(available_mice);

x11_xinput_init_failed:
    xinput_cleanup();
    return(-1);
} /* x11_xinput_init */


static void x11_xinput_quit(void)
{
    xinput_cleanup();
} /* x11_xinput_quit */


static const char *x11_xinput_name(unsigned int index)
{
    if (index < available_mice)
        return(mice[index].name);
    return(NULL);
} /* x11_xinput_name */


static int x11_xinput_poll(ManyMouseEvent *event)
{
    return(0);  /* !!! FIXME */
} /* x11_xinput_poll */


ManyMouseDriver ManyMouseDriver_xinput =
{
    x11_xinput_init,
    x11_xinput_quit,
    x11_xinput_name,
    x11_xinput_poll
};

#endif /* SUPPORT_XINPUT blocker */

/* end of x11_xinput.c ... */

