#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Ti84 Plus needs two pages dumped:
 *   - The first dump is the boot page on all calculators
 *   - The second dump (only on 84+ series) is the USB code page
 *
 * Default file names follow: D8[34][PC][BS]E[12].8xv
 *
 * .8Xu (TIFL) header layout — first bytes of the file:
 *   0x00: "**TIFL**"       (8-byte magic)
 *   0x08: major version    (BCD-packed byte: 0x02 = "2")
 *   0x09: minor version    (BCD-packed byte: 0x55 = "55")
 *   0x0A: flags / reserved
 *   0x0B: object type
 *   0x0C..0x11: date       (BCD: DD MM YYYY, year as two bytes)
 *   0x12: name length
 *   0x13..0x1A: name       (8 bytes, zero-padded)
 *   0x1B..0x2F: filler / padding
 *   0x30..0x33: HEX record count (little-endian uint32, informational)
 *   0x34..0x49: filler / padding
 *   0x4A..0x4D: payload byte length (little-endian uint32)
 *   0x4E: start of payload (Intel HEX records)
 */

#define TIFL_MAGIC "**TIFL**"
#define TIFL_MAGIC_LEN 8
#define OFF_VERSION 0x08
#define OFF_RECORD_COUNT 0x30
#define OFF_DATA_LEN 0x4A
#define HEADER_LEN 0x4E

struct tifl_header {
    unsigned major;
    unsigned minor;
    uint32_t record_count;
    uint32_t data_len;
};

enum tifl_err {
    TIFL_OK = 0,
    TIFL_ERR_TRUNCATED = -1,
    TIFL_ERR_BAD_MAGIC = -2,
};

enum hex_err {
    HEX_OK = 0,
    HEX_ERR_NO_COLON = -1,
    HEX_ERR_BAD_HEX = -2,
    HEX_ERR_TRUNCATED = -3,
    HEX_ERR_CHECKSUM = -4,
};

struct hex_record {
    uint8_t type;
    uint16_t addr;
    uint8_t data[255];
    uint8_t len;
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
    out->record_count = (uint32_t)buf[OFF_RECORD_COUNT] |
                        (uint32_t)buf[OFF_RECORD_COUNT + 1] << 8 |
                        (uint32_t)buf[OFF_RECORD_COUNT + 2] << 16 |
                        (uint32_t)buf[OFF_RECORD_COUNT + 3] << 24;
    out->data_len = (uint32_t)buf[OFF_DATA_LEN] | (uint32_t)buf[OFF_DATA_LEN + 1] << 8 |
                    (uint32_t)buf[OFF_DATA_LEN + 2] << 16 |
                    (uint32_t)buf[OFF_DATA_LEN + 3] << 24;

    return TIFL_OK;
}

/* Returns 0..15 for a valid hex digit, -1 otherwise. */
static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* Two ASCII hex chars → one byte. Returns -1 if either nibble is invalid. */
static int hex_byte(const char *p) {
    int hi = hex_nibble(p[0]);
    int lo = hex_nibble(p[1]);
    if (hi < 0 || lo < 0) return -1;
    return (hi << 4) | lo;
}

/* Parses one Intel HEX line (':' + ASCII hex, no trailing newline assumed).
 * The line length `n` is the number of chars after the ':' is verified.
 *
 * Layout after the colon:
 *   2 chars  byte count (LL)
 *   4 chars  address    (AAAA, big-endian)
 *   2 chars  type       (TT)
 *   2*LL     data
 *   2 chars  checksum   (CC)
 *
 * Checksum: sum of all decoded bytes (LL, addr hi/lo, TT, data, CC) is 0 mod 256. */
static int parse_hex_record(const char *line, size_t n, struct hex_record *out) {
    if (n < 1 || line[0] != ':') return HEX_ERR_NO_COLON;

    /* Need at least LL+AAAA+TT+CC = 10 hex chars after the colon. */
    if (n < 1 + 10) return HEX_ERR_TRUNCATED;

    int bc = hex_byte(line + 1);
    if (bc < 0) return HEX_ERR_BAD_HEX;

    /* Total chars required after ':' = 2 (LL) + 4 (addr) + 2 (type) + 2*LL + 2 (csum). */
    size_t need = 1 + 2 + 4 + 2 + (size_t)bc * 2 + 2;
    if (n < need) return HEX_ERR_TRUNCATED;

    int ah = hex_byte(line + 3);
    int al = hex_byte(line + 5);
    int tt = hex_byte(line + 7);
    if (ah < 0 || al < 0 || tt < 0) return HEX_ERR_BAD_HEX;

    /* Running sum used for checksum verification. */
    uint8_t sum = (uint8_t)bc + (uint8_t)ah + (uint8_t)al + (uint8_t)tt;

    for (int i = 0; i < bc; i++) {
        int b = hex_byte(line + 9 + i * 2);
        if (b < 0) return HEX_ERR_BAD_HEX;
        out->data[i] = (uint8_t)b;
        sum = (uint8_t)(sum + (uint8_t)b);
    }

    int cs = hex_byte(line + 9 + bc * 2);
    if (cs < 0) return HEX_ERR_BAD_HEX;
    sum = (uint8_t)(sum + (uint8_t)cs);
    if (sum != 0) return HEX_ERR_CHECKSUM;

    out->len = (uint8_t)bc;
    out->addr = (uint16_t)((ah << 8) | al);
    out->type = (uint8_t)tt;
    return HEX_OK;
}

/* Reads from `pos` up to (but not including) the next '\n', returning the line
 * length and writing the index of the first char past the line into *next.
 * Trailing '\r' is stripped. End-of-buffer counts as end-of-line. */
