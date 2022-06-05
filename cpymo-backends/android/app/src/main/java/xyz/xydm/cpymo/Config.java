package xyz.xydm.cpymo;

public class Config {
    public static native boolean nativeGetNonVisuallyImpairedHelp();

    public static boolean needAccessibility() {
        return !nativeGetNonVisuallyImpairedHelp();
    }
}
