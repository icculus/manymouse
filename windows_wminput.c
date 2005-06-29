/*
 * Support for Windows via the WM_INPUT message.
 *
 * Please see the file LICENSE in the source's root directory.
 *
 *  This file written by Ryan C. Gordon.
 */

#if (defined(_WIN32) || defined(__CYGWIN__))

/* WinUser.h won't include rawinput stuff without this... */
#if (_WIN32_WINNT < 0x0501)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include <windows.h>
#include <malloc.h>  /* needed for alloca(). */

/* Cygwin's headers don't have WM_INPUT right now... */
#ifndef WM_INPUT
#define WM_INPUT 0x00FF
#endif

#include "manymouse.h"

/* that should be enough, knock on wood. */
#define MAX_MICE 32

/*
 * Just trying to avoid malloc() here...we statically allocate a buffer
 *  for events and treat it as a ring buffer.
 */
/* !!! FIXME: tweak this? */
#define MAX_EVENTS 1024
static ManyMouseEvent input_events[MAX_EVENTS];
static volatile int input_events_read = 0;
static volatile int input_events_write = 0;
static int available_mice = 0;
static int did_api_lookup = 0;
static HWND raw_hwnd = NULL;
static const char *class_name = "ManyMouseRawInputCatcher";
static const char *win_name = "ManyMouseRawInputMsgWindow";
static ATOM class_atom = 0;

typedef struct
{
    HANDLE handle;
    char name[256];
} MouseStruct;
static MouseStruct mice[MAX_MICE];


/*
 * The RawInput APIs only exist in Windows XP and later, so you want this
 *  to fail gracefully on earlier systems instead of refusing to start the
 *  process due to missing symbols. To this end, we do a symbol lookup on
 *  User32.dll to get the entry points.
 */
static UINT (WINAPI *pGetRawInputDeviceList)(
    PRAWINPUTDEVICELIST pRawInputDeviceList,
    PUINT puiNumDevices,
    UINT cbSize
);
/* !!! FIXME: use unicode version */
static UINT (WINAPI *pGetRawInputDeviceInfoA)(
    HANDLE hDevice,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize
);
static BOOL (WINAPI *pRegisterRawInputDevices)(
    PCRAWINPUTDEVICE pRawInputDevices,
    UINT uiNumDevices,
    UINT cbSize
);
static LRESULT (WINAPI *pDefRawInputProc)(
    PRAWINPUT *paRawInput,
    INT nInput,
    UINT cbSizeHeader
);
static UINT (WINAPI *pGetRawInputBuffer)(
    PRAWINPUT pData,
    PUINT pcbSize,
    UINT cbSizeHeader
);
static UINT (WINAPI *pGetRawInputData)(
    HRAWINPUT hRawInput,
    UINT uiCommand,
    LPVOID pData,
    PUINT pcbSize,
    UINT cbSizeHeader
);

static int symlookup(HMODULE dll, void **addr, const char *sym)
{
    *addr = GetProcAddress(dll, sym);
    if (*addr == NULL)
    {
        FreeLibrary(dll);
        return(0);
    } /* if */

    return(1);
} /* symlookup */

static int find_api_symbols(void)
{
    HMODULE dll;

    if (did_api_lookup)
        return(1);

    dll = LoadLibrary("user32.dll");
    if (dll == NULL)
        return(0);

    #define LOOKUP(x) { if (!symlookup(dll, (void **) &p##x, #x)) return(0); }
    LOOKUP(GetRawInputDeviceInfoA);
    LOOKUP(RegisterRawInputDevices);
    LOOKUP(GetRawInputDeviceList);
    LOOKUP(DefRawInputProc);
    LOOKUP(GetRawInputBuffer);
    LOOKUP(GetRawInputData);
    #undef LOOKUP

    /* !!! FIXME: store user32dll and free it on quit? */

    did_api_lookup = 1;
    return(1);
} /* find_api_symbols */


