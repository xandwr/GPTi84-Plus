#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Inverse of decoder.c: rebuild a byte-identical .8Xu from os.bin + os.bin.meta.
 *
 * The output structure is fully determined by the inputs:
 *   - 78-byte TIFL header is copied verbatim from meta[0x00..0x4E].
 *   - Stream 0: one 27-byte type-00 record at page 0 / addr 0x0000 carrying
 *     meta[0x4E..0x69], then ":00000001FF\r\n".
 *   - Stream 1: ":020000020000FC\r\n" (page-0 selector), then for each touched
 *     page (ascending), 512 type-00 records of 32 bytes covering the full page.
 *     Page 0 records use addresses 0x0000..0x3FE0; banked pages use
 *     0x4000..0x7FE0. Type-02 selectors are emitted at each page boundary
 *     (except before page 0, since the leading selector already covers it).
 *     For page 0, the first 96 bytes come from meta's stream-1 prefix snapshot,
 *     not from os.bin (which stores the post-stream-2 flash state). After all
 *     pages, ":00000001FF\r\n".
 *   - Stream 2: 3 type-00 records on page 0 covering addresses 0x0000..0x005F,
 *     32 bytes each, drawn from os.bin's page-0 prefix (the post-overwrite
 *     flash state). Then ":00000001FF" with no trailing CRLF — instead,
 *     three spaces, "-- CONVERT 2.6 --\r\n\x1a" appended directly.
 *
 * Why os.bin's page-0 prefix is the right source for stream 2: stream 2 is the
 * last writer to those addresses, so flash ends up with stream-2's bytes there.
 * Our decoder records the final flash state into os.bin, so os.bin's first 96
 * bytes of page 0 ARE stream 2's data. */

#define TIFL_MAGIC "**TIFL**"
#define HEADER_LEN 0x4E

#define PAGE_SIZE 0x4000
#define MAX_PAGES 0x80

#define META_LEN 0xD9
#define META_OFF_HEADER 0x00
#define META_OFF_S0 0x4E
#define META_LEN_S0 27
#define META_OFF_S1_PREFIX 0x69
#define META_LEN_S1_PREFIX 96
#define META_OFF_BITMAP 0xC9
#define META_LEN_BITMAP 16

#define REC_BYTES 32   /* TI's data-record size */
#define S2_RECORDS 3   /* stream 2 has exactly 3 data records */

static const char TRAILER[] = "   -- CONVERT 2.6 --\r\n\x1a";
static const size_t TRAILER_LEN = sizeof(TRAILER) - 1;

static int read_file(const char *path, uint8_t **out_buf, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return -1;
    }
    long total = ftell(f);
    if (total < 0) {
        perror("ftell");
        fclose(f);
        return -1;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        perror("fseek");
        fclose(f);
        return -1;
    }
    uint8_t *buf = malloc((size_t)total);
    if (!buf) {
        fprintf(stderr, "out of memory (%ld bytes)\n", total);
        fclose(f);
        return -1;
    }
    if (fread(buf, 1, (size_t)total, f) != (size_t)total) {
        fprintf(stderr, "%s: short read\n", path);
        free(buf);
        fclose(f);
        return -1;
    }
    fclose(f);
    *out_buf = buf;
    *out_len = (size_t)total;
    return 0;
}

/* Emits one Intel HEX record into `out`, returning the number of bytes written.
 * Caller must ensure `out` has room for 1 + 2 + 4 + 2 + 2*len + 2 + 2 = 13+2*len. */
