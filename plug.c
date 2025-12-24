#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <raylib.h>

typedef struct {
    Color background;
} Plug;

static Plug *p = NULL;

#define PLUG_IMPL
#include "plug.h"

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p != NULL);
    memset(p, 0, sizeof(*p));
    p->background = RED;
    TraceLog(LOG_INFO, "----------------------------");
    TraceLog(LOG_INFO, "Initialized plugin");
    TraceLog(LOG_INFO, "----------------------------");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    return p;
}

PLUG_EXPORT void plug_post_reload(void *state) {
    p = state;
}

PLUG_EXPORT void plug_update(void) {
    float w = GetScreenWidth();
    float h = GetScreenHeight();
    float t = GetTime();

    BeginDrawing();
    ClearBackground(GetColor(0x181818FF));

    float rw = 100.0;
    float rh = 100.0;
    float padding = rw * 0.15;
    Color cell_color = RED;

    for(size_t i = 0; i < 20; ++i) {
        DrawRectangle(i * (rw + padding) - w * (sinf(t * 2) + 1) * 0.5 , h/2 - rh/2, rw, rh, cell_color);
    }

    EndDrawing();
}
