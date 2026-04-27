/*
 * mk_link_loopback_8xp: emit LOOPBACK.8Xp, a TI-BASIC program that:
 *
 *   Send({65,66,67})
 *   Get(L1)
 *
 * The Send( pushes the list out over the link port. The Get( blocks until
 * the remote side sends a list back, storing it in L1. The link_loopback
 * harness reads those bytes and echoes them back as a list.
 *
 * Token encoding for TI-83+/84+ (cross-checked against TI-Toolkit
 * tokens-wiki and vendored libticonv/tokens.cc):
 *   Send(   0xE7   (the trailing "(" is part of the token; do NOT also
 *                   emit 0x10 after it)
 *   Get(    0xE8   (same; on 83+/84+ Get( with no CBL falls back to the
 *                   calc-to-calc behaviour of GetCalc()
 *   (       0x10
 *   )       0x11
 *   {       0x08
 *   }       0x09
 *   ,       0x2B
 *   newline 0x3F   (statement separator; the same byte the editor inserts
 *                   when you press ENTER inside a program)
 *   L1      0x5D 0x00   (0x5D is the "list" 2-byte prefix; 0x00 selects
 *                        L1, 0x01 would be L2, etc.)
 *   digits  0x30-0x39 (literal ASCII digit chars, 1 byte each)
 *
 * Common pitfall this file used to fall into: 0xE6 is Menu(, 0xE7 is
 * Send(, 0xE8 is Get( - off-by-one across all three. And 0x29 is the
 * space character, NOT ")"; ")" is 0x11.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <tifiles.h>
#include <types83p.h>

static const uint8_t LOOPBACK_BODY[] = {
    /* Send({65,66,67}) : Get(L1) */
    0xE7,                   /* Send( */
    0x08,                   /* {     */
    0x36, 0x35,             /* 65    */
    0x2B,                   /* ,     */
    0x36, 0x36,             /* 66    */
    0x2B,                   /* ,     */
    0x36, 0x37,             /* 67    */
    0x09,                   /* }     */
    0x11,                   /* )     */
    0x3F,                   /* newline */
    0xE8,                   /* Get(  */
    0x5D, 0x00,             /* L1    */
    0x11,                   /* )     */
};

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <out.8Xp>\n", argv[0]);
        return 2;
    }
    const char *out_path = argv[1];

    tifiles_library_init();

    FileContent *fc = tifiles_content_create_regular(CALC_TI84P);
    if (!fc) {
        fprintf(stderr, "tifiles_content_create_regular failed\n");
        return 1;
    }

    VarEntry *ve = tifiles_ve_create_alloc_data(sizeof LOOPBACK_BODY);
    if (!ve) {
        fprintf(stderr, "tifiles_ve_create_alloc_data failed\n");
        return 1;
    }
    strncpy(ve->name, "LOOPBACK", sizeof ve->name);
    ve->folder[0] = '\0';
    ve->type = TI83p_PRGM;
    ve->attr = ATTRB_NONE;
    ve->version = 0;
    ve->size = sizeof LOOPBACK_BODY;
    memcpy(ve->data, LOOPBACK_BODY, sizeof LOOPBACK_BODY);

    if (tifiles_content_add_entry(fc, ve) < 0) {
        fprintf(stderr, "tifiles_content_add_entry failed\n");
        return 1;
    }

    int e = tifiles_file_write_regular(out_path, fc, NULL);
    if (e) {
        fprintf(stderr, "tifiles_file_write_regular(%s): %d\n", out_path, e);
        return 1;
    }

    tifiles_content_delete_regular(fc);
    tifiles_library_exit();

    fprintf(stderr, "wrote %s (%zu body bytes)\n", out_path, sizeof LOOPBACK_BODY);
    return 0;
}
