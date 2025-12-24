#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include <raylib.h>
#include <raymath.h>

#define PLUG_IMPL
#include "plug.h"

// --- Constants & Config ---
#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024

#define INPUT_ROWS 8
#define INPUT_COLS 8
#define INPUT_SIZE (INPUT_ROWS * INPUT_COLS)
#define HIDDEN1_SIZE 16
#define HIDDEN2_SIZE 16
#define OUTPUT_SIZE 10
#define TOTAL_LAYERS 4

// Colors
#define COL_BG          (Color){ 10, 10, 15, 255 }      // Deep Dark Blue/Black
#define COL_ACCENT      (Color){ 0, 120, 255, 255 }     // Electric Blue
#define COL_ACCENT_HOVER (Color){ 50, 150, 255, 255 }   // Lighter Blue
#define COL_TEXT_MAIN   (Color){ 240, 240, 240, 255 }   // Off-White
#define COL_TEXT_DIM    (Color){ 100, 100, 120, 255 }   // Dim Grey

// Bitmaps for Digits 0-9
const unsigned char FONT_DIGITS[10][8] = {
    { 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C }, // 0
    { 0x10, 0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38 }, // 1
    { 0x3C, 0x42, 0x02, 0x02, 0x3C, 0x40, 0x40, 0x7E }, // 2
    { 0x3C, 0x42, 0x02, 0x1C, 0x02, 0x02, 0x42, 0x3C }, // 3
    { 0x0C, 0x14, 0x24, 0x44, 0x7E, 0x04, 0x04, 0x04 }, // 4
    { 0x7E, 0x40, 0x40, 0x7C, 0x02, 0x02, 0x42, 0x3C }, // 5
    { 0x3C, 0x40, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x3C }, // 6
    { 0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x40 }, // 7
    { 0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x42, 0x3C }, // 8
    { 0x3C, 0x42, 0x42, 0x3E, 0x02, 0x02, 0x02, 0x3C }  // 9
};

// --- Structures ---

typedef enum {
    STATE_INPUT,
    STATE_PROPAGATE,
    STATE_OUTPUT,
    STATE_LEARN
} TrainState;

typedef enum {
    PLUG_MENU,
    PLUG_DEMO
} PlugState;

typedef struct {
    Vector3 position;
    float activation;
    float target;
    float error;
} Neuron;

typedef struct {
    int count;
    Neuron *neurons;
} Layer;

typedef struct {
    int layer_count;
    Layer layers[TOTAL_LAYERS];
} Network;

typedef struct {
    float time;
    Camera3D camera;
    AudioStream stream;
    
    // Core State
    PlugState state;
    float transition_alpha; // 0.0 -> 1.0 for fade
    
    // Synth
    float phase;
    float freq;
    float target_freq;
    
    // NN
    Network nn;
    TrainState train_state;
    float tr_timer;
    int current_digit;
    float signal_progress;
} Plug;

static Plug *p = NULL;

// --- Audio ---
void PlugAudioCallback(void *bufferData, unsigned int frames) {
    if (!p) return;
    short *d = (short *)bufferData;
    for (unsigned int i = 0; i < frames; i++) {
        p->phase += p->freq / SAMPLE_RATE;
        if (p->phase > 1.0f) p->phase -= 1.0f;
        
        float val = sinf(2.0f * PI * p->phase);
        float vol = 0.05f; 
        if (p->freq < 20.0f) vol = 0.0f;
        
        d[i*2] = (short)(val * vol * 32000.0f);
        d[i*2+1] = (short)(val * vol * 32000.0f);
    }
}

// --- NN & init ---
static void init_layer(Layer *l, int count) {
    l->count = count;
    l->neurons = malloc(sizeof(Neuron) * count);
    assert(l->neurons);
    memset(l->neurons, 0, sizeof(Neuron) * count);
}