static int accept_device(const RAWINPUTDEVICELIST *dev)
{
    const char rdp_ident[] = "\\??\\Root#RDP_MOU#0000#";
    char *buf = NULL;
    UINT ct = 0;

    if (dev->dwType != RIM_TYPEMOUSE)
        return(0);  /* keyboard or some other fruity thing. */

    if (pGetRawInputDeviceInfoA(dev->hDevice, RIDI_DEVICENAME, NULL, &ct) < 0)
        return(0);

    /* ct == is chars, not bytes, but we used the ASCII version. */
    buf = (char *) alloca(ct);
    if (buf == NULL)
        return(0);

    if (pGetRawInputDeviceInfoA(dev->hDevice, RIDI_DEVICENAME, buf, &ct) < 0)
        return(0);

    /*
     * Apparently there's a fake "RDP" device...I guess this is
     *  "Remote Desktop Protocol" for controlling the system pointer
     *  remotely via Windows Remote Desktop, but that's just a guess.
     * At any rate, we don't want that device, so skip it if detected.
     *
     * Idea for this found here:
     *   http://link.mywwwserver.com/~jstookey/arcade/rawmouse/raw_mouse.c
     */

    /* avoiding memcmp here so we don't get a C runtime dependency... */
    if (ct >= sizeof (rdp_ident) - 1)
    {
        int i;
        for (i = 0; i < sizeof (rdp_ident) - 1; i++)
        {
            if (buf[i] != rdp_ident[i])
                break;
        } /* for */

        if (i == sizeof (rdp_ident) - 1)
            return(0);  /* this is an RDP thing. Skip this device. */
    } /* if */

    return(1);  /* we want this device. */
} /* reject_device */


/* !!! FIXME: this code sucks. */
static void get_device_product_name(char *name, size_t namesize,
                                     const RAWINPUTDEVICELIST *dev)
{
    const char regkeyroot[] = "System\\CurrentControlSet\\Enum\\";
    const char default_device_name[] = "Unidentified input device";
    DWORD outbufsize = namesize;
    DWORD regtype = REG_SZ;
    char *buf = NULL;
    char *ptr = NULL;
    char *keyname = NULL;
    UINT i = 0;
    UINT ct = 0;
    LONG rc = 0;
    HKEY hkey;

    *name = '\0';  /* really insane default. */
    if (sizeof (default_device_name) >= namesize)
        return;

    /* in case we can't stumble upon something better... */
    CopyMemory(name, default_device_name, sizeof (default_device_name));

    if (pGetRawInputDeviceInfoA(dev->hDevice, RIDI_DEVICENAME, NULL, &ct) < 0)
        return;

    /* ct == is chars, not bytes, but we used the ASCII version. */
    buf = (char *) alloca(ct+1);
    keyname = (char *) alloca(ct + sizeof (regkeyroot));
    if ((buf == NULL) || (keyname == NULL))
        return;

    if (pGetRawInputDeviceInfoA(dev->hDevice, RIDI_DEVICENAME, buf, &ct) < 0)
        return;

    /*
     * This string tap dancing gets us a registry keyname in this form:
     *   SYSTEM\CurrentControlSet\Enum\BUSTYPE\DEVICECLASS\DEVICEID
     * (those are my best-guess for the actual elements, but the format
     *  appears to be sound.)
     */
    ct -= 4;
    buf += 4;  /* skip the "\\??\\" on the front of the string. */
    for (i = 0, ptr = buf; i < ct; i++, ptr++)  /* convert '#' to '\\' ... */
    {
        if (*ptr == '#')
            *ptr = '\\';
        else if (*ptr == '{')  /* hit the GUID part of the string. */
            break;
    } /* for */

    *ptr = '\0';
    CopyMemory(keyname, regkeyroot, sizeof (regkeyroot) - 1);
    CopyMemory(keyname + (sizeof (regkeyroot) - 1), buf, i + 1);
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyname, 0, KEY_READ, &hkey);
    if (rc != ERROR_SUCCESS)
        return;

    rc = RegQueryValueEx(hkey, "DeviceDesc", NULL, &regtype, name, &outbufsize);
    RegCloseKey(hkey);
    if (rc != ERROR_SUCCESS)
    {
        /* msdn says failure may mangle the buffer, so default it again. */
        CopyMemory(name, default_device_name, sizeof (default_device_name));
        return;
    } /* if */
    name[namesize-1] = '\0';  /* just in case. */
} /* get_device_product_name */


/* !!! FIXME: move this closer to the init entry point... */
static void init_mouse(const RAWINPUTDEVICELIST *dev)
{
    MouseStruct *mouse = &mice[available_mice];

    if (!accept_device(dev))
        return;

    ZeroMemory(mouse, sizeof (MouseStruct));
    get_device_product_name(mouse->name, sizeof (mouse->name), dev);
    mouse->handle = dev->hDevice;
    available_mice++;  /* we're good. */
} /* init_mouse */


