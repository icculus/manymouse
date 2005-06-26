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
    MULTIMOUSE_EVENT_MAX
} MultiMouseEventType;

typedef union
{
    MultiMouseEventType type;
    struct
    {
        MultiMouseEventType type;
        int device;
        int x;
        int y;
    } motion;

    struct
    {
        MultiMouseEventType type;
        int device;
        int number;
        int state;
    } button;
} MultiMouseEvent;


typedef struct
{
    int (*init)(void);
    void (*quit)(void);
    int (*poll)(MultiMouseEvent *event);
} MultiMouseDriver;


int MultiMouse_Init(void);
void MultiMouse_Quit(void);
int MultiMouse_PollEvent(MultiMouseEvent *event);

#ifdef __cplusplus
}
#endif

#endif  /* !defined _INCLUDE_MULTIMOUSE_H_ */

/* end of multimouse.h ... */

