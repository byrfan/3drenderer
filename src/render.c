#include "mesh.h"
#include "render.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 600

// Global camera instance
static Camera camera = {
    .x = 0,
    .y = 0,
    .z = -300,
    .pitch = 0,
    .yaw = 0,
    .focal_length = 500
};

// Transform world point to camera space
static void transform_to_camera(float world_x, float world_y, float world_z, 
                                float* cam_x_out, float* cam_y_out, float* cam_z_out) {
    // Translate world to camera space
    float tx = world_x - camera.x;
    float ty = world_y - camera.y;
    float tz = world_z - camera.z;
    
    // Rotate by yaw (around Y axis)
    float rotated_x = tx * cos(camera.yaw) - tz * sin(camera.yaw);
    float rotated_z = tx * sin(camera.yaw) + tz * cos(camera.yaw);
    float rotated_y = ty;
    
    // Rotate by pitch (around X axis)
    float final_y = rotated_y * cos(camera.pitch) - rotated_z * sin(camera.pitch);
    float final_z = rotated_y * sin(camera.pitch) + rotated_z * cos(camera.pitch);
    float final_x = rotated_x;
    
    *cam_x_out = final_x;
    *cam_y_out = final_y;
    *cam_z_out = final_z;
}

// Project camera space point to screen
static int project_to_screen(float cam_x, float cam_y, float cam_z, 
                             int* screen_x, int* screen_y) {
    if (cam_z <= 0) return 0;  // Behind camera
    
    float projected_x = (cam_x * camera.focal_length) / cam_z;
    float projected_y = (cam_y * camera.focal_length) / cam_z;
    
    *screen_x = (int)(projected_x + WIDTH / 2);
    *screen_y = (int)(HEIGHT / 2 - projected_y);
    
    return (*screen_x >= 0 && *screen_x < WIDTH && 
            *screen_y >= 0 && *screen_y < HEIGHT);
}

// Plot a point in world coordinates using camera
void plot_point(SDL_Renderer* renderer, float x, float y, float z) {
    float cam_space_x, cam_space_y, cam_space_z;
    transform_to_camera(x, y, z, &cam_space_x, &cam_space_y, &cam_space_z);
    
    int screen_x, screen_y;
    if (project_to_screen(cam_space_x, cam_space_y, cam_space_z, &screen_x, &screen_y)) {
        SDL_RenderDrawPoint(renderer, screen_x, screen_y);
    }
}

// draw a vertex
void Draw_Vertex(SDL_Renderer *renderer, Vertex* v) {
    plot_point(renderer, v->x, v->y, v->z);
}

//draw a line between two points (vertices)
void Draw_Line(SDL_Renderer *renderer, Vertex* v1, Vertex* v2) {
    float steps = 50;
    for (int i = 0; i <= steps; i++) {
        float t = i / steps;
        float x = v1->x + t * (v2->x - v1->x);
        float y = v1->y + t * (v2->y - v1->y);
        float z = v1->z + t * (v2->z - v1->z);
        plot_point(renderer, x, y, z);
    }
}

// boilerplate for a cube
void Draw_Cube(SDL_Renderer *renderer, Vertex* vertices) {
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };
    
    for (int i = 0; i < 12; i++) {
        Draw_Line(renderer, &vertices[edges[i][0]], &vertices[edges[i][1]]);
    }
}

// teleport camera
void Camera_SetPosition(float x, float y, float z) {
    camera.x = x;
    camera.y = y;
    camera.z = z;
}

// set viewport
void Camera_SetRotation(float pitch, float yaw) {
    camera.pitch = pitch;
    camera.yaw = yaw;
}

// move the camera through space
void Camera_Move(float forward, float right, float up) {
    camera.x += forward * sin(camera.yaw);
    camera.z += forward * cos(camera.yaw);
    
    camera.x += right * cos(camera.yaw);
    camera.z += right * -sin(camera.yaw);
    
    camera.y += up;
}

// rotate the screen viewport
void Camera_Rotate(float pitch_delta, float yaw_delta) {
    camera.pitch += pitch_delta;
    camera.yaw += yaw_delta;
    
    // Clamp pitch to avoid flipping
    if (camera.pitch > 1.5f) camera.pitch = 1.5f;
    if (camera.pitch < -1.5f) camera.pitch = -1.5f;
}

// change focal length / i.e. zooming
void Camera_SetFocalLength(float focal_length) {
    camera.focal_length = focal_length;
}

// Take an instance of the camera
void Camera_GetInfo(Camera* out_camera) {
    *out_camera = camera;
}

int Camera_Controls(const Uint8* keyboard_state) {
    float move_speed = 5.0f;
    float rotate_speed = 0.05f;

    if (keyboard_state[SDL_SCANCODE_W]) {
        Camera_Move(move_speed, 0, 0);  // Forward
    }
    if (keyboard_state[SDL_SCANCODE_S]) {
        Camera_Move(-move_speed, 0, 0); // Backward
    }
    if (keyboard_state[SDL_SCANCODE_A]) {
        Camera_Move(0, -move_speed, 0); // Left strafe
    }
    if (keyboard_state[SDL_SCANCODE_D]) {
        Camera_Move(0, move_speed, 0);  // Right strafe
    }
    if (keyboard_state[SDL_SCANCODE_Q]) {
        Camera_Move(0, 0, move_speed);  // Up
    }
    if (keyboard_state[SDL_SCANCODE_E]) {
        Camera_Move(0, 0, -move_speed); // Down
    }
    if (keyboard_state[SDL_SCANCODE_LEFT]) {
        Camera_Rotate(0, -rotate_speed); // Turn left
    }
    if (keyboard_state[SDL_SCANCODE_RIGHT]) {
        Camera_Rotate(0, rotate_speed);  // Turn right
    }
    if (keyboard_state[SDL_SCANCODE_UP]) {
        Camera_Rotate(rotate_speed, 0);  // Look up
    }
    if (keyboard_state[SDL_SCANCODE_DOWN]) {
        Camera_Rotate(-rotate_speed, 0); // Look down
    }
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        return 0;
    }
    return 1;
}

void Run_Window() {
    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);
    
    Camera_SetPosition(0, 0, -300);
    Camera_SetFocalLength(500);
    
    Vertex cube_vertices[8] = {
        {-50, -50, 0}, {50, -50, 0}, {50, 50, 0}, {-50, 50, 0},
        {-50, -50, 100}, {50, -50, 100}, {50, 50, 100}, {-50, 50, 100}
    };
    
    float time = 0;
    int running = 1;
    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    
    while (running) {
        // Handle events (for quit)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        
        running = Camera_Controls(keyboard_state); 

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        
        Draw_Cube(renderer, cube_vertices);
        
        SDL_RenderPresent(renderer);
        
        time += 0.01f;
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
