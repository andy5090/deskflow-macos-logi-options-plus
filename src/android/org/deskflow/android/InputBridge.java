/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

package org.deskflow.android;

import java.io.BufferedReader;
import java.io.FileDescriptor;
import java.io.InputStreamReader;
import java.lang.reflect.Array;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

/**
 * A tiny stdin-driven input injector launched by adb shell/app_process.
 *
 * The source deliberately has no compile-time Android dependencies. This lets
 * Termux javac compile it without a full Android SDK; Android framework classes
 * are resolved by reflection inside app_process.
 */
public final class InputBridge {
    private static final class UhidKeyboard implements AutoCloseable {
        private static final int UHID_CREATE2 = 11;
        private static final int UHID_INPUT2 = 12;
        // UHID emulates a USB HID keyboard. BUS_USB also makes Android classify
        // it as an external keyboard, which is required for hardware-keyboard
        // shortcuts in Samsung DeX and several IMEs.
        private static final short BUS_USB = 0x03;

        // Standard six-key rollover keyboard with eight independent modifiers.
        // LANG1 (usage 0x90) is included for the Korean Hangul/English key.
        private static final byte[] REPORT_DESCRIPTOR = new byte[] {
            0x05, 0x01, 0x09, 0x06, (byte) 0xA1, 0x01,
            0x05, 0x07, 0x19, (byte) 0xE0, 0x29, (byte) 0xE7,
            0x15, 0x00, 0x25, 0x01, 0x75, 0x01, (byte) 0x95, 0x08,
            (byte) 0x81, 0x02,
            (byte) 0x95, 0x01, 0x75, 0x08, (byte) 0x81, 0x01,
            (byte) 0x95, 0x06, 0x75, 0x08, 0x15, 0x00,
            0x26, (byte) 0xE7, 0x00, 0x05, 0x07,
            0x19, 0x00, 0x2A, (byte) 0xE7, 0x00, (byte) 0x81, 0x00,
            (byte) 0xC0
        };

        private final FileDescriptor fileDescriptor;
        private final Method writeMethod;
        private final Method closeMethod;
        private final Set<Integer> keys = new LinkedHashSet<>();
        private int modifiers;

        UhidKeyboard() throws Exception {
            Class<?> osClass = Class.forName("android.system.Os");
            Class<?> constantsClass = Class.forName("android.system.OsConstants");
            Method openMethod = osClass.getMethod("open", String.class, int.class, int.class);
            writeMethod = osClass.getMethod(
                    "write", FileDescriptor.class, byte[].class, int.class, int.class);
            closeMethod = osClass.getMethod("close", FileDescriptor.class);
            int readWrite = constantsClass.getField("O_RDWR").getInt(null);
            fileDescriptor = (FileDescriptor) openMethod.invoke(null, "/dev/uhid", readWrite, 0);

            try {
                write(createRequest());
                // InputReader registers the new device asynchronously.
                Thread.sleep(200);
            } catch (Exception error) {
                closeMethod.invoke(null, fileDescriptor);
                throw error;
            }
        }

        private static byte[] createRequest() {
            ByteBuffer buffer = ByteBuffer.allocate(280 + REPORT_DESCRIPTOR.length)
                    .order(ByteOrder.nativeOrder());
            buffer.putInt(UHID_CREATE2);
            byte[] name = "Deskflow virtual keyboard".getBytes(StandardCharsets.UTF_8);
            buffer.position(4);
            buffer.put(name, 0, Math.min(name.length, 127));
            buffer.position(4 + 256);
            buffer.putShort((short) REPORT_DESCRIPTOR.length);
            buffer.putShort(BUS_USB);
            buffer.putInt(0); // vendor
            buffer.putInt(0); // product
            buffer.putInt(0); // version
            buffer.putInt(0); // country
            buffer.put(REPORT_DESCRIPTOR);
            return buffer.array();
        }

        private void write(byte[] request) throws Exception {
            int written = (Integer) writeMethod.invoke(
                    null, fileDescriptor, request, 0, request.length);
            if (written != request.length) {
                throw new IllegalStateException("short write to /dev/uhid");
            }
        }

        private void sendReport() throws Exception {
            byte[] report = new byte[8];
            report[0] = (byte) modifiers;
            int index = 2;
            for (int usage : keys) {
                if (index == report.length) {
                    break;
                }
                report[index++] = (byte) usage;
            }
            ByteBuffer request = ByteBuffer.allocate(6 + report.length)
                    .order(ByteOrder.nativeOrder());
            request.putInt(UHID_INPUT2);
            request.putShort((short) report.length);
            request.put(report);
            write(request.array());
        }

