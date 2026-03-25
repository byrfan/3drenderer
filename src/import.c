#include "mesh.h"
#include "dynarr.h"
#include "import.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>


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

// Helper function to compare vertices
int vertices_equal(const Vertex* a, const Vertex* b) {
    const float vertex_epsilon = 0.000001f;
    return fabs(a->x - b->x) < vertex_epsilon &&
           fabs(a->y - b->y) < vertex_epsilon &&
           fabs(a->z - b->z) < vertex_epsilon;
}

// Helper to find or add a vertex (returns index)
int mesh_find_or_add_vertex(Mesh* mesh, const Vertex* v) {
    if (!mesh || !v) return -1;
    
    // Get the current vertex count
    size_t vertex_count = arr_len(mesh->vertices);
    
    // Search through existing vertices (only if there are any)
    for (size_t i = 0; i < vertex_count; i++) {
        // Safety check: make sure mesh->vertices[i] is accessible
        Vertex* existing = &mesh->vertices[i];
        if (vertices_equal(existing, v)) {
            return (int)i;  // Found existing vertex
        }
    }
    
    // Not found, add new vertex
    mesh_add_vertex(mesh, *v);
    return (int)arr_len(mesh->vertices) - 1;
}

// Load binary STL file
int load_stl_binary(Mesh* mesh, const char* filename) {
    if (!mesh) {
        fprintf(stderr, "Error: NULL mesh pointer\n");
        return -1;
    }
    
    // Initialize mesh (though it should already be initialized)
    mesh_init(mesh);
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
       
    STLHeader header;
    if (fread(&header, sizeof(STLHeader), 1, file) != 1) {
        fprintf(stderr, "Error: Failed to read STL header\n");
        fclose(file);
        return -1;
    }
    
    printf("Loading binary STL with %u triangles\n", header.triangle_count);
    
    // Validate triangle count
    if (header.triangle_count == 0 || header.triangle_count > 10000000) {
        fprintf(stderr, "Error: Invalid triangle count: %u\n", header.triangle_count);
        fclose(file);
        return -1;
    }
    
    // Process each triangle
    for (uint32_t i = 0; i < header.triangle_count; i++) {
        STLTriangle triangle;
        
        if (fread(&triangle, sizeof(STLTriangle), 1, file) != 1) {
            fprintf(stderr, "Error: Failed to read triangle %u\n", i);
            fclose(file);
            return -1;
        }
        
        // Add vertices with deduplication
        int indices[3];
        int valid = 1;
        
        for (int j = 0; j < 3; j++) {
            Vertex v = {
                .x = triangle.vertices[j][0],
                .y = triangle.vertices[j][1],
                .z = triangle.vertices[j][2]
            };
            
            int idx = mesh_find_or_add_vertex(mesh, &v);
            if (idx < 0) {
                fprintf(stderr, "Error: Failed to add vertex for triangle %u\n", i);
                valid = 0;
                break;
            }
            indices[j] = idx;
        }
        
        if (valid) {
            // Add indices for the triangle
            mesh_add_index(mesh, indices[0]);
            mesh_add_index(mesh, indices[1]);
            mesh_add_index(mesh, indices[2]);
        }
        
        // Progress indicator
        if ((i + 1) % 100000 == 0) {
            printf("Processed %u triangles...\n", i + 1);
        }
    }
    
    printf("Loaded %zu unique vertices, %zu indices\n", 
           arr_len(mesh->vertices), arr_len(mesh->indices));
    
    fclose(file);
    return 0;
}

// Load ASCII STL file
int load_stl_ascii(Mesh* mesh, const char* filename) {
    if (!mesh) {
        fprintf(stderr, "Error: NULL mesh pointer\n");
        return -1;
    }
    
    mesh_init(mesh);
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    char line[1024];
    int line_num = 0;
    int triangle_count = 0;
    float current_normal[3] = {0, 0, 0};
    Vertex current_vertices[3];
    int vertices_in_triangle = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // Skip comments and empty lines
        if (line[0] == '\n' || line[0] == '\0') continue;
        
        // Trim whitespace
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (trimmed[0] == '\0' || trimmed[0] == '\n') continue;
        
        // Parse facet normal
        if (strncmp(trimmed, "facet normal", 12) == 0) {
            if (sscanf(trimmed, "facet normal %f %f %f", 
                       &current_normal[0], &current_normal[1], &current_normal[2]) != 3) {
                fprintf(stderr, "Warning: malformed facet normal at line %d\n", line_num);
            }
            vertices_in_triangle = 0;
        }
        // Parse vertex lines
        else if (strncmp(trimmed, "vertex", 6) == 0) {
            if (vertices_in_triangle >= 3) {
                fprintf(stderr, "Warning: Extra vertex in triangle at line %d\n", line_num);
                continue;
            }
            
            Vertex v;
            if (sscanf(trimmed, "vertex %f %f %f", &v.x, &v.y, &v.z) != 3) {
                fprintf(stderr, "Warning: malformed vertex at line %d\n", line_num);
                continue;
            }
            
            current_vertices[vertices_in_triangle++] = v;
        }
        // When facet ends, add the triangle
        else if (strncmp(trimmed, "endfacet", 8) == 0) {
            if (vertices_in_triangle != 3) {
                fprintf(stderr, "Warning: Triangle with %d vertices at line %d\n", 
                        vertices_in_triangle, line_num);
                continue;
            }
            
            // Add vertices with deduplication
            int indices[3];
            int valid = 1;
            
            for (int i = 0; i < 3; i++) {
                int idx = mesh_find_or_add_vertex(mesh, &current_vertices[i]);
                if (idx < 0) {
                    valid = 0;
                    break;
                }
                indices[i] = idx;
            }
            
            if (valid) {
                mesh_add_index(mesh, indices[0]);
                mesh_add_index(mesh, indices[1]);
                mesh_add_index(mesh, indices[2]);
                triangle_count++;
            }
        }
    }
    
    printf("Loaded %d triangles, %zu unique vertices, %zu indices\n", 
           triangle_count, arr_len(mesh->vertices), arr_len(mesh->indices));
    
    fclose(file);
    return 0;
}

int load_stl(Mesh* mesh, const char* filename) {
    if (!mesh) {
        fprintf(stderr, "Error: NULL mesh pointer\n");
        return -1;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return -1;
    }
    
    char first_line[128];
    if (!fgets(first_line, sizeof(first_line), file)) {
        fclose(file);
        return -1;
    }
    fclose(file);
    
    int is_ascii = 0;
    if (strncmp(first_line, "solid", 5) == 0) {
        size_t len = strlen(first_line);
        if (len < 100) {
            is_ascii = 1;
        }
    }
    
    if (is_ascii) {
        return load_stl_ascii(mesh, filename);
    } else {
        return load_stl_binary(mesh, filename);
    }
}

const char* get_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

int insensitive_strcmp(const char* a, const char* b) {
    while (*a && *b) {
        if (tolower(*a) != tolower(*b)) return 1;
        a++; b++;
    }
    return (*a != *b);
}

int load_mesh_from_file(Mesh* mesh, const char* filename) {
    const char* ext = get_extension(filename);
    
    if (insensitive_strcmp(ext, "obj") == 0) {
        return load_obj(mesh, filename);
    } 
    else if (insensitive_strcmp(ext, "stl") == 0) {
        return load_stl(mesh, filename);
    }
    else {
        fprintf(stderr, "Error: Unsupported file extension '%s' for file: %s\n", 
                ext, filename);
        fprintf(stderr, "Supported formats: .obj, .stl\n");
        return -1;
    }
}
