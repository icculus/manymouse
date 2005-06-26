/*
 * A test file for MultiMouse that dumps input data to stdout.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multimouse.h"

int main(int argc, char **argv)
{
    MultiMouseEvent event;
    int available_mice = MultiMouse_Init();
    if (available_mice <= 0)
    {
        printf("No mice detected!\n");
        return(1);
    }
    else
    {
        int i;
        for (i = 0; i < available_mice; i++)
            printf("#%d: %s\n", i, MultiMouse_DeviceName(i));
    }
    printf("\n");

    printf("Use your mice, CTRL-C to exit.\n");
    while (1)
    {
        while (MultiMouse_PollEvent(&event))
        {
            if (event.type == MULTIMOUSE_EVENT_RELMOTION)
            {
                printf("Mouse #%u relative motion %s %d\n", event.device,
                        event.item == 0 ? "X" : "Y", event.value);
            }

            else if (event.type == MULTIMOUSE_EVENT_ABSMOTION)
            {
                printf("Mouse #%u absolute motion %s %d\n", event.device,
                        event.item == 0 ? "X" : "Y", event.value);
            }

            else if (event.type == MULTIMOUSE_EVENT_BUTTON)
            {
                printf("Mouse #%u button %u %s\n", event.device,
                        event.item, event.value ? "down" : "up");
            }

            else if (event.type == MULTIMOUSE_EVENT_SCROLL)
            {
                const char *wheel;
                const char *direction;
                if (event.item == 0)
                {
                    wheel = "vertical";
                    direction = ((event.value > 0) ? "up" : "down");
                }
                else
                {
                    wheel = "horizontal";
                    direction = ((event.value > 0) ? "right" : "left");
                }
                printf("Mouse #%u wheel %s %s\n", event.device,
                        wheel, direction);
            }

            else if (event.type == MULTIMOUSE_EVENT_DISCONNECT)
                printf("Mouse #%u disconnect\n", event.device);

            else
            {
                printf("Mouse #%u unhandled event type %d\n", event.device,
                        event.type);
            }
        }
    }

    MultiMouse_Quit();
    return(0);
}

/* end of test_multimouse_stdio.c ... */

