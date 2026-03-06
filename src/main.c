#include "raylib.h"
// implementation gui

int main()
{
    const int sw = 800;
    const int sh = 450;

    InitWindow(sw, sh, "3drenderer");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("window", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
