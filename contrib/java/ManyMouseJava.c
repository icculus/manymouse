/* The C interface... */
#include "manymouse.h"

/* The JNI interface... */
#include "ManyMouseJava.h"


/* The JNI implementation... */

JNIEXPORT jint JNICALL Java_ManyMouse_Init
  (JNIEnv *env, jclass obj)
{
    return ManyMouse_Init();
} /* JNI org.icculus.ManyMouse.Init */


JNIEXPORT void JNICALL Java_ManyMouse_Quit
  (JNIEnv *env, jclass obj)
{
    ManyMouse_Quit();
} /* JNI org.icculus.ManyMouse.Quit */


JNIEXPORT jstring JNICALL Java_ManyMouse_DeviceName
  (JNIEnv *env, jclass obj, jint mouse)
{
    const char *str = ManyMouse_DeviceName(mouse);
    return (*env)->NewStringUTF(env, str);
} /* JNI org.icculus.ManyMouse.DeviceName */


static jboolean setInt
  (JNIEnv *env, jobject jevent, jclass cls, const char *fieldname, int val)
{
    jfieldID fid = (*env)->GetFieldID(env, cls, fieldname, "I");
    if (fid == 0)
        return JNI_FALSE;

    (*env)->SetIntField(env, jevent, fid, val);
    return JNI_TRUE;
} /* setInt */


JNIEXPORT jboolean JNICALL Java_ManyMouse_PollEvent
  (JNIEnv *env, jclass obj, jobject jevent)
{
    ManyMouseEvent event;
    jclass cls = (*env)->GetObjectClass(env, jevent);
    if (cls == 0)
        return JNI_FALSE;  /* !!! FIXME: throw an exception? */

    if (ManyMouse_PollEvent(&event) == 0)
        return JNI_FALSE;  /* no new events. */

    #define SETINT(field) \
        if (!setInt(env, jevent, cls, #field, event.field)) return JNI_FALSE;
    SETINT(type);
    SETINT(device);
    SETINT(item);
    SETINT(value);
    SETINT(minval);
    SETINT(maxval);
    #undef SETINT

    return JNI_TRUE;
} /* JNI org.icculus.ManyMouse.PollEvent */

/* end of ManyMouseJava.c ... */

