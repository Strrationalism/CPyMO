package xyz.xydm.cpymo.gesture;

import android.view.MotionEvent;

import androidx.annotation.NonNull;

public interface OnGestureListener {
//    void onStateChanged(GestureDetector.State state);

    void onTap(MotionEvent event, float x, float y);

    void onLongPress(MotionEvent event);

    void onScan(MotionEvent event, float x, float y);

    void onSlide(MotionEvent event, @NonNull SlideDetector.Direction direction);
//    void onLongPressAfterTap(MotionEvent e);
//    void onScanAfterTap(MotionEvent e);
//    void onFlingAfterTap(MotionEvent e);
    void onDoubleTap(MotionEvent event);
//    void onTwoTap(MotionEvent event);
    void onTwoDoubleTap(MotionEvent event);

    void onTwoSlide(MotionEvent event, SlideDetector.Direction direction);

    void onTwoDoublePressStart(MotionEvent event);

    void onTwoDoublePressEnd(MotionEvent event);
}