static void queue_event(const ManyMouseEvent *event)
{
    input_events_write = ((input_events_write + 1) % MAX_EVENTS);

    /* Ring buffer full? Lose oldest event. */
    if (input_events_write == input_events_read)
    {
        /* !!! FIXME: we need to not lose mouse buttons here. */
        input_events_read = ((input_events_read + 1) % MAX_EVENTS);
    } /* if */

    /* copy the event info. We'll process it in ManyMouse_PollEvent(). */
    CopyMemory(&input_events[input_events_write], event, sizeof (ManyMouseEvent));
} /* queue_event */


static void queue_from_rawinput(const RAWINPUT *raw)
{
    int i;
    const RAWINPUTHEADER *header = &raw->header;
    const RAWMOUSE *mouse = &raw->data.mouse;
    ManyMouseEvent event;

    if (raw->header.dwType != RIM_TYPEMOUSE)
        return;

    for (i = 0; i < available_mice; i++)  /* find the device for event. */
    {
        if (mice[i].handle == header->hDevice)
            break;
    } /* for */

    if (i == available_mice)
        return;  /* not found?! */

    /*
     * RAWINPUT packs a bunch of events into one, so we split it up into
     *  a bunch of ManyMouseEvents here and store them in an internal queue.
     *  Then ManyMouse_PollEvent() just shuffles items off that queue
     *  without any complicated processing.
     */

    event.device = i;

    if (mouse->usFlags & MOUSE_MOVE_ABSOLUTE)
    {
        /* !!! FIXME: How do we get the min and max values for absmotion? */
        event.type = MANYMOUSE_EVENT_ABSMOTION;
        event.item = 0;
        event.value = mouse->lLastX;
        queue_event(&event);
        event.item = 1;
        event.value = mouse->lLastY;
        queue_event(&event);
    } /* if */

    else /*if (mouse->usFlags & MOUSE_MOVE_RELATIVE)*/
    {
        event.type = MANYMOUSE_EVENT_RELMOTION;
        if (mouse->lLastX != 0)
        {
            event.item = 0;
            event.value = mouse->lLastX;
            queue_event(&event);
        } /* if */

        if (mouse->lLastY != 0)
        {
            event.item = 1;
            event.value = mouse->lLastY;
            queue_event(&event);
        } /* if */
    } /* else if */

    event.type = MANYMOUSE_EVENT_BUTTON;

    #define QUEUE_BUTTON(x) { \
        if (mouse->usButtonFlags & RI_MOUSE_BUTTON_##x##_DOWN) { \
            event.item = x-1; \
            event.value = 1; \
            queue_event(&event); \
        } \
        if (mouse->usButtonFlags & RI_MOUSE_BUTTON_##x##_UP) { \
            event.item = x-1; \
            event.value = 0; \
            queue_event(&event); \
        } \
    }

    QUEUE_BUTTON(1);
    QUEUE_BUTTON(2);
    QUEUE_BUTTON(3);
    QUEUE_BUTTON(4);
    QUEUE_BUTTON(5);

    #undef QUEUE_BUTTON

    if (mouse->usButtonFlags & RI_MOUSE_WHEEL)
    {
        if (mouse->usButtonData != 0)  /* !!! FIXME: can this ever be zero? */
        {
            event.type = MANYMOUSE_EVENT_SCROLL;
            event.item = 0;
            event.value = (mouse->usButtonData > 0) ? 1 : -1;
            queue_event(&event);
        } /* if */
    } /* if */
} /* queue_from_rawinput */


static void wminput_handler(WPARAM wParam, LPARAM lParam)
{
    UINT dwSize = 0;
    LPBYTE lpb;

    pGetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &dwSize,
                      sizeof (RAWINPUTHEADER));

    if (dwSize < sizeof (RAWINPUT))
        return;  /* unexpected packet? */

    lpb = (LPBYTE) alloca(dwSize);
    if (lpb == NULL) 
        return;
    if (pGetRawInputData((HRAWINPUT) lParam, RID_INPUT, lpb, &dwSize,
                          sizeof (RAWINPUTHEADER)) != dwSize)
        return;

    queue_from_rawinput((RAWINPUT *) lpb);
} /* wminput_handler */


static LRESULT CALLBACK RawWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_INPUT)
        wminput_handler(wParam, lParam);

    else if (Msg == WM_DESTROY)
        return(0);

    return DefWindowProc(hWnd, Msg, wParam, lParam);
} /* RawWndProc */


