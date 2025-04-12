package com.example.nativecpptest1;

public class NativeLib {

    static {
        System.loadLibrary("nativecpptest1");
    }
    public static native void init(int width, int height);
    public static native void step();
    public static native void swipeRight();
    public static native void swipeLeft();
}
