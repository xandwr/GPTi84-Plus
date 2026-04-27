/*
 * link_loopback: simulated Pico <-> TI-84+ link port loopback test.
 *
 * Both sides run in-process:
 *   calc side  - TilEm emulating a TI-84+ running LOOPBACK.8Xp
 *   "Pico" side - this main loop reading/writing graylink bytes
 *
 * The calc program does:
 *   Send({65,66,67})   -- pushes three bytes out over the link port
 *   Get(L1)            -- waits to receive a list back
 *
 * This harness:
 *   1. Boots the calc and sends LOOPBACK.8Xp via the graylink.
 *   2. Runs prgmLOOPBACK from the homescreen.
 *   3. Drains incoming bytes from the calc, printing each one.
 *   4. Echoes them back as a list so Get(L1) completes.
 *   5. Lets the program finish, then dumps the LCD.
 *
 * Usage: link_loopback <rom> <LOOPBACK.8Xp> [sav]
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <ticables.h>
#include <ticalcs.h>
#include <tifiles.h>

#include <tilem.h>
#include <scancodes.h>

/* ------------------------------------------------------------------ */
/* helpers shared with headless.c                                       */
/* ------------------------------------------------------------------ */

static int run_for_us(TilemCalc *calc, int microseconds)
{
    int rem = 0;
    tilem_z80_run_time(calc, microseconds, &rem);
    return microseconds - rem;
}

static void press_key(TilemCalc *calc, int key)
{
    run_for_us(calc, 50000);
    tilem_keypad_press_key(calc, key);
    run_for_us(calc, 100000);
    tilem_keypad_release_key(calc, key);
    run_for_us(calc, 50000);
}

static int run_until_link_ready(TilemCalc *calc, int timeout_us)
{
    calc->linkport.linkemu = TILEM_LINK_EMULATOR_GRAY;
    while (timeout_us > 0) {
        if (tilem_linkport_graylink_ready(calc))
            return 0;
        timeout_us -= run_for_us(calc, 1000);
    }
    return -1;
}

static int send_byte(TilemCalc *calc, uint8_t value, int timeout_us)
{
    if (run_until_link_ready(calc, timeout_us))
        return -1;
    if (tilem_linkport_graylink_send_byte(calc, value))
        return -1;
    if (run_until_link_ready(calc, timeout_us))
        return -1;
    return 0;
}

