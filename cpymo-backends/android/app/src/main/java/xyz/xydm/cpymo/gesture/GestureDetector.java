package xyz.xydm.cpymo.gesture;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.MotionEvent;

import androidx.annotation.NonNull;

import java.util.HashMap;

public class GestureDetector {
    // distinguish press and tap
    private static final int TIME_DELAY1 = 200;
    private static final int WHAT_DELAY1 = 1;
    // distinguish single tap and double tap
    private static final int TIME_DELAY2 = 200;
    private static final int WHAT_DELAY2 = 2;
    // distinguish short press and long press
    private static final int TIME_DELAY3 = 500;
    private static final int WHAT_DELAY3 = 3;
    // whether two finger release at the same time or not
    private static final int TIME_DELAY4 = 400;
    private static final int WHAT_DELAY4 = 4;

    public enum State {
        Initial,
        OneDown,        // 单指按下
        OnePress,       // 单指按住
        OneMove,        // 单指移动
        OneTap,         // 单指点击
        OneDoubleDown,  // 单指第二次按下
        //        OneDoubleTap,
        TwoDown,        // 双指按下
        //        TwoPress,
        TwoMove,        // 双指移动
        TwoMoveUpOne,   // 双指移动后松开一指
        TwoDownUpOne,   // 双指按下后松开一指
        TwoTap,         // 双指点击
        TwoDoubleOneDown,   // 双指松开后按下一指
        TwoDoubleDown,      // 第二次双指按下
        TwoDoublePress,     // 双指第二次按下后按住
        TwoDoubleDownUpOne, // 第二次双指按下后松开一指
//        TwoDoubleTap,
    }

    private final OnGestureListener mListener;
    private final Handler mHandler;
    private final HashMap<Integer, SlideDetector> pointerSlideDetectors = new HashMap<>();

    private State state = State.Initial;


    private void gotoState(State state) {
        this.state = state;
//        mListener.onStateChanged(state);
    }

    public GestureDetector(OnGestureListener listener) {
        this.mListener = listener;
        this.mHandler = new Handler(Looper.myLooper(), msg -> {
            switch (msg.what) {
                case WHAT_DELAY1:
                    callbackDelay1((MotionEvent) msg.obj);
                    break;
                case WHAT_DELAY2:
                    callbackDelay2((MotionEvent) msg.obj);
                    break;
                case WHAT_DELAY3:
                    callbackDelay3((MotionEvent) msg.obj);
                    break;
                case WHAT_DELAY4:
                    callbackDelay4((MotionEvent) msg.obj);
                    break;
            }
            return true;
        });
    }

    public boolean onTouchEvent(@NonNull MotionEvent event) {
        boolean result;
        switch (this.state) {
            case Initial:
                result = dispatchInitial(event);
                break;
            case OneDown:
                result = dispatchOneDown(event);
                break;
            case OnePress:
                result = dispatchOnePress(event);
                break;
            case OneMove:
                result = dispatchOneMove(event);
                break;
            case OneTap:
                result = dispatchOneTap(event);
                break;
            case OneDoubleDown:
                result = dispatchOneDoubleDown(event);
                break;
            case TwoDown:
                result = dispatchTwoDown(event);
                break;
            case TwoMove:
                result = dispatchTwoMove(event);
                break;
            case TwoMoveUpOne:
                result = dispatchTwoMoveUpOne(event);
                break;
            case TwoDownUpOne:
                result = dispatchTwoDownUpOne(event);
                break;
            case TwoTap:
                result = dispatchTwoTap(event);
                break;
            case TwoDoubleOneDown:
                result = dispatchTwoDoubleOneDown(event);
                break;
            case TwoDoubleDown:
                result = dispatchTwoDoubleDown(event);
                break;
            case TwoDoubleDownUpOne:
                result = dispatchTwoDoubleDownUpOne(event);
                break;
            case TwoDoublePress:
                result = dispatchTwoDoublePress(event);
                break;
            default:
                result = false;
        }
        if (result) return true;
        gotoState(State.Initial);
        return false;
    }