static void init_network(void) {
    p->nn.layer_count = TOTAL_LAYERS;
    
    // Input Grid (8x8)
    init_layer(&p->nn.layers[0], INPUT_SIZE);
    float grid_spacing = 0.5f;
    for (int y = 0; y < INPUT_ROWS; y++) {
        for (int x = 0; x < INPUT_COLS; x++) {
            int idx = y * INPUT_COLS + x;
            p->nn.layers[0].neurons[idx].position = (Vector3){
                (x - INPUT_COLS/2.0f) * grid_spacing,
                (INPUT_ROWS/2.0f - y) * grid_spacing + 5.0f,
                -10.0f
            };
        }
    }
    
    // Hidden 1
    init_layer(&p->nn.layers[1], HIDDEN1_SIZE);
    for (int i = 0; i < HIDDEN1_SIZE; i++) {
        p->nn.layers[1].neurons[i].position = (Vector3){ 0, (i - HIDDEN1_SIZE/2.0f) * 1.5f + 3.0f, -2.0f };
    }
    // Hidden 2
    init_layer(&p->nn.layers[2], HIDDEN2_SIZE);
    for (int i = 0; i < HIDDEN2_SIZE; i++) {
        p->nn.layers[2].neurons[i].position = (Vector3){ 0, (i - HIDDEN2_SIZE/2.0f) * 1.5f + 3.0f, 6.0f };
    }
    // Output
    init_layer(&p->nn.layers[3], OUTPUT_SIZE);
    for (int i = 0; i < OUTPUT_SIZE; i++) {
         p->nn.layers[3].neurons[i].position = (Vector3){ (i - OUTPUT_SIZE/2.0f) * 2.5f, -5.0f, 14.0f };
    }
}

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));

    p->camera.position = (Vector3){ 20.0f, 15.0f, 20.0f };
    p->camera.target = (Vector3){ 0.0f, 0.0f, 5.0f };
    p->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    p->camera.fovy = 50.0f;
    p->camera.projection = CAMERA_PERSPECTIVE;

    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    p->stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(p->stream, PlugAudioCallback);
    PlayAudioStream(p->stream);

    init_network();
    
    // Start at Menu
    p->state = PLUG_MENU;
    p->transition_alpha = 0.0f;
    
    TraceLog(LOG_INFO, "CONA Production Engine Initialized");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    if (p) {
        StopAudioStream(p->stream);
        UnloadAudioStream(p->stream);
        for (int i=0; i<p->nn.layer_count; i++) {
            if (p->nn.layers[i].neurons) free(p->nn.layers[i].neurons);
        }
    }
    return p;
}

PLUG_EXPORT void plug_post_reload(void *state) {
    p = state;
    if (p) {
        p->stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
        SetAudioStreamCallback(p->stream, PlugAudioCallback);
        PlayAudioStream(p->stream);
    }
}

static float get_digit_pixel(int digit, int x, int y) {
    if (digit < 0 || digit > 9) return 0.0f;
    unsigned char row = FONT_DIGITS[digit][y];
    return (row & (1 << (7 - x))) ? 1.0f : 0.0f;
}