static int recv_byte(TilemCalc *calc, int timeout_us)
{
    int v;
    calc->linkport.linkemu = TILEM_LINK_EMULATOR_GRAY;
    while (timeout_us > 0) {
        v = tilem_linkport_graylink_get_byte(calc);
        if (v >= 0)
            return v;
        timeout_us -= run_for_us(calc, 1000);
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* internal cable (same adapter as headless.c)                          */
/* ------------------------------------------------------------------ */

#define CBL_TIMEOUT_US(cbl) ((cbl)->timeout * 100000)

static int ilp_open(CableHandle *cbl)
{
    tilem_linkport_graylink_reset((TilemCalc *)cbl->priv);
    return 0;
}

static int ilp_close(CableHandle *cbl)
{
    TilemCalc *calc = cbl->priv;
    calc->linkport.linkemu = TILEM_LINK_EMULATOR_NONE;
    tilem_linkport_graylink_reset(calc);
    return 0;
}

static int ilp_reset(CableHandle *cbl)
{
    tilem_linkport_graylink_reset((TilemCalc *)cbl->priv);
    return 0;
}

static int ilp_send(CableHandle *cbl, uint8_t *data, uint32_t count)
{
    TilemCalc *calc = cbl->priv;
    int timeout = CBL_TIMEOUT_US(cbl);
    for (uint32_t i = 0; i < count; i++) {
        if (send_byte(calc, data[i], timeout))
            return ERROR_WRITE_TIMEOUT;
    }
    return 0;
}

static int ilp_recv(CableHandle *cbl, uint8_t *data, uint32_t count)
{
    TilemCalc *calc = cbl->priv;
    int timeout = CBL_TIMEOUT_US(cbl);
    for (uint32_t i = 0; i < count; i++) {
        int v = recv_byte(calc, timeout);
        if (v < 0)
            return ERROR_READ_TIMEOUT;
        data[i] = (uint8_t)v;
    }
    run_for_us(calc, 10000);
    return 0;
}

static int ilp_check(CableHandle *cbl, int *status)
{
    TilemCalc *calc = cbl->priv;
    *status = STATUS_NONE;
    if (calc->linkport.lines)
        *status |= STATUS_RX;
    if (calc->linkport.extlines)
        *status |= STATUS_TX;
    return 0;
}

static CableHandle *make_internal_cable(TilemCalc *calc)
{
    CableHandle *cbl = ticables_handle_new(CABLE_ILP, PORT_0);
    if (!cbl)
        return NULL;
    cbl->priv = calc;
    cbl->cable->open = ilp_open;
    cbl->cable->close = ilp_close;
    cbl->cable->reset = ilp_reset;
    cbl->cable->send = ilp_send;
    cbl->cable->recv = ilp_recv;
    cbl->cable->check = ilp_check;
    return cbl;
}

/* ------------------------------------------------------------------ */
/* LCD dump                                                             */
/* ------------------------------------------------------------------ */

static void dump_lcd_ascii(TilemCalc *calc, FILE *out)
{
    TilemLCDBuffer *buf = tilem_lcd_buffer_new();
    tilem_lcd_get_frame1(calc, buf);
    fprintf(out, "+");
    for (int x = 0; x < buf->width; x++)
        fputc('-', out);
    fprintf(out, "+\n");
    for (int y = 0; y < buf->height; y++) {
        fputc('|', out);
        for (int x = 0; x < buf->width; x++) {
            uint8_t v = buf->data[y * buf->rowstride + x];
            fputc(v ? '#' : ' ', out);
        }
        fputc('|', out);
        fputc('\n', out);
    }
    fprintf(out, "+");
    for (int x = 0; x < buf->width; x++)
        fputc('-', out);
    fprintf(out, "+\n");
    tilem_lcd_buffer_free(buf);
}

/* ------------------------------------------------------------------ */
/* "fake Pico" link port driver                                          */
/*                                                                      */
/* The TI-BASIC Send( command transmits a list as a raw DBUS packet.    */
/* TilEm's graylink layer decodes the DBUS framing and delivers the     */
/* payload bytes one at a time via graylink_get_byte.                   */
/*                                                                      */
/* We read bytes until the port goes quiet (no new byte within          */
/* IDLE_TIMEOUT_US), then echo them back as a list so Get(L1) can       */
/* complete.                                                             */
/*                                                                      */
/* The echo-back uses the raw graylink byte API because we are not      */
/* running a full ticalcs session at this point (the calc is mid-       */
/* program, waiting in Get(L1)).  We send the same DBUS framing that    */
/* ticalcs would: the silent period after Send() ends signals the calc  */
/* to switch to receive mode for Get().                                  */
/* ------------------------------------------------------------------ */

#define RX_TIMEOUT_US   (5  * 1000 * 1000)  /* wait up to 5 s for first byte */
#define IDLE_TIMEOUT_US (500 * 1000)         /* 500 ms silence = end of burst  */
#define TX_TIMEOUT_US   (5  * 1000 * 1000)

/*
 * Receive all bytes the calc sends via Send().
 * Returns number of bytes received, or -1 on error.
 */
static int pico_rx(TilemCalc *calc, uint8_t *buf, int bufsz)
{
    int n = 0;

    /* Wait for the first byte (the program may not have started yet). */
    int v = recv_byte(calc, RX_TIMEOUT_US);
    if (v < 0) {
        fprintf(stderr, "[pico] timeout waiting for first byte from calc\n");
        return -1;
    }

    do {
        if (n >= bufsz) {
            fprintf(stderr, "[pico] RX buffer full\n");
            return -1;
        }
        buf[n++] = (uint8_t)v;
        fprintf(stderr, "[pico] RX byte %d: 0x%02X ('%c')\n",
                n, buf[n - 1],
                (buf[n - 1] >= 0x20 && buf[n - 1] < 0x7F) ? buf[n - 1] : '.');
        v = recv_byte(calc, IDLE_TIMEOUT_US);
    } while (v >= 0);

    return n;
}

/*
 * Send a list of bytes back to the calc as a DBUS list packet so Get(L1)
 * can receive it.
 *
 * We use a fresh ticalcs session over a new internal cable handle rather
 * than raw graylink bytes because ticalcs handles the DBUS framing (type
 * byte, length, checksum) that Get( expects.  We build a FileContent
 * holding a real-valued list and push it with ticalcs_calc_send_var.
 */
static int pico_tx(TilemCalc *calc, const uint8_t *data, int n)
{
    FileContent *fc = tifiles_content_create_regular(CALC_TI84P);
    if (!fc)
        return -1;

    /* Each element of a TI real list is a BCD float (9 bytes on-calc).
     * tifiles_ve_create_alloc_data handles the buffer; we set the values
     * by building the list body ourselves.
     *
     * A TI-83+ real list var body layout:
     *   2 bytes: element count (little-endian)
     *   n * 9 bytes: TI real number (sign + exponent byte + 7 BCD digits)
     *
     * For small non-negative integers the BCD encoding is straightforward:
     *   byte 0: 0x00 (positive, exponent-biased 0x80 notation)
     *            actually: sign=0, exponent = 0x80 + power-of-10
     *   For integer k (0 <= k <= 99):
     *     byte 0 = 0x00
     *     byte 1 = 0x80 + 1   (exponent: 10^1 => value = mantissa * 10)
     *              wait -- for k=65: 65 = 6.5e1, exponent=1, mantissa bytes = 65 00 00 00 00 00 00
     *     So byte 1 = 0x81 (0x80 | 1), bytes 2-8 = BCD digits of 6500000
     *
     * TI float format (9 bytes):
     *   [0] sign/special: 0x00 positive, 0x80 negative
     *   [1] exponent: biased by 0x80, so exponent byte = 0x80 + (floor(log10(x)))
     *       for x in [10,99]: floor(log10)=1, byte=0x81
     *       for x in [1,9]:   floor(log10)=0, byte=0x80
     *       for x=0: special all-zero
     *   [2-8] 7 BCD digit pairs (14 significant digits), big-endian
     *       the leftmost digit is the leading digit of the mantissa.
     *       e.g. 65 -> mantissa 6.5... -> BCD bytes: 65 00 00 00 00 00 00
     */

    int body_len = 2 + n * 9;
    VarEntry *ve = tifiles_ve_create_alloc_data(body_len);
    if (!ve) {
        tifiles_content_delete_regular(fc);
        return -1;
    }

    /* element count */
    ve->data[0] = (uint8_t)(n & 0xFF);
    ve->data[1] = (uint8_t)((n >> 8) & 0xFF);

    for (int i = 0; i < n; i++) {
        uint8_t *f = ve->data + 2 + i * 9;
        uint8_t k = data[i];
        memset(f, 0, 9);
        if (k == 0) {
            /* zero: all bytes zero */
        } else if (k < 10) {
            /* single digit: e.g. 7 = 7.0e0 */
            f[1] = 0x80;
            f[2] = (uint8_t)(k << 4);
        } else {
            /* two digits: e.g. 65 = 6.5e1 */
            f[1] = 0x81;
            f[2] = (uint8_t)(((k / 10) << 4) | (k % 10));
        }
    }

    strncpy(ve->name, "L1", sizeof ve->name);
    ve->folder[0] = '\0';
    ve->type = TI83p_LIST;
    ve->attr = ATTRB_NONE;
    ve->version = 0;
    ve->size = body_len;

    if (tifiles_content_add_entry(fc, ve) < 0) {
        tifiles_content_delete_regular(fc);
        return -1;
    }

    CableHandle *cbl = make_internal_cable(calc);
    if (!cbl) {
        tifiles_content_delete_regular(fc);
        return -1;
    }
    CalcHandle *ch = ticalcs_handle_new(CALC_TI84P);
    if (!ch) {
        ticables_handle_del(cbl);
        tifiles_content_delete_regular(fc);
        return -1;
    }

    ticables_options_set_timeout(cbl, 30 * 10);
    ticalcs_cable_attach(ch, cbl);

    fprintf(stderr, "[pico] TX %d elements back to calc as L1...\n", n);
    int e = ticalcs_calc_send_var(ch, MODE_SEND_LAST_VAR, fc);
    if (e)
        fprintf(stderr, "[pico] ticalcs_calc_send_var: %d\n", e);

    ticalcs_cable_detach(ch);
    ticalcs_handle_del(ch);
    ticables_handle_del(cbl);
    tifiles_content_delete_regular(fc);

    return e ? -1 : 0;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s <rom> <LOOPBACK.8Xp> [sav]\n"
            "  Boots the calc, loads LOOPBACK.8Xp, runs prgmLOOPBACK,\n"
            "  reads bytes the calc sends, echoes them back, dumps LCD.\n",
            argv0);
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        usage(argv[0]);
        return 2;
    }
    const char *rom_path  = argv[1];
    const char *prog_path = argv[2];
    const char *sav_path  = (argc == 4) ? argv[3] : NULL;

    ticables_library_init();
    ticalcs_library_init();
    tifiles_library_init();

    TilemCalc *calc = tilem_calc_new(TILEM_CALC_TI84P);
    if (!calc) {
        fprintf(stderr, "tilem_calc_new failed\n");
        return 1;
    }

    FILE *romf = fopen(rom_path, "rb");
    if (!romf) { perror(rom_path); return 1; }
    FILE *savf = NULL;
    if (sav_path) {
        savf = fopen(sav_path, "rb");
        if (!savf) { perror(sav_path); return 1; }
    }
    if (tilem_calc_load_state(calc, romf, savf)) {
        fprintf(stderr, "tilem_calc_load_state failed\n");
        return 1;
    }
    fclose(romf);
    if (savf) fclose(savf);

    fprintf(stderr, "[loopback] booting...\n");
    run_for_us(calc, 3 * 1000 * 1000);

    /* Load LOOPBACK.8Xp into the calc. */
    FileContent *filec = tifiles_content_create_regular(CALC_TI84P);
    int e = tifiles_file_read_regular(prog_path, filec);
    if (e) {
        fprintf(stderr, "tifiles_file_read_regular(%s): %d\n", prog_path, e);
        return 1;
    }

    CableHandle *cbl = make_internal_cable(calc);
    if (!cbl) { fprintf(stderr, "make_internal_cable failed\n"); return 1; }
    CalcHandle *ch = ticalcs_handle_new(CALC_TI84P);
    if (!ch) { fprintf(stderr, "ticalcs_handle_new failed\n"); return 1; }
    ticables_options_set_timeout(cbl, 30 * 10);
    ticalcs_cable_attach(ch, cbl);

    fprintf(stderr, "[loopback] sending %s...\n", prog_path);
    e = ticalcs_calc_send_var(ch, MODE_SEND_LAST_VAR, filec);
    if (e) { fprintf(stderr, "ticalcs_calc_send_var: %d\n", e); return 1; }

    ticalcs_cable_detach(ch);
    ticalcs_handle_del(ch);
    ticables_handle_del(cbl);
    tifiles_content_delete_regular(filec);

    run_for_us(calc, 1 * 1000 * 1000);
    press_key(calc, TILEM_KEY_CLEAR);
    run_for_us(calc, 500 * 1000);

    /* Run prgmLOOPBACK: PRGM -> EXEC -> 1 -> ENTER.
     * The clean.sav has no programs; LOOPBACK is the only one we sent,
     * so it is always item 1 in the EXEC menu.
     *
     * Subtle: pre-arming the graylink (setting linkemu=GRAY) BEFORE the
     * homescreen is fully idle interferes with the OS's link state and
     * makes ENTER land the program in an ERR:SYNTAX state - even for a
     * known-good program like HELLO.8Xp. We arm only after ENTER has
     * been registered and the program has started, but before Send( is
     * reached.
     *
     * The 200 ms gap below is empirical: long enough for the OS to
     * decode "prgmLOOPBACK" and start executing, short enough to be in
     * place before Send( drives the link. */
    fprintf(stderr, "[loopback] running prgmLOOPBACK...\n");
    press_key(calc, TILEM_KEY_PRGM);
    press_key(calc, TILEM_KEY_1);
    press_key(calc, TILEM_KEY_ENTER);

    run_for_us(calc, 200 * 1000);
    calc->linkport.linkemu = TILEM_LINK_EMULATOR_GRAY;
    tilem_linkport_graylink_reset(calc);

    /* Wait up to 3 sim-seconds for the program to reach Send(). */
    run_for_us(calc, 3 * 1000 * 1000);

    fprintf(stderr, "[dbg] post-ENTER linkemu=%d lines=%02x extlines=%02x mode=%04x assistflags=%04x\n",
            calc->linkport.linkemu,
            calc->linkport.lines,
            calc->linkport.extlines,
            calc->linkport.mode,
            calc->linkport.assistflags);
    fprintf(stderr, "[dbg] LCD at this point:\n");
    dump_lcd_ascii(calc, stderr);

    /* ---- fake Pico RX ---- */
    uint8_t rxbuf[256];
    int nrx = pico_rx(calc, rxbuf, (int)(sizeof rxbuf));
    if (nrx <= 0) {
        fprintf(stderr, "[loopback] FAIL: received no bytes from calc\n");
        return 1;
    }
    fprintf(stderr, "[loopback] received %d byte(s) from calc\n", nrx);

    /* ---- fake Pico TX (echo back) ---- */
    if (pico_tx(calc, rxbuf, nrx) < 0) {
        fprintf(stderr, "[loopback] FAIL: could not send echo back to calc\n");
        return 1;
    }

    /* Let the program finish and paint the screen. */
    run_for_us(calc, 2 * 1000 * 1000);

    fprintf(stderr, "[loopback] LCD:\n");
    dump_lcd_ascii(calc, stdout);

    tilem_calc_free(calc);
    tifiles_library_exit();
    ticalcs_library_exit();
    ticables_library_exit();
    return 0;
}
