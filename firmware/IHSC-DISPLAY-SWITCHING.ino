"""
=============================================================
  display_switch.py — Output Mode Switcher & Monitor
  PC-side utility for the XIAO RP2040 Thermal Camera
=============================================================

OVERVIEW
--------
This script lets you switch the RP2040's output port between:
  • USB-CDC   — frames streamed to PC over Type-C cable
  • I2C-DISP  — frames rendered on 4-pin I2C connector display
  • BOTH       — USB-CDC stream AND I2C display simultaneously

Switching is done by writing a 1-byte command into the saved
config file (output_mode.json) on the RP2040's flash via the
USB-CDC serial connection. The RP2040 reads this on the next
boot, or you can trigger a soft-reset remotely.

Requirements
------------
  pip install pyserial

Usage
-----
  # Show current status
  python display_switch.py --port COM3 --status

  # Switch to I2C display output (4-pin connector)
  python display_switch.py --port COM3 --set i2c

  # Switch to USB-CDC output
  python display_switch.py --port COM3 --set usb

  # Stream to BOTH simultaneously
  python display_switch.py --port COM3 --set both

  # Monitor live frames and print FPS / temperature
  python display_switch.py --port COM3 --monitor

  # Auto-detect port + switch + reset in one step
  python display_switch.py --set i2c --reset

  # Auto-detect port (no --port needed if RP2040 is the only CDC device)
  python display_switch.py --status

Port examples
-------------
  Windows  : COM3, COM4, …
  Linux    : /dev/ttyACM0, /dev/ttyACM1
  macOS    : /dev/tty.usbmodem14201

=============================================================
"""

import argparse
import json
import struct
import sys
import time
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("[ERROR] pyserial not installed. Run:  pip install pyserial")
    sys.exit(1)

# ──────────────────────────────────────────────
#  Mode constants — must match main.py
# ──────────────────────────────────────────────
MODE_USB  = 0
MODE_I2C  = 1
MODE_BOTH = 2
MODE_NAMES = {
    MODE_USB:  "USB-CDC",
    MODE_I2C:  "I2C-DISPLAY",
    MODE_BOTH: "BOTH",
}
MODE_KEYS = {
    "usb":  MODE_USB,
    "i2c":  MODE_I2C,
    "both": MODE_BOTH,
    "0":    MODE_USB,
    "1":    MODE_I2C,
    "2":    MODE_BOTH,
}

# ──────────────────────────────────────────────
#  Frame protocol — must match main.py
# ──────────────────────────────────────────────
MAGIC        = 0xDEADBEEF
FRAME_WIDTH  = 32
FRAME_HEIGHT = 24
N_PIXELS     = FRAME_WIDTH * FRAME_HEIGHT
HEADER_FMT   = "<IHHIBxxxii"
HEADER_SIZE  = struct.calcsize(HEADER_FMT)    # 24 bytes
PIXEL_BYTES  = N_PIXELS * 2                   # 1536 bytes
FOOTER_SIZE  = 2
FRAME_SIZE   = HEADER_SIZE + PIXEL_BYTES + FOOTER_SIZE   # 1562 bytes

CONFIG_FILE  = "output_mode.json"


# ══════════════════════════════════════════════
#  Port auto-detection
# ══════════════════════════════════════════════
def find_rp2040_port():
    """Scan serial ports and return the most likely RP2040 CDC port."""
    candidates = []
    for p in serial.tools.list_ports.comports():
        desc = (p.description or "").lower()
        mfr  = (p.manufacturer or "").lower()
        if any(k in desc + mfr for k in
               ("xiao", "rp2040", "raspberry pi", "micropython", "pico", "seeed")):
            candidates.insert(0, p.device)   # prefer exact match at front
        elif "acm" in p.device.lower() or "usbmodem" in p.device.lower():
            candidates.append(p.device)      # generic ACM fallback

    if candidates:
        print(f"[AUTO] Found RP2040 at: {candidates[0]}")
        return candidates[0]
    return None


def open_port(port_name, baud=115200, timeout=2.0):
    try:
        ser = serial.Serial(port_name, baud, timeout=timeout)
        time.sleep(0.1)   # let CDC settle
        return ser
    except serial.SerialException as e:
        print(f"[ERROR] Cannot open {port_name}: {e}")
        sys.exit(1)


