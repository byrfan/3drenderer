#include "mesh.h"
#include "dynarr.h"
#include "render.h"
#include "console.h"
#include "import.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_ttf.h>
#include <iso646.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#define WIDTH 1280
#define HEIGHT 720

typedef Uint32 PixelBuffer;

static AppContext app;

static float depth_buffer[WIDTH * HEIGHT];

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

// =======================================================================================
//                                Cam and Mesh API functions
// =======================================================================================

void Camera_GetInfo(Camera* out_camera) {
    *out_camera = app.camera;
}

void Mesh_GetInfo(Mesh** out_mesh) {
    *out_mesh = app.mesh; 
}

void Camera_SetPosition(float x, float y, float z) {
    app.camera.x = x;
    app.camera.y = y;
    app.camera.z = z;
}

void Camera_SetRotation(float pitch, float yaw) {
    app.camera.pitch = pitch;
    app.camera.yaw = yaw;
}

void Camera_Move(float forward, float right, float up) {
    app.camera.x += forward * sin(app.camera.yaw);
    app.camera.z += forward * cos(app.camera.yaw);

    app.camera.x += right * cos(app.camera.yaw);
    app.camera.z += right * -sin(app.camera.yaw);

    app.camera.y += up;
}

void Camera_Rotate(float pitch_delta, float yaw_delta) {
    app.camera.pitch += pitch_delta;
    app.camera.yaw += yaw_delta;

    if (app.camera.pitch > 1.5f) app.camera.pitch = 1.5f;
    if (app.camera.pitch < -1.5f) app.camera.pitch = -1.5f;
}

void Camera_SetFocalLength(float focal_length) {
    app.camera.focal_length = focal_length;
}

int Camera_Controls(const Uint8* keyboard_state) {
    float move_speed = 5.0f;
    float rotate_speed = 0.05f;

    if (keyboard_state[SDL_SCANCODE_W]) {
        Camera_Move(move_speed, 0, 0);
    }
    if (keyboard_state[SDL_SCANCODE_S]) {
        Camera_Move(-move_speed, 0, 0);
    }
    if (keyboard_state[SDL_SCANCODE_A]) {
        Camera_Move(0, -move_speed, 0);
    }
    if (keyboard_state[SDL_SCANCODE_D]) {
        Camera_Move(0, move_speed, 0);
    }
    if (keyboard_state[SDL_SCANCODE_Q]) {
        Camera_Move(0, 0, move_speed);
    }
    if (keyboard_state[SDL_SCANCODE_E]) {
        Camera_Move(0, 0, -move_speed);
    }
    if (keyboard_state[SDL_SCANCODE_LEFT]) {
        Camera_Rotate(0, -rotate_speed);
    }
    if (keyboard_state[SDL_SCANCODE_RIGHT]) {
        Camera_Rotate(0, rotate_speed);
    }
    if (keyboard_state[SDL_SCANCODE_UP]) {
        Camera_Rotate(rotate_speed, 0);
    }
    if (keyboard_state[SDL_SCANCODE_DOWN]) {
        Camera_Rotate(-rotate_speed, 0);
    }
    if (keyboard_state[SDL_SCANCODE_ESCAPE]) {
        return 0;
    }
    return 1;
}


// =======================================================================================
//                                  Rendering Functions
// =======================================================================================

static void transform_to_camera(Camera* cam, float world_x, float world_y, float world_z,
                                float* cam_x_out, float* cam_y_out, float* cam_z_out) {
    float tx = world_x - cam->x;
    float ty = world_y - cam->y;
    float tz = world_z - cam->z;

    float rotated_x = tx * cos(cam->yaw) - tz * sin(cam->yaw);
    float rotated_z = tx * sin(cam->yaw) + tz * cos(cam->yaw);
    float rotated_y = ty;

    float final_y = rotated_y * cos(cam->pitch) - rotated_z * sin(cam->pitch);
    float final_z = rotated_y * sin(cam->pitch) + rotated_z * cos(cam->pitch);
    float final_x = rotated_x;

    *cam_x_out = final_x;
    *cam_y_out = final_y;
    *cam_z_out = final_z;
}

static int project_to_screen(Camera* cam, float cam_x, float cam_y, float cam_z,
                             int* screen_x, int* screen_y) {
    if (cam_z <= 0) return 0;

    float projected_x = (cam_x * cam->focal_length) / cam_z;
    float projected_y = (cam_y * cam->focal_length) / cam_z;

    *screen_x = (int)(projected_x + WIDTH / 2);
    *screen_y = (int)(HEIGHT / 2 - projected_y);

    return (*screen_x >= 0 && *screen_x < WIDTH &&
            *screen_y >= 0 && *screen_y < HEIGHT);
}


