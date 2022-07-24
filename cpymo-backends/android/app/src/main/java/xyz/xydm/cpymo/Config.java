package xyz.xydm.cpymo;

public class Config {
    public static native boolean nativeNeedAccessibility();

    public static boolean needAccessibility() {
        return nativeNeedAccessibility();
    }
}
