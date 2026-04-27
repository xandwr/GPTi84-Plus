#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Ti84 Plus needs two pages dumped:
 *   - The first dump is the boot page on all calculators
 *   - The second dump (only on 84+ series) is the USB code page
 *
 * Default file names follow: D8[34][PC][BS]E[12].8xv
 *
 * .8Xu (TIFL) header layout — first bytes of the file:
 *   0x00: "**TIFL**"      (8-byte magic)
 *   0x08: major version   (BCD-packed byte: 0x02 = "2")
 *   0x09: minor version   (BCD-packed byte: 0x55 = "55")
 *   0x0A: flags / reserved
 *   0x0B: object type
 *   0x0C..0x11: date (BCD: DD MM YYYY, year as two bytes)
 *   0x12: name length
 *   0x13..0x1A: name (8 bytes, zero-padded)
 *   0x1B..0x2F: filler / padding
 *   0x30..0x33: payload record count (little-endian uint32)
 *   0x34: start of payload (Intel HEX records)
 */

#define TIFL_MAGIC "**TIFL**"
#define TIFL_MAGIC_LEN 8
#define OFF_VERSION 0x08
#define OFF_DATA_LEN 0x30
#define HEADER_LEN 0x34

struct tifl_header {
    unsigned major;
    unsigned minor;
    uint32_t records;
};

enum tifl_err {
    TIFL_OK = 0,
    TIFL_ERR_TRUNCATED = -1,
    TIFL_ERR_BAD_MAGIC = -2,
};

/* BCD: each nibble is one decimal digit. 0x55 → 55, 0x02 → 2. */
static unsigned bcd_to_uint(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }

/* Reads the TIFL header from f (which must be positioned at offset 0).
 * On success, fills *out and leaves the file positioned at HEADER_LEN. */
static int read_tifl_header(FILE *f, struct tifl_header *out) {
    uint8_t buf[HEADER_LEN];
    if (fread(buf, 1, HEADER_LEN, f) != HEADER_LEN) {
        return TIFL_ERR_TRUNCATED;
    }
    if (memcmp(buf, TIFL_MAGIC, TIFL_MAGIC_LEN) != 0) {
        return TIFL_ERR_BAD_MAGIC;
    }

    out->major = bcd_to_uint(buf[OFF_VERSION]);
    out->minor = bcd_to_uint(buf[OFF_VERSION + 1]);

    /* Little-endian: low byte first. Casts force unsigned shifts. */
    out->records = (uint32_t)buf[OFF_DATA_LEN] | (uint32_t)buf[OFF_DATA_LEN + 1] << 8 |
                   (uint32_t)buf[OFF_DATA_LEN + 2] << 16 | (uint32_t)buf[OFF_DATA_LEN + 3] << 24;

    return TIFL_OK;
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "ti84_plus/ti84_plus_255/TI84Plus_OS255.8Xu";

    FILE *f = fopen(path, "rb");
    if (!f) {
        perror(path);
        return 1;
    }

    struct tifl_header h;
    int rc = read_tifl_header(f, &h);
    fclose(f);

    switch (rc) {
    case TIFL_OK:
        break;
    case TIFL_ERR_TRUNCATED:
        fprintf(stderr, "%s: file shorter than TIFL header (%d bytes)\n", path, HEADER_LEN);
        return 1;
    case TIFL_ERR_BAD_MAGIC:
        fprintf(stderr, "%s: not a TIFL file (bad magic)\n", path);
        return 1;
    }

    printf("file:    %s\n", path);
    printf("version: %u.%02u\n", h.major, h.minor);
    printf("records: %u\n", h.records);

    return 0;
}
