#include "mesh.h"

#include <stdio.h>
#include <stdlib.h>

int load_obj(Mesh* mesh, const char* filename) {
    // Ensure the mesh is initialised
    mesh_init(mesh);

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return -1;
    }

    char line[1024];
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        if (line[0] == 'v' && line[1] == ' ') {
            // Vertex line: "v x y z"
            Vertex v;
            if (sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z) != 3) {
                fprintf(stderr, "Warning: malformed vertex at line %d\n", line_num);
                continue;
            }
            mesh_add_vertex(mesh, v);
        }
        else if (line[0] == 'f') {
            // Face line: "f v1 v2 v3 ..."
            int face_verts[64];   // enough for most polygons
            int count = 0;

            // Skip the "f" and tokenise the rest
            char* token = strtok(line + 2, " \t\n");
            while (token) {
                // Convert to 0‑based index
                int idx = atoi(token) - 1;
                if (idx < 0) {
                    fprintf(stderr, "Warning: invalid index at line %d\n", line_num);
                } else {
                    if (count >= 64) {
                        fprintf(stderr, "Warning: face with more than 64 vertices at line %d – truncating\n", line_num);
                        break;
                    }
                    face_verts[count++] = idx;
                }
                token = strtok(NULL, " \t\n");
            }

            // Triangulate the polygon
            if (count == 3) {
                // Already a triangle
                mesh_add_index(mesh, face_verts[0]);
                mesh_add_index(mesh, face_verts[1]);
                mesh_add_index(mesh, face_verts[2]);
            } else if (count > 3) {
                // Fan triangulation from the first vertex
                for (int i = 1; i < count - 1; i++) {
                    mesh_add_index(mesh, face_verts[0]);
                    mesh_add_index(mesh, face_verts[i]);
                    mesh_add_index(mesh, face_verts[i+1]);
                }
            }
            // count < 3 is ignored (invalid face)
        }
        // Other lines (vt, vn, etc.) are silently ignored
    }

    fclose(file);
    return 0;
}
