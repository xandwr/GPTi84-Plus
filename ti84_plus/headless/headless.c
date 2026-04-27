/*
 * tilem_headless: drive a TI-84 Plus emulator entirely without a GUI.
 *
 * Loads a ROM (and optional .sav), feeds a .8Xp into the calc via the
 * graylink, synthesizes a few keypresses to run prgmHELLO from the
 * homescreen, then dumps the LCD as ASCII art on stdout.
 *
 * Built on top of libtilemcore (vendored) plus libticables2/libticalcs2/
 * libtifiles2 from the system. The "internal cable" pattern is a direct
 * port of vendor/tilem/gui/link.c, minus the GTK threading.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ticables.h>
#include <ticalcs.h>
#include <tifiles.h>

#include <tilem.h>
#include <scancodes.h>

/* --- helpers --- */

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

/* Wait until the graylink is ready to accept another byte (or until
 * timeout_us elapses). Returns 0 on ready, -1 on timeout. */
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

/* --- internal cable adapter (libticables -> libtilemcore) --- */

/* Per-cable timeout converted from libticables tenths-of-second to us. */
#define CBL_TIMEOUT_US(cbl) ((cbl)->timeout * 100000)

static int ilp_open(CableHandle *cbl)
{
    TilemCalc *calc = cbl->priv;
    tilem_linkport_graylink_reset(calc);
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
    /* short cool-down after a receive burst */
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

/* --- LCD dump --- */

static void dump_lcd_ascii(TilemCalc *calc, FILE *out)
{
    TilemLCDBuffer *buf = tilem_lcd_buffer_new();
    tilem_lcd_get_frame1(calc, buf);
    /* buf->data is width*height bytes, 0/1 valued. 96x64 on 84+. */
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

/* --- main --- */

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s <rom> <program.8Xp> [sav]\n"
            "  Cold-boots the calc from <rom> (or warm-boots from [sav]),\n"
            "  sends <program.8Xp> via the graylink, runs prgmHELLO from\n"
            "  the homescreen, dumps the LCD as ASCII to stdout.\n",
            argv0);
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        usage(argv[0]);
        return 2;
    }
    const char *rom_path = argv[1];
    const char *prog_path = argv[2];
    const char *sav_path = (argc == 4) ? argv[3] : NULL;

    /* tilibs init */
    ticables_library_init();
    ticalcs_library_init();
    tifiles_library_init();

    /* calc setup */
    TilemCalc *calc = tilem_calc_new(TILEM_CALC_TI84P);
    if (!calc) {
        fprintf(stderr, "tilem_calc_new failed\n");
        return 1;
    }

    FILE *romf = fopen(rom_path, "rb");
    if (!romf) {
        perror(rom_path);
        return 1;
    }
    FILE *savf = NULL;
    if (sav_path) {
        savf = fopen(sav_path, "rb");
        if (!savf) {
            perror(sav_path);
            return 1;
        }
    }
    if (tilem_calc_load_state(calc, romf, savf) != 0) {
        fprintf(stderr, "tilem_calc_load_state failed\n");
        return 1;
    }
    fclose(romf);
    if (savf)
        fclose(savf);

    /* Boot for a couple sim-seconds so the OS reaches the homescreen. */
    fprintf(stderr, "[headless] booting...\n");
    run_for_us(calc, 3 * 1000 * 1000);

    /* Read the .8Xp into a FileContent. */
    FileContent *filec = tifiles_content_create_regular(CALC_TI84P);
    int e = tifiles_file_read_regular(prog_path, filec);
    if (e) {
        fprintf(stderr, "tifiles_file_read_regular(%s): %d\n", prog_path, e);
        return 1;
    }

    /* Open an internal cable + calc handle, send the var. */
    CableHandle *cbl = make_internal_cable(calc);
    if (!cbl) {
        fprintf(stderr, "make_internal_cable failed\n");
        return 1;
    }
    CalcHandle *ch = ticalcs_handle_new(CALC_TI84P);
    if (!ch) {
        fprintf(stderr, "ticalcs_handle_new failed\n");
        return 1;
    }
    ticables_options_set_timeout(cbl, 30 * 10);
    ticalcs_cable_attach(ch, cbl);

    fprintf(stderr, "[headless] sending %s ...\n", prog_path);
    e = ticalcs_calc_send_var(ch, MODE_SEND_LAST_VAR, filec);
    if (e) {
        fprintf(stderr, "ticalcs_calc_send_var: %d\n", e);
        return 1;
    }

    ticalcs_cable_detach(ch);
    ticalcs_handle_del(ch);
    ticables_handle_del(cbl);

    /* Let the calc settle after the transfer (clear the "Done" prompt). */
    run_for_us(calc, 1 * 1000 * 1000);
    press_key(calc, TILEM_KEY_CLEAR);
    run_for_us(calc, 500 * 1000);

    /* Run prgmHELLO from the homescreen:
     *   PRGM            -> EXEC menu (prgmHELLO is item 1)
     *   1               -> pastes "prgmHELLO" onto the homescreen
     *   ENTER           -> executes
     *
     * Asm() wrapping is intentionally skipped: HELLO.8Xp is a TI-BASIC
     * program ("calc the calc"), so prgmHELLO runs directly. */
    fprintf(stderr, "[headless] running prgmHELLO ...\n");
    press_key(calc, TILEM_KEY_PRGM);
    press_key(calc, TILEM_KEY_1);
    press_key(calc, TILEM_KEY_ENTER);

    /* Let the program run + paint. */
    run_for_us(calc, 2 * 1000 * 1000);

    fprintf(stderr, "[headless] LCD:\n");
    dump_lcd_ascii(calc, stdout);

    tifiles_content_delete_regular(filec);
    tilem_calc_free(calc);
    tifiles_library_exit();
    ticalcs_library_exit();
    ticables_library_exit();
    return 0;
}