        private static int modifierBit(int keyCode) {
            switch (keyCode) {
            case 113: return 0x01; // CTRL_LEFT
            case 59: return 0x02;  // SHIFT_LEFT
            case 57: return 0x04;  // ALT_LEFT
            case 117: return 0x08; // META_LEFT
            case 114: return 0x10; // CTRL_RIGHT
            case 60: return 0x20;  // SHIFT_RIGHT
            case 58: return 0x40;  // ALT_RIGHT
            case 118: return 0x80; // META_RIGHT
            default: return 0;
            }
        }

        private static int hidUsage(int keyCode) {
            if (keyCode >= 29 && keyCode <= 54) { // A-Z
                return 0x04 + keyCode - 29;
            }
            if (keyCode >= 8 && keyCode <= 16) { // 1-9
                return 0x1E + keyCode - 8;
            }
            if (keyCode >= 131 && keyCode <= 142) { // F1-F12
                return 0x3A + keyCode - 131;
            }
            if (keyCode >= 145 && keyCode <= 153) { // keypad 1-9
                return 0x59 + keyCode - 145;
            }
            switch (keyCode) {
            case 7: return 0x27;   // 0
            case 19: return 0x52;  // up
            case 20: return 0x51;  // down
            case 21: return 0x50;  // left
            case 22: return 0x4F;  // right
            case 55: return 0x36;  // comma
            case 56: return 0x37;  // period
            case 61: return 0x2B;  // tab
            case 62: return 0x2C;  // space
            case 66: return 0x28;  // enter
            case 67: return 0x2A;  // backspace
            case 68: return 0x35;  // grave
            case 69: return 0x2D;  // minus
            case 70: return 0x2E;  // equals
            case 71: return 0x2F;  // left bracket
            case 72: return 0x30;  // right bracket
            case 73: return 0x31;  // backslash
            case 74: return 0x33;  // semicolon
            case 75: return 0x34;  // apostrophe
            case 76: return 0x38;  // slash
            case 82: return 0x65;  // application/menu
            case 92: return 0x4B;  // page up
            case 93: return 0x4E;  // page down
            case 111: return 0x29; // escape
            case 112: return 0x4C; // delete forward
            case 115: return 0x39; // caps lock
            case 116: return 0x47; // scroll lock
            case 120: return 0x46; // print screen
            case 121: return 0x48; // pause
            case 122: return 0x4A; // home
            case 123: return 0x4D; // end
            case 124: return 0x49; // insert
            case 143: return 0x53; // num lock
            case 144: return 0x62; // keypad 0
            case 154: return 0x54; // keypad divide
            case 155: return 0x55; // keypad multiply
            case 156: return 0x56; // keypad subtract
            case 157: return 0x57; // keypad add
            case 158: return 0x63; // keypad decimal
            case 160: return 0x58; // keypad enter
            case 161: return 0x67; // keypad equals
            case 204: return 0x90; // LANG1: Hangul/English
            default: return 0;
            }
        }

        boolean key(boolean down, int keyCode) throws Exception {
            int modifier = modifierBit(keyCode);
            if (modifier != 0) {
                if (down) {
                    modifiers |= modifier;
                } else {
                    modifiers &= ~modifier;
                }
                sendReport();
                return true;
            }

            int usage = hidUsage(keyCode);
            if (usage == 0) {
                return false;
            }
            if (down) {
                if (!keys.contains(usage) && keys.size() >= 6) {
                    return false;
                }
                keys.add(usage);
            } else {
                keys.remove(usage);
            }
            sendReport();
            return true;
        }

        @Override
        public void close() throws Exception {
            if (modifiers != 0 || !keys.isEmpty()) {
                modifiers = 0;
                keys.clear();
                sendReport();
            }
            closeMethod.invoke(null, fileDescriptor);
        }
    }

    private static final class UhidMouse implements AutoCloseable {
        private static final int UHID_CREATE2 = 11;
        private static final int UHID_INPUT2 = 12;
        private static final short BUS_VIRTUAL = 0x06;

