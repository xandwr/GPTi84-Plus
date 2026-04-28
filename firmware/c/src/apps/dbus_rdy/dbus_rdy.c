// DBUS byte-level round-trip via the OS "ready check" packet.
//
// Goal: prove a sculpted byte survives the round trip in both directions
// against a real TI-84+ on the home screen, no calc-side code required.
//
// Packet (linkguide ti83+/packet.html):
//   PC  -> calc: MID=0x73 CID=0x68 LEN=0x0000  (ready check)
//   calc -> PC : MID=0x73 CID=0x56 LEN=0x0000  (ack)
//
// Each byte is 8 bit-handshakes, LSB first. Per linkguide hardware.html:
//   bit=0: sender pulls TIP (D0/red) low first; receiver acks by pulling
//          RING (D1/white) low. Sender releases TIP. Receiver releases RING.
//   bit=1: same with TIP/RING swapped.
//
// Wiring: TIP/red = GP6, RING/white = GP7, GND through sleeve.
// Bit value is defined by which wire drops first: TIP first = 0, RING first = 1.

#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"

#define TIP_PIN 6   // red, encodes bit=0 when leading
#define RING_PIN 7  // white, encodes bit=1 when leading

#define BIT_TIMEOUT_US 100000  // 100 ms per half-bit edge wait
#define SETTLE_US 0            // ArTICL on 16MHz AVR has no settle and works.
                               // Try 0 first; if calc misses bits, raise.

typedef enum {
    DBUS_OK = 0,
    DBUS_TIMEOUT_ACK,      // calc never pulled the ack line low
    DBUS_TIMEOUT_RELEASE,  // calc never released its ack line
    DBUS_TIMEOUT_BIT,      // no bit arrived (rx idle past timeout)
} dbus_err;

// Pin model: open-drain. We pre-write 0 to the pad, then flip direction
// to "assert" (drive low) or "release" (Hi-Z, pull-up wins). Never drive high.
static void pin_setup(uint pin) {
    gpio_init(pin);
    gpio_put(pin, 0);
    gpio_pull_up(pin);
    gpio_set_dir(pin, GPIO_IN);
}
static inline void line_assert(uint pin) { gpio_set_dir(pin, GPIO_OUT); }
static inline void line_release(uint pin) { gpio_set_dir(pin, GPIO_IN); }
static inline bool line_is_low(uint pin) { return !gpio_get(pin); }

// Spin until pin reaches `want_low` or timeout. Returns true on success.
static bool wait_for(uint pin, bool want_low, uint32_t timeout_us) {
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        if (line_is_low(pin) == want_low) return true;
    }
    return false;
}

// Send one bit. bit=0 -> TIP leads; bit=1 -> RING leads. (Polarity verified
// 2026-04-28: inverting it breaks 0xff without fixing anything else.)
static dbus_err put_bit(int bit) {
    uint lead = bit ? RING_PIN : TIP_PIN;
    uint ack = bit ? TIP_PIN : RING_PIN;

    // Both lines must be idle (high) before we start. If they aren't, the
    // calc is mid-something or we got out of sync; let the caller see it.
    if (!wait_for(TIP_PIN, false, BIT_TIMEOUT_US)) return DBUS_TIMEOUT_RELEASE;
    if (!wait_for(RING_PIN, false, BIT_TIMEOUT_US)) return DBUS_TIMEOUT_RELEASE;

    line_assert(lead);                                       // 1. drive lead low
    if (!wait_for(ack, true, BIT_TIMEOUT_US)) {              // 2. wait for calc ack
        line_release(lead);
        return DBUS_TIMEOUT_ACK;
    }
    busy_wait_us(SETTLE_US);                                 // let calc ISR latch the bit
    line_release(lead);                                      // 3. release lead
    if (!wait_for(ack, false, BIT_TIMEOUT_US)) {             // 4. wait for calc release
        return DBUS_TIMEOUT_RELEASE;
    }
    busy_wait_us(SETTLE_US);                                 // let bus settle before next bit
    return DBUS_OK;
}

static dbus_err put_byte(uint8_t b) {
    for (int i = 0; i < 8; i++) {
        dbus_err e = put_bit(b & 1);
        if (e != DBUS_OK) return e;
        b >>= 1;
    }
    return DBUS_OK;
}

