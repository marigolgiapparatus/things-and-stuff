#!/usr/bin/env python3
"""
Keyboard controller simulator for the MXEN2003 rover.

Connects to an XBee USB adapter or directly via USB (USB_SIM mode) and sends
framed control packets matching the physical controller board protocol.

Install dependencies:
    pip install pyserial pynput

Usage:
    XBee adapter:  python3 controller_sim.py COM3
    USB_SIM mode:  python3 controller_sim.py COM7 115200

Controls:
    W / S        Forward / Backward
    A / D        Turn left / right
    Up / Down    Gripper open / close
    Q            Quit
"""

import sys
import time
import threading
import serial
from pynput import keyboard as kb

BAUD_RATE     = int(sys.argv[2]) if len(sys.argv) >= 3 else 38400
SEND_INTERVAL = 0.05    # 50 ms, matches controller timing

NEUTRAL = 126
STEP    = 96

pressed = set()
sensors = [0, 0, 0]
running = True
lock    = threading.Lock()


# ── Packet helpers ────────────────────────────────────────────────────────────

def make_packet(x: int, y: int, g: int) -> bytes:
    return bytes([0xFF, 3, x, y, g, 0xFE])


# ── Keyboard listener ─────────────────────────────────────────────────────────

def on_press(key):
    with lock:
        try:
            pressed.add(key.char.lower())
        except AttributeError:
            pressed.add(key)


def on_release(key):
    with lock:
        try:
            pressed.discard(key.char.lower())
        except AttributeError:
            pressed.discard(key)


def get_control() -> tuple:
    with lock:
        keys = set(pressed)

    ctrl_x = NEUTRAL
    if 'w' in keys: ctrl_x -= STEP
    if 's' in keys: ctrl_x += STEP

    ctrl_y = NEUTRAL
    if 'a' in keys: ctrl_y -= STEP
    if 'd' in keys: ctrl_y += STEP

    ctrl_g = NEUTRAL
    if kb.Key.up   in keys: ctrl_g -= STEP
    if kb.Key.down in keys: ctrl_g += STEP

    def clamp(v):
        return max(0, min(252, v)) & 0xFD

    return clamp(ctrl_x), clamp(ctrl_y), clamp(ctrl_g)


# ── Serial reader thread ──────────────────────────────────────────────────────

def serial_reader(ser):
    global running
    buf = bytearray()
    while running:
        try:
            chunk = ser.read(ser.in_waiting or 1)
            buf.extend(chunk)
            while True:
                start = buf.find(0xFF)
                if start == -1:
                    buf.clear()
                    break
                buf = buf[start:]
                if len(buf) < 2:
                    break
                num = buf[1]
                pkt_len = 2 + num + 1
                if len(buf) < pkt_len:
                    break
                if buf[pkt_len - 1] == 0xFE:
                    data = list(buf[2:2 + num])
                    if num >= 3:
                        with lock:
                            sensors[:3] = data[:3]
                    buf = buf[pkt_len:]
                else:
                    buf = buf[1:]
        except Exception:
            running = False
            break
        time.sleep(0.005)


# ── Display ───────────────────────────────────────────────────────────────────

def draw(cx: int, cy: int, cg: int, connected: bool):
    with lock:
        keys = set(pressed)
        s    = list(sensors)

    fwd  = 'w' in keys
    back = 's' in keys
    lft  = 'a' in keys
    rgt  = 'd' in keys
    g_up = kb.Key.up   in keys
    g_dn = kb.Key.down in keys

    if   fwd  and lft:  direction = 'NW'
    elif fwd  and rgt:  direction = 'NE'
    elif back and lft:  direction = 'SW'
    elif back and rgt:  direction = 'SE'
    elif fwd:           direction = 'FWD'
    elif back:          direction = 'REV'
    elif lft:           direction = 'LEFT'
    elif rgt:           direction = 'RIGHT'
    else:               direction = 'STOPPED'

    gripper_str = 'OPEN' if g_up else 'CLOSE' if g_dn else 'HOLD'
    status_str  = 'CONNECTED' if connected else 'ERROR - check port'

    lines = [
        "====== ROVER KEYBOARD CONTROLLER ======",
        f"  Status  : {status_str}",
        "---------------------------------------",
        f"  Drive   : {direction}",
        f"  Gripper : {gripper_str}",
        "---------------------------------------",
        f"  TX  x={cx:3d}  y={cy:3d}  gripper={cg:3d}",
        f"  Sensors (cm)  F={s[0]:3d}  R={s[1]:3d}  L={s[2]:3d}",
        "---------------------------------------",
        "  W/S = Drive    A/D = Turn",
        "  Up/Down = Gripper    Q = Quit",
        "=======================================",
    ]
    # jump cursor to top-left and overwrite — no cls flicker
    sys.stdout.write('\033[H')
    sys.stdout.write('\n'.join(f'{l:<40}' for l in lines) + '\n')
    sys.stdout.flush()


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    global running

    if len(sys.argv) < 2:
        print("Usage: python3 controller_sim.py <port> [baud]")
        print("")
        print("  XBee adapter:  python3 controller_sim.py /dev/cu.usbserial-XXXX")
        print("  USB_SIM mode:  python3 controller_sim.py COM7 115200")
        sys.exit(1)

    port = sys.argv[1]

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0)
        connected = True
        print(f"Opened {port} at {BAUD_RATE} baud")
    except serial.SerialException as e:
        print(f"Could not open {port}: {e}")
        sys.exit(1)

    sys.stdout.write('\033[2J')   # clear screen once on startup
    sys.stdout.flush()

    reader = threading.Thread(target=serial_reader, args=(ser,), daemon=True)
    reader.start()

    listener = kb.Listener(on_press=on_press, on_release=on_release)
    listener.start()

    last_send = 0.0
    last_draw = 0.0

    while running:
        with lock:
            if 'q' in pressed:
                break

        now = time.time()
        cx, cy, cg = get_control()

        if now - last_send >= SEND_INTERVAL:
            try:
                ser.write(make_packet(cx, cy, cg))
            except serial.SerialException:
                connected = False
                running = False
            last_send = now

        if now - last_draw >= 0.1:     # redraw at 10 Hz to avoid flicker
            draw(cx, cy, cg, connected)
            last_draw = now

        time.sleep(0.01)

    running = False
    listener.stop()
    ser.close()


if __name__ == '__main__':
    main()
