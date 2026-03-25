#ifndef RENDER_H
#define RENDER_H

#include "mesh.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>

// -----------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------

typedef struct {
    float x, y, z;          // camera position
    float pitch, yaw;       // rotation (radians)
    float focal_length;     // for perspective projection
} Camera;

typedef struct {
    int r, g, b;    
} Colour;

typedef struct {
    Mesh* mesh;
    Camera camera;
} AppContext;

// Fn def
void Run_Window(char* filename);
void Camera_SetPosition(float x, float y, float z);
void Camera_SetRotation(float pitch, float yaw);
void Camera_Move(float forward, float right, float up);
void Camera_Rotate(float pitch_delta, float yaw_delta);
void Camera_SetFocalLength(float focal_length);
void Camera_GetInfo(Camera* out_camera);
void Mesh_GetInfo(Mesh** out_mesh);
void scale_vertices(Mesh* mesh, float scale);

#endif
