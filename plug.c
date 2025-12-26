#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

#include "cJSON.h"

#define PLUG_IMPL
#include "plug.h"

// --- Constants & Config ---
#define SAMPLE_RATE 44100
#define STREAM_BUFFER_SIZE 1024

// --- Structures ---

typedef enum {
    SHAPE_CIRCLE,
    SHAPE_RECT,
    SHAPE_LINE
} ShapeType;

typedef struct {
    int id;
    ShapeType type;
    Vector2 position;
    Vector2 size; // x=width/radius, y=height
    Color color;
} Shape;

typedef enum {
    ANIM_TRANSLATE_X,
    ANIM_TRANSLATE_Y,
    ANIM_OPACITY
} AnimationType;

typedef struct {
    char id[64];
    int shapeId;
    AnimationType type;
    float startVal;
    float endVal;
    float startTime;
    float duration;
    bool loop;
} Animation;

#define MAX_SHAPES 100
#define MAX_ANIMS 100

typedef struct {
    int shape_count;
    Shape shapes[MAX_SHAPES];
    int anim_count;
    Animation anims[MAX_ANIMS];
} Scene;

typedef struct {
    float time;
    Camera2D camera;
    
    // Audio
    AudioStream stream;
    float phase;
    float freq;
    
    // Scene
    Scene scene;
    time_t last_file_mod;
    
    // Recording
    bool is_recording;
    float record_start_time;
    float record_duration;
    int record_fps;
    char record_output[256];
    int frame_count;
    time_t last_record_check;
    RenderTexture2D render_target;  // For clean video recording
} Plug;

static Plug *p = NULL;

// --- Helpers ---
Color ParseColor(const char *hex) {
    if (!hex || hex[0] != '#') return RED;
    unsigned int r, g, b;
    sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    return (Color){r, g, b, 255};
}

// --- Scene Loading ---
static void load_scene(const char *filename) {
    if (!p) return;
    
    FILE *f = fopen(filename, "rb");
    if (!f) return;
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(length + 1);
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);
    
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        printf("Error parsing JSON\n");
        free(data);
        return;
    }
    
    // Parse Shapes
    p->scene.shape_count = 0;
    cJSON *shapes = cJSON_GetObjectItem(json, "shapes");
    cJSON *shape = NULL;
    
    cJSON_ArrayForEach(shape, shapes) {
        if (p->scene.shape_count >= MAX_SHAPES) break;
        
        Shape *s = &p->scene.shapes[p->scene.shape_count++];
        
        cJSON *id = cJSON_GetObjectItem(shape, "id");
        if (id) s->id = id->valueint; // Note: Date.now() might exceed int range
        
        cJSON *type = cJSON_GetObjectItem(shape, "type");
        if (type && type->valuestring) {
            if (strcmp(type->valuestring, "circle") == 0) s->type = SHAPE_CIRCLE;
            else if (strcmp(type->valuestring, "rect") == 0) s->type = SHAPE_RECT;
            else if (strcmp(type->valuestring, "line") == 0) s->type = SHAPE_LINE;
        }
        
        cJSON *pos = cJSON_GetObjectItem(shape, "pos");
        if (pos) {
            s->position.x = (float)cJSON_GetArrayItem(pos, 0)->valuedouble;
            s->position.y = (float)cJSON_GetArrayItem(pos, 1)->valuedouble;
        }
        
        cJSON *size = cJSON_GetObjectItem(shape, "size");
        if (size) {
            if (cJSON_IsArray(size)) {
                s->size.x = (float)cJSON_GetArrayItem(size, 0)->valuedouble;
                s->size.y = (float)cJSON_GetArrayItem(size, 1)->valuedouble;
            } else {
                 s->size.x = (float)size->valuedouble;
            }
        }
        
        cJSON *color = cJSON_GetObjectItem(shape, "color");
        if (color && color->valuestring) {
            s->color = ParseColor(color->valuestring);
        } else {
            s->color = WHITE;
        }
    }

    // Parse Animations
    p->scene.anim_count = 0;
    cJSON *anims = cJSON_GetObjectItem(json, "animations");
    
    if (anims && cJSON_IsArray(anims)) {
        cJSON *anim = NULL;
        cJSON_ArrayForEach(anim, anims) {
        if (p->scene.anim_count >= MAX_ANIMS) break;
        
        Animation *a = &p->scene.anims[p->scene.anim_count++];
        
        cJSON *id = cJSON_GetObjectItem(anim, "id");
        if (id && id->valuestring) strncpy(a->id, id->valuestring, 63);
        
        cJSON *shapeId = cJSON_GetObjectItem(anim, "shapeId");
        if (shapeId) a->shapeId = shapeId->valueint;
        
        cJSON *type = cJSON_GetObjectItem(anim, "type");
        if (type && type->valuestring) {
            if (strcmp(type->valuestring, "translate_x") == 0) a->type = ANIM_TRANSLATE_X;
            else if (strcmp(type->valuestring, "translate_y") == 0) a->type = ANIM_TRANSLATE_Y;
            else if (strcmp(type->valuestring, "opacity") == 0) a->type = ANIM_OPACITY;
        }
        
        cJSON *start = cJSON_GetObjectItem(anim, "start");
        if (start) a->startVal = (float)start->valuedouble;
        
        cJSON *end = cJSON_GetObjectItem(anim, "end");
        if (end) a->endVal = (float)end->valuedouble;
        
        cJSON *startTime = cJSON_GetObjectItem(anim, "startTime");
        if (startTime) a->startTime = (float)startTime->valuedouble;
        
        cJSON *duration = cJSON_GetObjectItem(anim, "duration");
        if (duration) a->duration = (float)duration->valuedouble;
        
        cJSON *loop = cJSON_GetObjectItem(anim, "loop");
        if (loop) a->loop = cJSON_IsTrue(loop);
        }
    }
    
    cJSON_Delete(json);
    free(data);
    printf("Reloaded scene. shapes: %d, anims: %d\n", p->scene.shape_count, p->scene.anim_count);
}