static size_t emit_record(uint8_t *out, uint8_t type, uint16_t addr, const uint8_t *data,
                          uint8_t len) {
    static const char HEX[] = "0123456789ABCDEF";
    size_t p = 0;
    out[p++] = ':';

    uint8_t sum = (uint8_t)(len + (uint8_t)(addr >> 8) + (uint8_t)(addr & 0xFF) + type);

    out[p++] = (uint8_t)HEX[len >> 4];
    out[p++] = (uint8_t)HEX[len & 0xF];
    out[p++] = (uint8_t)HEX[(addr >> 12) & 0xF];
    out[p++] = (uint8_t)HEX[(addr >> 8) & 0xF];
    out[p++] = (uint8_t)HEX[(addr >> 4) & 0xF];
    out[p++] = (uint8_t)HEX[addr & 0xF];
    out[p++] = (uint8_t)HEX[type >> 4];
    out[p++] = (uint8_t)HEX[type & 0xF];

    for (uint8_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        sum = (uint8_t)(sum + b);
        out[p++] = (uint8_t)HEX[b >> 4];
        out[p++] = (uint8_t)HEX[b & 0xF];
    }

    uint8_t cs = (uint8_t)(-sum);
    out[p++] = (uint8_t)HEX[cs >> 4];
    out[p++] = (uint8_t)HEX[cs & 0xF];

    out[p++] = '\r';
    out[p++] = '\n';
    return p;
}

/* Emits a type-02 page selector. The two-byte data field is the page number
 * in big-endian. */
static size_t emit_page_select(uint8_t *out, uint16_t page) {
    uint8_t data[2] = { (uint8_t)(page >> 8), (uint8_t)(page & 0xFF) };
    return emit_record(out, 0x02, 0x0000, data, 2);
}

static size_t emit_eof(uint8_t *out) {
    return emit_record(out, 0x01, 0x0000, NULL, 0);
}

/* Walks the os.bin page-image. The bin file stores touched pages back-to-back
 * in ascending page order, so the i-th touched page lives at offset i*PAGE_SIZE
 * inside the file. Returns a pointer into bin for `page`, or NULL if the page
 * is not marked touched in the bitmap. */
static const uint8_t *page_data(const uint8_t *bin, size_t bin_len,
                                const uint8_t *bitmap, unsigned page) {
    if (!(bitmap[page >> 3] & (1u << (page & 7)))) return NULL;
    unsigned idx = 0;
    for (unsigned p = 0; p < page; p++) {
        if (bitmap[p >> 3] & (1u << (p & 7))) idx++;
    }
    size_t off = (size_t)idx * PAGE_SIZE;
    if (off + PAGE_SIZE > bin_len) return NULL;
    return bin + off;
}

