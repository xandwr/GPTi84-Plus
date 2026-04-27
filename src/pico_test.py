# Pico W bring-up validation.
#
# Wiring:
#   Single jumper between GP2 (physical pin 4) and GP3 (physical pin 5).
#   GP2 is driven, GP3 is read.
#
# Run with:
#   mpremote run src/pico_test.py
#
# Each check exits the process on failure so output makes the failing
# claim obvious.

import sys
import time
from machine import Pin
import rp2


def fail(msg):
    print("FAIL:", msg)
    sys.exit(1)


# 1. CPU GPIO: drive GP2, read GP3. Confirms the jumper and basic Pin API.
out = Pin(2, Pin.OUT)
inp = Pin(3, Pin.IN)

out.value(0)
time.sleep_ms(2)
if inp.value() != 0:
    fail("GP2 driven LOW but GP3 reads HIGH (jumper or pin map wrong)")

out.value(1)
time.sleep_ms(2)
if inp.value() != 1:
    fail("GP2 driven HIGH but GP3 reads LOW (jumper or pin map wrong)")

print("PASS: CPU GPIO drives GP2 and GP3 follows it.")


# 2. PIO can claim a pin and hold it. Confirms PIO programs assemble,
#    load, and execute.
@rp2.asm_pio(set_init=rp2.PIO.OUT_LOW)
def hold_high():
    set(pins, 1)
    wrap_target()
    nop()
    wrap()


sm = rp2.StateMachine(0, hold_high, freq=1_000_000, set_base=Pin(2))
sm.active(1)
time.sleep_ms(5)
held = inp.value()
sm.active(0)
if held != 1:
    fail("PIO program ran but did not drive GP2 HIGH")

print("PASS: PIO program drives a pin independently of CPU.")


# 3. PIO shifts FIFO data onto a pin at a configured bit rate.
#    Sampling a fast stream from CPU-side Python is unreliable due to
#    Python call latency, so this test verifies the shift by counting
#    edges over a known number of bytes.
#
#    Pattern is alternating 0xAA bytes -> exactly 8 edges per byte
#    (every bit flips). With N bytes at BIT_HZ, we expect 8*N edges
#    in (8*N / BIT_HZ) seconds.

BIT_HZ = 2_000  # slow enough for Python edge-counting to keep up
N_BYTES = 4
EXPECTED_EDGES = 8 * N_BYTES - 1  # N transitions between N+1 levels


@rp2.asm_pio(
    out_init=rp2.PIO.OUT_LOW,
    autopull=True,
    pull_thresh=8,
    out_shiftdir=rp2.PIO.SHIFT_RIGHT,
)
def shift_out():
    wrap_target()
    out(pins, 1)        [1]
    wrap()


sm = rp2.StateMachine(0, shift_out, freq=BIT_HZ * 2, out_base=Pin(2))

for _ in range(N_BYTES):
    sm.put(0xAA)

edges = 0
last = inp.value()
sm.active(1)
deadline = time.ticks_add(time.ticks_ms(), (1000 * 8 * N_BYTES) // BIT_HZ + 50)
while time.ticks_diff(deadline, time.ticks_ms()) > 0:
    v = inp.value()
    if v != last:
        edges += 1
        last = v
sm.active(0)

# Allow some slop -- Python sampling will miss edges if too fast,
# but at 2 kHz bit rate we should catch nearly all of them.
if edges < EXPECTED_EDGES // 2:
    fail("PIO FIFO shift produced {} edges, expected ~{}".format(edges, EXPECTED_EDGES))

print("PASS: PIO shifts FIFO bytes onto pin (counted {} edges, expected ~{}).".format(
    edges, EXPECTED_EDGES))


# 4. WiFi radio is functional. Activates STA mode, scans, deactivates.
import network

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
mac = wlan.config('mac').hex()
aps = wlan.scan()
wlan.active(False)

if not mac or mac == '0' * 12:
    fail("WiFi MAC reads as zero -- CYW43 driver not initialized")

print("PASS: WiFi radio active, MAC {}, {} APs visible.".format(mac, len(aps)))

print()
print("All checks passed. Pico W is ready for DBUS bring-up.")
