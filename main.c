#include <stdio.h>
#include <stdbool.h>

#include <raylib.h>

#include "plug.h"
#include "platform.h"

/* ------------------------------------------------------------
   Platform-specific plugin filenames
------------------------------------------------------------ */

#if defined(_WIN32)
    #define SPLASH_PLUG "cona_splash.dll"
    #define BOOT_PLUG   "cona_boot.dll"
    #define MAIN_PLUG   "plug.dll"
#elif defined(__APPLE__)
    #define SPLASH_PLUG "libcona_splash.dylib"
    #define BOOT_PLUG   "libcona_boot.dylib"
    #define MAIN_PLUG   "libplug.dylib"
#else
    #define SPLASH_PLUG "libcona_splash.so"
    #define BOOT_PLUG   "libcona_boot.so"
    #define MAIN_PLUG   "libplug.so"
#endif

/* ------------------------------------------------------------
   Engine phase state machine
------------------------------------------------------------ */

typedef enum {
    PHASE_SPLASH,
    PHASE_BOOT,
    PHASE_MAIN
} EnginePhase;

/* ------------------------------------------------------------
   Plugin state
------------------------------------------------------------ */

static void *libplug = NULL;

static plug_init_t        plug_init        = NULL;
static plug_pre_reload_t plug_pre_reload   = NULL;
static plug_post_reload_t plug_post_reload = NULL;
static plug_update_t     plug_update       = NULL;
static plug_finished_t   plug_finished     = NULL;

/* ------------------------------------------------------------
   Plugin loader
------------------------------------------------------------ */

#define LOAD_SYM_REQ(name)                                      \
    do {                                                        \
        name = (name##_t)platform_get_symbol(libplug, #name);  \
        if (!name) {                                           \
            fprintf(stderr, "Missing symbol: %s\n", #name);    \
            return false;                                      \
        }                                                       \
    } while (0)

#define LOAD_SYM_OPT(name)                                      \
    do {                                                        \
        name = (name##_t)platform_get_symbol(libplug, #name);  \
    } while (0)

static bool load_plugin(const char *filename) {
    if (libplug) {
        platform_free_library(libplug);
        libplug = NULL;
    }

    libplug = platform_load_library(filename);
    if (!libplug) {
        fprintf(stderr, "Failed to load plugin: %s\n", filename);
        return false;
    }

    LOAD_SYM_REQ(plug_init);
    LOAD_SYM_REQ(plug_pre_reload);
    LOAD_SYM_REQ(plug_post_reload);
    LOAD_SYM_REQ(plug_update);
    LOAD_SYM_OPT(plug_finished); /* optional */

    plug_init();
    return true;
}

/* ------------------------------------------------------------
   Entry point
------------------------------------------------------------ */

int main(void) {

    /* Window setup */
    float factor = 80.0f;
    InitWindow((int)(16 * factor), (int)(9 * factor), "CONA");
    InitAudioDevice();
    SetTargetFPS(60);

    EnginePhase phase = PHASE_SPLASH;

    /* Load splash plugin */
    if (!load_plugin(SPLASH_PLUG)) {
        CloseWindow();
        return 1;
    }

    while (!WindowShouldClose()) {

        /* Hot reload only during MAIN phase */
        if (phase == PHASE_MAIN && IsKeyPressed(KEY_R)) {
            void *state = plug_pre_reload();
            if (!load_plugin(MAIN_PLUG)) {
                CloseWindow();
                return 1;
            }
            plug_post_reload(state);
        }

        /* Update active plugin */
        plug_update();

        /* Phase transitions */
        if (plug_finished && plug_finished()) {
            if (phase == PHASE_SPLASH) {
                phase = PHASE_BOOT;
                if (!load_plugin(BOOT_PLUG)) break;
            }
            else if (phase == PHASE_BOOT) {
                phase = PHASE_MAIN;
                if (!load_plugin(MAIN_PLUG)) break;
            }
        }
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}

