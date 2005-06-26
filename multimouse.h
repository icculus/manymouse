/* Please see LICENSE in the root of this source tree. */

#ifndef _INCLUDE_MULTIMOUSE_H_
#define _INCLUDE_MULTIMOUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    MULTIMOUSE_EVENT_ABSMOTION = 0,
    MULTIMOUSE_EVENT_RELMOTION,
    MULTIMOUSE_EVENT_BUTTON,
    MULTIMOUSE_EVENT_SCROLL,
    MULTIMOUSE_EVENT_DISCONNECT,
    MULTIMOUSE_EVENT_MAX
} MultiMouseEventType;

typedef struct
{
    MultiMouseEventType type;
    unsigned int device;
    unsigned int item;
    int value;
    int minval;
    int maxval;
} MultiMouseEvent;


/* internal use only. */
typedef struct
{
    int (*init)(void);
    void (*quit)(void);
    const char *(*name)(unsigned int index);
    int (*poll)(MultiMouseEvent *event);
} MultiMouseDriver;


int MultiMouse_Init(void);
void MultiMouse_Quit(void);
const char *MultiMouse_DeviceName(unsigned int index);
int MultiMouse_PollEvent(MultiMouseEvent *event);

#ifdef __cplusplus
}
#endif

#endif  /* !defined _INCLUDE_MULTIMOUSE_H_ */

/* end of multimouse.h ... */