        // USB HID 1.11, Appendix E.10, extended with five buttons and AC Pan.
        private static final byte[] REPORT_DESCRIPTOR = new byte[] {
            0x05, 0x01, 0x09, 0x02, (byte) 0xA1, 0x01, 0x09, 0x01,
            (byte) 0xA1, 0x00, 0x05, 0x09, 0x19, 0x01, 0x29, 0x05,
            0x15, 0x00, 0x25, 0x01, (byte) 0x95, 0x05, 0x75, 0x01,
            (byte) 0x81, 0x02, (byte) 0x95, 0x01, 0x75, 0x03,
            (byte) 0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31,
            0x09, 0x38, 0x15, (byte) 0x81, 0x25, 0x7F, 0x75, 0x08,
            (byte) 0x95, 0x03, (byte) 0x81, 0x06, 0x05, 0x0C,
            0x0A, 0x38, 0x02, 0x15, (byte) 0x81, 0x25, 0x7F,
            0x75, 0x08, (byte) 0x95, 0x01, (byte) 0x81, 0x06,
            (byte) 0xC0, (byte) 0xC0
        };

        private final FileDescriptor fileDescriptor;
        private final Method writeMethod;
        private final Method closeMethod;
        private int buttons;

        UhidMouse() throws Exception {
            Class<?> osClass = Class.forName("android.system.Os");
            Class<?> constantsClass = Class.forName("android.system.OsConstants");
            Method openMethod = osClass.getMethod("open", String.class, int.class, int.class);
            writeMethod = osClass.getMethod(
                    "write", FileDescriptor.class, byte[].class, int.class, int.class);
            closeMethod = osClass.getMethod("close", FileDescriptor.class);
            int readWrite = constantsClass.getField("O_RDWR").getInt(null);
            fileDescriptor = (FileDescriptor) openMethod.invoke(null, "/dev/uhid", readWrite, 0);

            try {
                write(createRequest());
                // InputReader registers the new device asynchronously.
                Thread.sleep(200);
            } catch (Exception error) {
                closeMethod.invoke(null, fileDescriptor);
                throw error;
            }
        }

        private static byte[] createRequest() {
            ByteBuffer buffer = ByteBuffer.allocate(280 + REPORT_DESCRIPTOR.length)
                    .order(ByteOrder.nativeOrder());
            buffer.putInt(UHID_CREATE2);
            byte[] name = "Deskflow virtual mouse".getBytes(StandardCharsets.UTF_8);
            buffer.position(4);
            buffer.put(name, 0, Math.min(name.length, 127));
            buffer.position(4 + 256);
            buffer.putShort((short) REPORT_DESCRIPTOR.length);
            buffer.putShort(BUS_VIRTUAL);
            buffer.putInt(0); // vendor
            buffer.putInt(0); // product
            buffer.putInt(0); // version
            buffer.putInt(0); // country
            buffer.put(REPORT_DESCRIPTOR);
            return buffer.array();
        }

        private void write(byte[] request) throws Exception {
            int written = (Integer) writeMethod.invoke(
                    null, fileDescriptor, request, 0, request.length);
            if (written != request.length) {
                throw new IllegalStateException("short write to /dev/uhid");
            }
        }

        private void sendReport(int dx, int dy, int vertical, int horizontal) throws Exception {
            byte[] report = new byte[] {
                (byte) buttons, (byte) dx, (byte) dy,
                (byte) vertical, (byte) horizontal
            };
            ByteBuffer request = ByteBuffer.allocate(6 + report.length)
                    .order(ByteOrder.nativeOrder());
            request.putInt(UHID_INPUT2);
            request.putShort((short) report.length);
            request.put(report);
            write(request.array());
        }

        void move(int dx, int dy) throws Exception {
            while (dx != 0 || dy != 0) {
                int stepX = Math.max(-127, Math.min(127, dx));
                int stepY = Math.max(-127, Math.min(127, dy));
                sendReport(stepX, stepY, 0, 0);
                dx -= stepX;
                dy -= stepY;
            }
        }

        void button(boolean down, int button) throws Exception {
            if (down) {
                buttons |= button;
            } else {
                buttons &= ~button;
            }
            sendReport(0, 0, 0, 0);
        }

        void scroll(float horizontal, float vertical) throws Exception {
            int horizontalStep = Math.max(-127, Math.min(127, Math.round(horizontal)));
            int verticalStep = Math.max(-127, Math.min(127, Math.round(vertical)));
            if (horizontalStep != 0 || verticalStep != 0) {
                sendReport(0, 0, verticalStep, horizontalStep);
            }
        }

