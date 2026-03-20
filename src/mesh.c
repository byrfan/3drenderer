#include "mesh.h"
#include "dynarr.h"
#include <stdio.h>
#include <string.h>

void mesh_free(Mesh* m) {
    arr_free(m->indices);
    arr_free(m->vertices);
}

void mesh_init(Mesh* m) {
    m->vertices = NULL;
    m->indices = NULL;
}

void mesh_add_vertex(Mesh* m, Vertex v) {
    arr_push(m->vertices, v);
}

void mesh_add_index(Mesh* m, int i) {
    arr_push(m->indices, i);
}