int main(int argc, char **argv) {
    if (argc < 4 || (argc > 1 && (strcmp(argv[1], "-h") == 0 ||
                                  strcmp(argv[1], "--help") == 0))) {
        fprintf(stderr, "usage: %s <os.bin> <os.bin.meta> <out.8Xu>\n", argv[0]);
        return argc < 4 ? 1 : 0;
    }
    const char *bin_path = argv[1];
    const char *meta_path = argv[2];
    const char *out_path = argv[3];

    uint8_t *bin = NULL, *meta = NULL;
    size_t bin_len = 0, meta_len = 0;
    if (read_file(bin_path, &bin, &bin_len) != 0) return 1;
    if (read_file(meta_path, &meta, &meta_len) != 0) {
        free(bin);
        return 1;
    }
    if (meta_len != META_LEN) {
        fprintf(stderr, "%s: expected %d bytes, got %zu\n", meta_path, META_LEN, meta_len);
        free(bin);
        free(meta);
        return 1;
    }
    if (memcmp(meta + META_OFF_HEADER, TIFL_MAGIC, 8) != 0) {
        fprintf(stderr, "%s: bad TIFL magic in header field\n", meta_path);
        free(bin);
        free(meta);
        return 1;
    }

    const uint8_t *bitmap = meta + META_OFF_BITMAP;
    unsigned pages_touched = 0;
    for (unsigned p = 0; p < MAX_PAGES; p++) {
        if (bitmap[p >> 3] & (1u << (p & 7))) pages_touched++;
    }
    if (bin_len != (size_t)pages_touched * PAGE_SIZE) {
        fprintf(stderr, "%s: size %zu does not match %u touched pages * %u\n",
                bin_path, bin_len, pages_touched, PAGE_SIZE);
        free(bin);
        free(meta);
        return 1;
    }

    /* Upper bound on output size: header + 27-byte stream-0 record (~70 B) +
     * stream-1 selectors (~22 * 16 B) + 22*512 records of 76 B each + 3 EOFs +
     * 3 stream-2 records (~76 B each) + trailer. ~870 KiB; allocate 1 MiB. */
    size_t cap = 1u << 20;
    uint8_t *out = malloc(cap);
    if (!out) {
        fprintf(stderr, "out of memory\n");
        free(bin);
        free(meta);
        return 1;
    }
    size_t op = 0;

    memcpy(out + op, meta + META_OFF_HEADER, HEADER_LEN);
    op += HEADER_LEN;

    /* Stream 0: boot signature record + EOF. */
    op += emit_record(out + op, 0x00, 0x0000, meta + META_OFF_S0, META_LEN_S0);
    op += emit_eof(out + op);

    /* Stream 1: leading page-0 selector, then per-page data. */
    op += emit_page_select(out + op, 0);
    int first_page = 1;
    for (unsigned p = 0; p < MAX_PAGES; p++) {
        if (!(bitmap[p >> 3] & (1u << (p & 7)))) continue;

        if (!first_page) {
            op += emit_page_select(out + op, (uint16_t)p);
        }
        first_page = 0;

        const uint8_t *src = page_data(bin, bin_len, bitmap, p);
        if (!src) {
            fprintf(stderr, "internal: missing page 0x%02X data\n", p);
            free(out);
            free(bin);
            free(meta);
            return 1;
        }

        /* For page 0, override the first 96 bytes with the stream-1 prefix
         * snapshot so we re-emit what stream 1 originally wrote, not what
         * stream 2 left in flash. */
        uint8_t page_buf[PAGE_SIZE];
        memcpy(page_buf, src, PAGE_SIZE);
        if (p == 0) {
            memcpy(page_buf, meta + META_OFF_S1_PREFIX, META_LEN_S1_PREFIX);
        }

        uint16_t base = (p == 0) ? 0x0000 : 0x4000;
        for (uint16_t off = 0; off < PAGE_SIZE; off += REC_BYTES) {
            op += emit_record(out + op, 0x00, (uint16_t)(base + off),
                              page_buf + off, REC_BYTES);
        }
    }
    op += emit_eof(out + op);

    /* Stream 2: 3 records overwriting page 0 [0..0x60). The flash buffer's
     * first 96 bytes are stream 2's bytes (last writer wins). */
    const uint8_t *p0 = page_data(bin, bin_len, bitmap, 0);
    if (!p0) {
        fprintf(stderr, "internal: page 0 missing for stream 2\n");
        free(out);
        free(bin);
        free(meta);
        return 1;
    }
    for (int i = 0; i < S2_RECORDS; i++) {
        uint16_t off = (uint16_t)(i * REC_BYTES);
        op += emit_record(out + op, 0x00, off, p0 + off, REC_BYTES);
    }

    /* Final EOF: emit *without* the standard CRLF; trailer takes its place.
     * emit_eof always appends \r\n, so back up over those two bytes after. */
    op += emit_eof(out + op);
    op -= 2;
    memcpy(out + op, TRAILER, TRAILER_LEN);
    op += TRAILER_LEN;

    FILE *f = fopen(out_path, "wb");
    if (!f) {
        perror(out_path);
        free(out);
        free(bin);
        free(meta);
        return 1;
    }
    if (fwrite(out, 1, op, f) != op) {
        fprintf(stderr, "%s: short write\n", out_path);
        fclose(f);
        free(out);
        free(bin);
        free(meta);
        return 1;
    }
    fclose(f);
    printf("wrote %zu bytes to %s\n", op, out_path);

    free(out);
    free(bin);
    free(meta);
    return 0;
}