# ══════════════════════════════════════════════
#  MicroPython raw REPL helper
# ══════════════════════════════════════════════
class RawREPL:
    """
    Enter/exit MicroPython raw REPL and execute short Python expressions.
    This lets us read/write files on the RP2040 flash from the PC.
    """

    CTRL_C = b'\x03'
    CTRL_D = b'\x04'
    CTRL_E = b'\x05'   # raw REPL mode

    def __init__(self, ser: serial.Serial):
        self.ser = ser

    def _flush(self):
        time.sleep(0.05)
        self.ser.read(self.ser.in_waiting)

    def enter(self):
        """Interrupt running code and enter raw REPL."""
        self.ser.write(self.CTRL_C)
        time.sleep(0.2)
        self.ser.write(self.CTRL_C)
        time.sleep(0.1)
        self._flush()
        self.ser.write(self.CTRL_E)
        time.sleep(0.1)
        resp = self.ser.read(self.ser.in_waiting)
        if b"raw REPL" not in resp and b">" not in resp:
            # Try once more
            self.ser.write(self.CTRL_E)
            time.sleep(0.2)
        self._flush()

    def exit(self):
        """Exit raw REPL, return to normal REPL."""
        self.ser.write(self.CTRL_D)
        time.sleep(0.2)
        self._flush()

    def exec(self, code: str, timeout=5.0) -> str:
        """
        Execute a Python code string in raw REPL.
        Returns stdout output as a string.
        """
        self._flush()
        # Send code + Ctrl-D to execute
        payload = code.encode() + self.CTRL_D
        self.ser.write(payload)

        # Collect response until ">" prompt or timeout
        t0   = time.time()
        buf  = b""
        while time.time() - t0 < timeout:
            chunk = self.ser.read(self.ser.in_waiting or 1)
            if chunk:
                buf += chunk
            if b"\x04>" in buf:   # OK marker
                break
            time.sleep(0.01)

        # Parse: response is  "OK<stdout>\x04<stderr>\x04>"
        try:
            ok_idx = buf.index(b"OK") + 2
            end    = buf.index(b"\x04", ok_idx)
            return buf[ok_idx:end].decode(errors="replace").strip()
        except (ValueError, IndexError):
            return buf.decode(errors="replace").strip()


# ══════════════════════════════════════════════
#  High-level RP2040 operations
# ══════════════════════════════════════════════
def get_current_mode(repl: RawREPL) -> int | None:
    """Read output_mode.json from the RP2040 flash and return mode int."""
    code = f"""
import json
try:
    f = open('{CONFIG_FILE}')
    d = json.load(f)
    f.close()
    print(d.get('mode', -1))
except:
    print(-1)
"""
    result = repl.exec(code.strip())
    try:
        m = int(result.split()[-1])
        return m if m in (MODE_USB, MODE_I2C, MODE_BOTH) else None
    except (ValueError, IndexError):
        return None


def set_mode_on_device(repl: RawREPL, mode: int) -> bool:
    """Write output_mode.json to RP2040 flash."""
    code = f"""
import json
f = open('{CONFIG_FILE}', 'w')
json.dump({{'mode': {mode}}}, f)
f.close()
print('OK')
"""
    result = repl.exec(code.strip())
    return "OK" in result


def soft_reset(repl: RawREPL):
    """Trigger a soft reset so the new mode takes effect immediately."""
    code = "import machine; machine.soft_reset()"
    repl.ser.write(code.encode() + RawREPL.CTRL_D)
    time.sleep(0.5)


def scan_i2c0(repl: RawREPL) -> list:
    """Scan the 4-pin I2C connector bus and return found addresses."""
    code = """
from machine import I2C, Pin
i2c = I2C(0, sda=Pin(4), scl=Pin(5), freq=400000)
print(i2c.scan())
"""
    result = repl.exec(code.strip())
    try:
        # Result looks like "[60, 61]"
        addrs = eval(result.split('\n')[-1])
        return addrs if isinstance(addrs, list) else []
    except Exception:
        return []