static void check_hot_reload_scene() {
    if (!p) return;
    struct stat attr;
    if (stat("scene.json", &attr) == 0) {
        if (attr.st_mtime > p->last_file_mod) {
            load_scene("scene.json");
            p->last_file_mod = attr.st_mtime;
        }
    }
}

// --- Recording Control ---
static void check_recording_control() {
    if (!p) return;
    
    struct stat attr;
    if (stat("record_control.json", &attr) != 0) return;
    
    // Only check every second to avoid excessive file reads
    if (attr.st_mtime <= p->last_record_check) return;
    p->last_record_check = attr.st_mtime;
    
    FILE *f = fopen("record_control.json", "rb");
    if (!f) return;
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *data = malloc(length + 1);
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);
    
    cJSON *json = cJSON_Parse(data);
    if (!json) {
        free(data);
        return;
    }
    
    cJSON *recording = cJSON_GetObjectItem(json, "recording");
    bool should_record = recording && cJSON_IsTrue(recording);
    
    if (should_record && !p->is_recording) {
        // Start recording
        p->is_recording = true;
        p->record_start_time = p->time;
        p->frame_count = 0;
        
        cJSON *duration = cJSON_GetObjectItem(json, "duration");
        p->record_duration = duration ? (float)duration->valuedouble : 10.0f;
        
        cJSON *fps = cJSON_GetObjectItem(json, "fps");
        p->record_fps = fps ? fps->valueint : 60;
        
        cJSON *output = cJSON_GetObjectItem(json, "output");
        if (output && output->valuestring) {
            strncpy(p->record_output, output->valuestring, 255);
        } else {
            strcpy(p->record_output, "animation.mp4");
        }
        
        // Create frames directory
        system("mkdir -p frames");
        
        printf("Started recording: %s (%.1fs @ %d fps)\n", 
               p->record_output, p->record_duration, p->record_fps);
    } else if (!should_record && p->is_recording) {
        // Stop recording
        p->is_recording = false;
        printf("Stopped recording. Captured %d frames\n", p->frame_count);
        
        // Move frames to frames directory (Raylib saves to root)
        system("mkdir -p frames");
        system("mv frame_*.png frames/ 2>/dev/null");
        
        // Convert frames to video using ffmpeg
        char cmd[512];
        snprintf(cmd, sizeof(cmd), 
                 "ffmpeg -y -framerate %d -i frames/frame_%%04d.png -c:v libx264 -pix_fmt yuv420p %s 2>/dev/null",
                 p->record_fps, p->record_output);
        
        printf("Encoding video...\n");
        int result = system(cmd);
        
        if (result == 0) {
            printf("Video saved to: %s\n", p->record_output);
            // Clean up frames
            system("rm -rf frames");
        } else {
            printf("Error encoding video. Make sure ffmpeg is installed (brew install ffmpeg)\n");
            printf("Frames saved in ./frames/\n");
        }
    }
    
    cJSON_Delete(json);
    free(data);
}

