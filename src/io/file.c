#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#define MAX_PATH_LENGTH 512
const char* REF_DIR = PROJECT_ROOT;

char* loadShaderSource(const char* filename) {
    char path[MAX_PATH_LENGTH];
    strcpy(path, REF_DIR);
    strcat(path, filename);
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open shader file: %s\n", path);
        return NULL;
    }

    printf("Found shader file: %s\n", path);

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* source = (char*)malloc(length + 1);
    fread(source, 1, length, file);
    source[length] = '\0';
    fclose(file);

    // // Write to debug file
    // char debugPath[MAX_PATH_LENGTH];
    // snprintf(debugPath, MAX_PATH_LENGTH, "%s/%s_w_", REF_DIR, filename);
    // FILE* debugFile = fopen(debugPath, "w");
    // if (debugFile) {
    //     fprintf(debugFile, "%s", source);
    //     fclose(debugFile);
    //     printf("Shader copied for debugging to: %s\n", debugPath);
    // } else {
    //     printf("Failed to write debug shader file: %s\n", debugPath);
    // }

    return source;
}
