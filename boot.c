#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#include <raylib.h>

#define PLUG_IMPL
#include "plug.h"

#define MAX_LOG_LINES 12
#define MATRIX_COLS 100
#define MATRIX_ROWS 40

typedef struct {
    float start_time;
    bool finished;
    int line_count;
    char lines[MAX_LOG_LINES][64];
    float line_times[MAX_LOG_LINES];
    
    // Matrix Rain state
    float drops[MATRIX_COLS];
    char chars[MATRIX_COLS][MATRIX_ROWS];
} Plug;

static Plug *p = NULL;

static float rand_float(void) {
    return (float)rand() / (float)RAND_MAX;
}

static char rand_char(void) {
    return (char)('0' + (rand() % 10)); // 0-9
}

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));

    p->start_time = (float)GetTime();
    
    // Fake boot log
    const char *boot_msgs[] = {
        "Initializing kernel...",
        "Loading drivers...",
        "Mounting file system...",
        "Allocating memory pages...",
        "Checking integrity...",
        "Starting services...",
        "Connecting to network...",
        "Fetching remote config...",
        "Optimizing shaders...",
        "Pre-rendering lightmaps...",
        "System READY."
    };
    
    float current_time = 0.5f;
    for (int i = 0; i < 11; ++i) {
        if (i >= MAX_LOG_LINES) break;
        strcpy(p->lines[i], boot_msgs[i]);
        p->line_times[i] = current_time;
        current_time += 0.2f + rand_float() * 0.3f;
        p->line_count++;
    }

    // Init Matrix
    for (int i = 0; i < MATRIX_COLS; ++i) {
        p->drops[i] = rand_float() * -MATRIX_ROWS;
        for (int j = 0; j < MATRIX_ROWS; ++j) {
            p->chars[i][j] = rand_char();
        }
    }

    TraceLog(LOG_INFO, "CONA boot initialized");
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
    float dt = GetFrameTime();

    float t = (float)GetTime() - p->start_time;
    const float duration = 5.0f;

    if (t > duration) {
        p->finished = true;
    }

    // Update Matrix
    for (int i = 0; i < MATRIX_COLS; ++i) {
        p->drops[i] += dt * 15.0f; // Speed
        if (p->drops[i] > MATRIX_ROWS) {
            p->drops[i] = rand_float() * -10.0f;
            for (int j = 0; j < MATRIX_ROWS; ++j) {
                p->chars[i][j] = rand_char();
            }
        }
    }

    BeginDrawing();
    ClearBackground(BLACK);

    // Draw Matrix Rain
    int font_size = 14;
    for (int i = 0; i < MATRIX_COLS; ++i) {
        for (int j = 0; j < MATRIX_ROWS; ++j) {
            float y = (float)j;
            if (y < p->drops[i] && y > p->drops[i] - 10.0f) {
                float intensity = 1.0f - (p->drops[i] - y) / 10.0f;
                Color color = (j % 3 == 0) ? GREEN : DARKGREEN;
                color = Fade(color, intensity * 0.5f); // Background layer opacity
                
                // Randomly flicker characters
                if (rand() % 100 < 2) p->chars[i][j] = rand_char();

                DrawText(TextFormat("%c", p->chars[i][j]), i * font_size, j * font_size, font_size, color);
            }
        }
    }

    // Draw Terminal Text (Foreground)
    DrawRectangle(0, 0, w, h, Fade(BLACK, 0.7f)); // Dim background for text readability

    int start_y = h / 2 - (p->line_count * 20) / 2;
    for (int i = 0; i < p->line_count; ++i) {
        if (t >= p->line_times[i]) {
            DrawText(p->lines[i], 100, start_y + i * 25, 20, LIME);
        }
    }
    
    // Loading bar at bottom
    if (t > 0.2f) {
        float progress = (t - 0.2f) / (duration - 1.0f);
        if (progress > 1.0f) progress = 1.0f;
        
        float bar_w = w * 0.6f;
        float bar_h = 4.0f;
        float bar_x = (w - bar_w) / 2;
        float bar_y = h - 100;
        
        DrawRectangleLines(bar_x, bar_y, bar_w, bar_h, DARKGREEN);
        DrawRectangle(bar_x, bar_y, bar_w * progress, bar_h, GREEN);
    }
    
    // Fade out
    if (t > duration - 0.5f) {
       float alpha = (t - (duration - 0.5f)) / 0.5f;
       DrawRectangle(0, 0, w, h, Fade(BLACK, alpha));
    }

    EndDrawing();
}

PLUG_EXPORT bool plug_finished(void) {
    return p && p->finished;
}