    private boolean dispatchInitial(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            gotoState(State.OneDown);
            SlideDetector detector = getPointerDetector(pointerId);
            detector.start(event.getX(), event.getY());
            sendDelay1(event);
            return true;
        }
        return false;
    }

    private boolean dispatchOneDown(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);
        SlideDetector detector = getPointerDetector(pointerId);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_POINTER_DOWN: {
                cancelDelay1();
                gotoState(State.TwoDown);
                detector.start(event.getX(index), event.getY(index));
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                if (detector.ignoreTinyMove(event)) return true;
                cancelDelay1();
                gotoState(State.OneMove);
                detector.move(event.getX(), event.getY());
                return true;
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay1();
                gotoState(State.OneTap);
                sendDelay2(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchOnePress(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                if (detector.ignoreTinyMove(event)) return true;
                cancelDelay3();
                mListener.onScan(event, event.getX(), event.getY());
                return true;
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay3();
                gotoState(State.Initial);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchOneMove(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);
        SlideDetector detector = getPointerDetector(pointerId);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                detector.move(event.getX(), event.getY());
                return true;
            }
            case MotionEvent.ACTION_UP: {
                SlideDetector.Direction[] directions = detector.getDirections();
                if (directions.length != 1) return false;
                gotoState(State.Initial);
                mListener.onSlide(event, directions[0]);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchOneTap(@NonNull MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            int index = event.getActionIndex();
            int pointerId = event.getPointerId(index);
            SlideDetector detector = getPointerDetector(pointerId);

            detector.start(event.getX(index), event.getY(index));
            cancelDelay2();
            gotoState(State.OneDoubleDown);
            sendDelay1(event);
            return true;
        }
        return false;
    }

    private boolean dispatchOneDoubleDown(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                if (detector.ignoreTinyMove(event)) return true;
                cancelDelay1();
                gotoState(State.OneMove);
                return true;
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay1();
                gotoState(State.Initial);
                mListener.onDoubleTap(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchTwoDown(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                if (detector.ignoreTinyMove(event)) return true;
                gotoState(State.TwoMove);
                assert event.getPointerCount() == 2;
                int id0 = event.getPointerId(0);
                int id1 = event.getPointerId(1);
                getPointerDetector(id0).move(event.getX(0), event.getY(0));
                getPointerDetector(id1).move(event.getX(1), event.getY(1));
                return true;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                gotoState(State.TwoDownUpOne);
                sendDelay4(event);
                return true;
            }
            case MotionEvent.ACTION_UP: {
//                throw new RuntimeException("不应该出现这个事件");
                return false;
            }
        }
        return false;
    }

    private boolean dispatchTwoMove(@NonNull MotionEvent event) {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                assert event.getPointerCount() == 2;
                int id0 = event.getPointerId(0);
                int id1 = event.getPointerId(1);
                getPointerDetector(id0).move(event.getX(0), event.getY(0));
                getPointerDetector(id1).move(event.getX(1), event.getY(1));
                return true;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                assert event.getPointerCount() == 2;
                int id0 = event.getPointerId(0);
                int id1 = event.getPointerId(1);
                SlideDetector.Direction[] directions1 = getPointerDetector(id0).getDirections();
                SlideDetector.Direction[] directions2 = getPointerDetector(id1).getDirections();
                if (directions1.length == 1 && directions2.length == 1) {
                    SlideDetector.Direction direction1 = directions1[0];
                    SlideDetector.Direction direction2 = directions2[0];
                    if (direction1 == direction2) {
                        gotoState(State.TwoMoveUpOne);
                        sendDelay4(event);
                        return true;
                    }
                }
                return false;
            }
            case MotionEvent.ACTION_UP: {
//                throw new RuntimeException("不应该出现这个事件");
                return false;
            }
        }
        return false;
    }

    private boolean dispatchTwoMoveUpOne(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);
        SlideDetector detector = getPointerDetector(pointerId);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                detector.move(event.getX(), event.getY());
                return true;
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay4();
                SlideDetector.Direction[] directions = detector.getDirections();
                if (directions.length == 1) {
                    gotoState(State.Initial);
                    mListener.onTwoSlide(event, directions[0]);
                    return true;
                }
                return false;
            }
        }
        return false;
    }

    private boolean dispatchTwoDownUpOne(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                if (detector.ignoreTinyMove(event)) return true;
                cancelDelay4();
                return false;
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay4();
                gotoState(State.TwoTap);
                sendDelay2(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchTwoTap(@NonNull MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            int index = event.getActionIndex();
            int pointerId = event.getPointerId(index);
            SlideDetector detector = getPointerDetector(pointerId);

            detector.start(event.getX(index), event.getY(index));
            cancelDelay2();
            gotoState(State.TwoDoubleOneDown);
            return true;
        }
        return false;
    }

    private boolean dispatchTwoDoubleOneDown(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);
        SlideDetector detector = getPointerDetector(pointerId);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                return detector.ignoreTinyMove(event);
            }
            case MotionEvent.ACTION_POINTER_DOWN: {
                gotoState(State.TwoDoubleDown);
                detector.start(event.getX(index), event.getY(index));
                sendDelay1(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchTwoDoubleDown(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                if (detector.ignoreTinyMove(event)) return true;
                cancelDelay1();
                return false;
            }
            case MotionEvent.ACTION_POINTER_UP: {
                cancelDelay1();
                gotoState(State.TwoDoubleDownUpOne);
                sendDelay4(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchTwoDoubleDownUpOne(@NonNull MotionEvent event) {
        int index = event.getActionIndex();
        int pointerId = event.getPointerId(index);

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_MOVE: {
                SlideDetector detector = getPointerDetector(pointerId);
                return detector.ignoreTinyMove(event);
            }
            case MotionEvent.ACTION_UP: {
                cancelDelay4();
                gotoState(State.Initial);
                mListener.onTwoDoubleTap(event);
                return true;
            }
        }
        return false;
    }

    private boolean dispatchTwoDoublePress(@NonNull MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_MOVE) {
            return true;
        }
        mListener.onTwoDoublePressEnd(event);
        return false;
    }

    private void sendDelay1(MotionEvent event) {
        Message msg = mHandler.obtainMessage(WHAT_DELAY1, event);
        mHandler.sendMessageDelayed(msg, TIME_DELAY1);
    }

    private void sendDelay2(MotionEvent event) {
        Message msg = mHandler.obtainMessage(WHAT_DELAY2, event);
        mHandler.sendMessageDelayed(msg, TIME_DELAY2);
    }

    private void sendDelay3(MotionEvent event) {
        Message msg = mHandler.obtainMessage(WHAT_DELAY3, event);
        mHandler.sendMessageDelayed(msg, TIME_DELAY3);
    }

    private void sendDelay4(MotionEvent event) {
        Message msg = mHandler.obtainMessage(WHAT_DELAY4, event);
        mHandler.sendMessageDelayed(msg, TIME_DELAY4);
    }

    private void cancelDelay1() {
        mHandler.removeMessages(WHAT_DELAY1);
    }

    private void cancelDelay2() {
        mHandler.removeMessages(WHAT_DELAY2);
    }

    private void cancelDelay3() {
        mHandler.removeMessages(WHAT_DELAY3);
    }

    private void cancelDelay4() {
        mHandler.removeMessages(WHAT_DELAY4);
    }

    @SuppressWarnings("DuplicateBranchesInSwitch")
    private void callbackDelay1(MotionEvent event) {
        switch (state) {
            case OneDown: {
                gotoState(State.OnePress);
                sendDelay3(event);
                return;
            }
            case OneDoubleDown: {
//                gotoState(State.OneDoublePress);
                gotoState(State.Initial);
                return;
            }
            case TwoDown: {
//                gotoState(State.TwoPress);
                gotoState(State.Initial);
                return;
            }
            case TwoDoubleDown: {
                gotoState(State.TwoDoublePress);
                mListener.onTwoDoublePressStart(event);
                return;
            }
        }
//        throw new RuntimeException(String.format("不应该出现这个事件, 当前状态: %s", state));
        gotoState(State.Initial);
    }

    private void callbackDelay2(MotionEvent event) {
        switch (state) {
            case OneTap: {
                gotoState(State.Initial);
                mListener.onTap(event, event.getX(), event.getY());
                break;
            }
            case TwoTap: {
                gotoState(State.Initial);
//                mListener.onTwoTap(event);
            }
        }
    }

    private void callbackDelay3(MotionEvent event) {
        gotoState(State.Initial);
        mListener.onLongPress(event);
    }

    private void callbackDelay4(MotionEvent event) {
        gotoState(State.Initial);
    }

    private SlideDetector getPointerDetector(int pointerId) {
        if (pointerSlideDetectors.containsKey(pointerId)) {
            return pointerSlideDetectors.get(pointerId);
        }
        SlideDetector detector = new SlideDetector();
        pointerSlideDetectors.put(pointerId, detector);
        return detector;
    }
}
