#include <stdio.h>

int main() {
    FILE *fileptr;
    char *buffer;
    long filelen;

    fileptr = fopen("firmware/ti84_plus_255/TI84Plus_OS255.8Xu", "rb");
    fseek(fileptr, 0, SEEK_END);
    filelen = ftell(fileptr);
    rewind(fileptr);

    buffer = (char *)malloc(filelen * sizeof(char));
    fread(buffer, 1, filelen, fileptr);
    fclose(fileptr);

    println({:?}, buffer);
    return 0;
}