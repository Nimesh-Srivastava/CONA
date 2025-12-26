#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

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
   Web server process management
------------------------------------------------------------ */

static pid_t web_server_pid = -1;

static void cleanup_web_server(void) {
    if (web_server_pid > 0) {
        printf("Stopping web server (PID: %d)...\n", web_server_pid);
        kill(web_server_pid, SIGTERM);
        waitpid(web_server_pid, NULL, 0);
        web_server_pid = -1;
    }
}

static void signal_handler(int sig) {
    (void)sig;
    cleanup_web_server();
    exit(0);
}

static bool spawn_web_server(void) {
    pid_t pid = fork();
    
    if (pid < 0) {
        fprintf(stderr, "Failed to fork web server process\n");
        return false;
    }
    
    if (pid == 0) {
        // Child process
        chdir("studio");
        
        // Redirect output to /dev/null to keep terminal clean
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
        
        execlp("npm", "npm", "run", "dev", NULL);
        
        // If exec fails
        fprintf(stderr, "Failed to start web server\n");
        exit(1);
    }
    
    // Parent process
    web_server_pid = pid;
    printf("Web server started (PID: %d)\n", web_server_pid);
    printf("Web UI available at http://localhost:3000\n");
    
    // Give the server time to start
    sleep(3);
    
    // Auto-open browser
    printf("Opening browser...\n");
    system("open http://localhost:3000");
    
    return true;
}

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
    LOAD_SYM_OPT(plug_finished);

    plug_init();
    return true;
}

/* ------------------------------------------------------------
   Entry point
------------------------------------------------------------ */

int main(void) {
    
    // Set up signal handlers for cleanup
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Register cleanup function
    atexit(cleanup_web_server);
    
    // Start web server
    if (!spawn_web_server()) {
        fprintf(stderr, "Warning: Failed to start web server\n");
    }

    /* Window setup */
    float factor = 80.0f;
    InitWindow((int)(16 * factor), (int)(9 * factor), "CONA Animation Studio");
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

