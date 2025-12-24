#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <raylib.h>

#define PLUG_IMPL
#include "plug.h"

typedef struct {
    float start_time;
    bool finished;
} Plug;

static Plug *p = NULL;

// Smoothstep helper
static float smooth(float t) {
    return t * t * (3.0f - 2.0f * t);
}

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));

    p->start_time = (float)GetTime();

    TraceLog(LOG_INFO, "CONA splash initialized");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    return p;
}

PLUG_EXPORT void plug_post_reload(void *state) {
    p = state;
}

PLUG_EXPORT void plug_update(void) {
    float w = (float)GetScreenWidth();
    float h = (float)GetScreenHeight();

    float t = (float)GetTime() - p->start_time;
    const float duration = 3.5f;

    if (t > duration) {
        p->finished = true;
    }

    BeginDrawing();
    ClearBackground(GetColor(0x0B0B0FFF));

    float cx = w * 0.5f;
    float cy = h * 0.5f;

    // --------------------------------------------------
    // Pulse
    // --------------------------------------------------
    if (t < 1.0f) {
        float pulse = smooth(fminf(t / 0.8f, 1.0f));
        float r = 24.0f + pulse * 36.0f;
        DrawCircle(cx, cy, r, Fade(WHITE, 0.9f));
    }

    // --------------------------------------------------
    // Expanding ring
    // --------------------------------------------------
    if (t > 0.4f && t < 1.6f) {
        float rt = smooth((t - 0.4f) / 1.2f);
        float r = 60.0f + rt * 180.0f;
        DrawCircleLines(cx, cy, r, Fade(WHITE, 0.6f * (1.0f - rt)));
    }

    // --------------------------------------------------
    // Text reveal
    // --------------------------------------------------
    if (t > 1.0f) {
        float tt = smooth(fminf((t - 1.0f) / 0.8f, 1.0f));
        float alpha = tt;

        const char *title = "CONA";
        int font_size = 64;
        Vector2 size = MeasureTextEx(GetFontDefault(), title, font_size, 4);

        DrawTextEx(
            GetFontDefault(),
            title,
            (Vector2){ cx - size.x * 0.5f, cy + 80.0f },
            font_size,
            4,
            Fade(WHITE, alpha)
        );

        DrawText(
            "Clean Open Native Animation",
            cx - 140,
            cy + 150,
            16,
            Fade(WHITE, alpha * 0.7f)
        );
    }

    // --------------------------------------------------
    // Fade out
    // --------------------------------------------------
    if (t > 2.5f) {
        float ft = smooth((t - 2.5f) / 1.0f);
        DrawRectangle(0, 0, w, h, Fade(BLACK, ft));
    }

    EndDrawing();
}

PLUG_EXPORT bool plug_finished(void) {
    return p && p->finished;
}