static int init_event_queue(void)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEX wce;
    RAWINPUTDEVICE rid;

    ZeroMemory(input_events, sizeof (input_events));
    input_events_read = input_events_write = 0;

    ZeroMemory(&wce, sizeof (wce));
    wce.cbSize = sizeof(WNDCLASSEX);
    wce.lpfnWndProc = RawWndProc;
    wce.lpszClassName = class_name;
    wce.hInstance = hInstance;
    class_atom = RegisterClassEx(&wce);
    if (class_atom == 0)
        return(0);

    raw_hwnd = CreateWindow(class_name, win_name, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);

    if (raw_hwnd == NULL)
        return(0);

    ZeroMemory(&rid, sizeof (rid));
    rid.usUsagePage = 1; /* GenericDesktop page */
    rid.usUsage = 2; /* GeneralDestop Mouse usage. */
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = raw_hwnd;
    if (!pRegisterRawInputDevices(&rid, 1, sizeof (rid)))
        return(0);

    return(1);
} /* init_event_queue */


static void cleanup_window(void)
{
    if (raw_hwnd)
    {
        MSG Msg;
        DestroyWindow(raw_hwnd);
        while (PeekMessage(&Msg, raw_hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        } /* while */
        raw_hwnd = 0;
    } /* if */

    if (class_atom)
    {
        UnregisterClass(class_name, GetModuleHandle(NULL));
        class_atom = 0;
    } /* if */
} /* cleanup_window */


static int windows_wminput_init(void)
{
    RAWINPUTDEVICELIST *devlist = NULL;
    UINT ct = 0;
    UINT i;

    available_mice = 0;

    if (!find_api_symbols())  /* only supported on WinXP and later. */
        return(0);

    pGetRawInputDeviceList(NULL, &ct, sizeof (RAWINPUTDEVICELIST));
    if (ct == 0)  /* no devices. */
        return(0);

    devlist = (PRAWINPUTDEVICELIST) alloca(sizeof (RAWINPUTDEVICELIST) * ct);
    pGetRawInputDeviceList(devlist, &ct, sizeof (RAWINPUTDEVICELIST));
    for (i = 0; i < ct; i++)
        init_mouse(&devlist[i]);

    if (!init_event_queue())
    {
        cleanup_window();
        available_mice = 0;
    } /* if */

    return(available_mice);
} /* windows_wminput_init */


static void windows_wminput_quit(void)
{
    /* unregister WM_INPUT devices... */
    RAWINPUTDEVICE rid;
    ZeroMemory(&rid, sizeof (rid));
    rid.usUsagePage = 1; /* GenericDesktop page */
    rid.usUsage = 2; /* GeneralDestop Mouse usage. */
    rid.dwFlags |= RIDEV_REMOVE;
    pRegisterRawInputDevices(&rid, 1, sizeof (rid));
    cleanup_window();
    available_mice = 0;
} /* windows_wminput_quit */


static const char *windows_wminput_name(unsigned int index)
{
    if (index < available_mice)
        return mice[index].name;
    return(NULL);
} /* windows_wminput_name */


static int windows_wminput_poll(ManyMouseEvent *outevent)
{
    MSG Msg;  /* run the queue for WM_INPUT messages, etc ... */

    /* ...favor existing events in the queue... */
    if (input_events_read != input_events_write)  /* no events if equal. */
    {
        CopyMemory(outevent, &input_events[input_events_read], sizeof (*outevent));
        input_events_read = ((input_events_read + 1) % MAX_EVENTS);
        return(1);
    } /* if */

    /* pump Windows for new hardware events... */
    while (PeekMessage(&Msg, raw_hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    } /* while */

    /* In case something new came in, give it to the app... */
    if (input_events_read != input_events_write)  /* no events if equal. */
    {
        /* take event off the queue. */
        CopyMemory(outevent, &input_events[input_events_read], sizeof (*outevent));
        input_events_read = ((input_events_read + 1) % MAX_EVENTS);
        return(1);
    } /* if */

    return(0);  /* no events at the moment. */
} /* windows_wminput_poll */


ManyMouseDriver ManyMouseDriver_windows =
{
    windows_wminput_init,
    windows_wminput_quit,
    windows_wminput_name,
    windows_wminput_poll
};

#endif  /* ifdef WINDOWS blocker */

/* end of windows_wminput.c ... */