# ══════════════════════════════════════════════
#  Frame monitor (live feed from USB-CDC)
# ══════════════════════════════════════════════
class FrameMonitor:
    def __init__(self, ser: serial.Serial):
        self.ser  = ser
        self._buf = bytearray()

    def _fill(self, n):
        deadline = time.time() + 3.0
        while len(self._buf) < n:
            chunk = self.ser.read(self.ser.in_waiting or 1)
            if chunk:
                self._buf.extend(chunk)
            if time.time() > deadline:
                raise TimeoutError("Frame read timeout")

    def _sync(self):
        magic_bytes = struct.pack("<I", MAGIC)
        for _ in range(FRAME_SIZE * 2):
            self._fill(4)
            if bytes(self._buf[:4]) == magic_bytes:
                return True
            del self._buf[0]
        return False

    def read_one_frame(self):
        """Returns a dict with frame info or None on error."""
        if not self._sync():
            return None
        self._fill(FRAME_SIZE)
        data = bytes(self._buf[:FRAME_SIZE])
        del self._buf[:FRAME_SIZE]

        try:
            magic, w, h, fidx, fmode, t_min_mC, t_max_mC = \
                struct.unpack_from(HEADER_FMT, data, 0)
        except struct.error:
            return None

        if magic != MAGIC:
            return None

        pixel_data = data[HEADER_SIZE: HEADER_SIZE + PIXEL_BYTES]
        ck_recv    = struct.unpack_from("<H", data, HEADER_SIZE + PIXEL_BYTES)[0]
        ck_calc    = sum(pixel_data) & 0xFFFF

        temps = [v / 100.0 - 40.0
                 for v in struct.unpack_from(f"<{N_PIXELS}H", pixel_data)]

        return {
            "frame_idx":  fidx,
            "mode":       fmode,
            "mode_name":  MODE_NAMES.get(fmode, f"?{fmode}"),
            "t_min":      t_min_mC / 1000.0,
            "t_max":      t_max_mC / 1000.0,
            "t_avg":      sum(temps) / len(temps),
            "checksum_ok": ck_calc == ck_recv,
            "temps":      temps,
        }

    def run(self, max_frames=None):
        """
        Print a live summary of incoming frames.
        Press Ctrl-C to stop.
        """
        print("─" * 62)
        print(f"{'Frame':>7}  {'Mode':<12}  {'T-min':>7}  {'T-max':>7}  "
              f"{'T-avg':>7}  {'CK':>4}")
        print("─" * 62)

        t0     = time.time()
        count  = 0
        errors = 0

        try:
            while True:
                frame = self.read_one_frame()
                if frame is None:
                    errors += 1
                    if errors > 10:
                        print("[WARN] Too many frame errors — is device in USB mode?")
                        errors = 0
                    continue

                count += 1
                elapsed = time.time() - t0
                fps     = count / elapsed if elapsed > 0 else 0.0

                print(f"{frame['frame_idx']:>7}  "
                      f"{frame['mode_name']:<12}  "
                      f"{frame['t_min']:>6.1f}C  "
                      f"{frame['t_max']:>6.1f}C  "
                      f"{frame['t_avg']:>6.1f}C  "
                      f"{'OK' if frame['checksum_ok'] else 'BAD':>4}  "
                      f"{fps:>5.1f}fps")

                if max_frames and count >= max_frames:
                    break

        except KeyboardInterrupt:
            print(f"\n[MONITOR] Stopped. {count} frames in {elapsed:.1f}s "
                  f"({count/elapsed:.1f} fps avg)")


# ══════════════════════════════════════════════
#  ASCII status display
# ══════════════════════════════════════════════
def print_status(port_name, current_mode, i2c_devices):
    w = 50
    print("╔" + "═" * w + "╗")
    print(f"║  {'XIAO RP2040 Thermal Camera — Output Status':<{w-2}}║")
    print("╠" + "═" * w + "╣")
    print(f"║  Port        : {port_name:<{w-16}}║")
    mode_str = f"{MODE_NAMES.get(current_mode, 'Unknown')} (mode {current_mode})"
    print(f"║  Output mode : {mode_str:<{w-16}}║")
    print("╠" + "═" * w + "╣")
    print(f"║  4-pin I2C connector (GP4 SDA / GP5 SCL):{'':<{w-43}}║")
    if i2c_devices:
        for addr in i2c_devices:
            tag = ""
            if addr in (0x3C, 0x3D): tag = "← OLED display"
            elif addr == 0x33:        tag = "← MLX90640 (wrong bus!)"
            print(f"║    {hex(addr):<8} {tag:<{w-13}}║")
    else:
        print(f"║    No devices found on 4-pin connector{'':<{w-39}}║")
    print("╠" + "═" * w + "╣")
    print(f"║  Active outputs:{'':<{w-17}}║")
    usb_active  = current_mode in (MODE_USB, MODE_BOTH)
    i2c_active  = current_mode in (MODE_I2C, MODE_BOTH)
    print(f"║    USB-CDC  (Type-C → PC)   : "
          f"{'✓ ACTIVE' if usb_active else '✗ off':<{w-31}}║")
    print(f"║    I2C disp (4-pin header)  : "
          f"{'✓ ACTIVE' if i2c_active else '✗ off':<{w-31}}║")
    print("╚" + "═" * w + "╝")