// --- Audio ---
void PlugAudioCallback(void *bufferData, unsigned int frames) {
    if (!p) return;
    short *d = (short *)bufferData;
    for (unsigned int i = 0; i < frames; i++) {
        p->phase += p->freq / SAMPLE_RATE;
        if (p->phase > 1.0f) p->phase -= 1.0f;
        float val = sinf(2.0f * PI * p->phase);
        d[i*2] = (short)(val * 0.05f * 32000.0f);
        d[i*2+1] = (short)(val * 0.05f * 32000.0f);
    }
}

// --- Init ---
PLUG_EXPORT void plug_init(void) {
    p = malloc(sizeof(*p));
    assert(p);
    memset(p, 0, sizeof(*p));
    
    p->camera.zoom = 1.0f;
    p->camera.target = (Vector2){0, 0};
    p->camera.offset = (Vector2){0, 0}; 
    
    SetAudioStreamBufferSizeDefault(STREAM_BUFFER_SIZE);
    p->stream = LoadAudioStream(SAMPLE_RATE, 16, 2);
    SetAudioStreamCallback(p->stream, PlugAudioCallback);
    PlayAudioStream(p->stream);
    
    p->last_file_mod = 0;
    // Initial load
    check_hot_reload_scene();
    
    // Create render texture for clean recording (800x600 canvas size)
    p->render_target = LoadRenderTexture(800, 600);
    
    TraceLog(LOG_INFO, "CONA Animation Studio Initialized");
}