// --- Logic ---
static void UpdateNN(float dt) {
     switch (p->train_state) {
        case STATE_INPUT:
            if (p->tr_timer > 1.0f) {
                p->train_state = STATE_PROPAGATE;
                p->tr_timer = 0.0f;
                p->signal_progress = 0.0f;
                p->target_freq = 220.0f;
            } else {
                Layer *in = &p->nn.layers[0];
                for (int y=0; y<8; y++) {
                    for (int x=0; x<8; x++) {
                        float target = get_digit_pixel(p->current_digit, x, y);
                        in->neurons[y*8 + x].activation = Lerp(in->neurons[y*8 + x].activation, target, dt * 10.0f);
                    }
                }
                for (int i=1; i<TOTAL_LAYERS; i++) {
                     for(int j=0; j<p->nn.layers[i].count; j++) {
                         p->nn.layers[i].neurons[j].activation = Lerp(p->nn.layers[i].neurons[j].activation, 0.0f, dt * 5.0f);
                         p->nn.layers[i].neurons[j].error = 0.0f;
                     }
                 }
                 p->target_freq = 0.0f;
            }
            break;
        case STATE_PROPAGATE:
             p->signal_progress += dt * 1.5f;
             if (p->signal_progress >= 1.0f) {
                 p->train_state = STATE_OUTPUT;
                 p->tr_timer = 0.0f;
                 p->target_freq = 440.0f;
                 Layer *out = &p->nn.layers[TOTAL_LAYERS-1];
                 for (int i=0; i<OUTPUT_SIZE; i++) {
                     float val = (i == p->current_digit) ? 0.9f : 0.1f;
                     out->neurons[i].activation = val;
                 }
             }
             break;
        case STATE_OUTPUT:
             p->target_freq = 0.0f;
             if (p->tr_timer > 1.0f) {
                 p->train_state = STATE_LEARN;
                 p->tr_timer = 0.0f;
                 p->target_freq = 110.0f; 
             }
             break;
        case STATE_LEARN:
             p->target_freq = 0.0f;
             Layer *out = &p->nn.layers[TOTAL_LAYERS-1];
             for (int i=0; i<OUTPUT_SIZE; i++) {
                 if (i == p->current_digit) out->neurons[i].error = 1.0f;
                 else if (out->neurons[i].activation > 0.2f) out->neurons[i].error = -1.0f;
             }
             if (p->tr_timer > 1.0f) {
                 p->current_digit = (p->current_digit + 1) % 10;
                 p->train_state = STATE_INPUT;
                 p->tr_timer = 0.0f;
             }
             break;
    }
}

// --- Draw ---
static void DrawNN3D() {
    // Draw Connections
    for (int i=0; i<TOTAL_LAYERS-1; i++) {
        Layer *l1 = &p->nn.layers[i];
        Layer *l2 = &p->nn.layers[i+1];
        for (int j=0; j<l1->count; j++) {
            if (l1->neurons[j].activation < 0.1f && p->train_state != STATE_PROPAGATE) continue;
            for (int k=0; k<l2->count; k++) {
                float layer_start_t = (float)i / (TOTAL_LAYERS-1);
                float layer_end_t = (float)(i+1) / (TOTAL_LAYERS-1);
                bool active_path = false;
                if (p->train_state == STATE_PROPAGATE && p->signal_progress >= layer_start_t && p->signal_progress <= layer_end_t) {
                    float local_t = (p->signal_progress - layer_start_t) / (layer_end_t - layer_start_t);
                    Vector3 pos = Vector3Lerp(l1->neurons[j].position, l2->neurons[k].position, local_t);
                    DrawSphere(pos, 0.15f, GOLD);
                    active_path = true;
                }
                if (active_path || (l1->neurons[j].activation > 0.5f && l2->neurons[k].activation > 0.5f)) {
                     DrawLine3D(l1->neurons[j].position, l2->neurons[k].position, Fade(WHITE, 0.15f));
                } else if ((j+k)%7 == 0) {
                     DrawLine3D(l1->neurons[j].position, l2->neurons[k].position, Fade(GRAY, 0.03f));
                }
            }
        }
    }
    // Draw Neurons
    for (int i=0; i<TOTAL_LAYERS; i++) {
        Layer *l = &p->nn.layers[i];
        for (int j=0; j<l->count; j++) {
             Neuron *n = &l->neurons[j];
             Color c = WHITE;
             if (i == 0) c = COL_ACCENT;
             if (i == TOTAL_LAYERS-1) c = GOLD;
             if (n->error > 0.1f) c = GREEN;
             else if (n->error < -0.1f) c = RED;
             float alpha = (n->activation > 0.1f) ? (0.5f + n->activation*0.5f) : 0.2f;
             DrawSphere(n->position, 0.2f + n->activation * 0.3f, ColorAlpha(c, alpha));
             
             if (i == TOTAL_LAYERS-1) {
                 Vector2 sc = GetWorldToScreen(n->position, p->camera);
                 if (sc.x > 0) DrawText(TextFormat("%d", j), sc.x-5, sc.y-25, 20, RAYWHITE);
             }
        }
    }
    DrawGrid(20, 1.0f);
}

