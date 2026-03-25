#include "console.h"
#include "export.h"
#include "render.h"  // For camera functions
#include "dynarr.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <math.h>

static struct termios original_termios;
static int console_initialized = 0;
static int exit_requested = 0;

// Command handlers
// Rotate mesh using pitch (X-axis) and yaw (Y-axis)

void cmd_rotate_mesh(float pitch, float yaw, float roll) {
    Mesh *mesh;
    Mesh_GetInfo(&mesh);
    
    if (!mesh) {
        printf("\n  Error: No mesh loaded!\n");
        printf("> ");
        fflush(stdout);
        return;
    }
    
    if (arr_len(mesh->vertices) == 0) {
        printf("\n  Error: Mesh has no vertices!\n");
        printf("> ");
        fflush(stdout);
        return;
    }

    // Convert to radians
    float pitch_rad = pitch * M_PI / 180.0f;
    float yaw_rad = yaw * M_PI / 180.0f;
    float roll_rad = roll * M_PI / 180.0f;
     
    // Precompute trig values
    float cp = cosf(pitch_rad), sp = sinf(pitch_rad);
    float cy = cosf(yaw_rad), sy = sinf(yaw_rad);
    float cr = cosf(roll_rad), sr = sinf(roll_rad);
    
    // Combined rotation matrix: Roll (Z) * Pitch (X) * Yaw (Y)
    // Or whatever order you prefer. Here's ZYX order (roll, pitch, yaw)
    float m00 = cy * cr + sy * sp * sr;
    float m01 = -cy * sr + sy * sp * cr;
    float m02 = sy * cp;
    
    float m10 = cp * sr;
    float m11 = cp * cr;
    float m12 = -sp;
    
    float m20 = -sy * cr + cy * sp * sr;
    float m21 = sy * sr + cy * sp * cr;
    float m22 = cy * cp;
    
    // Apply to all vertices
    size_t vertex_count = arr_len(mesh->vertices);
    for (size_t i = 0; i < vertex_count; i++) {
        Vertex* v = &mesh->vertices[i];
        float x = v->x, y = v->y, z = v->z;
        
        v->x = m00 * x + m01 * y + m02 * z;
        v->y = m10 * x + m11 * y + m12 * z;
        v->z = m20 * x + m21 * y + m22 * z;
    }

    printf("\n  Rotation set to pitch=%.2f, yaw=%.2f, roll=%.2f\n", pitch, yaw, roll);
    printf("> ");
    fflush(stdout);
}

void cmd_export_mesh(const char* filename) {
    Mesh *mesh;
    Mesh_GetInfo(&mesh);
    load_mesh_to_file(mesh, filename);     

    printf("\nExported: %s\n", filename);
    printf("> ");
    fflush(stdout);
}

static void cmd_teleport(float x, float y, float z) {
    Camera_SetPosition(x, y, z);
    printf("\n  Teleported to (%.1f, %.1f, %.1f)\n", x, y, z);
    printf("> ");
    fflush(stdout);
}

static void cmd_home(void) {
    Camera_SetPosition(0, 0, -300);
    Camera_SetRotation(0, 0);
    printf("\n  Teleported home\n> ");
    fflush(stdout);
}

static void cmd_lookat(float x, float y, float z) {
    Camera cam;
    Camera_GetInfo(&cam);
    
    float dx = x - cam.x;
    float dy = y - cam.y;
    float dz = z - cam.z;
    
    float yaw = atan2(dx, dz);
    float horiz = sqrt(dx*dx + dz*dz);
    float pitch = atan2(dy, horiz);
    
    Camera_SetRotation(pitch, yaw);
    printf("\n  Now looking at (%.1f, %.1f, %.1f)\n", x, y, z);
    printf("> ");
    fflush(stdout);
}

static void cmd_rotate_cam(float pitch, float yaw) {
    Camera_SetRotation(pitch, yaw);
    printf("\n  Rotation set to pitch=%.2f, yaw=%.2f\n", pitch, yaw);
    printf("> ");
    fflush(stdout);
}

