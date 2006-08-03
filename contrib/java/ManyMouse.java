public class ManyMouse
{
    // Native method hooks.
    public native static synchronized int Init();
    public native static synchronized void Quit();
    public native static synchronized String DeviceName(int index);
    public native static synchronized boolean PollEvent(ManyMouseEvent event);

    // JNI link.
    static { System.loadLibrary("ManyMouse"); }
} // ManyMouse

// end of ManyMouse.java ...

