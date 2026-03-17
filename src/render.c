#include "mesh.h"
#include "dynarr.h"

#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 1280
#define HEIGHT 720

// -----------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------

typedef struct {
    float x, y, z;          // camera position
    float pitch, yaw;       // rotation (radians)
    float focal_length;     // for perspective projection
} Camera;

// Global camera instance
static Camera camera = {
    .x = 0,
    .y = 0,
    .z = -300,
    .pitch = 0,
    .yaw = 0,
    .focal_length = 500
};

// Depth buffer – one float per pixel
static float depth_buffer[WIDTH * HEIGHT];

// Helper macros
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

// -----------------------------------------------------------------
// Camera transformations
// -----------------------------------------------------------------
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

static int project_to_screen(float cam_x, float cam_y, float cam_z,
                             int* screen_x, int* screen_y) {
    if (cam_z <= 0) return 0;   // behind camera

    float projected_x = (cam_x * camera.focal_length) / cam_z;
    float projected_y = (cam_y * camera.focal_length) / cam_z;

    *screen_x = (int)(projected_x + WIDTH / 2);
    *screen_y = (int)(HEIGHT / 2 - projected_y);

    return (*screen_x >= 0 && *screen_x < WIDTH &&
            *screen_y >= 0 && *screen_y < HEIGHT);
}

// -----------------------------------------------------------------
// Camera control API
// -----------------------------------------------------------------
void Camera_SetPosition(float x, float y, float z) {
    camera.x = x;
    camera.y = y;
    camera.z = z;
}

void Camera_SetRotation(float pitch, float yaw) {
    camera.pitch = pitch;
    camera.yaw = yaw;
}

void Camera_Move(float forward, float right, float up) {
    camera.x += forward * sin(camera.yaw);
    camera.z += forward * cos(camera.yaw);

    camera.x += right * cos(camera.yaw);
    camera.z += right * -sin(camera.yaw);

    camera.y += up;
}

void Camera_Rotate(float pitch_delta, float yaw_delta) {
    camera.pitch += pitch_delta;
    camera.yaw += yaw_delta;

    // Clamp pitch to avoid flipping
    if (camera.pitch > 1.5f) camera.pitch = 1.5f;
    if (camera.pitch < -1.5f) camera.pitch = -1.5f;
}

void Camera_SetFocalLength(float focal_length) {
    camera.focal_length = focal_length;
}

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
        return 0;  // signal to quit
    }
    return 1;  // continue running
}

// -----------------------------------------------------------------
// Low‑level drawing (wireframe helpers)
// -----------------------------------------------------------------
void plot_point(SDL_Renderer* renderer, float x, float y, float z) {
    float cam_x, cam_y, cam_z;
    transform_to_camera(x, y, z, &cam_x, &cam_y, &cam_z);

    int sx, sy;
    if (project_to_screen(cam_x, cam_y, cam_z, &sx, &sy)) {
        SDL_RenderDrawPoint(renderer, sx, sy);
    }
}

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

// -----------------------------------------------------------------
// Triangle rasterization (filled)
// -----------------------------------------------------------------
static int edge_function(int x1, int y1, int x2, int y2, int x, int y) {
    return (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
}

void draw_triangle(SDL_Renderer* renderer,
                    float cam1x, float cam1y, float cam1z, 
                    float cam2x, float cam2y, float cam2z, 
                    float cam3x, float cam3y, float cam3z,
                    Uint8 r, Uint8 g, Uint8 b) {


    int sx1, sy1, sx2, sy2, sx3, sy3;
    if (!project_to_screen(cam1x, cam1y, cam1z, &sx1, &sy1)) return;
    if (!project_to_screen(cam2x, cam2y, cam2z, &sx2, &sy2)) return;
    if (!project_to_screen(cam3x, cam3y, cam3z, &sx3, &sy3)) return;

    // 2D bounding box
    int min_x = min(sx1, min(sx2, sx3));
    int max_x = max(sx1, max(sx2, sx3));
    int min_y = min(sy1, min(sy2, sy3));
    int max_y = max(sy1, max(sy2, sy3));

    min_x = max(min_x, 0) ;
    max_x = min(max_x, WIDTH - 1);
    min_y = max(min_y, 0);
    max_y = min(max_y, HEIGHT - 1);

    // Double area of the triangle (used for barycentric weights)
    int area2 = edge_function(sx1, sy1, sx2, sy2, sx3, sy3);
    if (area2 == 0) return;

    // Rasterize
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            int e0 = edge_function(sx2, sy2, sx3, sy3, x, y);
            int e1 = edge_function(sx3, sy3, sx1, sy1, x, y);
            int e2 = edge_function(sx1, sy1, sx2, sy2, x, y);

            // Inside test (same sign as area2)
            if ((area2 > 0 && e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (area2 < 0 && e0 <= 0 && e1 <= 0 && e2 <= 0)) {

                // Barycentric coordinates (normalised)
                float w0 = (float)e0 / area2;
                float w1 = (float)e1 / area2;
                float w2 = (float)e2 / area2;

                // Interpolate depth (camera Z) – linear in screen space
                float z = w0 * cam1z + w1 * cam2z + w2 * cam3z;

                int idx = y * WIDTH + x;
                if (z < depth_buffer[idx]) {
                    depth_buffer[idx] = z;
                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            }
        }
    }
}

// -----------------------------------------------------------------
// SDL initialisation and main loop
// -----------------------------------------------------------------
int Init_Window(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 0;
    }
    if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, window, renderer) < 0) {
        SDL_Quit();
        return 0;
    }
    return 1;
}

