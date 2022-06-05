package xyz.xydm.cpymo.gesture;

import android.view.MotionEvent;

import java.util.ArrayList;


public class SlideDetector {
    private static final float TINY_THRESHOLD = 10;

    public enum Direction {
        None, Up, Left, Down, Right
    }

    private final ArrayList<Direction> history = new ArrayList<>();

    private Direction curDirection;

    private float lastX;
    private float lastY;

    public void start(float x, float y) {
        this.curDirection = Direction.None;
        this.history.clear();
        this.lastX = x;
        this.lastY = y;
    }

    public void move(float x, float y) {
        float dx = x - lastX;
        float dy = y - lastY;

        Direction direction;
        if (Math.abs(dx) < TINY_THRESHOLD && Math.abs(dy) < TINY_THRESHOLD) {
            // ignore tiny move
            direction = curDirection;
        } else if (Math.abs(dx) > Math.abs(dy)) {
            direction = dx >= 0 ? Direction.Right : Direction.Left;
        } else {
            direction = dy >= 0 ? Direction.Down : Direction.Up;
        }

        if (direction != curDirection) {
            history.add(direction);
            curDirection = direction;
        }

        lastX = x;
        lastY = y;
    }

    public Direction[] getDirections() {
        Direction[] result = new Direction[history.size()];
        return history.toArray(result);
    }

    public boolean ignoreTinyMove(MotionEvent event) {
        float dx = Math.abs(event.getX() - lastX);
        float dy = Math.abs(event.getY() - lastY);
        return dx < TINY_THRESHOLD && dy < TINY_THRESHOLD;
    }
}
