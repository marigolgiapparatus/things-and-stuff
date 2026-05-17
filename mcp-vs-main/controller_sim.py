#!/usr/bin/env python3
"""
Keyboard controller simulator for the MXEN2003 rover.

Connects to an XBee USB adapter and sends framed control packets that match
the protocol used by the physical controller board (serial2_write_bytes).

Hardware required:
    An XBee USB adapter (e.g. SparkFun XBee Explorer USB) configured on the
    same PAN ID / channel as the rover's XBee module. The adapter appears as
    a virtual COM port — pass that port as the argument below.

Install dependencies:
    pip install pyserial pynput

Usage:
    python3 controller_sim.py /dev/cu.usbserial-XXXX

Controls:
    W / S        Forward / Backward
    A / D        Turn left / right
    Up / Down    Gripper open / close
    Q            Quit
"""

import sys
import time
import threading
import curses
import serial
from pynput import keyboard as kb

BAUD_RATE     = 38400   # XBee default; pass 115200 as 2nd arg when using USB_SIM mode
SEND_INTERVAL = 0.05    # 50 ms, matches controller timing

NEUTRAL = 126
STEP    = 96            # how far from neutral for full input

# Shared state — all access guarded by lock
pressed = set()
sensors = [0, 0, 0]    # front, right, left raw ADC values (0-253)
running = True
lock    = threading.Lock()


# ── Packet helpers ────────────────────────────────────────────────────────────

def make_packet(x: int, y: int, g: int) -> bytes:
    """Build a framed 3-byte packet matching serial2_write_bytes(3, x, y, g)."""
    return bytes([0xFF, 3, x, y, g, 0xFE])


def raw_to_cm(raw: int) -> int:
    """Convert raw ADC byte to cm using the same formula as Controller.c."""
    voltage = raw * 19          # 5000/256 = 19 in integer arithmetic (matches C)
    if voltage <= 292:
        return 999              # out of range sentinel
    return 18900 // (voltage - 292)


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


def get_control() -> tuple[int, int, int]:
    """Return (ctrl_x, ctrl_y, ctrl_g) bytes to send, based on held keys."""
    with lock:
        keys = set(pressed)

    # controller xVal drives forward/back: low = forward, high = backward
    ctrl_x = NEUTRAL
    if 'w' in keys: ctrl_x -= STEP
    if 's' in keys: ctrl_x += STEP

    # controller yVal drives left/right: low = left, high = right
    ctrl_y = NEUTRAL
    if 'a' in keys: ctrl_y -= STEP
    if 'd' in keys: ctrl_y += STEP

    # second joystick y drives gripper servo
    ctrl_g = NEUTRAL
    if kb.Key.up   in keys: ctrl_g -= STEP
    if kb.Key.down in keys: ctrl_g += STEP

    def clamp(v: int) -> int:
        return max(0, min(252, v)) & 0xFD   # mask matches controller & 0xFD

    return clamp(ctrl_x), clamp(ctrl_y), clamp(ctrl_g)


# ── Serial reader thread ──────────────────────────────────────────────────────

def serial_reader(ser: serial.Serial):
    global running
    buf = bytearray()
    while running:
        try:
            chunk = ser.read(ser.in_waiting or 1)
            buf.extend(chunk)

            # Parse framed packets: 0xFF  numBytes  d0..dN  0xFE
            while True:
                start = buf.find(0xFF)
                if start == -1:
                    buf.clear()
                    break
                buf = buf[start:]           # discard anything before start byte
                if len(buf) < 2:
                    break
                num = buf[1]
                pkt_len = 2 + num + 1       # 0xFF + count + data bytes + 0xFE
                if len(buf) < pkt_len:
                    break                   # wait for more data
                if buf[pkt_len - 1] == 0xFE:
                    data = list(buf[2:2 + num])
                    if num >= 3:
                        with lock:
                            sensors[:3] = data[:3]
                    buf = buf[pkt_len:]
                else:
                    buf = buf[1:]           # bad packet, skip start byte and retry
        except Exception:
            running = False
            break
        time.sleep(0.005)


# ── Display ───────────────────────────────────────────────────────────────────

