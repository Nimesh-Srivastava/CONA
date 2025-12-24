#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <raylib.h>
#include <raymath.h>

#define PLUG_IMPL
#include "plug.h"

typedef struct {
    float time;
    Camera3D camera;
} Plug;

static Plug *p = NULL;

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));

    p->camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    p->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    p->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    p->camera.fovy = 45.0f;
    p->camera.projection = CAMERA_PERSPECTIVE;

    TraceLog(LOG_INFO, "CONA main plug initialized");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    return p;
}

PLUG_EXPORT void plug_post_reload(void *state) {
    p = state;
}

PLUG_EXPORT void plug_update(void) {
    float dt = GetFrameTime();
    p->time += dt;

    UpdateCamera(&p->camera, CAMERA_ORBITAL);

    BeginDrawing();
    ClearBackground(GetColor(0x181818FF)); // Soft dark grey

    BeginMode3D(p->camera);
    
    DrawGrid(20, 1.0f);
    
    // Dynamic sine wave of cubes
    for (int x = -10; x < 10; x++) {
        for (int z = -10; z < 10; z++) {
            float dist = sqrtf(x*x + z*z);
            float y = sinf(dist * 0.5f - p->time * 2.0f);
            
            Color c = ColorFromHSV((dist * 5.0f + p->time * 10.0f), 0.8f, 0.8f);
            
            Vector3 pos = { (float)x, y, (float)z };
            DrawCube(pos, 0.6f, 0.6f, 0.6f, c);
            DrawCubeWires(pos, 0.6f, 0.6f, 0.6f, Fade(BLACK, 0.3f));
        }
    }

    EndMode3D();

    DrawFPS(10, 10);
    DrawText("MAIN PHASE - PRESS R TO RELOAD", 10, 40, 20, RAYWHITE);
    
    EndDrawing();
}

PLUG_EXPORT bool plug_finished(void) {
    return false; // Endless loop
}