PLUG_EXPORT void plug_update(void) {
    float dt = GetFrameTime();
    p->time += dt;
    p->tr_timer += dt;
    p->freq = Lerp(p->freq, p->target_freq, dt * 5.0f);
    
    // Background Animation (always run a bit of NN update for visual flair in menu)
    if (p->state == PLUG_MENU) {
        UpdateCamera(&p->camera, CAMERA_ORBITAL); // Gentle auto-rotation allowed in menu? Or just static?
        // Let's do a slow auto-orbit for menu background
        Matrix rot = MatrixRotateY(dt * 0.2f);
        p->camera.position = Vector3Transform(p->camera.position, rot);
        
        // Still run NN but no sound freq
        UpdateNN(dt); 
        p->target_freq = 0.0f; // Silence in menu
    } else {
        UpdateCamera(&p->camera, CAMERA_THIRD_PERSON); // User control in demo
        UpdateNN(dt);
    }

    BeginDrawing();
    ClearBackground(COL_BG);
    
    // 1. Draw 3D Background/Network
    BeginMode3D(p->camera);
        DrawNN3D();
    EndMode3D();
    
    // 2. Draw UI Overlay
    if (p->state == PLUG_MENU) {
        // Dim the background
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
        
        // Title
        int titleFontSize = 60;
        const char *titleText = "CONA ENGINE";
        int titleWidth = MeasureText(titleText, titleFontSize);
        DrawText(titleText, GetScreenWidth()/2 - titleWidth/2, GetScreenHeight()/3, titleFontSize, COL_TEXT_MAIN);
        
        // Subtitle
        int subFontSize = 24;
        const char *subText = "Refined C Animation & ML Visualization";
        int subWidth = MeasureText(subText, subFontSize);
        DrawText(subText, GetScreenWidth()/2 - subWidth/2, GetScreenHeight()/3 + 70, subFontSize, COL_TEXT_DIM);
        
        // Button
        int btnWidth = 280;
        int btnHeight = 60;
        Rectangle btnRect = { 
            GetScreenWidth()/2 - btnWidth/2, 
            GetScreenHeight()/2 + 50, 
            btnWidth, btnHeight 
        };
        
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, btnRect);
        
        Color btnColor = hover ? COL_ACCENT_HOVER : COL_ACCENT;
        DrawRectangleRounded(btnRect, 0.3f, 8, btnColor);
        
        // Glow effect
        if (hover) {
            DrawRectangleRoundedLines(btnRect, 0.3f, 8, WHITE);
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        } else {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        
        const char *btnText = "LAUNCH SIMULATION";
        int btnTextW = MeasureText(btnText, 20);
        DrawText(btnText, btnRect.x + btnWidth/2 - btnTextW/2, btnRect.y + btnHeight/2 - 10, 20, WHITE);
        
        // Click Logic
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            p->state = PLUG_DEMO;
            // Reset Camera for demo
            p->camera.position = (Vector3){ 20.0f, 15.0f, 20.0f };
        }
        
    } else if (p->state == PLUG_DEMO) {
        // Simple Back Button
        Rectangle backRect = { 20, 20, 100, 30 };
        Vector2 mouse = GetMousePosition();
        bool hover = CheckCollisionPointRec(mouse, backRect);
        
        Color btnColor = hover ? COL_ACCENT_HOVER : Fade(COL_ACCENT, 0.5f);
        DrawRectangleRounded(backRect, 0.5f, 4, btnColor);
        if (hover) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        else SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        
        DrawText("<< MENU", backRect.x + 15, backRect.y + 8, 10, WHITE);
        
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            p->state = PLUG_MENU;
        }
        
        // Status HUD
        DrawText("3D TRAINING SIMULATION", 20, GetScreenHeight() - 40, 20, COL_TEXT_DIM);
    }
    
    EndDrawing();
}

PLUG_EXPORT bool plug_finished(void) {
    return false;
}