def direction_arrow(keys) -> str:
    fwd  = 'w' in keys
    back = 's' in keys
    lft  = 'a' in keys
    rgt  = 'd' in keys
    if   fwd  and lft:  return 'NW'
    elif fwd  and rgt:  return 'NE'
    elif back and lft:  return 'SW'
    elif back and rgt:  return 'SE'
    elif fwd:           return 'FWD'
    elif back:          return 'REV'
    elif lft:           return 'LEFT'
    elif rgt:           return 'RIGHT'
    return 'STOPPED'


def draw(stdscr, cx: int, cy: int, cg: int, connected: bool):
    with lock:
        keys = set(pressed)
        s    = list(sensors)

    g_up = kb.Key.up   in keys
    g_dn = kb.Key.down in keys

    direction   = direction_arrow(keys)
    gripper_str = 'OPEN' if g_up else 'CLOSE' if g_dn else 'HOLD'
    status_str  = 'CONNECTED' if connected else 'ERROR - check port'

    front_cm = raw_to_cm(s[0])
    right_cm = raw_to_cm(s[1])
    left_cm  = raw_to_cm(s[2])

    cm_fmt = lambda v: f"{v:3d}" if v < 999 else "---"

    stdscr.erase()
    row = 0
    def line(text):
        nonlocal row
        stdscr.addstr(row, 0, text)
        row += 1

    line("┌─────────────────────────────────────┐")
    line("│      ROVER KEYBOARD CONTROLLER      │")
    line(f"│  Status  : {status_str:<26}│")
    line("├─────────────────────────────────────┤")
    line(f"│  Drive   : {direction:<26}│")
    line(f"│  Gripper : {gripper_str:<26}│")
    line("├─────────────────────────────────────┤")
    line(f"│  TX  x={cx:3d}  y={cy:3d}  gripper={cg:3d}      │")
    line(f"│  Sensors (raw)  F={s[0]:3d}  R={s[1]:3d}  L={s[2]:3d}  │")
    line(f"│  Sensors  (cm)  F={cm_fmt(front_cm)}  R={cm_fmt(right_cm)}  L={cm_fmt(left_cm)}  │")
    line("├─────────────────────────────────────┤")
    line("│  W/S = Drive    A/D = Turn          │")
    line("│  ↑/↓ = Gripper  Q   = Quit          │")
    line("└─────────────────────────────────────┘")
    stdscr.refresh()


# ── Main ──────────────────────────────────────────────────────────────────────

def main(stdscr):
    global running

    if len(sys.argv) < 2:
        stdscr.addstr(0, 0, "Usage: python3 controller_sim.py <port> [baud]")
        stdscr.addstr(1, 0, "")
        stdscr.addstr(2, 0, "XBee adapter (default):  /dev/cu.usbserial-XXXX")
        stdscr.addstr(3, 0, "USB_SIM mode:            /dev/cu.usbserial-XXXX 115200")
        stdscr.addstr(4, 0, "Windows:                 COM3  or  COM3 115200")
        stdscr.addstr(5, 0, "")
        stdscr.addstr(6, 0, "Press any key to exit.")
        stdscr.nodelay(False)
        stdscr.getch()
        return

    port = sys.argv[1]
    if len(sys.argv) >= 3:
        BAUD_RATE = int(sys.argv[2])
    curses.curs_set(0)
    stdscr.nodelay(True)

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0)
        connected = True
    except serial.SerialException as e:
        stdscr.addstr(0, 0, f"Could not open {port}:")
        stdscr.addstr(1, 0, str(e))
        stdscr.addstr(2, 0, "Press any key to exit.")
        stdscr.nodelay(False)
        stdscr.getch()
        return

    reader = threading.Thread(target=serial_reader, args=(ser,), daemon=True)
    reader.start()

    listener = kb.Listener(on_press=on_press, on_release=on_release)
    listener.start()

    last_send = 0.0

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

        draw(stdscr, cx, cy, cg, connected)
        time.sleep(0.02)

    running = False
    listener.stop()
    ser.close()


if __name__ == '__main__':
    curses.wrapper(main)