        @Override
        public void close() throws Exception {
            if (buttons != 0) {
                buttons = 0;
                sendReport(0, 0, 0, 0);
            }
            closeMethod.invoke(null, fileDescriptor);
        }
    }

    private static final int ACTION_DOWN = 0;
    private static final int ACTION_UP = 1;
    private static final int ACTION_MOVE = 2;
    private static final int ACTION_SCROLL = 8;
    private static final int ACTION_BUTTON_PRESS = 11;
    private static final int ACTION_BUTTON_RELEASE = 12;

    private static final int SOURCE_KEYBOARD = 0x101;
    private static final int SOURCE_MOUSE = 0x2002;
    private static final int TOOL_TYPE_MOUSE = 3;
    private static final int AXIS_VSCROLL = 9;
    private static final int AXIS_HSCROLL = 10;
    private static final int VIRTUAL_KEYBOARD = -1;
    private static final int INJECT_ASYNC = 0;

    private final int displayId;
    private final Object inputManager;
    private final Method injectInputEvent;
    private final Method uptimeMillis;
    private final Constructor<?> keyEventConstructor;
    private final Method inputEventSetDisplayId;
    private final Method motionEventObtain;
    private final Method motionEventSetActionButton;
    private final Constructor<?> pointerPropertiesConstructor;
    private final Constructor<?> pointerCoordsConstructor;
    private final Field pointerPropertiesId;
    private final Field pointerPropertiesToolType;
    private final Field pointerCoordsX;
    private final Field pointerCoordsY;
    private final Method pointerCoordsSetAxisValue;
    private final Class<?> pointerPropertiesClass;
    private final Class<?> pointerCoordsClass;
    private final UhidKeyboard uhidKeyboard;
    private final UhidMouse uhidMouse;

    private final Map<Integer, Long> keyDownTimes = new HashMap<>();
    private long mouseDownTime;
    private int mouseButtons;
    private float pointerX;
    private float pointerY;

    private InputBridge(int displayId, boolean enableUhidMouse,
                        boolean enableUhidKeyboard) throws Exception {
        this.displayId = displayId;

        Class<?> inputEventClass = Class.forName("android.view.InputEvent");
        Object manager;
        Method inject;
        try {
            Class<?> managerClass = Class.forName("android.hardware.input.InputManager");
            Method getInstance = managerClass.getDeclaredMethod("getInstance");
            getInstance.setAccessible(true);
            manager = getInstance.invoke(null);
            inject = managerClass.getDeclaredMethod("injectInputEvent", inputEventClass, int.class);
        } catch (ReflectiveOperationException firstFailure) {
            Class<?> managerClass = Class.forName("android.hardware.input.InputManagerGlobal");
            Method getInstance = managerClass.getDeclaredMethod("getInstance");
            getInstance.setAccessible(true);
            manager = getInstance.invoke(null);
            inject = managerClass.getDeclaredMethod("injectInputEvent", inputEventClass, int.class);
        }
        inject.setAccessible(true);
        inputManager = manager;
        injectInputEvent = inject;

        Class<?> systemClockClass = Class.forName("android.os.SystemClock");
        uptimeMillis = systemClockClass.getMethod("uptimeMillis");

        Class<?> keyEventClass = Class.forName("android.view.KeyEvent");
        keyEventConstructor = keyEventClass.getConstructor(
                long.class, long.class, int.class, int.class, int.class,
                int.class, int.class, int.class, int.class, int.class);

        inputEventSetDisplayId = inputEventClass.getDeclaredMethod("setDisplayId", int.class);
        inputEventSetDisplayId.setAccessible(true);

        Class<?> motionEventClass = Class.forName("android.view.MotionEvent");
        pointerPropertiesClass = Class.forName("android.view.MotionEvent$PointerProperties");
        pointerCoordsClass = Class.forName("android.view.MotionEvent$PointerCoords");
        pointerPropertiesConstructor = pointerPropertiesClass.getConstructor();
        pointerCoordsConstructor = pointerCoordsClass.getConstructor();
        pointerPropertiesId = pointerPropertiesClass.getField("id");
        pointerPropertiesToolType = pointerPropertiesClass.getField("toolType");
        pointerCoordsX = pointerCoordsClass.getField("x");
        pointerCoordsY = pointerCoordsClass.getField("y");
        pointerCoordsSetAxisValue = pointerCoordsClass.getMethod("setAxisValue", int.class, float.class);

        Class<?> pointerPropertiesArray = Array.newInstance(pointerPropertiesClass, 0).getClass();
        Class<?> pointerCoordsArray = Array.newInstance(pointerCoordsClass, 0).getClass();
        motionEventObtain = motionEventClass.getMethod(
                "obtain", long.class, long.class, int.class, int.class,
                pointerPropertiesArray, pointerCoordsArray, int.class, int.class,
                float.class, float.class, int.class, int.class, int.class, int.class);
        motionEventSetActionButton = motionEventClass.getMethod("setActionButton", int.class);

        UhidMouse mouse = null;
        if (enableUhidMouse) {
            try {
                mouse = new UhidMouse();
            } catch (Exception error) {
                System.err.println("Deskflow UHID mouse unavailable, using SDK input: " + error);
            }
        }
        uhidMouse = mouse;

        UhidKeyboard keyboard = null;
        if (enableUhidKeyboard) {
            try {
                keyboard = new UhidKeyboard();
            } catch (Exception error) {
                System.err.println("Deskflow UHID keyboard unavailable, using SDK input: " + error);
            }
        }
        uhidKeyboard = keyboard;
    }