PLUG_EXPORT void *plug_pre_reload(void) {
    if (p) {
        StopAudioStream(p->stream);
        UnloadAudioStream(p->stream);
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

// --- Update ---
PLUG_EXPORT void plug_update(void) {
    if (!p) return;
    
    float dt = GetFrameTime();
    p->time += dt;
    
    check_hot_reload_scene();
    check_recording_control();
    
    // Handle recording
    if (p->is_recording) {
        float elapsed = p->time - p->record_start_time;
        if (elapsed >= p->record_duration) {
            // Auto-stop recording
            FILE *f = fopen("record_control.json", "w");
            if (f) {
                fprintf(f, "{\"recording\": false, \"duration\": %.1f, \"fps\": %d, \"output\": \"%s\"}", 
                        p->record_duration, p->record_fps, p->record_output);
                fclose(f);
            }
        }
    }
    
    // Pan/Zoom controls
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
         Vector2 delta = GetMouseDelta();
         delta = Vector2Scale(delta, -1.0f/p->camera.zoom);
         p->camera.target = Vector2Add(p->camera.target, delta);
    }
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), p->camera);
        p->camera.offset = GetMousePosition();
        p->camera.target = mouseWorldPos;
        float scaleFactor = 1.0f + (0.25f*fabsf(wheel));
        if (wheel < 0) scaleFactor = 1.0f/scaleFactor;
        p->camera.zoom = Clamp(p->camera.zoom*scaleFactor, 0.125f, 64.0f);
    }


    // Render scene to texture (for clean recording)
    BeginTextureMode(p->render_target);
    ClearBackground(GetColor(0x181818FF)); // Dark background
    
    BeginMode2D(p->camera);
    
    // Draw Shapes with Animation Applied
    for (int i=0; i<p->scene.shape_count; i++) {
        Shape s = p->scene.shapes[i]; // Copy to modify locally
        
        // Apply Animations
        for (int j=0; j<p->scene.anim_count; j++) {
            Animation *a = &p->scene.anims[j];
            if (a->shapeId != s.id) continue;
            
            float t = p->time - a->startTime;
            if (t < 0) {
                 // Not yet started
                 // Optional: apply start val? Or keep original?
                 // startVal overrides original pos
                 if (a->type == ANIM_TRANSLATE_X) s.position.x = a->startVal;
                 if (a->type == ANIM_TRANSLATE_Y) s.position.y = a->startVal;
                 if (a->type == ANIM_OPACITY) s.color.a = (unsigned char)(a->startVal * 255);
                 continue;
            } 
            
            if (a->duration <= 0.001f) continue;
            
            if (a->loop) {
                 t = fmodf(t, a->duration);
            } else if (t > a->duration) {
                 t = a->duration;
            }
            
            float alpha = t / a->duration;
            // Simple Linear interpolation
            float val = a->startVal + (a->endVal - a->startVal) * alpha;
            
            if (a->type == ANIM_TRANSLATE_X) s.position.x = val;
            else if (a->type == ANIM_TRANSLATE_Y) s.position.y = val;
            else if (a->type == ANIM_OPACITY) s.color = ColorAlpha(s.color, val);
        }
        
        // Render
        switch (s.type) {
            case SHAPE_CIRCLE:
                DrawCircleV(s.position, s.size.x, s.color);
                break;
            case SHAPE_RECT:
                DrawRectangleV(s.position, s.size, s.color);
                break;
            case SHAPE_LINE:
                DrawLineEx(s.position, Vector2Add(s.position, s.size), 2.0f, s.color);
                break;
        }
    }
    
    EndMode2D();
    EndTextureMode();
    
    // Draw to screen
    BeginDrawing();
    ClearBackground(GetColor(0x181818FF));
    
    // Draw the render texture
    DrawTextureRec(p->render_target.texture, 
                   (Rectangle){0, 0, (float)p->render_target.texture.width, -(float)p->render_target.texture.height},
                   (Vector2){0, 0}, WHITE);
    
    // Draw grid overlay on screen only (not in recording)
    BeginMode2D(p->camera);
    int cols = 20;
    int rows = 20;
    float spacing = 50.0f;
    for(int i=0; i<=cols; ++i) DrawLine(i*spacing, 0, i*spacing, rows*spacing, Fade(WHITE, 0.1f));
    for(int i=0; i<=rows; ++i) DrawLine(0, i*spacing, cols*spacing, i*spacing, Fade(WHITE, 0.1f));
    EndMode2D();
    
    // UI Overlay (right side, outside the canvas area)
    int screenWidth = GetScreenWidth();
    int uiX = screenWidth - 350; // Position from right edge
    
    DrawFPS(uiX, 10);
    DrawText("Animation Studio", uiX, 30, 20, LIGHTGRAY);
    DrawText("Controls:", uiX, 55, 16, GRAY);
    DrawText("Right Click: Pan", uiX, 75, 14, DARKGRAY);
    DrawText("Scroll: Zoom", uiX, 92, 14, DARKGRAY);
    
    if (p->is_recording) {
        float elapsed = p->time - p->record_start_time;
        DrawText(TextFormat("REC: %.1f/%.1fs", elapsed, p->record_duration), 
                 uiX, 120, 18, RED);
    }
    
    EndDrawing();
    
    // Capture frame if recording (from render texture, not screen)
    if (p->is_recording) {
        char filename[256];
        snprintf(filename, sizeof(filename), "frame_%04d.png", p->frame_count);
        
        // Export the render texture as image
        Image img = LoadImageFromTexture(p->render_target.texture);
        ImageFlipVertical(&img); // Flip because texture is upside down
        ExportImage(img, filename);
        UnloadImage(img);
        
        p->frame_count++;
    }
}

PLUG_EXPORT bool plug_finished(void) {
    return false;
}