int Handle_Events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return 0;
        }
    }
    return 1;
}

void Render_Frame(SDL_Renderer *renderer, Mesh *mesh) {
    // Clear colour buffer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Clear depth buffer
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        depth_buffer[i] = 1e10f;
    }

    // Draw the mesh
    if (mesh && mesh->vertices && mesh->indices) {
        size_t num_indices = arr_len(mesh->indices);
        /*
        TransformedVertex *tv = malloc(num_vertices * sizeof(TransformedVertex));

        // Transform and project all vertices
        for (size_t i = 0; i < num_vertices; i++) {
            // Transform to camera space
            transform_to_camera(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z,
                                &tv[i].cam_x, &tv[i].cam_y, &tv[i].cam_z);
            // Project to screen
            project_to_screen(tv[i].cam_x, tv[i].cam_y, tv[i].cam_z,
                              &tv[i].sx, &tv[i].sy);
        }
        */
        size_t num_triangles = num_indices / 3;

        // Choose a colour for the whole mesh (e.g., light gray)
        Uint8 r = 200, g = 200, b = 200;

        for (size_t i = 0; i < num_triangles; i++) {
            size_t idx0 = mesh->indices[i * 3 + 0];
            size_t idx1 = mesh->indices[i * 3 + 1];
            size_t idx2 = mesh->indices[i * 3 + 2];

            // Optional: add bounds checking
            if (idx0 >= arr_len(mesh->vertices) ||
                idx1 >= arr_len(mesh->vertices) ||
                idx2 >= arr_len(mesh->vertices)) {
                fprintf(stderr, "Warning: index out of bounds in triangle %zu\n", i);
                continue;
            }
            /*
            float cam1x, cam1y, cam1z, cam2x, cam2y, cam2z, cam3x, cam3y, cam3z;
            transform_to_camera(v1->x, v1->y, v1->z, &cam1x, &cam1y, &cam1z);
            transform_to_camera(v2->x, v2->y, v2->z, &cam2x, &cam2y, &cam2z);
            transform_to_camera(v3->x, v3->y, v3->z, &cam3x, &cam3y, &cam3z);
            */
             
            float cam1x, cam1y, cam1z, cam2x, cam2y, cam2z, cam3x, cam3y, cam3z;
            Vertex *v1, *v2, *v3;
            v1 = &mesh->vertices[idx0];
            v2 = &mesh->vertices[idx1];
            v3 = &mesh->vertices[idx2];

            transform_to_camera(v1->x, v1->y, v1->z, &cam1x, &cam1y, &cam1z);
            transform_to_camera(v2->x, v2->y, v2->z, &cam2x, &cam2y, &cam2z);
            transform_to_camera(v3->x, v3->y, v3->z, &cam3x, &cam3y, &cam3z);


            draw_triangle(renderer,
                     cam1x,  cam1y,  cam1z, 
                     cam2x,  cam2y,  cam2z, 
                     cam3x,  cam3y,  cam3z,
                     r,      g,      b
            );
        }
    }

    SDL_RenderPresent(renderer);
}

void Cleanup(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void Run_Window() {
    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;

    if (!Init_Window(&window, &renderer)) {
        printf("Failed to initialise SDL!\n");
        return;
    }

    // Load the mesh from an OBJ file
    Mesh mesh;
    mesh_init(&mesh);

    const char* filename = "Car.obj";  
    if (mesh_load_obj(&mesh, filename) != 0) {
        fprintf(stderr, "Failed to load mesh from %s\n", filename);
        Cleanup(window, renderer);
        return;
    }

    printf("Loaded %zu vertices, %zu indices (%zu triangles)\n",
           arr_len(mesh.vertices), arr_len(mesh.indices),
           arr_len(mesh.indices) / 3);
    
    float scale = 20;
    for (size_t i = 0; i < arr_len(mesh.vertices); i++) {
        mesh.vertices[i].x *= scale;
        mesh.vertices[i].y *= scale;
        mesh.vertices[i].z *= scale;
    }

    // Setup camera (optional, adjust as needed)
    Camera_SetPosition(0, 0, -300);
    Camera_SetFocalLength(500);

    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    int running = 1;

    while (running) {
        if (!Handle_Events()) {
            running = 0;
            break;
        }
        running = Camera_Controls(keyboard_state);
        Render_Frame(renderer, &mesh);
        SDL_Delay(16);  // ~60 FPS
    }

    // Clean up mesh and SDL
    mesh_free(&mesh);
    Cleanup(window, renderer);
}