    private long now() throws Exception {
        return (Long) uptimeMillis.invoke(null);
    }

    private boolean inject(Object event) throws Exception {
        inputEventSetDisplayId.invoke(event, displayId);
        return (Boolean) injectInputEvent.invoke(inputManager, event, INJECT_ASYNC);
    }

    private void injectRequired(Object event) throws Exception {
        if (!inject(event)) {
            throw new IllegalStateException("Android rejected the injected input event");
        }
    }

    private void key(int action, int keyCode, int metaState, int repeat) throws Exception {
        if (uhidKeyboard != null && uhidKeyboard.key(action == ACTION_DOWN, keyCode)) {
            return;
        }
        long eventTime = now();
        long downTime;
        if (action == ACTION_DOWN) {
            downTime = keyDownTimes.containsKey(keyCode) ? keyDownTimes.get(keyCode) : eventTime;
            keyDownTimes.put(keyCode, downTime);
        } else {
            downTime = keyDownTimes.containsKey(keyCode) ? keyDownTimes.remove(keyCode) : eventTime;
        }
        Object event = keyEventConstructor.newInstance(
                downTime, eventTime, action, keyCode, repeat, metaState,
                VIRTUAL_KEYBOARD, 0, 0, SOURCE_KEYBOARD);
        injectRequired(event);
    }

    private Object motionEvent(int action, int actionButton, int buttons,
                               float x, float y, float hScroll, float vScroll) throws Exception {
        long eventTime = now();
        long downTime = mouseDownTime == 0 ? eventTime : mouseDownTime;

        Object properties = pointerPropertiesConstructor.newInstance();
        pointerPropertiesId.setInt(properties, 0);
        pointerPropertiesToolType.setInt(properties, TOOL_TYPE_MOUSE);
        Object propertiesArray = Array.newInstance(pointerPropertiesClass, 1);
        Array.set(propertiesArray, 0, properties);

        Object coords = pointerCoordsConstructor.newInstance();
        pointerCoordsX.setFloat(coords, x);
        pointerCoordsY.setFloat(coords, y);
        if (action == ACTION_SCROLL) {
            pointerCoordsSetAxisValue.invoke(coords, AXIS_HSCROLL, hScroll);
            pointerCoordsSetAxisValue.invoke(coords, AXIS_VSCROLL, vScroll);
        }
        Object coordsArray = Array.newInstance(pointerCoordsClass, 1);
        Array.set(coordsArray, 0, coords);

        Object event = motionEventObtain.invoke(
                null, downTime, eventTime, action, 1, propertiesArray, coordsArray,
                0, buttons, 1.0f, 1.0f, 0, 0, SOURCE_MOUSE, 0);
        if (actionButton != 0) {
            motionEventSetActionButton.invoke(event, actionButton);
        }
        return event;
    }

    private void move(float x, float y) throws Exception {
        pointerX = x;
        pointerY = y;
        injectRequired(motionEvent(ACTION_MOVE, 0, mouseButtons, x, y, 0, 0));
    }

    private void relativeMove(int dx, int dy) throws Exception {
        if (uhidMouse != null) {
            uhidMouse.move(dx, dy);
            return;
        }
        move(pointerX + dx, pointerY + dy);
    }

