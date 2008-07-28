/*
 * A test file for ManyMouse that lists all seen mice.
 *
 * Please see the file LICENSE.txt in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manymouse.h"

int main(int argc, char **argv)
{
    int available_mice = ManyMouse_Init();
    if (available_mice <= 0)
    {
        printf("No mice detected!\n");
        return(1);
    }
    else
    {
        int i;
        for (i = 0; i < available_mice; i++)
            printf("#%d: %s\n", i, ManyMouse_DeviceName(i));
    }
    ManyMouse_Quit();
    return(0);
}

/* end of detect_mice.c ... */

