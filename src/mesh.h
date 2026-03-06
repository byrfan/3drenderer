#ifndef MESH_H
#define MESH_H

#include <stddef.h>

typedef struct {
    float x,y,z;
} Vertex;

typedef struct {
    Vertex* vertices;
    size_t* indices;
} Mesh;

void mesh_init(Mesh* m);
void mesh_free(Mesh* m);
void mesh_add_index(Mesh* m, int i);
void mesh_add_vertex(Mesh* m, Vertex v);


#endif
