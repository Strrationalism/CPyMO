package xyz.xydm.cpymo;

import android.content.Context;
import android.os.Build;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.widget.Toast;

import org.libsdl.app.SDLActivity;

public class VisualHelper {
    protected static TextToSpeech mTextToSpeech;
    protected static Vibrator mVibrator;
    protected static String mLastSpeakText;

    public static native void nativeSetupJNI();

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean textToSpeech(String text) {
        if (mTextToSpeech == null)
            return false;
        mTextToSpeech.speak(text, TextToSpeech.QUEUE_FLUSH, null);
        mLastSpeakText = text;
        return true;
    }

    public static boolean textToSpeechWithoutCopy(String text) {
        if (mTextToSpeech == null)
            return false;
        mTextToSpeech.speak(text, TextToSpeech.QUEUE_FLUSH, null);
        return true;
    }

    public static void copyLastSpeechText() {
        if (mLastSpeakText == null) return;

        SDLActivity.clipboardSetText(mLastSpeakText);
    }

    public static void appendCopyLastSpeechText() {
        if (mLastSpeakText == null) return;

        String text = SDLActivity.clipboardGetText();
        SDLActivity.clipboardSetText(text + "\n" + mLastSpeakText);
    }

    public static void sendKeyKnock(int keycode) {
        SDLActivity.onNativeKeyDown(keycode);
        try {
            Thread.sleep(16);
        } catch (Exception ignore) {
        }
        SDLActivity.onNativeKeyUp(keycode);
    }

    public static void vibrate(long milliseconds) {
        // Vibrate for 500 milliseconds
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mVibrator.vibrate(VibrationEffect.createOneShot(milliseconds, VibrationEffect.DEFAULT_AMPLITUDE));
        } else {
            //deprecated in API 26
            mVibrator.vibrate(milliseconds);
        }
    }

    public static void setup(Context context) {
        // Set up text speech
        if (!Config.needAccessibility()) return;

        mTextToSpeech = new TextToSpeech(context, status -> {
            if (status == TextToSpeech.SUCCESS)
                mTextToSpeech.speak("请先关闭系统读屏，然后上下滑动选择游戏", TextToSpeech.QUEUE_FLUSH, null);
            else
                Toast.makeText(context, "TTS初始化失败", Toast.LENGTH_SHORT).show();
        });

        // Set up vibrator
        mVibrator = (Vibrator) context.getSystemService(Context.VIBRATOR_SERVICE);

        nativeSetupJNI();
    }
}
