public class TestManyMouse
{
    public static void main(String args[])
    {
        int mice = ManyMouse.Init();
        System.out.println("ManyMouse.Init() reported " + mice + ".");
        for (int i = 0; i < mice; i++)
            System.out.println("Mouse #" + i + ": " + ManyMouse.DeviceName(i));
        System.out.println();

        // Allocate one that PollEvent fills in so we aren't spamming the
        //  memory manager with throwaway objects for each event.
        ManyMouseEvent event = new ManyMouseEvent();

        while (mice > 0)  // report events until process is killed.
        {
            if (!ManyMouse.PollEvent(event))
                try { Thread.sleep(100); } catch (InterruptedException e) {}
            else
            {
                System.out.print("Mouse #");
                System.out.print(event.device);
                System.out.print(" ");

                switch (event.type)
                {
                    case ManyMouseEvent.ABSMOTION:
                        System.out.print("absolute motion ");
                        if (event.item == 0) // x axis
                            System.out.print("X axis ");
                        else if (event.item == 1) // y axis
                            System.out.print("Y axis ");
                        else
                            System.out.print("? axis ");  // error?
                        System.out.print(event.value);
                        break;

                    case ManyMouseEvent.RELMOTION:
                        System.out.print("relative motion ");
                        if (event.item == 0) // x axis
                            System.out.print("X axis ");
                        else if (event.item == 1) // y axis
                            System.out.print("Y axis ");
                        else
                            System.out.print("? axis ");  // error?
                        System.out.print(event.value);
                        break;

                    case ManyMouseEvent.BUTTON:
                        System.out.print("mouse button ");
                        System.out.print(event.item);
                        if (event.value == 0)
                            System.out.print(" up");
                        else
                            System.out.print(" down");
                        break;

                    case ManyMouseEvent.SCROLL:
                        System.out.print("scroll wheel ");
                        if (event.item == 0)
                        {
                            if (event.value > 0)
                                System.out.print("up");
                            else
                                System.out.print("down");
                        }
                        else
                        {
                            if (event.value > 0)
                                System.out.print("right");
                            else
                                System.out.print("left");
                        }
                        break;

                    case ManyMouseEvent.DISCONNECT:
                        System.out.print("disconnect");
                        mice--;
                        break;

                    default:
                        System.out.print("Unknown event: ");
                        System.out.print(event.type);
                        break;
                } // switch
                System.out.println();
            } // if
        } // while

        ManyMouse.Quit();
    } // Main
} // TestManyMouse

// end of TestManyMouse.java ...