    private void button(boolean down, int actionButton, float x, float y) throws Exception {
        pointerX = x;
        pointerY = y;
        if (uhidMouse != null) {
            uhidMouse.button(down, actionButton);
            return;
        }
        if (down) {
            int newButtons = mouseButtons | actionButton;
            if (mouseButtons == 0) {
                mouseDownTime = now();
                injectRequired(motionEvent(ACTION_DOWN, 0, newButtons, x, y, 0, 0));
            }
            mouseButtons = newButtons;
            injectRequired(motionEvent(ACTION_BUTTON_PRESS, actionButton, mouseButtons, x, y, 0, 0));
        } else {
            int newButtons = mouseButtons & ~actionButton;
            injectRequired(motionEvent(ACTION_BUTTON_RELEASE, actionButton, newButtons, x, y, 0, 0));
            mouseButtons = newButtons;
            if (mouseButtons == 0) {
                injectRequired(motionEvent(ACTION_UP, 0, 0, x, y, 0, 0));
                mouseDownTime = 0;
            }
        }
    }

    private void scroll(float horizontal, float vertical, float x, float y) throws Exception {
        pointerX = x;
        pointerY = y;
        if (uhidMouse != null) {
            uhidMouse.scroll(horizontal, vertical);
            return;
        }
        injectRequired(motionEvent(ACTION_SCROLL, 0, mouseButtons, x, y, horizontal, vertical));
    }

    private void run() throws Exception {
        System.out.println("READY " + displayId
                + " MOUSE_" + (uhidMouse == null ? "SDK" : "UHID")
                + " KEYBOARD_" + (uhidKeyboard == null ? "SDK" : "UHID"));
        System.out.flush();
        BufferedReader reader = new BufferedReader(
                new InputStreamReader(System.in, StandardCharsets.UTF_8));
        String line;
        while ((line = reader.readLine()) != null) {
            String[] fields = line.trim().split(" +");
            if (fields.length == 0 || fields[0].isEmpty()) {
                continue;
            }
            try {
                switch (fields[0]) {
                case "K":
                    key(Integer.parseInt(fields[1]), Integer.parseInt(fields[2]),
                            Integer.parseInt(fields[3]), Integer.parseInt(fields[4]));
                    break;
                case "M":
                    move(Float.parseFloat(fields[1]), Float.parseFloat(fields[2]));
                    break;
                case "R":
                    relativeMove(Integer.parseInt(fields[1]), Integer.parseInt(fields[2]));
                    break;
                case "B":
                    button(Integer.parseInt(fields[1]) != 0, Integer.parseInt(fields[2]),
                            Float.parseFloat(fields[3]), Float.parseFloat(fields[4]));
                    break;
                case "S":
                    scroll(Float.parseFloat(fields[1]), Float.parseFloat(fields[2]),
                            Float.parseFloat(fields[3]), Float.parseFloat(fields[4]));
                    break;
                case "Q":
                    return;
                default:
                    System.err.println("Unknown Deskflow input command: " + fields[0]);
                    break;
                }
            } catch (Exception error) {
                System.err.println("Deskflow input command failed: " + error);
                error.printStackTrace(System.err);
            }
        }
    }

    public static void main(String[] args) {
        InputBridge bridge = null;
        int exitCode = 0;
        try {
            int displayId = args.length == 0 ? 0 : Integer.parseInt(args[0]);
            boolean enableUhidMouse = args.length < 2 || args[1].equalsIgnoreCase("uhid");
            boolean enableUhidKeyboard = args.length < 3 || args[2].equalsIgnoreCase("uhid");
            bridge = new InputBridge(displayId, enableUhidMouse, enableUhidKeyboard);
            bridge.run();
        } catch (Exception error) {
            System.err.println("Deskflow input bridge failed: " + error);
            error.printStackTrace(System.err);
            exitCode = 1;
        } finally {
            if (bridge != null && bridge.uhidKeyboard != null) {
                try {
                    bridge.uhidKeyboard.close();
                } catch (Exception error) {
                    System.err.println("Failed to close Deskflow UHID keyboard: " + error);
                }
            }
            if (bridge != null && bridge.uhidMouse != null) {
                try {
                    bridge.uhidMouse.close();
                } catch (Exception error) {
                    System.err.println("Failed to close Deskflow UHID mouse: " + error);
                }
            }
        }
        if (exitCode != 0) {
            System.exit(exitCode);
        }
    }
}