static size_t scan_line(const uint8_t *buf, size_t len, size_t pos, size_t *next) {
    size_t start = pos;
    while (pos < len && buf[pos] != '\n') pos++;
    size_t end = pos;
    *next = (pos < len) ? pos + 1 : pos;
    if (end > start && buf[end - 1] == '\r') end--;
    return end - start;
}

struct walk_stats {
    uint32_t data_records;
    uint32_t ext_records;
    uint32_t eof_records;
    uint32_t other_records;
    uint64_t data_bytes;
    uint32_t addr_min;
    uint32_t addr_max_end;
    int saw_data;
};

/* Walks the Intel HEX payload, verifying each record and tracking the
 * 32-bit base address set by type-04 (extended linear address) records. */
static int walk_records(const uint8_t *payload, size_t len, struct walk_stats *st) {
    memset(st, 0, sizeof(*st));
    uint32_t base = 0;
    size_t pos = 0;
    size_t lineno = 0;

    while (pos < len) {
        size_t next;
        size_t llen = scan_line(payload, len, pos, &next);
        lineno++;

        /* Skip blank lines and non-record lines. The Intel HEX spec allows
         * non-':' content (comments, EOF padding like ^Z) to be ignored. */
        if (llen == 0 || payload[pos] != ':') {
            pos = next;
            continue;
        }

        struct hex_record r;
        int rc = parse_hex_record((const char *)(payload + pos), llen, &r);
        if (rc != HEX_OK) {
            fprintf(stderr, "line %zu: parse error %d\n", lineno, rc);
            return rc;
        }

        switch (r.type) {
        case 0x00: {
            uint32_t full = base | r.addr;
            uint32_t end = full + r.len;
            if (!st->saw_data) {
                st->addr_min = full;
                st->addr_max_end = end;
                st->saw_data = 1;
            } else {
                if (full < st->addr_min) st->addr_min = full;
                if (end > st->addr_max_end) st->addr_max_end = end;
            }
            st->data_records++;
            st->data_bytes += r.len;
            break;
        }
        case 0x01:
            /* TI firmware concatenates multiple HEX streams (one per page),
             * each with its own EOF. Reset the base and keep going. */
            st->eof_records++;
            base = 0;
            break;
        case 0x02:
            /* Type 02: segmented extended address. base = paragraph << 4. */
            if (r.len != 2) {
                fprintf(stderr, "line %zu: type-02 with len %u (expected 2)\n", lineno, r.len);
                return HEX_ERR_BAD_HEX;
            }
            base = (((uint32_t)r.data[0] << 8) | (uint32_t)r.data[1]) << 4;
            st->ext_records++;
            break;
        case 0x04:
            /* Type 04: linear extended address. base = upper << 16. */
            if (r.len != 2) {
                fprintf(stderr, "line %zu: type-04 with len %u (expected 2)\n", lineno, r.len);
                return HEX_ERR_BAD_HEX;
            }
            base = ((uint32_t)r.data[0] << 24) | ((uint32_t)r.data[1] << 16);
            st->ext_records++;
            break;
        default:
            st->other_records++;
            break;
        }

        pos = next;
    }

    return HEX_OK;
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
    if (rc != TIFL_OK) {
        fclose(f);
        switch (rc) {
        case TIFL_ERR_TRUNCATED:
            fprintf(stderr, "%s: file shorter than TIFL header (%d bytes)\n", path, HEADER_LEN);
            return 1;
        case TIFL_ERR_BAD_MAGIC:
            fprintf(stderr, "%s: not a TIFL file (bad magic)\n", path);
            return 1;
        }
        return 1;
    }

    /* Cross-check the declared payload length against the file size. */
    if (fseek(f, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(f);
        return 1;
    }
    long total = ftell(f);
    if (total < 0 || (size_t)total < HEADER_LEN) {
        fprintf(stderr, "%s: file too short\n", path);
        fclose(f);
        return 1;
    }
    size_t tail = (size_t)total - HEADER_LEN;
    if (tail != h.data_len) {
        fprintf(stderr, "%s: header says %u payload bytes, file has %zu\n", path, h.data_len, tail);
        fclose(f);
        return 1;
    }
    size_t payload_len = h.data_len;

    if (fseek(f, HEADER_LEN, SEEK_SET) != 0) {
        perror("fseek");
        fclose(f);
        return 1;
    }

    uint8_t *payload = malloc(payload_len);
    if (!payload) {
        fprintf(stderr, "out of memory (%zu bytes)\n", payload_len);
        fclose(f);
        return 1;
    }
    if (fread(payload, 1, payload_len, f) != payload_len) {
        fprintf(stderr, "%s: short read on payload\n", path);
        free(payload);
        fclose(f);
        return 1;
    }
    fclose(f);

    printf("file:    %s\n", path);
    printf("version: %u.%02u\n", h.major, h.minor);
    printf("records: %u (declared)\n", h.record_count);
    printf("payload: %zu bytes (header says %u)\n", payload_len, h.data_len);

    struct walk_stats st;
    int wrc = walk_records(payload, payload_len, &st);
    free(payload);
    if (wrc != HEX_OK) return 1;

    printf("\nhex walk:\n");
    printf("  data records: %u (%llu bytes)\n", st.data_records,
           (unsigned long long)st.data_bytes);
    printf("  ext  records: %u\n", st.ext_records);
    printf("  eof  records: %u\n", st.eof_records);
    printf("  other:        %u\n", st.other_records);
    if (st.saw_data) {
        printf("  addr span:    0x%08X .. 0x%08X\n", st.addr_min, st.addr_max_end);
    }

    return 0;
}