# ══════════════════════════════════════════════
#  CLI
# ══════════════════════════════════════════════
def main():
    parser = argparse.ArgumentParser(
        description="Switch/monitor output port on XIAO RP2040 Thermal Camera",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python display_switch.py --status
  python display_switch.py --set i2c
  python display_switch.py --set usb --reset
  python display_switch.py --set both --port COM5
  python display_switch.py --monitor
  python display_switch.py --scan-i2c
        """,
    )
    parser.add_argument("--port",   default=None,
                        help="Serial port (e.g. COM3, /dev/ttyACM0). "
                             "Auto-detected if omitted.")
    parser.add_argument("--baud",   type=int, default=115200)
    parser.add_argument("--set",    choices=["usb", "i2c", "both"],
                        metavar="MODE",
                        help="Switch output mode: usb | i2c | both")
    parser.add_argument("--status", action="store_true",
                        help="Show current output mode and I2C bus status")
    parser.add_argument("--monitor", action="store_true",
                        help="Live-print incoming frame info (USB mode only)")
    parser.add_argument("--scan-i2c", action="store_true",
                        help="Scan the 4-pin I2C connector bus for devices")
    parser.add_argument("--reset",  action="store_true",
                        help="Soft-reset the RP2040 after changing mode "
                             "(new mode activates immediately)")
    parser.add_argument("--frames", type=int, default=None,
                        help="Stop --monitor after N frames")
    args = parser.parse_args()

    # Need at least one action
    if not any([args.set, args.status, args.monitor, args.scan_i2c]):
        parser.print_help()
        sys.exit(0)

    # ── Port ──────────────────────────────────
    port_name = args.port or find_rp2040_port()
    if port_name is None:
        print("[ERROR] RP2040 not found. Specify --port explicitly.")
        sys.exit(1)

    ser = open_port(port_name, args.baud)
    print(f"[OPEN] Connected to {port_name}")

    # ── Monitor mode (no REPL needed) ─────────
    if args.monitor:
        print(f"[MONITOR] Listening for frames on {port_name} …  Ctrl-C to stop")
        mon = FrameMonitor(ser)
        mon.run(max_frames=args.frames)
        ser.close()
        return

    # ── Enter raw REPL for all other actions ──
    repl = RawREPL(ser)
    print("[REPL] Entering raw REPL …")
    repl.enter()

    # ── Scan I2C connector ────────────────────
    i2c_devices = []
    if args.scan_i2c or args.status:
        print("[SCAN] Scanning 4-pin I2C connector (GP4/GP5) …")
        i2c_devices = scan_i2c0(repl)
        if i2c_devices:
            print(f"[SCAN] Found: {[hex(a) for a in i2c_devices]}")
        else:
            print("[SCAN] No devices found on 4-pin connector")

    # ── Current mode ──────────────────────────
    current_mode = get_current_mode(repl)
    if current_mode is None:
        print("[INFO] No saved mode on device (will auto-detect on boot)")
        current_mode = -1

    # ── Status ────────────────────────────────
    if args.status:
        print_status(port_name, current_mode, i2c_devices)

    # ── Set mode ──────────────────────────────
    if args.set:
        new_mode = MODE_KEYS[args.set]
        print(f"[SET] Switching: {MODE_NAMES.get(current_mode,'?')} "
              f"→ {MODE_NAMES[new_mode]}")

        if new_mode == MODE_I2C and not i2c_devices:
            print("[WARN] No I2C display detected on 4-pin connector!")
            print("       Check Pin1=3V3, Pin2=GND, Pin3=GP4(SDA), Pin4=GP5(SCL)")
            ans = input("       Continue anyway? [y/N] ").strip().lower()
            if ans != 'y':
                print("[ABORT] Mode not changed.")
                repl.exit()
                ser.close()
                return

        ok = set_mode_on_device(repl, new_mode)
        if ok:
            print(f"[OK] Mode set to: {MODE_NAMES[new_mode]}")
            print(f"     {'Restarting device …' if args.reset else 'Restart the device to apply (or use --reset).'}")
        else:
            print("[ERROR] Failed to write config to device flash.")

        if args.reset:
            print("[RESET] Triggering soft reset …")
            soft_reset(repl)
            time.sleep(1.5)
            print("[RESET] Device restarted. New mode is active.")

    repl.exit()
    ser.close()
    print("[DONE]")


if __name__ == "__main__":
    main() 
