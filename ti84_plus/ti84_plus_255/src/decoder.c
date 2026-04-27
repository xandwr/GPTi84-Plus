#include <stdio.h>
#include <stdlib.h>

/* NOTES
Ti84 Plus needs two pages dumped:
    - The first dump is the boot page on all calculators
    - The second dump (only on 84+ series calculators) is the USB code page

    The default file names are in the format:
        `D8[34][PC][BS]E[12].8xv`
*/

// size of ROM page (16k)
#define PAGE_SIZE 0x4000
#define NONFLASH 1
#define FLASH 2

// for Ti84 Plus basic edition
#define NUM_PAGES 0x40
#define FILE_NAME_LENGTH 128 // 128 bytes max?

int v_major, v_minor;
int GetOSVersion(FILE*, int*, int*); // OS file, v_major, v_minor

int GetOSVersion(FILE *file8xu, int *v_major, int *v_minor)
{
    // assuming the OS version is at specific offset
    // return `*file8xu fseek()`'ed to beginning

    // magic version number offset
    fseek(file8xu, 0x6D, SEEK_SET);
    fscanf(file8xu, "%2x", v_major);

    fseek(file8xu, 0x73, SEEK_SET);
    fscanf(file8xu, "%2x", v_minor);

    return (!fseek(file8xu, 0, SEEK_SET)); // beginning
}

int main(int argc, char **argv) {
    
    
    const char *path = argc > 1 ? argv[1] : "ti84_plus/ti84_plus_255/TI84Plus_OS255.8Xu";
    
    FILE *f = fopen(path, "rb");
    GetOSVersion(file8xu, &v_major, &v_minor);

    if (!f) {
        perror(path);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);

    unsigned char *buf = malloc(len);
    if (!buf) {
        fclose(f);
        fputs("out of memory\n", stderr);
        return 1;
    }

    if (fread(buf, 1, len, f) != (size_t)len) {
        perror("fread");
        free(buf);
        fclose(f);
        return 1;
    }
    fclose(f);


    printf("version: %d:%d" ,v_major, v_minor);
    printf("read %ld bytes from %s\n", len, path);
    printf("first 1024 bytes:");
    for (int i = 0; i < 1024 && i < len; i++) {
        printf(" %02x", buf[i]);
    }
    putchar('\n');

    free(buf);
    return 0;
}