void plot_point(SDL_Renderer* renderer, Camera* cam, float x, float y, float z) {
    float cam_x, cam_y, cam_z;
    transform_to_camera(cam, x, y, z, &cam_x, &cam_y, &cam_z);

    int sx, sy;
    if (project_to_screen(cam, cam_x, cam_y, cam_z, &sx, &sy)) {
        SDL_RenderDrawPoint(renderer, sx, sy);
    }
}

void Draw_Line(SDL_Renderer *renderer, Camera* cam, Vertex* v1, Vertex* v2) {
    float steps = 50;
    for (int i = 0; i <= steps; i++) {
        float t = i / steps;
        float x = v1->x + t * (v2->x - v1->x);
        float y = v1->y + t * (v2->y - v1->y);
        float z = v1->z + t * (v2->z - v1->z);
        plot_point(renderer, cam, x, y, z);
    }
}

static int edge_function(int x1, int y1, int x2, int y2, int x, int y) {
    return (x2 - x1) * (y - y1) - (y2 - y1) * (x - x1);
}

int isTriangleFrontFacing(float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          float x3, float y3, float z3) {
    float e1x = x2 - x1;
    float e1y = y2 - y1;
    float e1z = z2 - z1;
    
    float e2x = x3 - x1;
    float e2y = y3 - y1;
    float e2z = z3 - z1;
    
    float nx = e1y * e2z - e1z * e2y;
    float ny = e1z * e2x - e1x * e2z;
    float nz = e1x * e2y - e1y * e2x;
    
    float vx = -x1;
    float vy = -y1;
    float vz = -z1;
    
    float dot = nx * vx + ny * vy + nz * vz;
    
    return dot > 0;
}

void draw_triangle_projected(SDL_Renderer* renderer, Camera* cam,
                             PixelBuffer *pixels, 
                             TransformedVertex* tv1,
                             TransformedVertex* tv2,
                             TransformedVertex* tv3,
                             Uint8 r, Uint8 g, Uint8 b) {
    int sx1 = tv1->sx, sy1 = tv1->sy;
    int sx2 = tv2->sx, sy2 = tv2->sy;
    int sx3 = tv3->sx, sy3 = tv3->sy;

    float cam1z = tv1->cam_z;
    float cam2z = tv2->cam_z;
    float cam3z = tv3->cam_z;

    int min_x = min(sx1, min(sx2, sx3));
    int max_x = max(sx1, max(sx2, sx3));
    int min_y = min(sy1, min(sy2, sy3));
    int max_y = max(sy1, max(sy2, sy3));

    min_x = max(min_x, 0);
    max_x = min(max_x, WIDTH - 1);
    min_y = max(min_y, 0);
    max_y = min(max_y, HEIGHT - 1);

    int area2 = edge_function(sx1, sy1, sx2, sy2, sx3, sy3);
    if (area2 == 0) return;
    
    Uint32 colour = (r << 24) | (g << 16) | (b << 8) | 0xFF;

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            int e0 = edge_function(sx2, sy2, sx3, sy3, x, y);
            int e1 = edge_function(sx3, sy3, sx1, sy1, x, y);
            int e2 = edge_function(sx1, sy1, sx2, sy2, x, y);

            if ((area2 > 0 && e0 >= 0 && e1 >= 0 && e2 >= 0) ||
                (area2 < 0 && e0 <= 0 && e1 <= 0 && e2 <= 0)) {

                float w0 = (float)e0 / area2;
                float w1 = (float)e1 / area2;
                float w2 = (float)e2 / area2;

                float z = w0 * cam1z + w1 * cam2z + w2 * cam3z;

                int idx = y * WIDTH + x;
                if (z < depth_buffer[idx]) {
                    depth_buffer[idx] = z;
                    pixels[idx] = colour;
                }
            }
        }
    }
}
void Render_Frame(SDL_Renderer *renderer, PixelBuffer *pixels, Mesh *mesh, Colour* colours, Camera* cam) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        depth_buffer[i] = 1e10f;
    }

    memset(pixels, 0, WIDTH * HEIGHT * sizeof(PixelBuffer));

    size_t num_indices = arr_len(mesh->indices);
    size_t num_vertices = arr_len(mesh->vertices);
    TransformedVertex *tv = malloc(num_vertices * sizeof(TransformedVertex));
    
    for (size_t i = 0; i < num_vertices; i++) {
        transform_to_camera(cam, mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z,
                            &tv[i].cam_x, &tv[i].cam_y, &tv[i].cam_z);
        project_to_screen(cam, tv[i].cam_x, tv[i].cam_y, tv[i].cam_z,
                          &tv[i].sx, &tv[i].sy);
    }
    
    for (size_t i = 0; i < num_indices; i += 3) {
        size_t i0 = mesh->indices[i];
        size_t i1 = mesh->indices[i+1];
        size_t i2 = mesh->indices[i+2];
        
        if (!isTriangleFrontFacing(
            tv[i0].cam_x, tv[i0].cam_y, tv[i0].cam_z, 
            tv[i1].cam_x, tv[i1].cam_y, tv[i1].cam_z, 
            tv[i2].cam_x, tv[i2].cam_y, tv[i2].cam_z) or
            (tv[i0].cam_z <= 0 || tv[i1].cam_z <= 0 || tv[i2].cam_z <= 0)) {
                continue;
        }

        draw_triangle_projected(renderer, cam, pixels,
                                &tv[i0], &tv[i1], &tv[i2],
                                colours[i/3 % 3].r, 
                                colours[i/3 % 3].g, 
                                colours[i/3 % 3].b);
    }

    free(tv);

    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        WIDTH, HEIGHT);
    
    SDL_UpdateTexture(texture, NULL, pixels, WIDTH * sizeof(PixelBuffer));
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_DestroyTexture(texture);
}