// Receive one bit. Watch which line drops first; that line's identity is
// the bit value. Then ack on the other line and wait for the calc to release.
static dbus_err get_bit(int *out_bit, uint32_t idle_timeout_us) {
    // Wait for *either* line to drop. Long timeout because between bytes the
    // calc may pause, but within a byte it should be microseconds.
    absolute_time_t deadline = make_timeout_time_us(idle_timeout_us);
    int bit = -1;
    while (absolute_time_diff_us(get_absolute_time(), deadline) > 0) {
        bool tip = line_is_low(TIP_PIN);
        bool ring = line_is_low(RING_PIN);
        if (tip && !ring) { bit = 0; break; }   // TIP led -> bit 0
        if (ring && !tip) { bit = 1; break; }   // RING led -> bit 1
        // Both low simultaneously would be the abort signal; treat as timeout
        // for now and let the caller diagnose.
        if (tip && ring) return DBUS_TIMEOUT_BIT;
    }
    if (bit < 0) return DBUS_TIMEOUT_BIT;

    uint ack = bit ? TIP_PIN : RING_PIN;  // ack on the OTHER line
    uint led = bit ? RING_PIN : TIP_PIN;  // the line the calc drove

    busy_wait_us(SETTLE_US);                                 // let lead drive settle
    line_assert(ack);                                        // 1. ack
    if (!wait_for(led, false, BIT_TIMEOUT_US)) {             // 2. calc releases its line
        line_release(ack);
        return DBUS_TIMEOUT_RELEASE;
    }
    line_release(ack);                                       // 3. we release ack
    // Wait for our just-released ack line to rise back to high before
    // returning. Otherwise the next get_bit() call samples our own
    // still-low ack release as the calc's lead, producing a phantom bit.
    if (!wait_for(ack, false, BIT_TIMEOUT_US)) {
        return DBUS_TIMEOUT_RELEASE;
    }
    *out_bit = bit;
    return DBUS_OK;
}

static dbus_err get_byte(uint8_t *out, uint32_t first_bit_timeout_us) {
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        int bit;
        // Generous wait on bit 0 (calc may be thinking), tight inside a byte.
        uint32_t to = (i == 0) ? first_bit_timeout_us : BIT_TIMEOUT_US;
        dbus_err e = get_bit(&bit, to);
        if (e != DBUS_OK) return e;
        b |= ((uint8_t)bit) << i;
    }
    *out = b;
    return DBUS_OK;
}

static const char *err_name(dbus_err e) {
    switch (e) {
        case DBUS_OK: return "OK";
        case DBUS_TIMEOUT_ACK: return "TIMEOUT_ACK";
        case DBUS_TIMEOUT_RELEASE: return "TIMEOUT_RELEASE";
        case DBUS_TIMEOUT_BIT: return "TIMEOUT_BIT";
    }
    return "?";
}

// Send one byte, read one byte. ECHO running on calc should round-trip
// each byte byte-for-byte. Cycling test bytes lets us see encoding bugs
// (bit reversal, polarity inversion, off-by-one framing) by inspection.
static void echo_one(uint8_t tx) {
    absolute_time_t t0 = get_absolute_time();
    dbus_err e = put_byte(tx);
    int tx_us = (int)absolute_time_diff_us(t0, get_absolute_time());
    if (e != DBUS_OK) {
        printf("TX %02x -> %s (%dus)\n", tx, err_name(e), tx_us);
        return;
    }

    uint8_t rx = 0;
    t0 = get_absolute_time();
    e = get_byte(&rx, 500000);  // generous: ECHO loop has bcall overhead
    int rx_us = (int)absolute_time_diff_us(t0, get_absolute_time());

    printf("TX %02x (%dus) -> RX %02x (%s,%dus) %s\n",
           tx, tx_us, rx, err_name(e), rx_us,
           (e == DBUS_OK && rx == tx) ? "MATCH" : "");
}

static void run_once(void) {
    // Hammer 0xff three times: known-working pattern. If all three echo, the
    // wire is healthy. If they don't, it's a transient failure not byte value.
    echo_one(0xFF);
    echo_one(0xFF);
    echo_one(0xFF);
    // Then probe one specific other value. Vary across runs to map which
    // bytes work.
    echo_one(0x55);
}

int main(void) {
    stdio_init_all();
    pin_setup(TIP_PIN);
    pin_setup(RING_PIN);

    for (int i = 5; i > 0; i--) {
        printf("waiting for monitor... %d\n", i);
        sleep_ms(1000);
    }
    printf("\nDBUS ready-check round-trip  TIP=GP%d  RING=GP%d\n", TIP_PIN, RING_PIN);
    printf("================================================\n\n");

    while (true) {
        run_once();
        printf("---\n");
        sleep_ms(2000);
    }
}
