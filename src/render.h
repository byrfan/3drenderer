#ifndef RENDER_H
#define RENDER_H

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

// Fn def
void Run_Window(char* filename);

#endif
