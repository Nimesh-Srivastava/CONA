#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
    BeginDrawing();
    ClearBackground(p->background);
    EndDrawing();
}
