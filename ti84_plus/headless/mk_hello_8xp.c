/*
 * mk_hello_8xp: emit a minimal HELLO.8Xp containing the single line
 *
 *     Disp "HELLO WORLD"
 *
 * Uses libtifiles2 so we don't have to hand-roll the file header,
 * meta block, or checksum. The body itself is a tokenized byte stream
 * specific to the TI-83+/84+ token set.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tifiles.h>
#include <types83p.h>

/* TI-83+/84+ token bytes for: Disp "HELLO WORLD"
 *
 *   0xDE             Disp
 *   0x2A             "
 *   0x48..0x44       H, E, L, L, O                  (uppercase A-Z = 0x41..0x5A)
 *   0x29             space (TI string char)
 *   0x57..0x44       W, O, R, L, D
 *   0x2A             "
 *
 * Strings are not null-terminated on the calc; the closing quote is the
 * delimiter. No trailing newline (0x3F) is required for a single-line
 * program.
 */
static const uint8_t HELLO_BODY[] = {
    0xDE,                                           /* Disp */
    0x2A,                                           /* "    */
    0x48, 0x45, 0x4C, 0x4C, 0x4F,                   /* HELLO */
    0x29,                                           /* space */
    0x57, 0x4F, 0x52, 0x4C, 0x44,                   /* WORLD */
    0x2A,                                           /* "    */
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

    VarEntry *ve = tifiles_ve_create_alloc_data(sizeof HELLO_BODY);
    if (!ve) {
        fprintf(stderr, "tifiles_ve_create_alloc_data failed\n");
        return 1;
    }
    /* On-calc program name, max 8 chars, uppercase. */
    strncpy(ve->name, "HELLO", sizeof ve->name);
    ve->folder[0] = '\0';
    ve->type = TI83p_PRGM;
    ve->attr = ATTRB_NONE;
    ve->version = 0;
    /* tifiles_ve_create_alloc_data allocates the buffer but leaves
     * ve->size at 0; we have to set it ourselves or write_regular
     * will emit a zero-byte body. */
    ve->size = sizeof HELLO_BODY;
    memcpy(ve->data, HELLO_BODY, sizeof HELLO_BODY);

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

    fprintf(stderr, "wrote %s (%zu body bytes)\n", out_path, sizeof HELLO_BODY);
    return 0;
}
