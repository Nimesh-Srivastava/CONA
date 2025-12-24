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

// Audio Settings
#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024

// Visual settings
#define CUBE_ROWS 20
#define CUBE_COLS 20
#define CUBE_SPACING 1.2f

typedef struct {
    float time;
    Camera3D camera;
    AudioStream stream;
    float *wave_data; // Pointer to current wave data for visualization
    
    RenderTexture2D target;
    Shader bloom;
    
    // Synth state
    float phase;
    float freq;
    int note_index;
    float beat_timer;
} Plug;

static Plug *p = NULL;

// Cyberpunk arpeggio sequence
const float scale[] = { 110.0f, 130.81f, 146.83f, 164.81f, 196.00f, 220.0f, 261.63f, 293.66f };
const int sequence[] = { 0, 2, 4, 2, 5, 2, 4, 1, 0, 3, 5, 3, 6, 3, 5, 1 };


// Audio callback
void PlugAudioCallback(void *bufferData, unsigned int frames) {
    if (!p) return;
    
    short *d = (short *)bufferData;
    
    for (unsigned int i = 0; i < frames; i++) {
        p->phase += p->freq / SAMPLE_RATE;
        if (p->phase > 1.0f) p->phase -= 1.0f;
        
        // FM Synthesis: Carrier + Modulator
        float modulator = sinf(2.0f * PI * p->phase * 0.5f) * 0.5f;
        float carrier = sinf(2.0f * PI * p->phase + modulator);
        
        // Simple Envelope (sawtooth-ish volume)
        float vol = 0.5f; 
        
        d[i*2] = (short)(carrier * vol * 32000.0f);     // Left
        d[i*2+1] = (short)(carrier * vol * 32000.0f);   // Right
    }
    
    // Capture some data for visualization (Just grabbing the last frame's amplitude roughly)
    // In a real scenario we'd do a copy, but here we just let the main loop read state derived from phase/freq
}

PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));

    p->camera.position = (Vector3){ 18.0f, 18.0f, 18.0f };
    p->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    p->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    p->camera.fovy = 45.0f;
    p->camera.projection = CAMERA_PERSPECTIVE;

    // Audio Init
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    p->stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(p->stream, PlugAudioCallback);
    PlayAudioStream(p->stream);
    
    p->freq = 110.0f;
    
    // Shader Init
    p->target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    p->bloom = LoadShader(0, "bloom.fs");

    TraceLog(LOG_INFO, "CONA main plug initialized (Audio + Bloom)");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    if (p) {
        UnloadRenderTexture(p->target);
        UnloadShader(p->bloom);
        StopAudioStream(p->stream);
        UnloadAudioStream(p->stream);
    }
    return p;
}

PLUG_EXPORT void plug_post_reload(void *state) {
    p = state;
    // Re-init resources that can't be passed easily or if we want to refresh them
    p->target = LoadRenderTexture(GetScreenWidth(), GetScreenHeight());
    p->bloom = LoadShader(0, "bloom.fs");
    
    p->stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(p->stream, PlugAudioCallback);
    PlayAudioStream(p->stream);
}

PLUG_EXPORT void plug_update(void) {
    float dt = GetFrameTime();
    p->time += dt;
    
    // Sequencer
    p->beat_timer += dt;
    float bpm = 480.0f; // Fast arpeggio
    float beat_duration = 60.0f / bpm;
    if (p->beat_timer > beat_duration) {
        p->beat_timer -= beat_duration;
        p->note_index = (p->note_index + 1) % 16;
        p->freq = scale[sequence[p->note_index] % 8];
        if (sequence[p->note_index] >= 8) p->freq *= 2.0f;
    }

    UpdateCamera(&p->camera, CAMERA_ORBITAL);

    // Render to texture
    BeginTextureMode(p->target);
    ClearBackground(BLACK);
    
    BeginMode3D(p->camera);
    
    // Draw visualizer grid
    float offset_x = (CUBE_COLS * CUBE_SPACING) / 2.0f;
    float offset_z = (CUBE_ROWS * CUBE_SPACING) / 2.0f;
    
    for (int x = 0; x < CUBE_COLS; x++) {
        for (int z = 0; z < CUBE_ROWS; z++) {
            // Distance from center
            float cx = (x * CUBE_SPACING) - offset_x;
            float cz = (z * CUBE_SPACING) - offset_z;
            float dist = sqrtf(cx*cx + cz*cz);
            
            // Audio reactive height
            // We use the current frequency and time to create a ripple
            float wave = sinf(dist * 0.5f - p->time * 5.0f);
            float active_freq_mod = (p->freq / 440.0f); // Higher pitch = more intensity
            
            float height = 1.0f + wave * 2.0f * active_freq_mod;
            if (height < 0.2f) height = 0.2f;
            
            // Color based on height/intensity
            Color c = ColorFromHSV(fmodf(p->time * 20.0f + dist * 5.0f, 360.0f), 0.8f, 0.9f);
            
            // Highlight current ring corresponding to note
            if (fabs(dist - (float)(sequence[p->note_index]*2)) < 2.0f) {
                c = WHITE;
                height += 2.0f;
            }

            Vector3 pos = { cx, height / 2.0f, cz };
            DrawCube(pos, 1.0f, height, 1.0f, c);
            DrawCubeWires(pos, 1.0f, height, 1.0f, Fade(BLACK, 0.5f));
        }
    }
    
    EndMode3D();
    
    // HUD
    DrawText("CONA AUDIO VISUALIZER", 20, 20, 20, WHITE);
    DrawFPS(20, 50);
    
    EndTextureMode();

    // Post-Process Render
    BeginDrawing();
    ClearBackground(BLACK);
    
    BeginShaderMode(p->bloom);
        // We have to flip the texture vertically because of OpenGL coordinates
        DrawTextureRec(p->target.texture, 
                      (Rectangle){ 0, 0, (float)p->target.texture.width, (float)-p->target.texture.height }, 
                      (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
    
    EndDrawing();
}

PLUG_EXPORT bool plug_finished(void) {
    return false;
}