// =======================================================================================
//                                 Main Loop Helper Functions
// =======================================================================================


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

void Cleanup(SDL_Window *window, SDL_Renderer *renderer) {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

Colour* SeedColourMap() {
    srand(time(NULL));
    Colour* colours = malloc(sizeof(Colour)*3);
    for (int i = 0; i < 3; i++) {
        colours[i].r = rand() % 256;
        colours[i].g = rand() % 256;
        colours[i].b = rand() % 256;
    }
    return colours;
}

void Render_Text(SDL_Renderer *renderer, TTF_Font *font, const char *text, int x, int y) {
    SDL_Color color = {255, 255, 255, 255};

    SDL_Surface *surface = TTF_RenderText_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) return;

    SDL_Rect dst = {x, y, 0, 0};
    SDL_QueryTexture(texture, NULL, NULL, &dst.w, &dst.h);

    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

void Run_Window(char* filename) {
    SDL_Renderer *renderer = NULL;
    SDL_Window *window = NULL;
    
    if (!Init_Window(&window, &renderer)) {
        printf("Failed to initialise SDL!\n");
        return;
    }
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return;
    }

    PixelBuffer pixels[WIDTH*HEIGHT];

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/liberation/LiberationMono-Regular.ttf", 24);
    
    Colour *colours = SeedColourMap();

    Camera camera = {
        .x = 0,
        .y = 0,
        .z = -300,
        .pitch = 0,
        .yaw = 0,
        .focal_length = 500
    };

    Mesh mesh;

    if (load_mesh_from_file(&mesh, filename) != 0) {
        fprintf(stderr, "Failed to load mesh from %s\n", filename);
        Cleanup(window, renderer);
        return;
    }

    app.mesh = &mesh;
    app.camera = camera;

    printf("Loaded %zu vertices, %zu indices (%zu triangles)\n",
           arr_len(mesh.vertices), arr_len(mesh.indices),
           arr_len(mesh.indices) / 3);
    
    float scale = 20;
    for (size_t i = 0; i < arr_len(mesh.vertices); i++) {
        mesh.vertices[i].x *= scale;
        mesh.vertices[i].y *= scale;
        mesh.vertices[i].z *= scale;
    }

    Camera_SetPosition(0, 0, -300);
    Camera_SetFocalLength(500);

    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    int running = 1;
    
    Uint32 frameStart;
    int frameTime;
    const int targetFPS = 60;
    const int frameDelay = 1000 / targetFPS;
    
    Uint32 fpsLastTime = SDL_GetTicks();
    int frameCount = 0;
    float currentFPS = 0;
    
    Console_Init();
    while (running) {
        frameStart = SDL_GetTicks();
        
        if (!Handle_Events()) {
            running = 0;
            break;
        }

        if (!Console_Update()) {
            running = 0;
            break;
        }

        running = Camera_Controls(keyboard_state);
        
        Render_Frame(renderer, pixels, &mesh, colours, &app.camera);

        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", currentFPS);
        Render_Text(renderer, font, fpsText, 10, 10); 

        SDL_RenderPresent(renderer);
        frameCount++;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - fpsLastTime >= 1000) {
            currentFPS = frameCount / ((currentTime - fpsLastTime) / 1000.0f);
            frameCount = 0;
            fpsLastTime = currentTime;
        }      

        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
    Console_Cleanup();
    mesh_free(&mesh);
    Cleanup(window, renderer);
}
