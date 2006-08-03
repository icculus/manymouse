public class ManyMouseEvent
{
    // Event types...
    // !!! FIXME: can be real enums in Java 5.0.
    public static final int ABSMOTION = 0;
    public static final int RELMOTION = 1;
    public static final int BUTTON = 2;
    public static final int SCROLL = 3;
    public static final int DISCONNECT = 4;
    public static final int MAX = 5;  // Only for reference: should not be set.

    public int type;
    public int device;
    public int item;
    public int value;
    public int minval;
    public int maxval;
} // ManyMouseEvent

// end of ManyMouseEvent.java ...

