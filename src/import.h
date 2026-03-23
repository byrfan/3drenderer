#ifndef IMPORT_H
#define IMPORT_H

#include "mesh.h"

#include <stdint.h>

#define LOCALISE_PATH(filename) strcat("/assets/", filename)

typedef struct {
    char header[80];
    uint32_t triangle_count;
} STLHeader;

typedef struct __attribute__((packed)) {
    float normal[3];
    float vertices[3][3];
    uint16_t attribute;
} STLTriangle;


int load_mesh_from_file(Mesh* mesh, const char* filename);

#endif