static void cmd_info(void) {
    Camera cam;
    Camera_GetInfo(&cam);
    printf("\n  Position: (%.1f, %.1f, %.1f)\n", cam.x, cam.y, cam.z);
    printf("  Rotation: pitch=%.2f, yaw=%.2f\n", cam.pitch, cam.yaw);
    printf("  Focal length: %.1f\n", cam.focal_length);
    printf("> ");
    fflush(stdout);
}

static void cmd_help(void) {
    printf("\n");
    printf("  tp        (x, y, z)           - Teleport to coordinates\n");
    printf("  home      (none)              - Teleport to (0, 0, -300)\n");
    printf("  lookat    (x, y, z)           - Look at a point\n");
    printf("  rotcam    (pitch, yaw)        - Set rotation\n");
    printf("  rotmesh   (pitch, yaw, roll)  - Set rotation\n");
    printf("  info      (none)              - Show camera info\n");
    printf("  export    (filename)          - Export mesh to file\n");
    printf("  exit      (none)              - Exit the application\n");
    printf("  help      (none)              - Show this help\n");
    printf("  ===\n");
    printf("> ");
    fflush(stdout);
}

static void process_command(char* line) {
    // Remove newline if present
    line[strcspn(line, "\n")] = 0;
    
    // Skip empty lines
    if (strlen(line) == 0) {
        printf("> ");
        fflush(stdout);
        return;
    }
    
    // Parse command
    float x, y, z, pitch, yaw, roll;
    char filename[64];
    
    if (sscanf(line, "tp %f %f %f", &x, &y, &z) == 3) {
        cmd_teleport(x, y, z);
    }
    else if (strcmp(line, "home") == 0) {
        cmd_home();
    }
    else if (sscanf(line, "lookat %f %f %f", &x, &y, &z) == 3) {
        cmd_lookat(x, y, z);
    }
    else if (sscanf(line, "rotcam %f %f", &pitch, &yaw) == 2) {
        cmd_rotate_cam(pitch, yaw);
    }
    else if (sscanf(line, "rotmesh %f %f %f", &pitch, &yaw, &roll) == 3) {
        cmd_rotate_mesh(pitch, yaw, roll);
    }
    else if (sscanf(line, "export %s", filename) == 1) {
        cmd_export_mesh(filename);
    }
    else if (strcmp(line, "info") == 0) {
        cmd_info();
    }
    else if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
        printf("\n  Exiting application...\n");
        exit_requested = 1;
    }
    else if (strcmp(line, "help") == 0) {
        cmd_help();
    }
    else {
        printf("\n  Unknown command. Type 'help'\n> ");
        fflush(stdout);
    }
}

void Console_Init(void) {
    if (console_initialized) return;
    
    // Save original terminal attributes
    tcgetattr(STDIN_FILENO, &original_termios);
    
    // Set terminal to non-canonical mode (no line buffering)
    struct termios newt = original_termios;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    console_initialized = 1;
    exit_requested = 0;
    
    printf("> ");
    fflush(stdout);
}

bool Console_Update(void) {
    if (!console_initialized) return true;
    if (exit_requested) return false;
    
    static char line_buffer[256];
    static int buffer_pos = 0;
    char ch;
    
    // Read all available characters
    while (read(STDIN_FILENO, &ch, 1) > 0) {
        if (ch == '\n' || ch == '\r') {
            // Enter key - process command
            line_buffer[buffer_pos] = '\0';
            if (buffer_pos > 0) {
                process_command(line_buffer);
            } else {
                printf("\n> ");
                fflush(stdout);
            }
            buffer_pos = 0;
        }
        else if (ch == '\b' || ch == 127) {
            // Backspace
            if (buffer_pos > 0) {
                buffer_pos--;
                printf("\b \b");
                fflush(stdout);
            }
        }
        else if (ch >= 32 && ch < 127 && buffer_pos < 255) {
            // Printable character
            line_buffer[buffer_pos++] = ch;
            printf("%c", ch);
            fflush(stdout);
        }
    }
    
    return !exit_requested;
}

void Console_Cleanup(void) {
    if (!console_initialized) return;
    
    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
    
    // Restore blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    
    console_initialized = 0;
    printf("\n");
}
