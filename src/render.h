#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

typedef struct {
    float x;
    float y;
    float z;
    float pitch;
    float yaw;
    float focal_length;
} Camera;

void Run_Window();

#endif
